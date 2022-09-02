#include "common.h"
#include "sim.h"
#include "world.h"
#include "entity.h"
#include "goal.h"
#include "crew.h"
#include "gui.h"
#include "enemy.h"
#include "glm-ex.h"

#include "json.hpp"
#include <fstream>
using json = nlohmann::json;

#include <filesystem>
namespace fs = std::filesystem;

#include "catenate.h"
#include "flate.h"
#include <chrono>

namespace Save {

	std::string itemOut(uint iid) {
		return iid ? Item::get(iid)->name: "none";
	}

	uint itemIn(std::string name) {
		return (name == "" || name == "none") ? 0: Item::byName(name)->id;
	}

	std::string fluidOut(uint fid) {
		return fid ? Fluid::get(fid)->name: "none";
	}

	uint fluidIn(std::string name) {
		return (name == "" || name == "none") ? 0: Fluid::byName(name)->id;
	}

	json timeSeriesSave(TimeSeries* ts) {
		json state;
		state["tickMax"] = ts->tickMax;
		state["secondMax"] = ts->secondMax;
		state["minuteMax"] = ts->minuteMax;
		state["hourMax"] = ts->hourMax;
		for (uint i = 0; i < 60; i++) {
			state["ticks"][i] = ts->ticks[i];
		}
		for (uint i = 0; i < 60; i++) {
			state["seconds"][i] = ts->seconds[i];
		}
		for (uint i = 0; i < 60; i++) {
			state["minutes"][i] = ts->minutes[i];
		}
		for (uint i = 0; i < 60; i++) {
			state["hours"][i] = ts->hours[i];
		}
		return state;
	}

	void timeSeriesLoad(TimeSeries* ts, json state) {
		ts->tickMax = state["tickMax"];
		ts->secondMax = state["secondMax"];
		ts->minuteMax = state["minuteMax"];
		ts->hourMax = state["hourMax"];
		for (uint i = 0; i < 60; i++) {
			ts->ticks[i] = state["ticks"][i];
		}
		for (uint i = 0; i < 60; i++) {
			ts->seconds[i] = state["seconds"][i];
		}
		for (uint i = 0; i < 60; i++) {
			ts->minutes[i] = state["minutes"][i];
		}
		for (uint i = 0; i < 60; i++) {
			ts->hours[i] = state["hours"][i];
		}
	}

	void dumpRecipes() {
		auto out = std::ofstream("recipe.md");

		out << "| Recipe | Total Energy | Raw Materials |\n";
		out << "| --- | --- | --- |\n";

		for (auto [name,recipe]: Recipe::names) {
			std::string items;
			for (auto stack: recipe->totalRawItems()) {
				items += fmt("%s(%u)<br/>", Item::get(stack.iid)->name, stack.size);
			}

			std::string fluids;
			for (auto amount: recipe->totalRawFluids()) {
				fluids += fmt("%s(%u)<br/>", Fluid::get(amount.fid)->name, amount.size);
			}

			out << fmt("| %s | %s | %s %s |\n", name, recipe->totalEnergy().format(), items, fluids);
		}

		out.close();
	}
}

namespace Sim {
	channel<bool,3> saveTickets;

	bool save(const char* name, Point camPos, Point camDir, uint directing) {
		if (!saveTickets.send_if_empty(true)) return false;

		notef("Save to: %s", name);

		struct {
			StopWatch all;
			StopWatch sim;
			StopWatch world;
			StopWatch entity;
			StopWatch other;
		} watches;

		auto path = std::string(name);

		try {
			fs::remove_all(path);
			fs::create_directory(path);
		}
		catch (std::filesystem::filesystem_error& e) {
			notef("Save failed: %s", e.what());
			saveTickets.recv();
			return false;
		}

		ensuref(saveTickets.send(true), "saveTickets.send");
		ensuref(saveTickets.send(true), "saveTickets.send");

		watches.all.time([&]() {
			watches.sim.time([&]() {
				{
					auto out = std::ofstream(path + "/sim.json");

					json state;
					state["version"] = {Config::version.major, Config::version.minor, Config::version.patch};

					state["tick"] = tick;
					state["seed"] = seed;
					state["enemy"] = Enemy::enable;
					state["name"] = Config::mode.saveName;

					for (auto [iid,size]: Item::supplied) {
						state["supplied"][Save::itemOut(iid)] = size;
					}

					state["chits"] = Goal::chits;
					state["rates"]["mining"] = Recipe::miningRate;
					state["rates"]["drilling"] = Recipe::drillingRate;
					state["rates"]["crushing"] = Recipe::crushingRate;
					state["rates"]["smelting"] = Recipe::smeltingRate;
					state["rates"]["crafting"] = Recipe::craftingRate;
					state["rates"]["refining"] = Recipe::refiningRate;
					state["rates"]["centrifuging"] = Recipe::centrifugingRate;
					state["rates"]["drone"] = Drone::speedFactor;
					state["rates"]["arm"] = Arm::speedFactor;

					int i = 0;
					for (auto& label: Signal::labels) {
						state["signals"]["labels"][i++] = {label.id, label.name, label.used, label.drop};
					}

					out << state << "\n";
					out.close();
				}

				{
					auto out = std::ofstream(path + "/camera.json");

					json state;
					state["position"] = {camPos.x, camPos.y, camPos.z};
					state["direction"] = {camDir.x, camDir.y, camDir.z};
					state["directing"] = directing;

					out << state << "\n";
					out.close();
				}

				{
					auto out = std::ofstream(path + "/toolbar.json");

					json state;
					int i = 0;
					for (auto spec: gui.toolbar->specs) {
						state["specs"][i++] = spec->name.c_str();
					}

					out << state << "\n";
					out.close();
				}
			});

			watches.world.time([&]() { world.save(name, &saveTickets); });
			watches.entity.time([&]() { Entity::saveAll(name, &saveTickets); });

			watches.other.time([&]() {
				sky.save(name);
				Store::saveAll(name);
				Ghost::saveAll(name);
				Item::saveAll(name);
				Fluid::saveAll(name);
				Spec::saveAll(name);
				Recipe::saveAll(name);
				Arm::saveAll(name);
				Vehicle::saveAll(name);
				Cart::saveAll(name);
				CartStop::saveAll(name);
				CartWaypoint::saveAll(name);
				Effector::saveAll(name);
				Crafter::saveAll(name);
				Depot::saveAll(name);
				Drone::saveAll(name);
				Burner::saveAll(name);
				Charger::saveAll(name);
				Pipe::saveAll(name);
				Conveyor::saveAll(name);
				Unveyor::saveAll(name);
				Loader::saveAll(name);
				Balancer::saveAll(name);
				Computer::saveAll(name);
				Router::saveAll(name);
				Networker::saveAll(name);
				Turret::saveAll(name);
				Missile::saveAll(name);
				FlightPad::saveAll(name);
				FlightPath::saveAll(name);
				FlightLogistic::saveAll(name);
				Tube::saveAll(name);
				Monorail::saveAll(name);
				Monocar::saveAll(name);
				Teleporter::saveAll(name);
				Launcher::saveAll(name);
				PowerPole::saveAll(name);
				ElectricityNetwork::saveAll(name);
				Shipyard::saveAll(name);
				Ship::saveAll(name);

				Goal::saveAll(name);
				Message::saveAll(name);

				{
					auto out = std::ofstream(path + "/time-series.json");

					json state;
					state["statsEntityPre"] = Save::timeSeriesSave(&Sim::statsEntityPre);
					state["statsEntityPost"] = Save::timeSeriesSave(&Sim::statsEntityPost);
					state["statsStore"] = Save::timeSeriesSave(&Sim::statsStore);
					state["statsArm"] = Save::timeSeriesSave(&Sim::statsArm);
					state["statsCrafter"] = Save::timeSeriesSave(&Sim::statsCrafter);
					state["statsConveyor"] = Save::timeSeriesSave(&Sim::statsConveyor);
					state["statsPath"] = Save::timeSeriesSave(&Sim::statsPath);
					state["statsVehicle"] = Save::timeSeriesSave(&Sim::statsVehicle);
					state["statsPipe"] = Save::timeSeriesSave(&Sim::statsPipe);
					state["statsDepot"] = Save::timeSeriesSave(&Sim::statsDepot);
					state["statsDrone"] = Save::timeSeriesSave(&Sim::statsDrone);

					out << state << "\n";
					out.close();
				}
			});
		});

		notef("save %0.1fms", watches.all.milliseconds());
		notef("save %0.1fms sim", watches.sim.milliseconds());
		notef("save %0.1fms world", watches.world.milliseconds());
		notef("save %0.1fms entity", watches.entity.milliseconds());
		notef("save %0.1fms other", watches.other.milliseconds());

		saveTickets.recv();
		return true;
	}

	std::tuple<Point,Point,uint> load(const char* name) {
		notef("Load from: %s", name);

//		auto start = std::chrono::steady_clock::now();
		auto path = std::string(name);

		{
			auto in = std::ifstream(path + "/sim.json");

			for (std::string line; std::getline(in, line);) {
				auto state = json::parse(line);

				if (state.contains("version")) {
					int sMajor = (int)state["version"][0];
					int sMinor = (int)state["version"][1];
					int sPatch = (int)state["version"][2];
					int cMajor = Config::version.major;
					int cMinor = Config::version.minor;
					int cPatch = Config::version.patch;

					bool sameMajorMinor = sMajor == cMajor && sMinor == cMinor;

					throwf(sameMajorMinor,
						"Saved game from %d.%d.%d is incompatible with version %d.%d.%d.",
						sMajor, sMinor, sPatch, cMajor, cMinor, cPatch
					);

					bool sameOrNewer = sameMajorMinor && sPatch <= cPatch;

					throwf(sameOrNewer,
						"Saved game comes from %d.%d.%d but this is version %d.%d.%d."
						" Please update the game to continue.",
						sMajor, sMinor, sPatch, cMajor, cMinor, cPatch
					);
				}

				auto fpath = std::filesystem::path(name);
				if (fpath.stem().string().find("autosave") == 0)
					Config::mode.saveName = state["name"];

				init(state["seed"]);
				tick = state["tick"];

				if (state.contains("enemy")) {
					Enemy::enable = state["enemy"];
				}

				for (auto [name,item]: Item::names) {
					if (state["supplied"].contains(name)) {
						Item::supplied[item->id] = state["supplied"][name];
					}
				}

				if (state.contains("chits")) {
					Goal::chits = state["chits"];
				}

				if (state.contains("rates")) {
					if (state["rates"].contains("mining"))
						Recipe::miningRate = state["rates"]["mining"];
					if (state["rates"].contains("drilling"))
						Recipe::drillingRate = state["rates"]["drilling"];
					if (state["rates"].contains("crushing"))
						Recipe::crushingRate = state["rates"]["crushing"];
					if (state["rates"].contains("smelting"))
						Recipe::smeltingRate = state["rates"]["smelting"];
					if (state["rates"].contains("crafting"))
						Recipe::craftingRate = state["rates"]["crafting"];
					if (state["rates"].contains("refining"))
						Recipe::refiningRate = state["rates"]["refining"];
					if (state["rates"].contains("centrifuging"))
						Recipe::centrifugingRate = state["rates"]["centrifuging"];
					if (state["rates"].contains("drone"))
						Drone::speedFactor = state["rates"]["drone"];
					if (state["rates"].contains("arm"))
						Arm::speedFactor = state["rates"]["arm"];
				}

				if (state.contains("/signals/labels"_json_pointer)) {
					for (auto entry: state["signals"]["labels"]) {
						Signal::labels.push_back({
							.id = entry[0],
							.name = entry[1],
							.used = entry[2],
							.drop = entry[3],
						});
						Signal::sequence = std::max(Signal::sequence, Signal::labels.back().id);
					}
				}
			}

			in.close();
		}

		world.load(name);
		sky.load(name);
		Item::loadAll(name);
		Fluid::loadAll(name);
		Spec::loadAll(name);
		Recipe::loadAll(name);
		Entity::loadAll(name);
		Store::loadAll(name);
		Ghost::loadAll(name);
		Arm::loadAll(name);
		Vehicle::loadAll(name);
		Cart::loadAll(name);
		CartStop::loadAll(name);
		CartWaypoint::loadAll(name);
		Effector::loadAll(name);
		Crafter::loadAll(name);
		Depot::loadAll(name);
		Drone::loadAll(name);
		Burner::loadAll(name);
		Charger::loadAll(name);
		Pipe::loadAll(name);
		Conveyor::loadAll(name);
		Unveyor::loadAll(name);
		Loader::loadAll(name);
		Balancer::loadAll(name);
		Computer::loadAll(name);
		Router::loadAll(name);
		Networker::loadAll(name);
		Turret::loadAll(name);
		Missile::loadAll(name);
		FlightPad::loadAll(name);
		FlightPath::loadAll(name);
		FlightLogistic::loadAll(name);
		Tube::loadAll(name);
		Monorail::loadAll(name);
		Monocar::loadAll(name);
		Teleporter::loadAll(name);
		Launcher::loadAll(name);
		PowerPole::loadAll(name);
		ElectricityNetwork::loadAll(name);
		Shipyard::loadAll(name);
		Ship::loadAll(name);

		Goal::loadAll(name);
		Message::loadAll(name);

//		{
//			auto in = std::ifstream(path + "/time-series.json");
//
//			for (std::string line; std::getline(in, line);) {
//				auto state = json::parse(line);
//				Save::timeSeriesLoad(&Sim::statsEntity, state["statsEntity"]);
//				Save::timeSeriesLoad(&Sim::statsStore, state["statsStore"]);
//				Save::timeSeriesLoad(&Sim::statsArm, state["statsArm"]);
//				Save::timeSeriesLoad(&Sim::statsCrafter, state["statsCrafter"]);
//				Save::timeSeriesLoad(&Sim::statsPath, state["statsPath"]);
//				Save::timeSeriesLoad(&Sim::statsVehicle, state["statsVehicle"]);
//				//Save::timeSeriesLoad(&Sim::statsPipe, state["statsPipe"]);
//				Save::timeSeriesLoad(&Sim::statsShunt, state["statsShunt"]);
//				Save::timeSeriesLoad(&Sim::statsDepot, state["statsDepot"]);
//				Save::timeSeriesLoad(&Sim::statsDrone, state["statsDrone"]);
//			}
//
//			in.close();
//		}

		std::tuple<Point,Point,uint> camera = std::make_tuple(Point(-50,50,-50), Point::Zero, 0);

		{
			auto in = std::ifstream(path + "/camera.json");

			for (std::string line; std::getline(in, line);) {
				auto state = json::parse(line);

				camera = std::make_tuple(
					Point(state["position"][0], state["position"][1], state["position"][2]),
					Point(state["direction"][0], state["direction"][1], state["direction"][2]),
					state.contains("directing") ? (uint)state["directing"]: 0u
				);
			}

			in.close();
		}

		{
			auto in = std::ifstream(path + "/toolbar.json");

			for (std::string line; std::getline(in, line);) {
				auto state = json::parse(line);

				for (auto name: state["specs"]) {
					if (!Spec::all.count(name)) continue;
					gui.toolbar->add(Spec::byName(name));
				}
			}

			in.close();
		}

//		auto finish = std::chrono::steady_clock::now();
//		notef("Loaded %0.1fms", std::chrono::duration<double,std::milli>(finish-start).count());

		return camera;
	}
}

void Spec::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/specs.json");

	for (auto& pair: all) {
		Spec *spec = pair.second;
		json state;
		state["name"] = spec->name;
		state["licensed"] = spec->licensed;
		state["constructed"] = spec->count.constructed;
		out << state << "\n";
	}

	out.close();
}

void Spec::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/specs.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);

		if (!Spec::all.count(state["name"])) {
			notef("Specification %s removed", std::string(state["name"]));
			continue;
		}

		Spec* spec = Spec::byName(state["name"]);
		spec->licensed = state["licensed"];

		if (state.contains("constructed")) {
			spec->count.constructed = state["constructed"];
		}
	}

	in.close();
}

void Item::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/items.json");

	for (auto& [_,item]: names) {
		json state;
		state["name"] = item->name;

		int i = 0;
		for (auto& shipment: item->shipments) {
			state["shipments"][i][0] = shipment.tick;
			state["shipments"][i][1] = shipment.count;
			i++;
		}

		state["produced"] = item->produced;
		state["consumed"] = item->consumed;

		state["production"] = Save::timeSeriesSave(&item->production);
		state["consumption"] = Save::timeSeriesSave(&item->consumption);
		state["supplies"] = Save::timeSeriesSave(&item->supplies);

		out << state << "\n";
	}

	out.close();
}

void Item::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/items.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);

		if (!Item::names.count(state["name"])) {
			notef("Item %s removed", std::string(state["name"]));
			continue;
		}

		Item* item = Item::byName(state["name"]);

		item->produced = state["produced"];
		item->consumed = state["consumed"];

		for (auto pair: state["shipments"]) {
			item->shipments.push_back({pair[0], pair[1]});
		}

		Save::timeSeriesLoad(&item->production, state["production"]);
		Save::timeSeriesLoad(&item->consumption, state["consumption"]);
		Save::timeSeriesLoad(&item->supplies, state["supplies"]);
	}

	in.close();
}

void Fluid::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/fluids.json");

	for (auto& [_,fluid]: names) {
		json state;
		state["name"] = fluid->name;

		state["produced"] = fluid->produced;
		state["consumed"] = fluid->consumed;

		state["production"] = Save::timeSeriesSave(&fluid->production);
		state["consumption"] = Save::timeSeriesSave(&fluid->consumption);

		out << state << "\n";
	}

	out.close();
}

void Fluid::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/fluids.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);

		if (!Fluid::names.count(state["name"])) {
			notef("Fluid %s removed", std::string(state["name"]));
			continue;
		}

		Fluid* fluid = Fluid::byName(state["name"]);

		fluid->produced = state["produced"];
		fluid->consumed = state["consumed"];

		Save::timeSeriesLoad(&fluid->production, state["production"]);
		Save::timeSeriesLoad(&fluid->consumption, state["consumption"]);
	}

	in.close();
}

void Recipe::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/recipes.json");

	for (auto& [_,recipe]: names) {
		json state;
		state["name"] = recipe->name;
		state["licensed"] = recipe->licensed;
		out << state << "\n";
	}

	out.close();
}

void Recipe::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/recipes.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!Recipe::names.count(state["name"])) {
			notef("Recipe %s removed", std::string(state["name"]));
			continue;
		}

		Recipe* recipe = Recipe::byName(state["name"]);
		recipe->licensed = state["licensed"];
	}
}

void Entity::saveAll(const char* name, channel<bool,3>* tickets) {

	struct EntityState {
		Spec* spec;
		uint id;
		uint16_t state;
		uint16_t flags;
		Point pos;
		Point dir;
		Health health;
	};

	struct EntityName {
		uint id;
		std::string name;
	};

	struct EntityColor {
		uint id;
		Color color;
	};

	uint seq = sequence;
	auto estates = new std::vector<EntityState>;
	auto enames = new std::vector<EntityName>;
	auto ecolors = new std::vector<EntityColor>;
	auto path = std::string(name);

	for (Entity& en: all) {
		estates->push_back({
			.spec = en.spec,
			.id = en.id,
			.state = en.state,
			.flags = en.flags,
			.pos = en.pos(),
			.dir = en.dir(),
			.health = en.health,
		});
		if (en.spec->named) {
			enames->push_back({
				.id = en.id,
				.name = en.name(),
			});
		}
		if (en.spec->coloredCustom) {
			ecolors->push_back({
				.id = en.id,
				.color = en.color(),
			});
		}
	}

	crew2.job([=]() {
		deflation def;
		def.push(fmt("%u %u %u %u", seq, estates->size(), enames->size(), ecolors->size()));

		for (auto& es: *estates) {
			def.push(fmt("%u %s %u %u %d " realfmt3 " " realfmt3,
				es.id, es.spec->name, es.flags, es.state, es.health, es.pos.x, es.pos.y, es.pos.z, es.dir.x, es.dir.y, es.dir.z
			));
		}

		for (auto& enn: *enames) {
			def.push(fmt("%u %s", enn.id, enn.name));
		}

		for (auto& enc: *ecolors) {
			def.push(fmt("%u %f,%f,%f", enc.id, enc.color.r, enc.color.g, enc.color.b));
		}

		def.save(path + "/entities");
		delete estates;
		delete enames;
		delete ecolors;
		tickets->recv();
	});
}

void Entity::loadAll(const char* name) {
	auto path = std::string(name);

	inflation inf;
	auto lines = inf.load(path + "/entities").parts();
	auto it = lines.begin();
	auto line = *it++;
	char tmp[128];

	auto linetmp = [&]() {
		throwf(line.size() < sizeof(tmp), "linetmp");
		memmove(tmp, line.data(), line.size());
		tmp[line.size()] = 0;
	};

	uint count = 0;
	uint names = 0;
	uint colors = 0;

	linetmp();
	for (;;) {
		if (4 == std::sscanf(tmp, "%u %u %u %u", &sequence, &count, &names, &colors)) break;
		if (3 == std::sscanf(tmp, "%u %u %u", &sequence, &count, &names)) break;
		throwf(false, "invalid entity metadata");
	}

	infof("entity %u %u %u %u", sequence, count, names, colors);

	for (uint i = 0; i < count; i++) {
		ensure(it != lines.end());
		line = *it++;
		linetmp();
		char *ptr = tmp;

		// much faster than sscanf
		uint id = strtoul(ptr, &ptr, 10);
		ensure(isspace(*ptr++));

		char spec[100];
		char *dst = spec;
		while (*ptr && !isspace(*ptr)) {
			ensure(dst < spec+sizeof(spec)-1);
			*dst++ = *ptr++;
		}
		*dst++ = 0;
		ensure(isspace(*ptr));

		uint flags = strtoul(++ptr, &ptr, 10);
		ensure(isspace(*ptr));
		uint state = strtoul(++ptr, &ptr, 10);
		ensure(isspace(*ptr));
		int health = strtol(++ptr, &ptr, 10);
		ensure(isspace(*ptr));

		Point pos;
		pos.x = strtod(++ptr, &ptr);
		ensure(*ptr == ',');
		pos.y = strtod(++ptr, &ptr);
		ensure(*ptr == ',');
		pos.z = strtod(++ptr, &ptr);
		ensure(isspace(*ptr));

		Point dir;
		dir.x = strtod(++ptr, &ptr);
		ensure(*ptr == ',');
		dir.y = strtod(++ptr, &ptr);
		ensure(*ptr == ',');
		dir.z = strtod(++ptr, &ptr);
		ensure(!*ptr);

		if (!Spec::all.count(spec)) {
			notef("Specification %s removed, dropping entity %u", spec, id);
			continue;
		}

		Entity& en = create(id, Spec::byName(spec));
		en.unindex();

		dir = dir.normalize();

		if (en.spec->alignStrict(pos, dir) && !en.spec->monorailContainer) {
			if (!contains(en.spec->rotations, dir) && dir == Point::East && contains(en.spec->rotations, Point::West)) dir = Point::West;
			if (!contains(en.spec->rotations, dir) && dir == Point::West && contains(en.spec->rotations, Point::East)) dir = Point::East;
			if (!contains(en.spec->rotations, dir) && dir == Point::North && contains(en.spec->rotations, Point::South)) dir = Point::South;
			if (!contains(en.spec->rotations, dir) && dir == Point::South && contains(en.spec->rotations, Point::North)) dir = Point::North;
			if (!contains(en.spec->rotations, dir)) dir = en.spec->rotations.front();
			pos = en.spec->aligned(pos, dir);
		}

//		if (en.spec->junk) {
//			dir = Point::South.randomHorizontal();
//			pos.y = world.elevation(pos) + en.spec->collision.h*0.5;
//		}

		en.pos(pos);
		en.dir(dir);
		en.flags = flags;
		en.clearMarks();

		en.state = state;
		en.health = health;

		// in case spec state animations changed across save or mod upgrade
		en.state = (uint)std::max(0, std::min((int)en.state, (int)en.spec->states.size()-1));

		en.index();

		if (!en.isGhost())
			en.ghost().destroy();

		if (en.isConstruction())
			en.construct();

		if (en.isDeconstruction())
			en.deconstruct();

		if (en.isGhost())
			en.spec->count.ghosts++;

		if (!en.isGhost())
			en.spec->count.extant++;
	}

	for (uint i = 0; i < names; i++) {
		ensure(it != lines.end());
		line = *it++;
		linetmp();

		uint id;
		char name[100];

		throwf(2 == std::sscanf(tmp, "%u %[^\n]", &id, name), "%s", line);
		if (Entity::exists(id)) Entity::get(id).rename(name);
	}

	for (uint i = 0; i < colors; i++) {
		throwf(it != lines.end(), "entities truncated");
		line = *it++;
		linetmp();

		uint id;
		float r, g, b;

		throwf(4 == std::sscanf(tmp, "%u %f,%f,%f", &id, &r, &g, &b), "%s", line, "entities malformed");
		if (Entity::exists(id)) Entity::get(id).color(Color(r,g,b,1.0f));
	}
}

void Store::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/stores.json");

	for (Store& store: all) {
		json state;
		state["id"] = store.id;
		state["sid"] = store.sid;
		state["activity"] = store.activity;

		if (store.purge) {
			state["purge"] = store.purge;
		}

		if (store.block) {
			state["block"] = store.block;
		}

		int i = 0;
		for (Stack stack: store.stacks) {
			state["stacks"][i++] = {
				Save::itemOut(stack.iid),
				stack.size,
			};
		}

		i = 0;
		for (auto level: store.levels) {
			state["levels"][i++] = {
				Save::itemOut(level.iid),
				level.lower,
				level.upper,
			};
		}

		i = 0;
		for (uint did: store.drones) {
			state["drones"][i++] = did;
		}

		if (store.transmit) {
			state["transmit"] = store.transmit;
		}

		out << state << "\n";
	}

	out.close();
}

void Store::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/stores.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Store& store = get(state["id"]);

		// some components autoconfigure attached stores
		store.drones.clear();
		store.levels.clear();
		store.stacks.clear();

		store.sid = state["sid"];
		store.activity = state["activity"];

		if (state.contains("purge")) {
			store.purge = state["purge"];
		}

		if (state.contains("block")) {
			store.block = state["block"];
		}

		for (auto stack: state["stacks"]) {
			store.stacks.push_back({
				Save::itemIn(stack[0]),
				stack[1],
			});
			auto& stk = store.stacks.back();
			auto limit = store.limit().items(stk.iid);
			stk.size = std::min(stk.size, limit);
		}

		for (auto level: state["levels"]) {
			store.levels.push_back({
				.iid = Save::itemIn(level[0]),
				.lower = level[1],
				.upper = level[2],
			});
			auto& lvl = store.levels.back();
			auto limit = store.limit().items(lvl.iid);
			lvl.lower = std::min(lvl.lower, limit);
			lvl.upper = std::min(lvl.upper, limit);
		}

		for (uint did: state["drones"]) {
			store.drones.insert(did);
		}

		if (state.contains("transmit")) {
			store.transmit = state["transmit"];
		}
	}

	in.close();
}

void Ghost::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/ghosts.json");

	for (auto& ghost: all) {

		json state;
		state["id"] = ghost.id;

		state["store"]["sid"] = ghost.store.sid;
		state["store"]["activity"] = ghost.store.activity;

		int i = 0;
		for (Stack stack: ghost.store.stacks) {
			state["store"]["stacks"][i++] = {
				Item::get(stack.iid)->name,
				stack.size,
			};
		}

		i = 0;
		for (auto level: ghost.store.levels) {
			state["store"]["levels"][i++] = {
				Item::get(level.iid)->name,
				level.lower,
				level.upper,
			};
		}

		i = 0;
		for (uint did: ghost.store.drones) {
			state["store"]["drones"][i++] = did;
		}

		out << state << "\n";
	}

	out.close();
}

void Ghost::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/ghosts.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!Ghost::all.has(state["id"])) continue;

		Ghost& ghost = get(state["id"]);

		ghost.store.sid = state["store"]["sid"];
		ghost.store.activity = state["store"]["activity"];

		ghost.store.stacks.clear();
		for (auto stack: state["store"]["stacks"]) {
			ghost.store.stacks.push_back({
				Item::byName(stack[0])->id,
				stack[1],
			});
		}

		for (uint did: state["store"]["drones"]) {
			ghost.store.drones.insert(did);
		}

		// reapply levels taking spec changes into account
		auto& en = Entity::get(ghost.id);
		if (en.isConstruction()) en.construct();
		if (en.isDeconstruction()) en.deconstruct();
	}

	in.close();
}

void Vehicle::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/vehicles.json");

	for (auto& vehicle: all) {
		json state;

		state["id"] = vehicle.id;
		state["pause"] = vehicle.pause;
		state["patrol"] = vehicle.patrol;
		state["handbrake"] = vehicle.handbrake;

		int i = 0;
		for (Point point: vehicle.path) {
			state["path"][i++] = {point.x, point.y, point.z};
		}

		state["waypoint"] = -1;

		i = 0;
		for (auto wp: vehicle.waypoints) {

			if (wp == vehicle.waypoint) {
				state["waypoint"] = i;
			}

			json wstate;
			wstate["position"] = {wp->position.x, wp->position.y, wp->position.z};
			wstate["stopId"] = wp->stopId;

			int j = 0;
			for (auto condition: wp->conditions) {

				if_is<Vehicle::DepartInactivity>(condition, [&](Vehicle::DepartInactivity* con) {
					int k = j++;
					wstate["conditions"][k]["type"] = "inactivity";
					wstate["conditions"][k]["seconds"] = con->seconds;
				});

				if_is<Vehicle::DepartItem>(condition, [&](Vehicle::DepartItem* con) {
					if (con->iid) {
						int k = j++;
						wstate["conditions"][k]["type"] = "item";
						wstate["conditions"][k]["item"] = Save::itemOut(con->iid);
						wstate["conditions"][k]["op"] = con->op;
						wstate["conditions"][k]["count"] = con->count;
					}
				});
			}

			state["waypoints"][i++] = wstate;
		}

		out << state << "\n";
	}

	out.close();
}

void Vehicle::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/vehicles.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Vehicle& vehicle = get(state["id"]);
		vehicle.pause = state["pause"];
		vehicle.patrol = state["patrol"];
		vehicle.handbrake = state["handbrake"];

		for (auto array: state["path"]) {
			vehicle.path.push_back(Point(array[0], array[1], array[2]));
		}

		int i = 0;
		for (auto wstate: state["waypoints"]) {
			uint stopId = wstate["stopId"];

			if (stopId) {
				auto wp = vehicle.addWaypoint(stopId);

				for (auto cstate: wstate["conditions"]) {
					std::string type = cstate["type"];

					if (type == "inactivity") {
						auto con = new Vehicle::DepartInactivity();
						con->seconds = cstate["seconds"];
						wp->conditions.push_back(con);
					}

					if (type == "item") {
						auto con = new Vehicle::DepartItem();
						con->iid = Save::itemIn(cstate["item"]);
						con->op = cstate["op"];
						con->count = cstate["count"];
						wp->conditions.push_back(con);
					}

				}
			} else {
				auto pos = wstate["position"];
				vehicle.addWaypoint(Point(pos[0], pos[1], pos[2]));
			}

			if (i++ == state["waypoint"]) {
				vehicle.waypoint = vehicle.waypoints.back();
			}
		}
	}

	in.close();
}

void Cart::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/carts.json");

	for (auto& cart: all) {
		json state;

		state["id"] = cart.id;

		switch (cart.state) {
			case Cart::State::Start : state["state"] = "start" ; break;
			case Cart::State::Travel: state["state"] = "travel"; break;
			case Cart::State::Stop  : state["state"] = "stop"  ; break;
			case Cart::State::Ease  : state["state"] = "ease"  ; break;
		}

		state["target"] = {cart.target.x, cart.target.y, cart.target.z};
		state["next"] = {cart.next.x, cart.next.y, cart.next.z};
		state["line"] = cart.line;
		state["wait"] = cart.wait;
		state["pause"] = cart.pause;
		state["lost"] = cart.lost;
		state["halt"] = cart.halt;
		state["blocked"] = cart.blocked;

		if (cart.signal.valid()) {
			state["signal"] = cart.signal.serialize();
		}

		int i = 0;
		for (auto cid: cart.colliders) {
			state["colliders"][i++] = cid;
		}

		out << state << "\n";
	}

	out.close();
}

void Cart::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/carts.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Cart& cart = get(state["id"]);
		if (state["state"] == "start") cart.state = Cart::State::Start;
		if (state["state"] == "travel") cart.state = Cart::State::Travel;
		if (state["state"] == "stop") cart.state = Cart::State::Stop;
		if (state["state"] == "ease") cart.state = Cart::State::Ease;
		cart.target = Point(state["target"][0], state["target"][1], state["target"][2]);
		cart.next = Point(state["next"][0], state["next"][1], state["next"][2]);
		cart.line = state["line"];
		cart.wait = state["wait"];
		cart.pause = state["pause"];

		if (state.contains("lost")) cart.lost = state["lost"];
		if (state.contains("halt")) cart.halt = state["halt"];
		if (state.contains("blocked")) cart.blocked = state["blocked"];

		if (state.contains("signal")) {
			cart.signal = Signal::unserialize(state["signal"]);
		}

		for (auto cid: state["colliders"]) {
			cart.colliders.push_back(cid);
		}
	}

	in.close();
}

void CartStop::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/cart-stops.json");

	for (auto& stop: all) {
		json state;

		state["id"] = stop.id;

		switch (stop.depart) {
			case CartStop::Depart::Inactivity: state["depart"] = "inactivity"; break;
			case CartStop::Depart::Empty: state["depart"] = "empty"; break;
			case CartStop::Depart::Full: state["depart"] = "full"; break;
		}

		out << state << "\n";
	}

	out.close();
}

void CartStop::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/cart-stops.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		CartStop& stop = get(state["id"]);

		stop.depart = CartStop::Depart::Inactivity;
		if (state["depart"] == "empty") stop.depart = CartStop::Depart::Empty;
		if (state["depart"] == "full") stop.depart = CartStop::Depart::Full;
	}

	in.close();
}

void CartWaypoint::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/cart-waypoints.json");

	for (auto& waypoint: all) {
		json state;

		state["id"] = waypoint.id;
		state["red"] = {waypoint.relative[CartWaypoint::Red].x, waypoint.relative[CartWaypoint::Red].y, waypoint.relative[CartWaypoint::Red].z};
		state["blue"] = {waypoint.relative[CartWaypoint::Blue].x, waypoint.relative[CartWaypoint::Blue].y, waypoint.relative[CartWaypoint::Blue].z};
		state["green"] = {waypoint.relative[CartWaypoint::Green].x, waypoint.relative[CartWaypoint::Green].y, waypoint.relative[CartWaypoint::Green].z};

		int i = 0;
		for (auto& redirection: waypoint.redirections) {
			if (redirection.condition.valid()) {
				state["redirections"][i][0] = redirection.condition.serialize();
				state["redirections"][i][1] = redirection.line;
				i++;
			}
		}

		out << state << "\n";
	}

	out.close();
}

void CartWaypoint::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/cart-waypoints.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		CartWaypoint& waypoint = get(state["id"]);
		waypoint.relative[CartWaypoint::Red] = {state["red"][0], state["red"][1], state["red"][2]};
		waypoint.relative[CartWaypoint::Blue] = {state["blue"][0], state["blue"][1], state["blue"][2]};
		waypoint.relative[CartWaypoint::Green] = {state["green"][0], state["green"][1], state["green"][2]};

		for (auto rstate: state["redirections"]) {
			waypoint.redirections.push_back((CartWaypoint::Redirection){
				.condition = Signal::Condition::unserialize(rstate[0]),
				.line = rstate[1],
			});
		}
	}

	in.close();
}

void Arm::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/arms.json");

	for (auto& arm: all) {

		json state;
		state["id"] = arm.id;
		state["item"] = Save::itemOut(arm.iid);
		state["orientation"] = arm.orientation;
		state["stage"] = arm.stage;
		state["pause"] = arm.pause;

		state["io"][0] = arm.inputNear;
		state["io"][1] = arm.inputFar;
		state["io"][2] = arm.outputNear;
		state["io"][3] = arm.outputFar;

		int i = 0;
		for (uint iid: arm.filter) {
			state["filter"][i++] = Save::itemOut(iid);
		}

		switch (arm.monitor) {
			case Monitor::InputStore: {
				break;
			}
			case Monitor::OutputStore: {
				state["monitor"] = "outputstore";
				break;
			}
			case Monitor::Network: {
				state["monitor"] = "network";
				break;
			}
		}

		if (arm.condition.valid()) {
			state["condition"] = arm.condition.serialize();
		}

		out << state << "\n";
	}

	out.close();
}

void Arm::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/arms.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Arm& arm = get(state["id"]);
		arm.iid = Save::itemIn(state["item"]);
		arm.orientation = state["orientation"];
		arm.stage = state["stage"];
		arm.pause = state["pause"];

		if (state.contains("io")) {
			arm.inputNear = state["io"][0];
			arm.inputFar = state["io"][1];
			arm.outputNear = state["io"][2];
			arm.outputFar = state["io"][3];
		}

		for (std::string name: state["filter"]) {
			arm.filter.insert(Save::itemIn(name));
		}

		if (state.contains("monitor")) {
			if (state["monitor"] == "outputstore") {
				arm.monitor = Monitor::OutputStore;
			}
			if (state["monitor"] == "network") {
				arm.monitor = Monitor::Network;
			}
		}

		if (state.contains("condition")) {
			arm.condition = Signal::Condition::unserialize(state["condition"]);
		}
	}

	in.close();
}

void Crafter::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/crafters.json");

	for (auto& crafter: all) {

		json state;
		state["id"] = crafter.id;
		state["working"] = crafter.working;
		state["progress"] = crafter.progress;
		state["completed"] = crafter.completed;
		state["transmit"] = crafter.transmit;
		if (crafter.recipe) state["recipe"] = crafter.recipe->name;
		if (crafter.changeRecipe) state["changeRecipe"] = crafter.changeRecipe->name;

		out << state << "\n";
	}

	out.close();
}

void Crafter::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/crafters.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Crafter& crafter = get(state["id"]);
		crafter.working = state["working"];
		crafter.progress = state["progress"];
		crafter.completed = state["completed"];
		crafter.recipe = state["recipe"].is_null() ? NULL: Recipe::byName(state["recipe"]);
		crafter.changeRecipe = state["changeRecipe"].is_null() ? NULL: Recipe::byName(state["changeRecipe"]);

		if (state.contains("transmit")) {
			crafter.transmit = state["transmit"];
		}

		if (crafter.recipe && crafter.working) {
			crafter.energyUsed = crafter.recipe->energyUsage * crafter.progress;
		}
	}

	in.close();
}

void Depot::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/depots.json");

	for (auto& depot: all) {

		json state;
		state["id"] = depot.id;
		state["construction"] = depot.construction;
		state["deconstruction"] = depot.deconstruction;
		state["network"] = depot.network;

		int i = 0;
		for (uint did: depot.drones) {
			state["drones"][i++] = did;
		}

		out << state << "\n";
	}

	out.close();
}

void Depot::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/depots.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Depot& depot = get(state["id"]);
		depot.construction = state["construction"];
		depot.deconstruction = state["deconstruction"];
		depot.network = state["network"];

		for (uint did: state["drones"]) {
			depot.drones.insert(did);
		}
	}

	in.close();
}

void Drone::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/drones.json");

	for (auto& drone: all) {

		json state;
		state["id"] = drone.id;
		state["dep"] = drone.dep;
		state["src"] = drone.src;
		state["dst"] = drone.dst;
		state["srcGhost"] = drone.srcGhost;
		state["dstGhost"] = drone.dstGhost;
		state["stage"] = drone.stage;
		state["stack"] = { Item::get(drone.stack.iid)->name, drone.stack.size };
		state["altitude"] = drone.altitude;
		state["range"] = drone.range;
		state["repairing"] = drone.repairing;

		out << state << "\n";
	}

	out.close();
}

void Drone::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/drones.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Drone& drone = get(state["id"]);
		drone.dep = state["dep"];
		drone.src = state["src"];
		drone.dst = state["dst"];
		drone.srcGhost = state["srcGhost"];
		drone.dstGhost = state["dstGhost"];
		drone.stage = state["stage"];
		drone.stack = { Item::byName(state["stack"][0])->id, state["stack"][1] };

		if (state.contains("altitude")) {
			drone.altitude = state["altitude"];
		}

		if (state.contains("range")) {
			drone.range = state["range"];
		}

		if (state.contains("repairing")) {
			drone.repairing = state["repairing"];
		}

		if (drone.stage == Drone::ToDst) {
			drone.iid = drone.stack.iid;
		}

		if (drone.repairing) {
			Entity::repairActions[drone.dst] = drone.id;
		}
	}

	in.close();
}

void Burner::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/burners.json");

	for (auto& burner: all) {

		json state;
		state["id"] = burner.id;
		state["energy"] = burner.energy.value;
		state["buffer"] = burner.buffer.value;

		state["store"]["sid"] = burner.store.sid;
		state["store"]["activity"] = burner.store.activity;

		int i = 0;
		for (Stack stack: burner.store.stacks) {
			state["store"]["stacks"][i++] = {
				Item::get(stack.iid)->name,
				stack.size,
			};
		}

		i = 0;
		for (auto level: burner.store.levels) {
			state["store"]["levels"][i++] = {
				Item::get(level.iid)->name,
				level.lower,
				level.upper,
			};
		}

		i = 0;
		for (uint did: burner.store.drones) {
			state["store"]["drones"][i++] = did;
		}

		out << state << "\n";
	}

	out.close();
}

void Burner::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/burners.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!Burner::all.has(state["id"])) continue;

		Burner& burner = get(state["id"]);
		burner.energy.value = state["energy"];
		burner.buffer.value = state["buffer"];

		burner.store.sid = state["store"]["sid"];
		burner.store.activity = state["store"]["activity"];

		for (auto stack: state["store"]["stacks"]) {
			burner.store.stacks.push_back({
				Item::byName(stack[0])->id,
				stack[1],
			});
		}

		for (auto level: state["store"]["levels"]) {
			burner.store.levels.push_back({
				.iid = Item::byName(level[0])->id,
				.lower = level[1],
				.upper = level[2],
			});
		}

		for (uint did: state["drones"]) {
			burner.store.drones.insert(did);
		}
	}

	in.close();
}

void Charger::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/chargers.json");

	for (auto& charger: all) {

		json state;
		state["id"] = charger.id;
		state["energy"] = charger.energy.value;
		state["buffer"] = charger.buffer.value;

		out << state << "\n";
	}

	out.close();
}

void Charger::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/chargers.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!Charger::all.has(state["id"])) continue;

		Charger& charger = get(state["id"]);
		charger.energy.value = state["energy"];
		charger.buffer.value = state["buffer"];

		charger.buffer.value = charger.en->spec->consumeChargeBuffer;
	}

	in.close();
}

void Pipe::saveAll(const char* name) {

	for (auto network: PipeNetwork::all) {
		network->cacheState();
	}

	auto path = std::string(name);
	auto out = std::ofstream(path + "/pipes.json");

	for (auto& pipe: all) {

		json state;
		state["id"] = pipe.id;
		state["cacheFid"] = pipe.cacheFid ? Fluid::get(pipe.cacheFid)->name: "";
		state["cacheTally"] = pipe.cacheTally;
		state["connections"] = pipe.connections;
		state["managed"] = pipe.managed;
		state["transmit"] = pipe.transmit;
		state["partner"] = pipe.partner;
		state["overflow"] = pipe.overflow;
		state["filter"] = pipe.filter ? Fluid::get(pipe.filter)->name: "";
		state["src"] = pipe.src;
		state["dst"] = pipe.dst;
		out << state << "\n";
	}

	out.close();
}

void Pipe::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/pipes.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Pipe& pipe = get(state["id"]);
		pipe.cacheFid = state["cacheFid"] == "" ? 0: Fluid::byName(state["cacheFid"])->id;
		pipe.cacheTally = state["cacheTally"];
		for (uint pid: state["connections"]) pipe.connections.insert(pid);
		pipe.managed = state["managed"];
		pipe.partner = state["partner"];
		pipe.overflow = state["overflow"];
		pipe.filter = state["filter"] == "" ? 0: Fluid::byName(state["filter"])->id;
		pipe.src = state["src"];
		pipe.dst = state["dst"];

		changed.insert(pipe.id);

		if (state.contains("transmit")) {
			pipe.transmit = state["transmit"];
		}

		if (pipe.transmit && pipe.managed) {
			transmitters.insert(pipe.id);
		}
	}

	in.close();
}

void Conveyor::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/conveyors.txt");

	for (auto& conveyor: unmanaged) {
		out << fmt("%u %u %u %u %d",
			conveyor.id, conveyor.prev, conveyor.next, conveyor.side, 0
		);
		for (uint i = 0; i < conveyor.left.slots; i++) {
			out << fmt(" %s %u",
				Save::itemOut(conveyor.left.items[i].iid), conveyor.left.items[i].offset
			);
		}
		for (uint i = 0; i < conveyor.right.slots; i++) {
			out << fmt(" %s %u",
				Save::itemOut(conveyor.right.items[i].iid), conveyor.right.items[i].offset
			);
		}
		out << "\n";
	}

	for (auto& link: managed) {
		auto& conveyor = get(link.id);

		out << fmt("%u %u %u %u %d",
			conveyor.id, conveyor.prev, conveyor.next, conveyor.side, 1
		);
		for (uint i = 0; i < conveyor.left.slots; i++) {
			out << fmt(" %s %u",
				Save::itemOut(conveyor.left.items[i].iid), conveyor.left.items[i].offset
			);
		}
		for (uint i = 0; i < conveyor.right.slots; i++) {
			out << fmt(" %s %u",
				Save::itemOut(conveyor.right.items[i].iid), conveyor.right.items[i].offset
			);
		}
		out << "\n";
	}

	out.close();
}

void Conveyor::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/conveyors.txt");

	for (std::string line; std::getline(in, line);) {
		uint id = 0;
		uint prev = 0;
		uint next = 0;
		uint side = 0;
		int mgd = 0;

		char name[100];
		uint offset = 0;

		int n = 0, p = 0;

		throwf(5 == std::sscanf(line.c_str(), "%u %u %u %u %d%n", &id, &prev, &next, &side, &mgd, &n), "%s", line);
		p += n;

		Conveyor& conveyor = get(id);
		conveyor.prev = prev;
		conveyor.next = next;
		conveyor.side = side;
		//conveyor.managed = managed;

		for (uint i = 0; i < conveyor.left.slots; i++) {
			throwf(2 == std::sscanf(line.c_str()+p, " %s %u%n", name, &offset, &n), "%s", line);
			p += n;
			conveyor.left.items[i].iid = Save::itemIn(name);
			conveyor.left.items[i].offset = offset;
		}

		for (uint i = 0; i < conveyor.right.slots; i++) {
			throwf(2 == std::sscanf(line.c_str()+p, " %s %u%n", name, &offset, &n), "%s", line);
			p += n;
			conveyor.right.items[i].iid = Save::itemIn(name);
			conveyor.right.items[i].offset = offset;
		}

		Conveyor::changed.insert(id);

		if (mgd) {
			ConveyorBelt* belt = new ConveyorBelt;
			ConveyorBelt::all.insert(belt);
			ConveyorBelt::changed.insert(belt);
			belt->conveyors.push_back(conveyor);
			conveyor.belt = belt;
			managed[conveyor.id].belt = belt;
			extant[conveyor.en->spec]++;

			unmanaged.erase(conveyor.id);
		}
	}

	in.close();
}

void Unveyor::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/unveyors.txt");

	for (auto& unveyor: all) {
		out << fmt("%u %u",
			unveyor.id, unveyor.partner
		);
		if (unveyor.entry) {
			out << fmt(" %s %s", Save::itemOut(unveyor.left), Save::itemOut(unveyor.right));
		}
		out << "\n";
	}

	out.close();
}

void Unveyor::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/unveyors.txt");

	std::map<uint,std::string> lines;

	for (std::string line; std::getline(in, line);) {
		uint id = 0;
		int partner = 0;

		int n = 0;
		throwf(2 == std::sscanf(line.c_str(), "%u %u%n", &id, &partner, &n), "%s", line);

		Unveyor& unveyor = get(id);
		unveyor.partner = partner;

		lines[id] = line.substr(n, std::string::npos);
	}

	for (auto [id,line]: lines) {
		char left[100];
		char right[100];

		Unveyor& unveyor = get(id);
		if (!unveyor.partner) continue;
		if (!unveyor.entry) continue;

		unveyor.link();

		throwf(2 == std::sscanf(line.c_str(), " %s %s", left, right), "%s", line);
		unveyor.left = Save::itemIn(left);
		unveyor.right = Save::itemIn(right);
	}

	in.close();
}

void Loader::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/loaders.json");

	for (auto& loader: all) {

		json state;
		state["id"] = loader.id;
		state["storeId"] = loader.storeId;
		state["pause"] = loader.pause;
		state["ignore"] = loader.ignore;

		int i = 0;
		for (uint iid: loader.filter) {
			state["filter"][i++] = Item::get(iid)->name;
		}

		switch (loader.monitor) {
			case Monitor::Store: {
				break;
			}
			case Monitor::Network: {
				state["monitor"] = "network";
				break;
			}
		}

		if (loader.condition.valid()) {
			state["condition"] = loader.condition.serialize();
		}

		out << state << "\n";
	}

	out.close();
}

void Loader::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/loaders.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		Loader& loader = get(state["id"]);
		loader.storeId = state["storeId"];
		loader.pause = state["pause"];

		if (state.contains("ignore")) {
			loader.ignore = state["ignore"];
		}

		for (std::string name: state["filter"]) {
			loader.filter.insert(Item::byName(name)->id);
		}

		if (state.contains("monitor")) {
			if (state["monitor"] == "network") {
				loader.monitor = Monitor::Network;
			}
		}

		if (state.contains("condition")) {
			loader.condition = Signal::Condition::unserialize(state["condition"]);
		}
	}

	in.close();
}

void Balancer::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/balancers.json");

	for (auto& balancer: all) {
		json state;
		state["id"] = balancer.id;

		if (balancer.priority.input || balancer.priority.output) {
			state["priority"]["input"] = balancer.priority.input;
			state["priority"]["output"] = balancer.priority.output;
		}

		int i = 0;
		for (uint iid: balancer.filter) {
			state["filter"][i++] = Save::itemOut(iid);
		}

		out << state << "\n";
	}

	out.close();
}

void Balancer::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/balancers.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Balancer& balancer = get(state["id"]);

		if (state.contains("priority")) {
			balancer.priority.input = state["priority"]["input"];
			balancer.priority.output = state["priority"]["output"];
		}

		for (std::string name: state["filter"]) {
			balancer.filter.insert(Save::itemIn(name));
		}
	}

	in.close();

	for (auto& balancer: all) {
		if (!balancer.en->isGhost())
			balancer.manage();
	}
}

void Computer::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/computers.json");

	for (auto& computer: all) {

		json state;
		state["id"] = computer.id;
		state["ip"] = computer.ip;

		for (uint i = 0; i < computer.ds.size(); i++) state["ds"][i] = computer.ds[i];
		for (uint i = 0; i < computer.rs.size(); i++) state["rs"][i] = computer.rs[i];
		for (uint i = 0; i < computer.ram.size(); i++) state["ram"][i] = computer.ram[i];
		for (uint i = 0; i < computer.rom.size(); i++) state["rom"][i] = computer.rom[i];
		for (uint i = 0; i < computer.code.size(); i++) state["code"][i] = { (int)computer.code[i].opcode, computer.code[i].value };

		switch (computer.err) {
			case Computer::Ok:
				state["err"] = "Ok";
				break;
			case Computer::What:
				state["err"] = "What";
				break;
			case Computer::StackUnderflow:
				state["err"] = "StackUnderflow";
				break;
			case Computer::StackOverflow:
				state["err"] = "StackOverflow";
				break;
			case Computer::OutOfBounds:
				state["err"] = "OutOfBounds";
				break;
			case Computer::Syntax:
				state["err"] = "Syntax";
				break;
		}

		state["log"] = computer.log;
		state["source"] = computer.source;
		state["debug"] = computer.debug;

		int i = 0;
		for (auto& signal: computer.env) {
			state["env"][i++] = signal.serialize();
		}

		out << state << "\n";
	}

	out.close();
}

void Computer::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/computers.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Computer& computer = get(state["id"]);
		computer.ip = state["ip"];

		for (auto& n: state["ds"]) computer.ds.push(n);
		for (auto& n: state["rs"]) computer.rs.push(n);
		{ int i = 0; for (auto& n: state["ram"]) computer.ram[i++] = n; }
		{ int i = 0; for (auto& n: state["rom"]) computer.rom[i++] = n; }

		for (auto& instruction: state["code"]) {
			computer.code.push(Computer::Instruction((Computer::Opcode)instruction[0], (int)instruction[1]));
		}

		if (state["err"] == "Ok")
			computer.err = Computer::Ok;

		if (state["err"] == "What")
			computer.err = Computer::What;

		if (state["err"] == "StackUnderflow")
			computer.err = Computer::StackUnderflow;

		if (state["err"] == "StackOverflow")
			computer.err = Computer::StackOverflow;

		if (state["err"] == "OutOfBounds")
			computer.err = Computer::OutOfBounds;

		if (state["err"] == "Syntax")
			computer.err = Computer::Syntax;

		computer.log = state["log"];
		computer.source = state["source"];

		if (state.contains("debug")) {
			computer.debug = state["debug"];
		}

		for (auto signal: state["env"]) {
			computer.env.push_back(Signal::unserialize(signal));
		}

		computer.reboot();
	}

	in.close();
}

void Router::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/routers.json");

	for (auto& router: all) {
		json state;
		state["id"] = router.id;

		int i = 0;
		for (auto& rule: router.rules) {
			state["rules"][i]["condition"] = rule.condition.serialize();
			state["rules"][i]["signal"] = rule.signal.serialize();
			state["rules"][i]["nicSrc"] = rule.nicSrc;
			state["rules"][i]["nicDst"] = rule.nicDst;
			state["rules"][i]["icon"] = rule.icon;
			switch (rule.mode) {
				case Router::Rule::Mode::Forward:
					state["rules"][i]["mode"] = "forward";
					break;
				case Router::Rule::Mode::Generate:
					state["rules"][i]["mode"] = "generate";
					break;
				case Router::Rule::Mode::Alert:
					state["rules"][i]["mode"] = "alert";
					break;
			}
			i++;
		}

		out << state << "\n";
	}

	out.close();
}

void Router::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/routers.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Router& router = get(state["id"]);

		for (auto entry: state["rules"]) {
			Router::Rule rule;
			rule.condition = Signal::Condition::unserialize(entry["condition"]);
			rule.signal = Signal::unserialize(entry["signal"]);
			rule.nicSrc = entry["nicSrc"];
			rule.nicDst = entry["nicDst"];
			rule.icon = entry["icon"];
			rule.mode = Router::Rule::Mode::Forward;
			if (entry["mode"] == "generate") rule.mode = Router::Rule::Mode::Generate;
			if (entry["mode"] == "alert") rule.mode = Router::Rule::Mode::Alert;
			router.rules.push(rule);
		}
	}

	in.close();
}

void Networker::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/networkers.json");

	for (auto& networker: all) {
		if (!networker.used()) continue;

		json state;
		state["id"] = networker.id;

		int i = 0;
		for (auto& interface: networker.interfaces) {
			state["interfaces"][i]["ssid"] = interface.ssid;
			++i;
		}

		out << state << "\n";
	}

	out.close();
}

void Networker::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/networkers.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Networker& networker = get(state["id"]);

		uint i = 0;
		for (auto& interface: networker.interfaces) {
			if (state["interfaces"].size() > i) {
				interface.ssid = state["interfaces"][i]["ssid"];
			}
			++i;
		}
	}

	for (auto& networker: Networker::all) {
		if (!Entity::get(networker.id).isGhost()) {
			networker.manage();
		}
	}

	Networker::rebuild = true;

	in.close();
}

void Turret::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/turrets.json");

	for (auto& turret: all) {

		json state;
		state["id"] = turret.id;
		state["tid"] = turret.tid;
		state["aim"] = {turret.aim.x, turret.aim.y, turret.aim.z};
		state["cool"] = turret.cool;
		state["ammoId"] = Save::itemOut(turret.ammoId);
		state["ammoRounds"] = turret.ammoRounds;
		state["pause"] = turret.pause;

		out << state << "\n";
	}

	out.close();
}

void Turret::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/turrets.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Turret& turret = get(state["id"]);

		turret.tid = state["tid"];
		turret.aim = Point(state["aim"][0], state["aim"][1], state["aim"][2]);
		turret.cool = state["cool"];
		turret.ammoId = Save::itemIn(state["ammoId"]);
		turret.ammoRounds = state["ammoRounds"];
		turret.pause = state["pause"];
	}

	in.close();
}

void Missile::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/missiles.json");

	for (auto& missile: all) {

		json state;
		state["id"] = missile.id;
		state["tid"] = missile.tid;
		state["aim"] = {missile.aim.x, missile.aim.y, missile.aim.z};
		state["attacking"] = missile.attacking;

		out << state << "\n";
	}

	out.close();
}

void Missile::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/missiles.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Missile& missile = get(state["id"]);
		missile.tid = state["tid"];
		missile.aim = Point(state["aim"][0], state["aim"][1], state["aim"][2]);
		missile.attacking = state["attacking"];
	}

	in.close();
}

void FlightLogistic::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/flight-logistics.json");

	for (auto& logi: all) {
		json state;
		state["id"] = logi.id;
		state["dep"] = logi.dep;
		state["src"] = logi.src;
		state["dst"] = logi.dst;

		switch (logi.stage) {
			case Home: {
				state["stage"] = "Home";
				break;
			}
			case ToSrc: {
				state["stage"] = "ToSrc";
				break;
			}
			case ToSrcDeparting: {
				state["stage"] = "ToSrcDeparting";
				break;
			}
			case ToSrcFlight: {
				state["stage"] = "ToSrcFlight";
				break;
			}
			case Loading: {
				state["stage"] = "Loading";
				break;
			}
			case ToDst: {
				state["stage"] = "ToDst";
				break;
			}
			case ToDstDeparting: {
				state["stage"] = "ToDstDeparting";
				break;
			}
			case ToDstFlight: {
				state["stage"] = "ToDstFlight";
				break;
			}
			case Unloading: {
				state["stage"] = "Unloading";
				break;
			}
			case ToDep: {
				state["stage"] = "ToDep";
				break;
			}
			case ToDepDeparting: {
				state["stage"] = "ToDepDeparting";
				break;
			}
			case ToDepFlight: {
				state["stage"] = "ToDepFlight";
				break;
			}
		}

		out << state << "\n";
	}

	out.close();
}

void FlightLogistic::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/flight-logistics.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		FlightLogistic& logi = get(state["id"]);
		logi.dep = state["dep"];
		logi.src = state["src"];
		logi.dst = state["dst"];

		if (std::string(state["stage"]) == "Home") {
			logi.stage = Home;
		}
		if (std::string(state["stage"]) == "ToSrc") {
			logi.stage = ToSrc;
		}
		if (std::string(state["stage"]) == "ToSrcDeparting") {
			logi.stage = ToSrcDeparting;
		}
		if (std::string(state["stage"]) == "ToSrcFlight") {
			logi.stage = ToSrcFlight;
		}
		if (std::string(state["stage"]) == "Loading") {
			logi.stage = Loading;
		}
		if (std::string(state["stage"]) == "ToDst") {
			logi.stage = ToDst;
		}
		if (std::string(state["stage"]) == "ToDstDeparting") {
			logi.stage = ToDstDeparting;
		}
		if (std::string(state["stage"]) == "ToDstFlight") {
			logi.stage = ToDstFlight;
		}
		if (std::string(state["stage"]) == "Unloading") {
			logi.stage = Unloading;
		}
		if (std::string(state["stage"]) == "ToDep") {
			logi.stage = ToDep;
		}
		if (std::string(state["stage"]) == "ToDepDeparting") {
			logi.stage = ToDepDeparting;
		}
		if (std::string(state["stage"]) == "ToDepFlight") {
			logi.stage = ToDepFlight;
		}
	}

	in.close();
}

void FlightPad::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/flight-pads.json");

	for (auto& pad: all) {
		json state;
		state["id"] = pad.id;
		state["claimId"] = pad.claimId;
		out << state << "\n";
	}

	out.close();
}

void FlightPad::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/flight-pads.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		FlightPad& pad = get(state["id"]);
		pad.claimId = state["claimId"];
	}

	in.close();
}

void FlightPath::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/flight-paths.json");

	for (auto& flight: all) {
		json state;
		state["id"] = flight.id;
		state["moving"] = flight.moving;
		state["request"] = flight.request;
		state["fueled"] = flight.fueled;
		state["step"] = flight.step;
		state["speed"] = flight.speed;
		state["destination"] = {flight.destination.x, flight.destination.y, flight.destination.z};
		state["arrived"] = flight.arrived;
		state["origin"] = {flight.origin.x, flight.origin.y, flight.origin.z};
		state["departed"] = flight.departed;
		state["orient"] = {flight.orient.x, flight.orient.y, flight.orient.z};

		int i = 0;
		for (auto& block: flight.path.blocks) {
			state["blocks"][i++] = {block->x,block->y,block->z};
		}

		i = 0;
		for (auto& waypoint: flight.path.waypoints) {
			state["waypoints"][i++] = {waypoint.x,waypoint.y,waypoint.z};
		}

		out << state << "\n";
	}

	out.close();
}

void FlightPath::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/flight-paths.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		FlightPath& flight = get(state["id"]);

		flight.moving = state["moving"];
		flight.request = state["request"];
		flight.fueled = state["fueled"];
		flight.step = state["step"];
		flight.speed = state["speed"];
		flight.destination = {state["destination"][0], state["destination"][1], state["destination"][2]};
		flight.arrived = state["arrived"];
		flight.origin = {state["origin"][0], state["origin"][1], state["origin"][2]};
		flight.departed = state["departed"];
		flight.orient = {state["orient"][0], state["orient"][1], state["orient"][2]};

		for (auto& b: state["blocks"]) {
			flight.path.blocks.push_back(sky.get(b[0], b[1], b[2]));
		}

		for (auto& w: state["waypoints"]) {
			flight.path.waypoints.push_back(Point(w[0], w[1], w[2]));
		}
	}

	in.close();
}

void Tube::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/tubes.json");

	for (auto& tube: all) {
		json state;
		state["id"] = tube.id;
		state["next"] = tube.next;
		state["length"] = tube.length;

		if (tube.accepted) {
			state["accepted"] = Save::itemOut(tube.accepted);
		}

		auto mode = [&](Mode m) {
			switch (m) {
				case Mode::BeltOrTube: return "belt-or-tube";
				case Mode::BeltOnly: return "belt-only";
				case Mode::TubeOnly: return "tube-only";
				case Mode::BeltPriority: return "belt-priority";
				case Mode::TubePriority: return "tube-priority";
			}
			return "belt-or-tube";
		};

		state["input"] = mode(tube.input);
		state["output"] = mode(tube.output);

		int i = 0;
		for (auto& thing: tube.stuff) {
			state["stuff"][i][0] = Save::itemOut(thing.iid);
			state["stuff"][i][1] = thing.offset;
			i++;
		}

		out << state << "\n";
	}

	out.close();
}

void Tube::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/tubes.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Tube& tube = get(state["id"]);
		tube.next = state["next"];
		tube.length = state["length"];

		// corrupted saves 0.2.7
		if (state.contains("accepted") && state["accepted"].is_string() && Item::names.count(state["accepted"])) {
			tube.accepted = Save::itemIn(state["accepted"]);
		}

		auto mode = [&](std::string m) {
			if (m == "belt-only") return Mode::BeltOnly;
			if (m == "tube-only") return Mode::TubeOnly;
			if (m == "belt-priority") return Mode::BeltPriority;
			if (m == "tube-priority") return Mode::TubePriority;
			return Mode::BeltOrTube;
		};

		if (state.contains("input")) {
			tube.input = mode(state["input"]);
		}

		if (state.contains("output")) {
			tube.output = mode(state["output"]);
		}

		if (state.contains("beltInput")) {
			bool beltInput = state["beltInput"];
			if (!beltInput) tube.input = Mode::TubeOnly;
		}

		if (state.contains("beltOutput")) {
			bool beltOutput = state["beltOutput"];
			if (!beltOutput) tube.output = Mode::TubeOnly;
		}

		for (auto& pair: state["stuff"]) {
			tube.stuff.push_back({
				.iid = Save::itemIn(pair[0]),
				.offset = pair[1],
			});
		}
	}

	in.close();
}

void Monorail::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/monorails.json");

	for (auto& monorail: all) {
		json state;
		state["id"] = monorail.id;

		for (int i = 0; i < 3; i++) {
			state["out"][i] = monorail.out[i];
		}

		int i = 0;
		for (auto imid: monorail.in) {
			state["in"][i++] = imid;
		}

		state["claimer"] = monorail.claimer;
		state["claimed"] = monorail.claimed;
		state["filling"] = monorail.filling;
		state["emptying"] = monorail.emptying;
		state["transmit"]["contents"] = monorail.transmit.contents;

		i = 0;
		for (auto& container: monorail.containers) {
			state["containers"][i]["cid"] = container.cid;
			switch (container.state) {
				case Container::Unloaded: {
					state["containers"][i]["state"] = "unloaded";
					break;
				}
				case Container::Descending: {
					state["containers"][i]["state"] = "descending";
					break;
				}
				case Container::Empty: {
					state["containers"][i]["state"] = "empty";
					break;
				}
				case Container::Shunting: {
					state["containers"][i]["state"] = "shunting";
					break;
				}
				case Container::Fill: {
					state["containers"][i]["state"] = "fill";
					break;
				}
				case Container::Ascending: {
					state["containers"][i]["state"] = "ascending";
					break;
				}
				case Container::Loaded: {
					state["containers"][i]["state"] = "loaded";
					break;
				}
			}
			i++;
		}

		i = 0;
		for (auto& redirection: monorail.redirections) {
			if (redirection.condition.valid()) {
				state["redirections"][i][0] = redirection.condition.serialize();
				state["redirections"][i][1] = redirection.line;
				i++;
			}
		}

		out << state << "\n";
	}

	out.close();
}

void Monorail::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/monorails.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Monorail& monorail = get(state["id"]);

		for (int i = 0; i < 3; i++) {
			monorail.out[i] = (int)state["out"].size() > i ? (int)state["out"][i]: 0;
		}

		for (auto imid: state["in"]) {
			monorail.in.insert(imid);
		}

		if (state.contains("claimer")) monorail.claimer = state["claimer"];
		if (state.contains("claimed")) monorail.claimed = state["claimed"];
		if (state.contains("filling")) monorail.filling = state["filling"];
		if (state.contains("emptying")) monorail.emptying = state["emptying"];

		if (state.contains("transmit")) {
			monorail.transmit.contents = state["transmit"]["contents"];
		}

		if (state.contains("containers")) {
			for (auto& container: state["containers"]) {
				uint cid = container["cid"];

				auto cstate = Container::Unloaded;

				if (container["state"] == "descending")
					cstate = Container::Descending;
				if (container["state"] == "empty")
					cstate = Container::Empty;
				if (container["state"] == "shunting")
					cstate = Container::Shunting;
				if (container["state"] == "fill")
					cstate = Container::Fill;
				if (container["state"] == "ascending")
					cstate = Container::Ascending;
				if (container["state"] == "loaded")
					cstate = Container::Loaded;

				ensure(cid);

				if (!Entity::exists(cid)) continue;

				monorail.containers.push_back({
					.state = cstate,
					.cid = cid,
					.en = &Entity::get(cid),
					.store = &Store::get(cid),
				});
			}
		}

		for (auto rstate: state["redirections"]) {
			monorail.redirections.push_back((Monorail::Redirection){
				.condition = Signal::Condition::unserialize(rstate[0]),
				.line = rstate[1],
			});
		}
	}

	in.close();
}

void Monocar::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/monocars.json");

	for (auto& monocar: all) {
		json state;
		state["id"] = monocar.id;
		state["tower"] = monocar.tower;
		state["container"] = monocar.container;
		state["speed"] = monocar.speed;

		switch (monocar.state) {
			case State::Start:
				state["state"] = "start";
				break;
			case State::Acquire:
				state["state"] = "acquire";
				break;
			case State::Travel:
				state["state"] = "travel";
				break;
			case State::Stop:
				state["state"] = "stop";
				break;
			case State::Unload:
				state["state"] = "unload";
				break;
			case State::Unloading:
				state["state"] = "unloading";
				break;
			case State::Load:
				state["state"] = "load";
				break;
			case State::Loading:
				state["state"] = "loading";
				break;
		}

		state["dirArrive"] = {monocar.dirArrive.x, monocar.dirArrive.y, monocar.dirArrive.z};
		state["dirDepart"] = {monocar.dirDepart.x, monocar.dirDepart.y, monocar.dirDepart.z};

		state["bogieA"]["pos"] = {monocar.bogieA.pos.x, monocar.bogieA.pos.y, monocar.bogieA.pos.z};
		state["bogieA"]["dir"] = {monocar.bogieA.dir.x, monocar.bogieA.dir.y, monocar.bogieA.dir.z};
		state["bogieA"]["arrive"] = {monocar.bogieA.arrive.x, monocar.bogieA.arrive.y, monocar.bogieA.arrive.z};
		state["bogieA"]["step"] = monocar.bogieA.step;

		state["bogieB"]["pos"] = {monocar.bogieB.pos.x, monocar.bogieB.pos.y, monocar.bogieB.pos.z};
		state["bogieB"]["dir"] = {monocar.bogieB.dir.x, monocar.bogieB.dir.y, monocar.bogieB.dir.z};
		state["bogieB"]["arrive"] = {monocar.bogieB.arrive.x, monocar.bogieB.arrive.y, monocar.bogieB.arrive.z};
		state["bogieB"]["step"] = monocar.bogieB.step;

		state["rail"] = {
			{monocar.rail.p0.x, monocar.rail.p0.y, monocar.rail.p0.z},
			{monocar.rail.p1.x, monocar.rail.p1.y, monocar.rail.p1.z},
			{monocar.rail.p2.x, monocar.rail.p2.y, monocar.rail.p2.z},
			{monocar.rail.p3.x, monocar.rail.p3.y, monocar.rail.p3.z},
		};

		state["line"] = monocar.line;

		int i = 0;
		for (auto& signal: monocar.constants) {
			if (signal.valid())
				state["constants"][i++] = signal.serialize();
		}

		out << state << "\n";
	}

	out.close();
}

void Monocar::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/monocars.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Monocar& monocar = get(state["id"]);
		monocar.tower = state["tower"];
		monocar.speed = state["speed"];

		if (state.contains("container"))
			monocar.container = state["container"];

		monocar.state = State::Start;

		if (state["state"] == "acquire")
			monocar.state = State::Acquire;
		if (state["state"] == "travel")
			monocar.state = State::Travel;
		if (state["state"] == "stop")
			monocar.state = State::Stop;
		if (state["state"] == "unload")
			monocar.state = State::Unload;
		if (state["state"] == "unloading")
			monocar.state = State::Unloading;
		if (state["state"] == "load")
			monocar.state = State::Load;
		if (state["state"] == "loading")
			monocar.state = State::Loading;

		monocar.dirArrive = {state["dirArrive"][0],state["dirArrive"][1],state["dirArrive"][2]};
		monocar.dirDepart = {state["dirDepart"][0],state["dirDepart"][1],state["dirDepart"][2]};

		monocar.bogieA.pos = {state["bogieA"]["pos"][0],state["bogieA"]["pos"][1],state["bogieA"]["pos"][2]};
		monocar.bogieA.dir = {state["bogieA"]["dir"][0],state["bogieA"]["dir"][1],state["bogieA"]["dir"][2]};
		monocar.bogieA.arrive = {state["bogieA"]["arrive"][0],state["bogieA"]["arrive"][1],state["bogieA"]["arrive"][2]};
		monocar.bogieA.step = state["bogieA"]["step"];

		monocar.bogieB.pos = {state["bogieB"]["pos"][0],state["bogieB"]["pos"][1],state["bogieB"]["pos"][2]};
		monocar.bogieB.dir = {state["bogieB"]["dir"][0],state["bogieB"]["dir"][1],state["bogieB"]["dir"][2]};
		monocar.bogieB.arrive = {state["bogieB"]["arrive"][0],state["bogieB"]["arrive"][1],state["bogieB"]["arrive"][2]};
		monocar.bogieB.step = state["bogieB"]["step"];

		monocar.rail.p0 = {state["rail"][0][0], state["rail"][0][1], state["rail"][0][2]};
		monocar.rail.p1 = {state["rail"][1][0], state["rail"][1][1], state["rail"][1][2]};
		monocar.rail.p2 = {state["rail"][2][0], state["rail"][2][1], state["rail"][2][2]};
		monocar.rail.p3 = {state["rail"][3][0], state["rail"][3][1], state["rail"][3][2]};

		monocar.steps = monocar.rail.steps(1.0);
		monocar.steps.push_back(monocar.bogieA.arrive);

		if (state.contains("line")) {
			monocar.line = state["line"];
		}

		if (state.contains("signal")) {
			monocar.constants = {Signal::unserialize(state["signal"])};
		}

		if (state.contains("constants")) {
			for (auto& sig: state["constants"])
				monocar.constants.push(Signal::unserialize(sig));
		}
	}

	in.close();
}

void Teleporter::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/teleporters.json");

	for (auto& teleporter: all) {
		json state;
		state["id"] = teleporter.id;
		state["working"] = teleporter.working;
		state["trigger"] = teleporter.trigger;
		state["progress"] = teleporter.progress;
		state["energyUsed"] = teleporter.energyUsed.value;
		state["completed"] = teleporter.completed;
		state["partner"] = teleporter.partner;

		int i = 0;
		for (auto stack: teleporter.shipment) {
			state["shipment"][i++] = {Save::itemOut(stack.iid), stack.size};
		}

		out << state << "\n";
	}

	out.close();
}

void Teleporter::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/teleporters.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Teleporter& teleporter = get(state["id"]);

		teleporter.working = state["working"];
		teleporter.trigger = state["trigger"];
		teleporter.progress = state["progress"];
		teleporter.energyUsed.value = state["energyUsed"];
		teleporter.completed = state["completed"];
		teleporter.partner = state["partner"];

		for (auto stack: state["shipment"]) {
			teleporter.shipment.push_back({Save::itemIn(stack[0]), stack[1]});
		}
	}

	in.close();
}

void Effector::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/effectors.json");

	for (auto& effector: all) {
		json state;
		state["id"] = effector.id;
		state["tid"] = effector.tid;
		out << state << "\n";
	}

	out.close();
}

void Effector::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/effectors.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Effector& effector = get(state["id"]);
		effector.tid = state["tid"];
		if (effector.tid) assist[effector.tid].insert(effector.id);
	}

	in.close();
}

void Launcher::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/launchers.json");

	for (auto& launcher: all) {
		json state;
		state["id"] = launcher.id;
		state["working"] = launcher.working;
		state["activate"] = launcher.activate;
		state["completed"] = launcher.completed;
		state["progress"] = launcher.progress;

		int i = 0;
		for (auto iid: launcher.cargo) {
			state["cargo"][i++] = Save::itemOut(iid);
		}

		switch (launcher.monitor) {
			case Monitor::Network: {
				state["monitor"] = "network";
				break;
			}
		}

		if (launcher.condition.valid()) {
			state["condition"] = launcher.condition.serialize();
		}

		out << state << "\n";
	}

	out.close();
}

void Launcher::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/launchers.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		Launcher& launcher = get(state["id"]);
		launcher.working = state["working"];
		launcher.activate = state["activate"];
		launcher.completed = state["completed"];
		launcher.progress = state["progress"];

		for (auto name: state["cargo"]) {
			launcher.cargo.insert(Save::itemIn(name));
		}

		if (state.contains("monitor")) {
			if (state["monitor"] == "network") {
				launcher.monitor = Monitor::Network;
			}
		}

		if (state.contains("condition")) {
			launcher.condition = Signal::Condition::unserialize(state["condition"]);
		}
	}

	in.close();
}

void PowerPole::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/powerpoles.json");

	for (auto& pole: all) {
		json state;
		state["id"] = pole.id;
		state["managed"] = pole.managed;

		int i = 0;
		for (auto sid: pole.links) {
			state["links"][i++] = sid;
		}

		out << state << "\n";
	}

	out.close();
}

void PowerPole::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/powerpoles.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		PowerPole& pole = get(state["id"]);
		pole.managed = state["managed"];

		for (auto sid: state["links"]) {
			pole.links.push_back(sid);
		}

		if (pole.managed) {
			PowerPole::gridCoverage.insert(pole.coverage(), &pole);
		}
	}

	for (auto& pole: PowerPole::all) {
		if (!pole.managed && !pole.en->isGhost()) pole.manage();
	}

	in.close();
}

void ElectricityNetwork::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/electricity-networks.json");

	for (auto& network: all) {
		if (!network.poles.size()) continue;

		json state;
		state["id"] = network.id;
		state["pole"] = network.poles.front();
		out << state << "\n";
	}

	out.close();
}

void ElectricityNetwork::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/electricity-networks.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		auto& network = ElectricityNetwork::create(state["id"]);

		auto& pole = PowerPole::get(state["pole"]);
		pole.network = &network;
		network.poles.push(pole.id);

		ElectricityNetwork::sequence = std::max(ElectricityNetwork::sequence, network.id);
	}

	ElectricityNetwork::rebuild = true;
	in.close();
}

void Shipyard::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/shipyards.json");

	for (auto& shipyard: all) {
		json state;
		state["id"] = shipyard.id;
		state["delta"] = shipyard.delta;
		state["completed"] = shipyard.completed;

		switch (shipyard.stage) {
			case Stage::Start: state["stage"] = "start";
			case Stage::Build: state["stage"] = "build";
			case Stage::RollOut: state["stage"] = "rollout";
			case Stage::Launch: state["stage"] = "launch";
			case Stage::RollIn: state["stage"] = "rollin";
		}

		if (shipyard.ship) {
			state["ship"] = shipyard.ship->name;
		}

		out << state << "\n";
	}

	out.close();
}

void Shipyard::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/shipyards.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Shipyard& shipyard = get(state["id"]);
		shipyard.delta = state["delta"];
		shipyard.completed = state["completed"];

		if (state["stage"] == "start") shipyard.stage = Stage::Start;
		if (state["stage"] == "build") shipyard.stage = Stage::Build;
		if (state["stage"] == "rollout") shipyard.stage = Stage::RollOut;
		if (state["stage"] == "launch") shipyard.stage = Stage::Launch;
		if (state["stage"] == "rollin") shipyard.stage = Stage::RollIn;

		if (state.contains("ship")) {
			shipyard.ship = Spec::byName(state["ship"]);
		}

	}

	in.close();
}

void Ship::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/ships.json");

	for (auto& ship: all) {
		json state;
		state["id"] = ship.id;

		switch (ship.stage) {
			case Stage::Start: state["stage"] = "start";
			case Stage::Lift: state["stage"] = "lift";
			case Stage::Fly: state["stage"] = "fly";
		}

		int i = 0;
		for (Point point: ship.flight) {
			state["flight"][i++] = {point.x, point.y, point.z};
		}

		out << state << "\n";
	}

	out.close();
}

void Ship::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/ships.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		if (!all.has(state["id"])) continue;

		Ship& ship = get(state["id"]);
		if (state["stage"] == "start") ship.stage = Stage::Start;
		if (state["stage"] == "lift") ship.stage = Stage::Lift;
		if (state["stage"] == "fly") ship.stage = Stage::Fly;

		for (auto point: state["flight"]) {
			ship.flight.push_back({point[0], point[1], point[2]});
		}
	}

	in.close();
}

void Goal::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/goals.json");

	for (auto [_,goal]: all) {
		json state;
		state["name"] = goal->name;
		state["met"] = goal->met;
		out << state << "\n";
	}

	out.close();
}

void Goal::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/goals.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		Goal* goal = byName(state["name"]);
		goal->met = state["met"];
	}

	in.close();
}

void Message::saveAll(const char* name) {
	auto path = std::string(name);
	auto out = std::ofstream(path + "/messages.json");

	for (auto [_,message]: all) {
		json state;
		state["name"] = message->name;
		state["sent"] = message->sent;
		out << state << "\n";
	}

	out.close();
}

void Message::loadAll(const char* name) {
	auto path = std::string(name);
	auto in = std::ifstream(path + "/messages.json");

	for (std::string line; std::getline(in, line);) {
		auto state = json::parse(line);
		Message* message = byName(state["name"]);
		message->sent = state["sent"];
	}

	in.close();
}

namespace {
	json serialize(StoreSettings* store) {
		json state;
		state["transmit"] = store->transmit;
		state["purge"] = store->purge;
		state["block"] = store->block;

		int i = 0;
		for (auto level: store->levels)
			state["levels"][i++] = {level.iid, level.lower, level.upper};

		return state;
	}

	void unserialize(StoreSettings* store, json state) {
		store->transmit = state["transmit"];
		store->purge = state["purge"];
		store->block = state["block"];

		for (auto level: state["levels"])
			store->levels.push_back({level[0], level[1], level[2]});
	}

	json serialize(CrafterSettings* crafter) {
		json state;
		state["transmit"] = crafter->transmit;

		if (crafter->recipe)
			state["recipe"] = crafter->recipe->name;

		return state;
	}

	void unserialize(CrafterSettings* crafter, json state) {
		crafter->transmit = state["transmit"];

		if (state.contains("recipe"))
			crafter->recipe = Recipe::byName(state["recipe"]);
	}

	json serialize(ArmSettings* arm) {
		json state;
		state["io"][0] = arm->inputNear;
		state["io"][1] = arm->inputFar;
		state["io"][2] = arm->outputNear;
		state["io"][3] = arm->outputFar;

		int i = 0;
		for (uint iid: arm->filter) {
			state["filter"][i++] = Save::itemOut(iid);
		}

		switch (arm->monitor) {
			case Arm::Monitor::InputStore: {
				break;
			}
			case Arm::Monitor::OutputStore: {
				state["monitor"] = "outputstore";
				break;
			}
			case Arm::Monitor::Network: {
				state["monitor"] = "network";
				break;
			}
		}

		if (arm->condition.valid()) {
			state["condition"] = arm->condition.serialize();
		}

		return state;
	}

	void unserialize(ArmSettings* arm, json state) {
		if (state.contains("io")) {
			arm->inputNear = state["io"][0];
			arm->inputFar = state["io"][1];
			arm->outputNear = state["io"][2];
			arm->outputFar = state["io"][3];
		}

		for (std::string name: state["filter"]) {
			arm->filter.insert(Save::itemIn(name));
		}

		if (state.contains("monitor")) {
			if (state["monitor"] == "outputstore") {
				arm->monitor = Arm::Monitor::OutputStore;
			}
			if (state["monitor"] == "network") {
				arm->monitor = Arm::Monitor::Network;
			}
		}

		if (state.contains("condition")) {
			arm->condition = Signal::Condition::unserialize(state["condition"]);
		}
	}

	json serialize(LoaderSettings* loader) {
		json state;

		int i = 0;
		for (uint iid: loader->filter) {
			state["filter"][i++] = Item::get(iid)->name;
		}

		switch (loader->monitor) {
			case Loader::Monitor::Store: {
				break;
			}
			case Loader::Monitor::Network: {
				state["monitor"] = "network";
				break;
			}
		}

		if (loader->condition.valid()) {
			state["condition"] = loader->condition.serialize();
		}

		return state;
	}

	void unserialize(LoaderSettings* loader, json state) {
		for (std::string name: state["filter"]) {
			loader->filter.insert(Item::byName(name)->id);
		}

		if (state.contains("monitor")) {
			if (state["monitor"] == "network") {
				loader->monitor = Loader::Monitor::Network;
			}
		}

		if (state.contains("condition")) {
			loader->condition = Signal::Condition::unserialize(state["condition"]);
		}
	}

	json serialize(BalancerSettings* balancer) {
		json state;
		if (balancer->priority.input || balancer->priority.output) {
			state["priority"]["input"] = balancer->priority.input;
			state["priority"]["output"] = balancer->priority.output;
		}

		int i = 0;
		for (uint iid: balancer->filter) {
			state["filter"][i++] = Save::itemOut(iid);
		}

		return state;
	}

	void unserialize(BalancerSettings* balancer, json state) {
		if (state.contains("priority")) {
			balancer->priority.input = state["priority"]["input"];
			balancer->priority.output = state["priority"]["output"];
		}

		for (std::string name: state["filter"]) {
			balancer->filter.insert(Save::itemIn(name));
		}
	}

	json serialize(PipeSettings* pipe) {
		json state;
		state["overflow"] = pipe->overflow;

		if (pipe->filter)
			state["filter"] = Save::fluidOut(pipe->filter);

		return state;
	}

	void unserialize(PipeSettings* pipe, json state) {
		pipe->overflow = state["overflow"];

		if (state.contains("filter"))
			pipe->filter = Save::fluidIn(state["filter"]);
	}

	json serialize(CartSettings* cart) {
		json state;
		state["line"] = cart->line;

		if (cart->signal.valid()) {
			state["signal"] = cart->signal.serialize();
		}

		return state;
	}

	void unserialize(CartSettings* cart, json state) {
		cart->line = state["line"];

		if (state.contains("signal")) {
			cart->signal = Signal::unserialize(state["signal"]);
		}
	}

	json serialize(CartStopSettings* stop) {
		json state;

		switch (stop->depart) {
			case CartStop::Depart::Inactivity: state["depart"] = "inactivity"; break;
			case CartStop::Depart::Empty: state["depart"] = "empty"; break;
			case CartStop::Depart::Full: state["depart"] = "full"; break;
		}

		if (stop->condition.valid()) {
			state["signal"] = stop->condition.serialize();
		}

		return state;
	}

	void unserialize(CartStopSettings* stop, json state) {
		stop->depart = CartStop::Depart::Inactivity;
		if (state["depart"] == "empty") stop->depart = CartStop::Depart::Empty;
		if (state["depart"] == "full") stop->depart = CartStop::Depart::Full;

		if (state.contains("condition")) {
			stop->condition = Signal::Condition::unserialize(state["signal"]);
		}
	}

	json serialize(CartWaypointSettings* waypoint) {
		json state;

		state["dir"] = {waypoint->dir.x, waypoint->dir.y, waypoint->dir.z};

		state["red"] = {waypoint->relative[CartWaypoint::Red].x, waypoint->relative[CartWaypoint::Red].y, waypoint->relative[CartWaypoint::Red].z};
		state["blue"] = {waypoint->relative[CartWaypoint::Blue].x, waypoint->relative[CartWaypoint::Blue].y, waypoint->relative[CartWaypoint::Blue].z};
		state["green"] = {waypoint->relative[CartWaypoint::Green].x, waypoint->relative[CartWaypoint::Green].y, waypoint->relative[CartWaypoint::Green].z};

		int i = 0;
		for (auto& redirection: waypoint->redirections) {
			if (redirection.condition.valid()) {
				state["redirections"][i][0] = redirection.condition.serialize();
				state["redirections"][i][1] = redirection.line;
				i++;
			}
		}

		return state;
	}

	void unserialize(CartWaypointSettings* waypoint, json state) {
		waypoint->dir = Point(
			state["dir"][0],
			state["dir"][1],
			state["dir"][2]
		);

		waypoint->relative[CartWaypoint::Red] = {state["red"][0], state["red"][1], state["red"][2]};
		waypoint->relative[CartWaypoint::Blue] = {state["blue"][0], state["blue"][1], state["blue"][2]};
		waypoint->relative[CartWaypoint::Green] = {state["green"][0], state["green"][1], state["green"][2]};

		for (auto rstate: state["redirections"]) {
			waypoint->redirections.push_back((CartWaypoint::Redirection){
				.condition = Signal::Condition::unserialize(rstate[0]),
				.line = rstate[1],
			});
		}

	}

	json serialize(NetworkerSettings* networker) {
		json state;

		int i = 0;
		for (auto& ssid: networker->ssids) {
			state["ssids"][i++] = ssid;
		}

		return state;
	}

	void unserialize(NetworkerSettings* networker, json state) {
		for (auto& ssid: state["ssids"]) {
			networker->ssids.push_back(ssid);
		}
	}

	json serialize(TubeSettings* tube) {
		json state;
		state["target"] = {
			tube->target.x,
			tube->target.y,
			tube->target.z
		};

		auto mode = [&](Tube::Mode m) {
			switch (m) {
				case Tube::Mode::BeltOrTube: return "belt-or-tube";
				case Tube::Mode::BeltOnly: return "belt-only";
				case Tube::Mode::TubeOnly: return "tube-only";
				case Tube::Mode::BeltPriority: return "belt-priority";
				case Tube::Mode::TubePriority: return "tube-priority";
			}
			return "belt-or-tube";
		};

		state["input"] = mode(tube->input);
		state["output"] = mode(tube->output);

		return state;
	}

	void unserialize(TubeSettings* tube, json state) {
		tube->target = Point(
			state["target"][0],
			state["target"][1],
			state["target"][2]
		);

		auto mode = [&](std::string m) {
			if (m == "belt-only") return Tube::Mode::BeltOnly;
			if (m == "tube-only") return Tube::Mode::TubeOnly;
			if (m == "belt-priority") return Tube::Mode::BeltPriority;
			if (m == "tube-priority") return Tube::Mode::TubePriority;
			return Tube::Mode::BeltOrTube;
		};

		if (state.contains("input")) {
			tube->input = mode(state["input"]);
		}

		if (state.contains("output")) {
			tube->output = mode(state["output"]);
		}
	}

	json serialize(MonorailSettings* monorail) {
		json state;
		state["filling"] = monorail->filling;
		state["emptying"] = monorail->emptying;
		state["transmit"]["contents"] = monorail->transmit.contents;

		state["dir"] = {monorail->dir.x, monorail->dir.y, monorail->dir.z};
		state["out"][0] = {monorail->out[0].x, monorail->out[0].y, monorail->out[0].z};
		state["out"][1] = {monorail->out[1].x, monorail->out[1].y, monorail->out[1].z};
		state["out"][2] = {monorail->out[2].x, monorail->out[2].y, monorail->out[2].z};

		int i = 0;
		for (auto& redirection: monorail->redirections) {
			if (redirection.condition.valid()) {
				state["redirections"][i][0] = redirection.condition.serialize();
				state["redirections"][i][1] = redirection.line;
				i++;
			}
		}

		return state;
	}

	void unserialize(MonorailSettings* monorail, json state) {
		monorail->filling = state["filling"];
		monorail->emptying = state["emptying"];
		monorail->transmit.contents = state["transmit"]["contents"];

		monorail->dir = Point(state["dir"][0], state["dir"][1], state["dir"][2]);
		monorail->out[0] = Point(state["out"][0][0], state["out"][0][1], state["out"][0][2]);
		monorail->out[1] = Point(state["out"][1][0], state["out"][1][1], state["out"][1][2]);
		monorail->out[2] = Point(state["out"][2][0], state["out"][2][1], state["out"][2][2]);

		for (auto rstate: state["redirections"]) {
			monorail->redirections.push_back((Monorail::Redirection){
				.condition = Signal::Condition::unserialize(rstate[0]),
				.line = rstate[1],
			});
		}
	}

	json serialize(RouterSettings* router) {
		json state;
		int i = 0;
		for (auto& rule: router->rules) {
			state["rules"][i]["condition"] = rule.condition.serialize();
			state["rules"][i]["signal"] = rule.signal.serialize();
			state["rules"][i]["nicSrc"] = rule.nicSrc;
			state["rules"][i]["nicDst"] = rule.nicDst;
			state["rules"][i]["icon"] = rule.icon;
			switch (rule.mode) {
				case Router::Rule::Mode::Forward:
					state["rules"][i]["mode"] = "forward";
					break;
				case Router::Rule::Mode::Generate:
					state["rules"][i]["mode"] = "generate";
					break;
				case Router::Rule::Mode::Alert:
					state["rules"][i]["mode"] = "alert";
					break;
			}
			i++;
		}
		return state;
	}

	void unserialize(RouterSettings* router, json state) {
		for (auto entry: state["rules"]) {
			Router::Rule rule;
			rule.condition = Signal::Condition::unserialize(entry["condition"]);
			rule.signal = Signal::unserialize(entry["signal"]);
			rule.nicSrc = entry["nicSrc"];
			rule.nicDst = entry["nicDst"];
			rule.icon = entry["icon"];
			rule.mode = Router::Rule::Mode::Forward;
			if (entry["mode"] == "generate") rule.mode = Router::Rule::Mode::Generate;
			if (entry["mode"] == "alert") rule.mode = Router::Rule::Mode::Alert;
			router->rules.push(rule);
		}
	}

	json serializeSettings(Spec* spec, Entity::Settings* settings) {
		json state;
		state["enabled"] = settings->enabled;
		state["applicable"] = settings->applicable;
		if (spec->coloredCustom) {
			state["color"] = {
				settings->color.r,
				settings->color.g,
				settings->color.b,
				settings->color.a
			};
		}
		if (settings->store)
			state["store"] = serialize(settings->store);
		if (settings->crafter)
			state["crafter"] = serialize(settings->crafter);
		if (settings->arm)
			state["arm"] = serialize(settings->arm);
		if (settings->loader)
			state["loader"] = serialize(settings->loader);
		if (settings->balancer)
			state["balancer"] = serialize(settings->balancer);
		if (settings->pipe)
			state["pipe"] = serialize(settings->pipe);
		if (settings->cart)
			state["cart"] = serialize(settings->cart);
		if (settings->cartStop)
			state["cartStop"] = serialize(settings->cartStop);
		if (settings->cartWaypoint)
			state["cartWaypoint"] = serialize(settings->cartWaypoint);
		if (settings->networker)
			state["networker"] = serialize(settings->networker);
		if (settings->tube)
			state["tube"] = serialize(settings->tube);
		if (settings->monorail)
			state["monorail"] = serialize(settings->monorail);
		if (settings->router)
			state["router"] = serialize(settings->router);
		return state;
	}

	Entity::Settings* unserializeSettings(json state) {
		auto settings = new Entity::Settings();
		settings->enabled = state["enabled"];
		settings->applicable = state["applicable"];
		if (state.contains("color")) {
			settings->color = Color(
				(float)state["color"][0],
				(float)state["color"][1],
				(float)state["color"][2],
				(float)state["color"][3]
			);
		}
		if (state.contains("store")) {
			settings->store = new StoreSettings();
			unserialize(settings->store, state["store"]);
		}
		if (state.contains("crafter")) {
			settings->crafter = new CrafterSettings();
			unserialize(settings->crafter, state["crafter"]);
		}
		if (state.contains("arm")) {
			settings->arm = new ArmSettings();
			unserialize(settings->arm, state["arm"]);
		}
		if (state.contains("loader")) {
			settings->loader = new LoaderSettings();
			unserialize(settings->loader, state["loader"]);
		}
		if (state.contains("balancer")) {
			settings->balancer = new BalancerSettings();
			unserialize(settings->balancer, state["balancer"]);
		}
		if (state.contains("pipe")) {
			settings->pipe = new PipeSettings();
			unserialize(settings->pipe, state["pipe"]);
		}
		if (state.contains("cart")) {
			settings->cart = new CartSettings();
			unserialize(settings->cart, state["cart"]);
		}
		if (state.contains("cartStop")) {
			settings->cartStop = new CartStopSettings();
			unserialize(settings->cartStop, state["cartStop"]);
		}
		if (state.contains("cartWaypoint")) {
			settings->cartWaypoint = new CartWaypointSettings();
			unserialize(settings->cartWaypoint, state["cartWaypoint"]);
		}
		if (state.contains("networker")) {
			settings->networker = new NetworkerSettings();
			unserialize(settings->networker, state["networker"]);
		}
		if (state.contains("tube")) {
			settings->tube = new TubeSettings();
			unserialize(settings->tube, state["tube"]);
		}
		if (state.contains("monorail")) {
			settings->monorail = new MonorailSettings();
			unserialize(settings->monorail, state["monorail"]);
		}
		if (state.contains("router")) {
			settings->router = new RouterSettings();
			unserialize(settings->router, state["router"]);
		}
		return settings;
	}
}

void Plan::saveAll() {
	deflation def;

	for (auto plan: all) {
		if (!plan->save) continue;

		json state;
		state["title"] = plan->title;
		state["config"] = plan->config;

		int i = 0;

		for (auto tag: plan->tags) {
			state["tags"][i++] = tag;
		}

		i = 0;
		for (auto ge: plan->entities) {
			auto& gstate = state["entities"][i++];
			auto pos = ge->pos() - plan->position;
			auto dir = ge->dir();
			gstate["spec"] = ge->spec->name;
			gstate["pos"] = {pos.x, pos.y, pos.z};
			gstate["dir"] = {dir.x, dir.y, dir.z};

			if (ge->settings) {
				gstate["settings"] = serializeSettings(ge->spec, ge->settings);
			}
		}

		def.push(state.dump());
	}

	def.save(Config::plansPath());
}

void Plan::loadAll() {
	try {
		inflation inf;
		for (auto line: inf.load(Config::plansPath()).parts()) {
			auto state = json::parse(line);

			Plan* plan = new Plan();
			plan->title = state["title"];
			plan->config = state["config"];
			plan->save = true;

			for (auto tag: state["tags"]) {
				plan->tags.insert(std::string(tag));
			}

			for (auto estate: state["entities"]) {
				auto specName = estate["spec"];

				if (!Spec::all.count(specName)) {
					notef("Plan '%s' references missing specification '%s'; dropping ghost and disabling autosave", plan->title, specName);
					plan->save = false;
					continue;
				}

				auto ge = new GuiFakeEntity(Spec::byName(specName));

				try {
					ge->move(
						Point(estate["pos"][0], estate["pos"][1], estate["pos"][2]),
						Point(estate["dir"][0], estate["dir"][1], estate["dir"][2])
					);

					if (estate.contains("settings")) {
						ge->settings = unserializeSettings(estate["settings"]);
					}

					plan->add(ge);
				}
				catch (std::exception& e) {
					notef("Plan '%s' has a broken entity '%s'; dropping ghost and disabling autosave", plan->title, ge->spec->title);
					plan->save = false;
					delete ge;
					continue;
				}
			}

			if (!plan->entities.size()) delete plan;
		}
	}
	catch(std::exception& e) {
		notef("saved blueprints not loaded: %s", e.what());
		Plan::reset();
	}

	Plan::clipboard = nullptr;
}
