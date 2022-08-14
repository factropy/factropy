// Copyright (c) 2021 Dorfl <dorfl@factropy.com>

#include "gl-ex.h"
#include "../imgui/setup.h"

#include "common.h"
#include "crew.h"
#include "glm-ex.h"
#include "shader.h"
#include "mesh.h"
#include "part.h"
#include "config.h"
#include "world.h"
#include "sky.h"
#include "scene.h"
#include "gui.h"
#include "chunk.h"
#include "scenario.h"
#include "pulse.h"

#include <vector>
#include <functional>
#include <type_traits>
#include <typeinfo>
#include <cassert>
#include <chrono>
#include <thread>
#include <filesystem>
#include <random>

#include <unistd.h>
#include <csignal>
#include "stacktrace.h"

using namespace std::literals::chrono_literals;

FILE* errorLog = nullptr;
SDL_Window* window = nullptr;
SDL_GLContext context;
std::atomic<bool> quit = false;
const char* binary = nullptr;

SDL_Window* sdlWindow() {
	return window;
}

workers crew;
workers crew2;

struct {
	const char* argv[5];
	char title[1024];
	char title2[1024];
	char msg[1024];
	char msg2[1024];
} crash;

void crashargs() {
	crash.argv[0] = binary;
	crash.argv[1] = "abort";
	crash.argv[2] = nullptr;

#ifdef __linux__
	crash.argv[2] = crash.title;
	crash.argv[3] = crash.msg;
	crash.argv[4] = nullptr;
#endif

#ifdef _WIN32
	snprintf(crash.title2, sizeof(crash.title2), "\"%s\"", crash.title);
	snprintf(crash.msg2, sizeof(crash.msg2), "\"%s\"", crash.msg);
	crash.argv[2] = crash.title2;
	crash.argv[3] = crash.msg2;
	crash.argv[4] = nullptr;
#endif
}

void omg(int sig) {
	snprintf(crash.msg, sizeof(crash.msg), "signal %d", sig);
	fprintf(errorLog, "%s\n", crash.msg); fflush(errorLog);
	fprintf(stderr, "%s\n", crash.msg); fflush(stderr);
	print_stacktrace(errorLog);
	print_stacktrace(stderr);
	snprintf(crash.title, sizeof(crash.title), "Factropy %s Signal", Config::version.text);

	crashargs();
	execv(crash.argv[0], const_cast<char* const *>(crash.argv));
	std::terminate();
}

void wtf(const char* file, const char* func, int line, const char* err) {
	snprintf(crash.msg, sizeof(crash.msg), "abort: %s:%d %s()\n%s",
		file ? (char*)std::filesystem::path(file).filename().c_str(): "",
		line, func, err ? err: ""
	);

	fprintf(errorLog, "%s\n", crash.msg); fflush(errorLog);
	fprintf(stderr, "%s\n", crash.msg); fflush(stderr);
	print_stacktrace(errorLog);
	print_stacktrace(stderr);
	snprintf(crash.title, sizeof(crash.title), "Factropy %s Abort", Config::version.text);

	crashargs();
	execv(crash.argv[0], const_cast<char* const *>(crash.argv));
	std::terminate();
}

void game() {
	auto now = []() {
		return std::chrono::steady_clock::now();
	};

	auto elapsed = [&](auto since) {
		uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(now()-since).count();
		return (double)us / 1000000.0;
	};

	auto sleepRate = [&](auto stamp, double rate) {
		double period = 1.0/rate;
		while (elapsed(stamp) < period*0.5) std::this_thread::sleep_for(1ms);
		while (elapsed(stamp) < period) std::this_thread::yield();
	};

	pulser pulse;
	scene.init();
	gui.init();

	std::atomic<bool> run = {true};
	std::atomic<bool> done = {false};
	std::atomic<bool> readyScenario = {false};
	std::atomic<bool> readyIcons = {false};

	scenario = new ScenarioBase();

	crew.job([&]() {
		trigger chunkNoise;

		crew2.job([&]() {
			//Chunk::Terrain::noiseGen();
			Chunk::Terrain::noiseLoad();
			chunkNoise.now();
		});

		scenario->items();
		scenario->fluids();
		scenario->recipes();
		scenario->specifications();
		scenario->goals();
		scenario->messages();

		for (auto [_,item]: Item::names)
			if (!item->title.size()) item->title = fmt("(%s)", item->name);

		for (auto [_,fluid]: Fluid::names)
			if (!fluid->title.size()) fluid->title = fmt("(%s)", fluid->name);

		for (auto [_,recipe]: Recipe::names)
			if (!recipe->title.size()) recipe->title = fmt("(%s)", recipe->name);

		for (auto [_,spec]: Spec::all)
			if (!spec->title.size()) spec->title = fmt("(%s)", spec->name);

		for (auto [_,goal]: Goal::all)
			if (!goal->title.size()) goal->title = fmt("(%s)", goal->name);

		Item::categories["_"] = {"Other","zzz"};
		for (auto& [_,category]: Item::categories) category.groups["_"] = {"zzz"};

		for (auto [_,item]: Item::names) {
			if (!item->category) {
				item->category = &Item::categories["_"];
				item->group = &item->category->groups["_"];
			}
			if (!item->group) {
				item->group = &item->category->groups["_"];
			}
		}

		std::set<Item::Category*> categories;
		for (auto& [_,category]: Item::categories) categories.insert(&category);

		Item::display = {categories.begin(), categories.end()};
		std::sort(Item::display.begin(), Item::display.end(), [](const auto a, const auto b) {
			return a->order < b->order;
		});

		for (auto& [_,category]: Item::categories) {
			std::set<Item::Group*> groups;
			for (auto& [_,group]: category.groups) groups.insert(&group);

			category.display = {groups.begin(), groups.end()};
			std::sort(category.display.begin(), category.display.end(), [](const auto a, const auto b) {
				return a->order < b->order;
			});

			for (auto& [_,group]: category.groups) {
				std::set<Item*> items;
				for (auto [_,item]: Item::names) if (item->group == &group) items.insert(item);

				group.display = {items.begin(), items.end()};
				std::sort(group.display.begin(), group.display.end(), Item::sort);
			}
		}

		if (Config::mode.load) {
			try {
				auto cam = Sim::load(Config::savePath(Config::mode.saveName).c_str());
				scene.position = std::get<0>(cam);
				scene.direction = std::get<1>(cam);
				uint directing = std::get<2>(cam);
				if (directing && Entity::exists(directing))
					scene.directing = new GuiEntity(directing);
				scenario->load();
			}
			catch (const std::exception& e) {
				ensuref(false,
					"Cannot load \"%s\"\r\n"
					"Exception: %s\r\n"
					"If this is the result of a recent game crash please report a bug.\r\n",
					Config::savePath(Config::mode.saveName),
					e.what()
				);
			}
			catch (...) {
				ensuref(false,
					"Cannot load \"%s\"\r\n"
					"If this is the result of a recent game crash please report a bug.\r\n",
					Config::savePath(Config::mode.saveName)
				);
			}
		} else {
			notef("Generating a new world...");
			Sim::init(Config::mode.freshSeed);
			world.scenario.size = std::max(Config::mode.world, 4096);
			world.init();
			sky.init();
			scenario->create();
		}

		chunkNoise.wait();
		readyScenario = true;
	});

	gui.togglePopup(gui.loading);

	bool readyChunks = false;
	uint totalChunks = 0;

	SDL_GL_SetSwapInterval(0);

	SDL_Event event;
	while (!readyScenario || !readyIcons || !readyChunks) {
		// Ubuntu + Gnome3 + SDL_WINDOW_FULLSCREEN_DESKTOP + amdgpu + SDL 2.0.10
		// has a race condition that causes the first call to SDL_GL_MakeCurrent
		// to produce a viewport with a gap at the top the size of the Gnome top
		// bar + a non-fullscreen window titlebar. Calling it again during
		// loading realigns the viewport.
		SDL_GL_MakeCurrent(window, context);

		while (SDL_PollEvent(&event))
			ImGui_ImplSDL2_ProcessEvent(&event);

		Config::autoscale(window);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		scene.updateLoading(Config::window.width, Config::window.height);
		scene.renderLoading();

		gui.render();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);

		if (readyScenario) {
			if (!totalChunks) {
				totalChunks = Chunk::prepare();
				notef("Generating icons...");
				notef("Generating terrain...");
			}
			uint doneChunks = totalChunks - Chunk::prepare();
			gui.loading->progress = (float)doneChunks / (float)totalChunks;
			readyChunks = doneChunks == totalChunks;
		}

		if (readyScenario)
			readyIcons = !scene.renderSpecIcon() && !scene.renderItemIcon() && !scene.renderFluidIcon();

		std::this_thread::sleep_for(1ms);
	}

	SDL_GL_SetSwapInterval(Config::window.vsync ? 1:0);

	scene.prepare();
	gui.prepare();

	gui.togglePopup(gui.loading);
	gui.loading->progress = 1.0f;

	std::atomic<float> ups = {0};
	std::atomic<float> fps = {0};
	done = false;

	// Sim tries to run at 60 UPS regardless of FPS
	crew.job([&]() {

		int update = 0;
		std::array<double,10> updates;
		for (auto& slot: updates) slot = 0;

		auto stamp = now();

		auto maintainUPS = [&](double n) {
			// snap-lock UPS and FPS when roughly equal and stable to prevent stutter
			if (Config::engine.pulse || std::abs(fps-n) < 3.0f) pulse.wait();
			else sleepRate(stamp, n);

			updates[update++] = elapsed(stamp);
			if (update >= (int)updates.size()) update = 0;

			double sum = 0;
			for (auto& delta: updates) sum += delta;
			ups = 1.0 / (sum / (double)updates.size());

			stamp = now();
		};

		uint64_t autoSaveLast = Sim::tick;

		while (run && !quit) {
			if (Config::mode.pause) {
				std::this_thread::sleep_for(16ms);
				continue;
			}

			uint64_t autoSaveInterval = std::max(1, std::min(60, Config::mode.autosave));
			uint64_t autoSaveNext = autoSaveLast + (autoSaveInterval*60*60);

			if (autoSaveNext == Sim::tick+60) {
				scene.print("autosave...");
			}

			Sim::locked([&]() {
				Sim::update();

				if (autoSaveNext <= Sim::tick) {
					if (Config::mode.autosaveN == 3) Config::mode.autosaveN = 0;
					Sim::save(Config::savePath(fmt("autosave%d", Config::mode.autosaveN++)).c_str(),
						scene.position, scene.direction,
						scene.directing ? scene.directing->id : 0
					);
					autoSaveLast = Sim::tick;
				}
			});

			Sim::statsTick.set(Sim::tick, elapsed(stamp));
			maintainUPS(Config::engine.ups);
		}

		done = true;
	});

	auto stamp = now();

	// track average frame rate
	int frame = 0;
	std::array<double,10> frames;
	for (auto& slot: frames) slot = 0;

	auto maintainFPS = [&](double n) {
		if (!Config::window.vsync) sleepRate(stamp, n);

		frames[frame++] = elapsed(stamp);
		if (frame >= (int)frames.size()) frame = 0;

		double sum = 0;
		for (auto& delta: frames) sum += delta;
		fps = 1.0 / (sum / (double)frames.size());

		stamp = now();
	};

	// some scene stuff is done lazily and put off until the next frame,
	// so ensure everything runs at least once before the first frame
	// rendered to screen
	scene.update(Config::window.width, Config::window.height, fps);
	scene.advance();

	auto& io = ImGui::GetIO();

	while (run && !quit) {
		Config::autoscale(window);

		gui.focused = io.WantCaptureMouse || io.WantCaptureKeyboard;
		scene.focused = !gui.focused;

		gui.ups = ups;
		gui.fps = fps;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		scene.stats.update.track(scene.frame, [&]() {
			scene.advanceDone.wait();
			scene.update(Config::window.width, Config::window.height, fps);
			scene.advance();
		});

		gui.update();

		// swapping buffers doesn't mean the frame is instantly displayed,
		// nor that the back-buffer is immediately ready to start on the
		// next frame. Putting the first glClear() outside the time tracking
		// blocks avoids adding the real vsync/refresh delay to stats
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		scene.stats.render.track(scene.frame, [&]() { scene.render(); });

		gui.render();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		Chunk::tickPurge();
		crew2.job(Chunk::tickChange);
		pulse.now();

		SDL_GL_SwapWindow(window);
		maintainFPS(Config::window.fps);

		scene.wheel = 0;
		scene.keysLast = scene.keys;

		while(SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);

			switch(event.type)
			{
				case SDL_QUIT: {
					run = false;
					quit = true;
					break;
				}
				case SDL_KEYUP: {
					scene.keys[event.key.keysym.sym] = false;
					break;
				}
				case SDL_KEYDOWN: {
					scene.keys[event.key.keysym.sym] = true;
					break;
				}
				case SDL_MOUSEWHEEL: {
					if (event.wheel.x > 0) scene.wheel += 1;
					if (event.wheel.x < 0) scene.wheel -= 1;
					if (event.wheel.y > 0) scene.wheel += 1;
					if (event.wheel.y < 0) scene.wheel -= 1;
					break;
				}
			}
		}

		if (gui.doQuit) run = false;
	}

	scene.advanceDone.wait();

	while (!Sim::saveTickets.send_if_empty(true)) {
		pulse.now();
		std::this_thread::sleep_for(1ms);
	}

	Sim::saveTickets.recv();

	while (!done) {
		pulse.now();
		std::this_thread::sleep_for(1ms);
	}

	pulse.stop();

	delete scenario;
	scenario = nullptr;

	scene.reset();
	gui.reset();
	sky.reset();
	world.reset();
	Chunk::reset();
	Entity::reset();
	Spec::reset();
	Item::reset();
	Fluid::reset();
	Recipe::reset();
	Goal::reset();
	Message::reset();
	Part::reset();
	Mesh::reset();
	Sim::reset();

	ensure(Spec::all.size() == 0);
	ensure(Part::all.size() == 0);
	ensure(Mesh::all.size() == 0);
}

void menu() {
	using namespace ImGui;

	SDL_GL_SetSwapInterval(1);

	bool run = true;
	auto screen = new StartScreen();

	while (run && !quit) {
		// Ubuntu + Gnome3 + SDL_WINDOW_FULLSCREEN_DESKTOP + amdgpu + SDL 2.0.10
		// has a race condition that causes the first call to SDL_GL_MakeCurrent
		// to produce a viewport with a gap at the top the size of the Gnome top
		// bar + a non-fullscreen window titlebar. Calling it again during
		// loading realigns the viewport.
		SDL_GL_MakeCurrent(window, context);

		Config::autoscale(window);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		glClearColor(0,0,0,1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		screen->draw();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);

		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);

			switch(event.type)
			{
				case SDL_QUIT: {
					run = false;
					quit = true;
					break;
				}
			}
		}

		if (Config::mode.load || Config::mode.fresh) {
			game();
			Config::mode.load = false;
			Config::mode.fresh = false;
			SDL_GL_SetSwapInterval(1);
			delete screen;
			screen = new StartScreen();
		}
	}

	delete screen;
	SDL_GL_SetSwapInterval(Config::window.vsync ? 1:0);
}

int main(int argc, char* argv[]) {
	binary = argv[0];

	Config::version.major = 0;
	Config::version.minor = 2;
	Config::version.patch = 1;

	snprintf(Config::version.text, sizeof(Config::version.text),
		"%d.%d.%d-alpha",
		Config::version.major,
		Config::version.minor,
		Config::version.patch
	);

	std::signal(SIGSEGV, omg);

	if (argc > 1 && std::string(argv[1]) == "abort") {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			argc > 2 ? argv[2]: "(no detail)",
			argc > 3 ? argv[3]: "",
			nullptr
		);
		exit(1);
	}

	errorLog = fopen("error.log", "w+");

	notef("Factropy %s", Config::version.text);

	glewExperimental = GL_TRUE;

	bool SDLOk = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) == 0;
	ensuref(SDLOk, fmtc(
		"SDL_Init is failing to initialize with this error: \"%s\".\r\n"
		"This is likely a low level problem and not a bug in the game.\r\n"
		"Ensure your graphics driver is up to date with support for OpenGL 4.4.\r\n",
		SDL_GetError()
	));

	Config::load();
	Config::args(argc, argv);
	Config::sdl();
	Config::profile();

	window = SDL_CreateWindow("Factropy",
		 SDL_WINDOWPOS_CENTERED,
		 SDL_WINDOWPOS_CENTERED,
		 Config::window.width,
		 Config::window.height,
		 Config::window.sdlFlags
	);

	ensuref(window, fmtc(
		"SDL_CreateWindow is failing to create a window with this error: \"%s\".\r\n"
		"Ensure your graphics driver is up to date with support for OpenGL 4.4.\r\n",
		SDL_GetError()
	));

	context = SDL_GL_CreateContext(window);

	ensuref(context, fmtc(
		"SDL_GL_CreateContext OpenGL 4.4 setup is failing with this error: \"%s\".\r\n"
		"Ensure your graphics driver is up to date with support for OpenGL 4.4.\r\n",
		SDL_GetError()
	));

	SDL_GL_MakeCurrent(window, context);

	auto glewErr = glewInit();

	ensuref(glewErr == GLEW_OK, fmtc(
		"GLEW setup is failing with this error: \"%s\".\r\n"
		"Ensure your graphics driver is up to date with support for OpenGL 4.4.\r\n",
		(char*)glewGetErrorString(glewErr)
	));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplSDL2_InitForOpenGL(window, context);
	ImGui_ImplOpenGL3_Init("#version 330");

	ImPlot::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();

	io.IniFilename = nullptr;
	io.WantSaveIniSettings = false;

	auto& style = ImGui::GetStyle();
	style.ScaleAllSizes(0.6);
	Config::styleDefault = style;

	SDL_GetDisplayDPI(0, &Config::window.ddpi, &Config::window.hdpi, &Config::window.vdpi);
//	notef("DPI: diagonal %0.2f horizontal %0.2f vertical %0.2f", Config::window.ddpi, Config::window.hdpi, Config::window.vdpi);

	SDL_GL_SetSwapInterval(Config::window.vsync ? 1:0);

	glViewport(0, 0, Config::window.width, Config::window.height);
	glClearDepth(1.0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	if (Config::window.antialias) glEnable(GL_MULTISAMPLE);

	#if defined(_WIN32)
		timeBeginPeriod(1);
	#endif

	Config::imgui();

	crew.start(Config::engine.threads);
	crew2.start(Config::engine.cores);

	scene.initGL();
	menu();

	ImPlot::DestroyContext();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	Config::save();

	crew.stop();
	crew2.stop();

	#if defined(_WIN32)
		timeEndPeriod(1);
	#endif

	if (Config::mode.restart) {
		execv(argv[0], argv);
	}

	return 0;
}
