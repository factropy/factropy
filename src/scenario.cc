#include "scenario.h"
#include "crew.h"
#include "gui.h"
#include <fstream>
#include <filesystem>
#include "../rela/rela.hpp"

Scenario* scenario;

class RelaMod : public Rela {
public:

	struct {
		int common = 0;
		int items = 0;
		int recipes = 0;
		int specs = 0;
		int goals = 0;
		int messages = 0;
		int create = 0;
	} modules;

	typedef void (RelaMod::*callback)(void);

	struct func {
		const char* name = nullptr;
		callback cb;
		int id = 0;
	};

	struct {
		func sentinelA;
		func print              = { .name = "print",              .cb = &RelaMod::print              };
		func point_meta         = { .name = "point_meta",         .cb = &RelaMod::point_meta         };
		func matrix_meta        = { .name = "matrix_meta",        .cb = &RelaMod::matrix_meta        };
		func energy_J           = { .name = "J",                  .cb = &RelaMod::energy_J           };
		func energy_kJ          = { .name = "kJ",                 .cb = &RelaMod::energy_kJ          };
		func energy_MJ          = { .name = "MJ",                 .cb = &RelaMod::energy_MJ          };
		func energy_GJ          = { .name = "GJ",                 .cb = &RelaMod::energy_GJ          };
		func energy_W           = { .name = "W",                  .cb = &RelaMod::energy_W           };
		func energy_kW          = { .name = "kW",                 .cb = &RelaMod::energy_kW          };
		func energy_MW          = { .name = "MW",                 .cb = &RelaMod::energy_MW          };
		func energy_GW          = { .name = "GW",                 .cb = &RelaMod::energy_GW          };
		func liquid_L           = { .name = "L",                  .cb = &RelaMod::liquid_L           };
		func liquid_kL          = { .name = "kL",                 .cb = &RelaMod::liquid_kL          };
		func liquid_ML          = { .name = "ML",                 .cb = &RelaMod::liquid_ML          };
		func mass_kg            = { .name = "kg",                 .cb = &RelaMod::mass_kg            };
		func add_item           = { .name = "add_item",           .cb = &RelaMod::add_item           };
		func add_recipe         = { .name = "add_recipe",         .cb = &RelaMod::add_recipe         };
		func add_spec           = { .name = "add_spec",           .cb = &RelaMod::add_spec           };
		func add_goal           = { .name = "add_goal",           .cb = &RelaMod::add_goal           };
		func add_message        = { .name = "add_message",        .cb = &RelaMod::add_message        };
		func point_rotate       = { .name = "point_rotate",       .cb = &RelaMod::point_rotate       };
		func point_scale        = { .name = "point_scale",        .cb = &RelaMod::point_scale        };
		func point_add          = { .name = "point_add",          .cb = &RelaMod::point_add          };
		func point_transform    = { .name = "point_transform",    .cb = &RelaMod::point_transform    };
		func matrix_identity    = { .name = "matrix_identity",    .cb = &RelaMod::matrix_identity    };
		func matrix_scale       = { .name = "matrix_scale",       .cb = &RelaMod::matrix_scale       };
		func matrix_rotate      = { .name = "matrix_rotate",      .cb = &RelaMod::matrix_rotate      };
		func matrix_rotation    = { .name = "matrix_rotation",    .cb = &RelaMod::matrix_rotation    };
		func matrix_translate   = { .name = "matrix_translate",   .cb = &RelaMod::matrix_translate   };
		func matrix_translation = { .name = "matrix_translation", .cb = &RelaMod::matrix_translation };
		func matrix_multiply    = { .name = "matrix_multiply",    .cb = &RelaMod::matrix_multiply    };
		func matrix_invert      = { .name = "matrix_invert",      .cb = &RelaMod::matrix_invert      };
		func world_elevation    = { .name = "world_elevation",    .cb = &RelaMod::world_elevation    };
		func entity_create      = { .name = "entity_create",      .cb = &RelaMod::entity_create      };
		func entity_move        = { .name = "entity_move",        .cb = &RelaMod::entity_move        };
		func entity_materialize = { .name = "entity_materialize", .cb = &RelaMod::entity_materialize };
		func entity_insert      = { .name = "entity_insert",      .cb = &RelaMod::entity_insert      };
		func entity_level       = { .name = "entity_level",       .cb = &RelaMod::entity_level       };
		func entity_direct      = { .name = "entity_direct",      .cb = &RelaMod::entity_direct      };
		func toolbar_spec       = { .name = "toolbar_spec",       .cb = &RelaMod::toolbar_spec       };
		func license_spec       = { .name = "license_spec",       .cb = &RelaMod::license_spec       };
		func license_recipe     = { .name = "license_recipe",     .cb = &RelaMod::license_recipe     };
		func sentinelB;
	} funcs;

	RelaMod() {
		auto slurp = [&](auto path) {
			std::ifstream script(path);
			std::string content((std::istreambuf_iterator<char>(script)), (std::istreambuf_iterator<char>()));
			script.close();
			return content;
		};

		auto common   = slurp("scenario/common.rela");
		auto items    = slurp("scenario/items.rela");
		auto recipes  = slurp("scenario/recipes.rela");
		auto specs    = slurp("scenario/specs.rela");
		auto goals    = slurp("scenario/goals.rela");
		auto messages = slurp("scenario/messages.rela");
		auto create   = slurp("scenario/create.rela");

		modules.common   = module(common.c_str());
		modules.items    = module(items.c_str());
		modules.recipes  = module(recipes.c_str());
		modules.specs    = module(specs.c_str());
		modules.goals    = module(goals.c_str());
		modules.messages = module(messages.c_str());
		modules.create   = module(create.c_str());

		funcs.sentinelA.id = 0;
		funcs.sentinelB.id = &funcs.sentinelB - &funcs.sentinelA;

		for (int i = funcs.sentinelA.id+1; i < funcs.sentinelB.id; i++) {
			func* fn = &funcs.sentinelA + i; fn->id = i;
			map_set(map_core(), make_string(fn->name), make_function(fn->id));
		}

		ensure(funcs.entity_create.id == (int)(&funcs.entity_create - &funcs.sentinelA), "rela: function table broken");
	}

	void execute(int id) override {
		ensure(id > funcs.sentinelA.id && id < funcs.sentinelB.id, "invalid function id: %s", id);
		func* fn = &funcs.sentinelA + id;
		assert(fn->id == id);
		(this->*(fn->cb))();
	}

	void print() {
		char tmp[100];
		for (int i = 0, l = stack_depth(); i < l; i++) {
			notef("rela: %s", to_text(stack_pick(i), tmp, sizeof(tmp)));
		}
	}

	double pop_num() {
		return to_number(stack_pop());
	}

	void push_int(int64_t n) {
		stack_push(make_integer(n));
	}

	Point to_point(oitem vec) {
		return {
			to_number(vector_get(vec, 0)),
			to_number(vector_get(vec, 1)),
			to_number(vector_get(vec, 2)),
		};
	}

	oitem make_point(Point p) {
		auto vec = make_vector();
		vector_set(vec, 0, make_number(p.x));
		vector_set(vec, 1, make_number(p.y));
		vector_set(vec, 2, make_number(p.z));
		meta_set(vec, make_function(funcs.point_meta.id));
		return vec;
	}

	void energy_J() { push_int(Energy::J(pop_num())); }
	void energy_kJ() { push_int(Energy::kJ(pop_num())); }
	void energy_MJ() { push_int(Energy::MJ(pop_num())); }
	void energy_GJ() { push_int(Energy::GJ(pop_num())); }
	void energy_W() { push_int(Energy::W(pop_num())); }
	void energy_kW() { push_int(Energy::kW(pop_num())); }
	void energy_MW() { push_int(Energy::MW(pop_num())); }
	void energy_GW() { push_int(Energy::GW(pop_num())); }

	void liquid_L() { push_int(std::ceil(pop_num())); }
	void liquid_kL() { push_int(std::ceil(pop_num()*1000.0)); }
	void liquid_ML() { push_int(std::ceil(pop_num()*1000000.0)); }

	void mass_kg() { push_int((int64_t)(pop_num()*1000.0)); }

	Part* read_part(oitem fields) {
		ensuref(is_map(fields), "rela: read_part() expected map");

		auto field = [&](auto name) {
			return map_get_named(fields, name);
		};

		auto color = field("color");
		auto smoke = field("smoke");

		Part* part = nullptr;
		if (is_vector(field("spinner"))) {
			auto spinner = field("spinner");
			part = new PartSpinner(
				to_integer(color),
				to_point(vector_get(spinner, 0)),
				to_number(vector_get(spinner, 1))
			);
		}
		else
		if (is_true(field("spinner"))) {
			part = new PartSpinner(to_integer(color));
		}
		else
		if (is_integer(field("gridRepeat"))) {
			part = new PartGridRepeat(
				to_integer(color),
				to_integer(field("gridRepeat"))
			);
		}
		else
		if (is_vector(field("cycler"))) {
			auto cycle = field("cycler");
			std::vector<Mat4> states;
			for (int i = 0, l = item_count(cycle); i < l; i++) {
				Mat4 m = *((Mat4*)to_data(vector_get(cycle, i)));
				states.push_back(m);
			}
			part = new PartCycle(to_integer(color), states);
		}
		else
		if (is_map(smoke)) {
			auto smokeField = [&](auto name) {
				return map_get_named(smoke, name);
			};
			part = new PartSmoke(
				to_integer(color),
				to_point(smokeField("direction")),
				to_integer(smokeField("particlesMax")),
				to_integer(smokeField("particlesPerTick")),
				to_number(smokeField("particleRadius")),
				to_number(smokeField("emitRadius")),
				to_number(smokeField("spreadFactor")),
				to_number(smokeField("tickRadiusFactor")),
				to_number(smokeField("tickDirectionFactor")),
				to_number(smokeField("tickDecayFactor")),
				(uint)to_integer(smokeField("tickLifeLower")),
				(uint)to_integer(smokeField("tickLifeUpper"))
			);
		}
		else
		if (is_true(field("flame"))) {
			part = new PartFlame();
		}
		else
		if (is_true(field("tree"))) {
			part = new PartTree(to_integer(color));
		}
		else
		if (is_string(color) && std::string(to_string(color)) == "recipe") {
			part = new PartRecipeFluid();
		}
		else {
			part = new Part(to_integer(color));
		}

		auto gloss = field("gloss");
		if (is_number(gloss)) {
			part->gloss(to_number(gloss));
		}

		auto glow = field("glow");
		if (is_true(glow)) {
			part->glow = true;
		}

		auto translucent = field("translucent");
		if (is_true(translucent)) {
			part->translucent();
		}

		auto autoColor = field("autoColor");
		if (is_true(autoColor)) {
			part->autoColor = true;
		}

		auto customColor = field("customColor");
		if (is_true(customColor)) {
			part->customColor = true;
		}

		auto filter = field("filter");
		if (is_integer(filter)) {
			part->filter = to_integer(filter);
		}

		auto lods = field("lods");
		for (int i = 0, l = item_count(lods); i < l; i++) {
			auto lod = vector_get(lods, i);
			std::string stl = to_string(vector_get(lod, 0));
			std::string level = to_string(vector_get(lod, 1));
			bool shadow = to_bool(vector_get(lod, 2));

			auto def = Part::HD;
			if (level == "medium") def = Part::MD;
			if (level == "low") def = Part::LD;
			if (level == "verylow") def = Part::VLD;

			if (!scenario->meshes.count(stl)) scenario->meshes[stl] = new Mesh(stl);
			part->lod(scenario->meshes[stl], def, shadow);
		}

		Mat4 s = Mat4::identity;
		Mat4 r = Mat4::identity;
		Mat4 t = Mat4::identity;

		auto transform = field("transform");
		if (is_data(transform)) {
			part->transform(*((Mat4*)to_data(transform)));
		}
		else {
			auto scale = field("scale");
			if (is_number(scale)) {
				float n = to_number(scale);
				part->scale = {n,n,n};
				s = Mat4::scale(n);
			}
			if (is_vector(scale)) {
				part->scale = {
					to_number(vector_get(scale, 0)),
					to_number(vector_get(scale, 1)),
					to_number(vector_get(scale, 2))
				};
				s = Mat4::scale(part->scale.x, part->scale.y, part->scale.z);
			}

			auto rotate = field("rotate");
			if (is_vector(rotate)) {
				auto degrees = to_number(vector_get(rotate, 1));
				auto axis = to_point(vector_get(rotate, 0));
				r = Mat4::rotate(axis, glm::radians(degrees));
				part->rotateAxis = axis;
				part->rotateDegrees = degrees;
			}

			auto translate = field("translate");
			if (is_vector(translate)) {
				part->translate = {
					to_number(vector_get(translate, 0)),
					to_number(vector_get(translate, 1)),
					to_number(vector_get(translate, 2))
				};
				t = Mat4::translate(part->translate.x, part->translate.y, part->translate.z);
			}
			part->transform(s * r * t);
		}

		auto pivots = field("pivots");
		if (is_vector(pivots)) {
			uint flags = 0;
			Point bump = Point::Zero;
			for (int i = 0, l = item_count(pivots); i < l; i++) {
				auto item = vector_get(pivots, i);
				if (is_string(item) && std::string(to_string(item)) == "azimuth") flags |= Part::Azimuth;
				if (is_string(item) && std::string(to_string(item)) == "altitude") flags |= Part::Altitude;
				if (is_vector(item)) bump = to_point(item);
			}
			part->pivots(flags, bump);
		}

		return part;
	}

	void add_item() {
		auto data = stack_pop();

		//char tmp[1024];
		//notef("rela: item %s", to_text(data, tmp, sizeof(tmp)));

		auto field = [&](const char* name) {
			return map_get_named(data, name);
		};

		auto strField = [&](const char* name) {
			return to_string(field(name));
		};

		auto item = Item::create(Item::next(), strField("name"));
		item->title = strField("title");

		auto fuel = field("fuel");
		if (is_vector(fuel) && item_count(fuel) == 2) {
			auto type = to_string(vector_get(fuel, 0));
			auto energy = to_integer(vector_get(fuel, 1));
			item->fuel = Fuel(type, energy);
		}

		auto parts = field("parts");
		ensuref(is_vector(parts), "rela: add_item(%s) expected item.parts=vector", item->name);
		for (int i = 0, l = item_count(parts); i < l; i++) {
			item->parts.push_back(read_part(vector_get(parts, i)));
		}

		auto mass = field("mass");
		if (is_integer(mass)) {
			item->mass = to_integer(mass);
		}

		auto repair = field("repair");
		if (is_integer(repair)) {
			item->repair = to_integer(repair);
		}

		auto ammoDamage = field("ammoDamage");
		if (is_integer(ammoDamage)) {
			item->ammoDamage = to_integer(ammoDamage);
		}

		auto ammoRounds = field("ammoRounds");
		if (is_integer(ammoRounds)) {
			item->ammoRounds = to_integer(ammoRounds);
		}

		auto ore = field("ore");
		if (is_string(ore)) {
			item->ore = to_string(ore);
		}

		auto mining = field("mining");
		if (is_number(mining)) {
			Item::mining[item->id] = to_number(mining);
		}

		auto raw = field("raw");
		if (is_bool(raw)) {
			item->raw = to_bool(raw);
		}

		auto show = field("show");
		if (is_bool(show)) {
			item->show = to_bool(show);
		}

		auto armV = field("armV");
		if (is_number(armV)) {
			item->armV = to_number(armV);
		}

		auto tubeV = field("tubeV");
		if (is_number(tubeV)) {
			item->tubeV = to_number(tubeV);
		}

		auto beltV = field("beltV");
		if (is_number(beltV)) {
			item->beltV = to_number(beltV);
		}
	}

	void add_recipe() {
		auto data = stack_pop();

		//char tmp[1024];
		//notef("rela: recipe %s", to_text(data, tmp, sizeof(tmp)));

		auto field = [&](const char* name) {
			return map_get_named(data, name);
		};

		auto strField = [&](const char* name) {
			return to_string(field(name));
		};

		auto intField = [&](const char* name) {
			return to_integer(field(name));
		};

		auto recipe = new Recipe(strField("name"));
		recipe->title = strField("title");
		recipe->energyUsage = intField("energy");

		auto licensed = field("licensed");
		if (is_true(licensed)) {
			recipe->licensed = true;
		}

		auto tags = field("tags");
		for (int i = 0, l = item_count(tags); i < l; i++) {
			auto val = vector_get(tags, i);
			recipe->tags.insert(to_string(val));
		}

		auto input = field("input");
		for (int i = 0, l = item_count(input); i < l; i++) {
			auto key = map_key(input, i);
			auto val = map_get(input, key);
			auto name = to_string(key);
			if (Item::names.count(name)) {
				recipe->inputItems[Item::byName(name)->id] = to_integer(val);
			}
			else
			if (Fluid::names.count(name)) {
				recipe->inputFluids[Fluid::byName(name)->id] = to_integer(val);
			}
			else {
				fatalf("unknown recipe input: %s", name);
			}
		}

		auto output = field("output");
		for (int i = 0, l = item_count(output); i < l; i++) {
			auto key = map_key(output, i);
			auto val = map_get(output, key);
			auto name = to_string(key);
			if (Item::names.count(name)) {
				recipe->outputItems[Item::byName(name)->id] = to_integer(val);
			}
			else
			if (Fluid::names.count(name)) {
				recipe->outputFluids[Fluid::byName(name)->id] = to_integer(val);
			}
			else {
				fatalf("unknown recipe output: %s", name);
			}
		}

		auto mine = field("mine");
		if (!is_nil(mine)) {
			recipe->mine = Item::byName(to_string(mine))->id;
		}

		auto drill = field("drill");
		if (!is_nil(drill)) {
			recipe->drill = Fluid::byName(to_string(drill))->id;
		}

		auto fluid = field("fluid");
		if (!is_nil(fluid)) {
			recipe->fluid = Fluid::byName(to_string(fluid))->id;
		}

		collect();
	}

	void add_spec() {
		auto data = stack_pop();

		//char tmp[1024];
		//notef("rela: spec %s", to_text(data, tmp, sizeof(tmp)));

		auto field = [&](const char* name) {
			return map_get_named(data, name);
		};

		auto strField = [&](const char* name) {
			return to_string(field(name));
		};

		Spec* spec = new Spec(strField("name"));
		spec->title = strField("title");

		auto wiki = field("wiki");
		if (is_string(wiki)) {
			spec->wiki = to_string(wiki);
		}

		auto toolbar = field("toolbar");
		if (is_string(toolbar)) {
			spec->toolbar = to_string(toolbar);
		}

		auto collision = field("collision");
		if (is_vector(collision)) {
			spec->collision = {
				to_number(vector_get(collision, 0)),
				to_number(vector_get(collision, 1)),
				to_number(vector_get(collision, 2)),
				to_number(vector_get(collision, 3)),
				to_number(vector_get(collision, 4)),
				to_number(vector_get(collision, 5)),
			};
		}

		spec->selection = spec->collision;
		auto selection = field("selection");
		if (is_vector(selection)) {
			spec->selection = {
				to_number(vector_get(selection, 0)),
				to_number(vector_get(selection, 1)),
				to_number(vector_get(selection, 2)),
				to_number(vector_get(selection, 3)),
				to_number(vector_get(selection, 4)),
				to_number(vector_get(selection, 5)),
			};
		}

		auto materials = field("materials");
		if (is_map(materials)) {
			for (int i = 0, l = item_count(materials); i < l; i++) {
				auto key = map_key(materials, i);
				auto val = map_get(materials, key);
				auto name = to_string(key);
				auto count = to_integer(val);
				ensuref(count > 0, "rela: app_sec() expected material count > 0");
				spec->materials.push_back({Item::byName(name)->id, (uint)count});
			}
		}

		auto parts = field("parts");
		if (is_vector(parts)) {
			for (int i = 0, l = item_count(parts); i < l; i++) {
				spec->parts.push_back(read_part(vector_get(parts, i)));
			}
		}

		auto states = field("states");
		if (is_vector(states)) {
			for (int i = 0, il = item_count(states); i < il; i++) {
				auto& back = spec->states.emplace_back();
				auto state = vector_get(states, i);
				ensuref(item_count(state) == spec->parts.size(), "rela: add_spec(%s) parts/states mismatch %d %lu", spec->name, item_count(state), spec->parts.size());
				for (int j = 0, jl = item_count(state); j < jl; j++) {
					Mat4* mat = (Mat4*)to_data(vector_get(state, j));
					back.push_back(*mat);
				}
			}
		}

		auto statesShow = field("statesShow");
		if (is_vector(statesShow)) {
			for (int i = 0, il = item_count(statesShow); i < il; i++) {
				auto& back = spec->statesShow.emplace_back();
				auto state = vector_get(statesShow, i);
				ensuref(item_count(state) == spec->parts.size(), "rela: add_spec(%s) parts/statesShow mismatch %d %lu", spec->name, item_count(state), spec->parts.size());
				for (int j = 0, jl = item_count(state); j < jl; j++) {
					bool show = to_bool(vector_get(state, j));
					back.push_back(show);
				}
			}
		}

		auto enable = field("enable");
		if (is_bool(enable)) {
			spec->enable = to_bool(enable);
		}

		auto build = field("build");
		if (is_bool(build)) {
			spec->build = to_bool(build);
		}

		auto select = field("select");
		if (is_bool(select)) {
			spec->select = to_bool(select);
		}

		auto plan = field("plan");
		if (is_bool(plan)) {
			spec->plan = to_bool(plan);
		}

		auto clone = field("clone");
		if (is_bool(clone)) {
			spec->clone = to_bool(clone);
		}

		auto align = field("align");
		if (is_bool(align)) {
			spec->align = to_bool(align);
		}

		auto forceDelete = field("forceDelete");
		if (is_bool(forceDelete)) {
			spec->forceDelete = to_bool(forceDelete);
		}

		auto forceDeleteStore = field("forceDeleteStore");
		if (is_bool(forceDeleteStore)) {
			spec->forceDeleteStore = to_bool(forceDeleteStore);
		}

		auto slab = field("slab");
		if (is_bool(slab)) {
			spec->slab = to_bool(slab);
		}

		auto router = field("router");
		if (is_bool(router)) {
			spec->router = to_bool(router);
		}

		auto snapAlign = field("snapAlign");
		if (is_bool(snapAlign)) {
			spec->snapAlign = to_bool(snapAlign);
		}

		auto licensed = field("licensed");
		if (is_bool(licensed)) {
			spec->licensed = to_bool(licensed);
		}

		auto junk = field("junk");
		if (is_bool(junk)) {
			spec->junk = to_bool(junk);
		}

		auto health = field("health");
		if (is_integer(health)) {
			spec->health = to_integer(health);
		}

		auto upgrade = field("upgrade");
		if (is_string(upgrade)) {
			scenario->specUpgrades[spec] = to_string(upgrade);
		}

		auto upgradeCascade = field("upgradeCascade");
		if (is_vector(upgradeCascade)) {
			for (int i = 0, l = item_count(upgradeCascade); i < l; i++) {
				auto name = to_string(vector_get(upgradeCascade, i));
				scenario->specUpgradeCascades[spec].push_back(name);
			}
		}

		auto cycle = field("cycle");
		if (is_string(cycle)) {
			scenario->specCycles[spec] = to_string(cycle);
		}

		auto cycleReverseDirection = field("cycleReverseDirection");
		if (is_bool(cycleReverseDirection)) {
			spec->cycleReverseDirection = to_bool(cycleReverseDirection);
		}

		auto statsGroup = field("statsGroup");
		if (is_string(statsGroup)) {
			scenario->specStatsGroups[spec] = to_string(statsGroup);
		}

		auto pipette = field("pipette");
		if (is_string(pipette)) {
			scenario->specPipettes[spec] = to_string(pipette);
		}

		auto follow = field("follow");
		if (is_string(follow)) {
			scenario->specFollows[spec] = to_string(follow);
		}

		auto followConveyor = field("followConveyor");
		if (is_string(followConveyor)) {
			scenario->specFollowConveyors[spec] = to_string(followConveyor);
		}

		auto upward = field("upward");
		if (is_string(upward)) {
			scenario->specUpwards[spec] = to_string(upward);
		}

		auto downward = field("downward");
		if (is_string(downward)) {
			scenario->specDownwards[spec] = to_string(downward);
		}

		auto rotateGhost = field("rotateGhost");
		if (is_bool(rotateGhost)) {
			spec->rotateGhost = to_bool(rotateGhost);
		}

		auto rotateExtant = field("rotateExtant");
		if (is_bool(rotateExtant)) {
			spec->rotateExtant = to_bool(rotateExtant);
		}

		auto rotations = field("rotations");
		if (is_vector(rotations)) {
			spec->rotations.clear();
			for (int i = 0, l = item_count(rotations); i < l; i++) {
				spec->rotations.push_back(to_point(vector_get(rotations, i)));
			}
		}

		auto enemyTarget = field("enemyTarget");
		if (is_bool(enemyTarget)) {
			spec->enemyTarget = to_bool(enemyTarget);
		}

		auto consumeElectricity = field("consumeElectricity");
		if (is_bool(consumeElectricity)) {
			spec->consumeElectricity = to_bool(consumeElectricity);
		}

		auto consumeFuel = field("consumeFuel");
		if (is_bool(consumeFuel)) {
			spec->consumeFuel = to_bool(consumeFuel);
		}

		auto consumeFuelType = field("consumeFuelType");
		if (is_string(consumeFuelType)) {
			spec->consumeFuelType = to_string(consumeFuelType);
		}

		auto consumeCharge = field("consumeCharge");
		if (is_bool(consumeCharge)) {
			spec->consumeCharge = to_bool(consumeCharge);
		}
		if (spec->consumeCharge) {
			auto consumeChargeEffect = field("consumeChargeEffect");
			if (is_bool(consumeChargeEffect)) {
				spec->consumeChargeEffect = to_bool(consumeChargeEffect);
			}

			auto consumeChargeBuffer = field("consumeChargeBuffer");
			if (is_integer(consumeChargeBuffer)) {
				spec->consumeChargeBuffer = to_integer(consumeChargeBuffer);
			}

			auto consumeChargeRate = field("consumeChargeRate");
			if (is_integer(consumeChargeRate)) {
				spec->consumeChargeRate = to_integer(consumeChargeRate);
			}
		}

		auto consumeMagic = field("consumeMagic");
		if (is_bool(consumeMagic)) {
			spec->consumeMagic = to_bool(consumeMagic);
		}

		auto burnerState = field("burnerState");
		if (is_bool(burnerState)) {
			spec->burnerState = to_bool(burnerState);
		}

		auto energyConsume = field("energyConsume");
		if (is_integer(energyConsume)) {
			spec->energyConsume = to_integer(energyConsume);
		}

		auto energyGenerate = field("energyGenerate");
		if (is_integer(energyGenerate)) {
			spec->energyGenerate = to_integer(energyGenerate);
		}

		auto generateElectricity = field("generateElectricity");
		if (is_bool(generateElectricity)) {
			spec->generateElectricity = to_bool(generateElectricity);
		}

		auto powerpole = field("powerpole");
		if (is_bool(powerpole)) {
			spec->powerpole = to_bool(powerpole);
		}

		if (spec->powerpole) {
			auto powerpoleRange = field("powerpoleRange");
			if (is_number(powerpoleRange)) {
				spec->powerpoleRange = to_number(powerpoleRange);
			}

			auto powerpoleCoverage = field("powerpoleCoverage");
			if (is_number(powerpoleCoverage)) {
				spec->powerpoleCoverage = to_number(powerpoleCoverage);
			}
		}

		auto status = field("status");
		if (is_bool(status)) {
			spec->status = to_bool(status);
		}

		auto beacon = field("beacon");
		if (is_vector(beacon)) {
			spec->beacon = to_point(beacon);
		}

		auto effector = field("effector");
		if (is_bool(effector)) {
			spec->effector = to_bool(effector);
		}

		if (spec->effector) {
			auto effectorElectricity = field("effectorElectricity");
			if (is_number(effectorElectricity)) {
				spec->effectorElectricity = to_number(effectorElectricity);
			}

			auto effectorFuel = field("effectorFuel");
			if (is_number(effectorFuel)) {
				spec->effectorFuel = to_number(effectorFuel);
			}

			auto effectorCharge = field("effectorCharge");
			if (is_number(effectorCharge)) {
				spec->effectorCharge = to_number(effectorCharge);
			}

			auto effectorEnergyDrain = field("effectorEnergyDrain");
			if (is_integer(effectorEnergyDrain)) {
				spec->effectorEnergyDrain = to_integer(effectorEnergyDrain);
			}
		}

		auto bufferElectricity = field("bufferElectricity");
		if (is_bool(bufferElectricity)) {
			spec->bufferElectricity = to_bool(bufferElectricity);
		}

		auto bufferElectricityState = field("bufferElectricityState");
		if (is_bool(bufferElectricityState)) {
			spec->bufferElectricityState = to_bool(bufferElectricityState);
		}

		auto bufferDischargeRate = field("bufferDischargeRate");
		if (is_integer(bufferDischargeRate)) {
			spec->bufferDischargeRate = to_integer(bufferDischargeRate);
		}

		auto crafter = field("crafter");
		if (is_bool(crafter)) {
			spec->crafter = to_bool(crafter);
		}

		if (spec->crafter) {
			auto crafterRate = field("crafterRate");
			if (is_number(crafterRate)) {
				spec->crafterRate = to_number(crafterRate);
			}

			auto crafterState = field("crafterState");
			if (is_bool(crafterState)) {
				spec->crafterState = to_bool(crafterState);
			}

			auto crafterProgress = field("crafterProgress");
			if (is_bool(crafterProgress)) {
				spec->crafterProgress = to_bool(crafterProgress);
			}

			auto crafterOutput = field("crafterOutput");
			if (is_bool(crafterOutput)) {
				spec->crafterOutput = to_bool(crafterOutput);
			}

			auto crafterOutputPos = field("crafterOutputPos");
			if (is_vector(crafterOutputPos)) {
				spec->crafterOutputPos = to_point(crafterOutputPos);
			}

			auto crafterRecipe = field("crafterRecipe");
			if (is_string(crafterRecipe)) {
				spec->crafterRecipe = Recipe::byName(to_string(crafterRecipe));
			}

			auto crafterTransmitResources = field("crafterTransmitResources");
			if (is_bool(crafterTransmitResources)) {
				spec->crafterTransmitResources = to_bool(crafterTransmitResources);
			}
		}

		auto recipeTags = field("recipeTags");
		if (is_vector(recipeTags)) {
			for (int i = 0, l = item_count(recipeTags); i < l; i++) {
				spec->recipeTags.insert(to_string(vector_get(recipeTags, i)));
			}
		}

		auto venter = field("venter");
		if (is_bool(venter)) {
			spec->venter = to_bool(venter);
		}

		auto venterRate = field("venterRate");
		if (is_integer(venterRate)) {
			spec->venterRate = to_integer(venterRate);
		}

		auto pipe = field("pipe");
		if (is_bool(pipe)) {
			spec->pipe = to_bool(pipe);
		}

		if (spec->pipe) {
			auto pipeCapacity = field("pipeCapacity");
			if (is_integer(pipeCapacity)) {
				spec->pipeCapacity = to_integer(pipeCapacity);
			}

			auto pipeUnderground = field("pipeUnderground");
			if (is_bool(pipeUnderground)) {
				spec->pipeUnderground = to_bool(pipeUnderground);
			}

			auto pipeUndergroundRange = field("pipeUndergroundRange");
			if (is_number(pipeUndergroundRange)) {
				spec->pipeUndergroundRange = to_number(pipeUndergroundRange);
			}

			auto pipeUnderwater = field("pipeUnderwater");
			if (is_bool(pipeUnderwater)) {
				spec->pipeUnderwater = to_bool(pipeUnderwater);
			}

			auto pipeUnderwaterRange = field("pipeUnderwaterRange");
			if (is_number(pipeUnderwaterRange)) {
				spec->pipeUnderwaterRange = to_number(pipeUnderwaterRange);
			}

			auto pipeValve = field("pipeValve");
			if (is_bool(pipeValve)) {
				spec->pipeValve = to_bool(pipeValve);
			}

			auto pipeLevels = field("pipeLevels");
			if (is_vector(pipeLevels)) {
				for (int i = 0, l = item_count(pipeLevels); i < l; i++) {
					auto entry = vector_get(pipeLevels, i);
					auto box = vector_get(entry, 0);
					Box b = {
						to_number(vector_get(box, 0)),
						to_number(vector_get(box, 1)),
						to_number(vector_get(box, 2)),
						to_number(vector_get(box, 3)),
						to_number(vector_get(box, 4)),
						to_number(vector_get(box, 5)),
					};
					Mat4 m = *((Mat4*)to_data(vector_get(entry, 1)));
					spec->pipeLevels.push_back({.box = b, .trx = m});
				}
			}
		}

		auto pipeHints = field("pipeHints");
		if (is_bool(pipeHints)) {
			spec->pipeHints = to_bool(pipeHints);
		}

		auto pipeConnections = field("pipeConnections");
		if (is_vector(pipeConnections)) {
			for (int i = 0, l = item_count(pipeConnections); i < l; i++) {
				auto point = vector_get(pipeConnections, i);
				spec->pipeConnections.push_back(to_point(point));
			}
		}

		auto pipeInputConnections = field("pipeInputConnections");
		if (is_vector(pipeInputConnections)) {
			for (int i = 0, l = item_count(pipeInputConnections); i < l; i++) {
				auto point = vector_get(pipeInputConnections, i);
				spec->pipeInputConnections.push_back(to_point(point));
			}
		}

		auto pipeOutputConnections = field("pipeOutputConnections");
		if (is_vector(pipeOutputConnections)) {
			for (int i = 0, l = item_count(pipeOutputConnections); i < l; i++) {
				auto point = vector_get(pipeOutputConnections, i);
				spec->pipeOutputConnections.push_back(to_point(point));
			}
		}

		auto networker = field("networker");
		if (is_bool(networker)) {
			spec->networker = to_bool(networker);
		}

		if (spec->networker) {
			auto networkHub = field("networkHub");
			if (is_bool(networkHub)) {
				spec->networkHub = to_bool(networkHub);
			}

			auto networkInterfaces = field("networkInterfaces");
			if (is_integer(networkInterfaces)) {
				spec->networkInterfaces = to_integer(networkInterfaces);
			}

			auto networkRange = field("networkRange");
			if (is_number(networkRange)) {
				spec->networkRange = to_number(networkRange);
			}

			auto networkWifi = field("networkWifi");
			if (is_vector(networkWifi)) {
				spec->networkWifi = {
					to_number(vector_get(networkWifi, 0)),
					to_number(vector_get(networkWifi, 1)),
					to_number(vector_get(networkWifi, 2)),
				};
			}
		}

		auto store = field("store");
		if (is_bool(store)) {
			spec->store = to_bool(store);
		}

		if (spec->store) {
			auto capacity = field("capacity");
			if (is_integer(capacity)) {
				spec->capacity = to_integer(capacity);
			}

			auto priority = field("priority");
			if (is_integer(priority)) {
				spec->priority = to_integer(priority);
			}

			auto storeSetUpper = field("storeSetUpper");
			if (is_bool(storeSetUpper)) {
				spec->storeSetUpper = to_bool(storeSetUpper);
			}

			auto storeSetLower = field("storeSetLower");
			if (is_bool(storeSetLower)) {
				spec->storeSetLower = to_bool(storeSetLower);
			}

			auto storeAnything = field("storeAnything");
			if (is_bool(storeAnything)) {
				spec->storeAnything = to_bool(storeAnything);
			}

			auto storeAnythingDefault = field("storeAnythingDefault");
			if (is_bool(storeAnythingDefault)) {
				spec->storeAnythingDefault = to_bool(storeAnythingDefault);
			}

			auto storeUpgradePreserve = field("storeUpgradePreserve");
			if (is_bool(storeUpgradePreserve)) {
				spec->storeUpgradePreserve = to_bool(storeUpgradePreserve);
			}

			auto storeMagic = field("storeMagic");
			if (is_bool(storeMagic)) {
				spec->storeMagic = to_bool(storeMagic);
			}

			auto logistic = field("logistic");
			if (is_bool(logistic)) {
				spec->logistic = to_bool(logistic);
			}

			auto overflow = field("overflow");
			if (is_bool(overflow)) {
				spec->overflow = to_bool(overflow);
			}

			auto construction = field("construction");
			if (is_bool(construction)) {
				spec->construction = to_bool(construction);
			}

			auto tipStorage = field("tipStorage");
			if (is_bool(tipStorage)) {
				spec->tipStorage = to_bool(tipStorage);
			}
		}

		auto showItem = field("showItem");
		if (is_bool(showItem)) {
			spec->showItem = to_bool(showItem);
		}

		if (spec->showItem) {
			auto showItemPos = field("showItemPos");
			if (is_vector(showItemPos)) {
				spec->showItemPos = to_point(showItemPos);
			}
		}

		auto place = field("place");
		if (is_vector(place)) {
			spec->place = 0;
			for (int i = 0, l = item_count(place); i < l; i++) {
				std::string placement = to_string(vector_get(place, i));
				if (placement == "land") spec->place |= Spec::Land;
				if (placement == "hill") spec->place |= Spec::Hill;
				if (placement == "water") spec->place |= Spec::Water;
				if (placement == "monorail") spec->place |= Spec::Monorail;
				if (placement == "footings") spec->place = Spec::Footings; // disables the others
			}
		}
		auto footings = field("footings");
		if (is_vector(footings)) {
			spec->place = Spec::Footings;
			for (int i = 0, l = item_count(footings); i < l; i++) {
				auto footing = vector_get(footings, i);
				std::string placement = to_string(vector_get(footing, 0));
				auto point = to_point(vector_get(footing, 1));
				Spec::Place place = Spec::Land;
				if (placement == "hill") place = Spec::Hill;
				if (placement == "water") place = Spec::Water;
				spec->footings.push_back({.place = place, .point = point});
			}
		}

		auto placeOnHill = field("placeOnHill");
		if (is_bool(placeOnHill)) {
			spec->placeOnHill = to_bool(placeOnHill);
		}

		auto placeOnHillPlatform = field("placeOnHillPlatform");
		if (is_number(placeOnHillPlatform)) {
			spec->placeOnHillPlatform = to_number(placeOnHillPlatform);
		}

		auto materialsMultiplyHill = field("materialsMultiplyHill");
		if (is_number(materialsMultiplyHill)) {
			spec->materialsMultiplyHill = to_number(materialsMultiplyHill);
		}

		auto turret = field("turret");
		if (is_bool(turret)) {
			spec->turret = to_bool(turret);
		}

		if (spec->turret) {
			auto turretLaser = field("turretLaser");
			if (is_bool(turretLaser)) {
				spec->turretLaser = to_bool(turretLaser);
			}

			auto turretRange = field("turretRange");
			if (is_number(turretRange)) {
				spec->turretRange = to_number(turretRange);
			}

			auto turretPivotSpeed = field("turretPivotSpeed");
			if (is_number(turretPivotSpeed)) {
				spec->turretPivotSpeed = to_number(turretPivotSpeed);
			}

			auto turretCooldown = field("turretCooldown");
			if (is_integer(turretCooldown)) {
				spec->turretCooldown = to_integer(turretCooldown);
			}

			auto turretDamage = field("turretDamage");
			if (is_number(turretDamage)) {
				spec->turretDamage = to_number(turretDamage);
			}

			auto turretPivotPoint = field("turretPivotPoint");
			if (is_vector(turretPivotPoint)) {
				spec->turretPivotPoint = to_point(turretPivotPoint);
			}

			auto turretPivotFirePoint = field("turretPivotFirePoint");
			if (is_vector(turretPivotFirePoint)) {
				spec->turretPivotFirePoint = to_point(turretPivotFirePoint);
			}

			auto turretTrail = field("turretTrail");
			if (is_integer(turretTrail)) {
				spec->turretTrail = to_integer(turretTrail);
			}

			auto turretStateAlternate = field("turretStateAlternate");
			if (is_bool(turretStateAlternate)) {
				spec->turretStateAlternate = to_bool(turretStateAlternate);
			}

			auto turretStateRevert = field("turretStateRevert");
			if (is_bool(turretStateRevert)) {
				spec->turretStateRevert = to_bool(turretStateRevert);
			}
		}

		// this entity explodes when destroyed
		auto explodes = field("explodes");
		if (is_bool(explodes)) {
			spec->explodes = to_bool(explodes);
		}

		// this entity destroys hills
		auto explosive = field("explosive");
		if (is_bool(explosive)) {
			spec->explosive = to_bool(explosive);
		}

		// this exploding entity creates another explosion entity
		auto explosionSpec = field("explosionSpec");
		if (is_string(explosionSpec)) {
			spec->explosionSpec = Spec::byName(to_string(explosionSpec));
		}

		// this entity is an explosion
		auto explosion = field("explosion");
		if (is_bool(explosion)) {
			spec->explosion = to_bool(explosion);
		}

		if (spec->explosion) {
			auto explosionDamage = field("explosionDamage");
			if (is_integer(explosionDamage)) {
				spec->explosionDamage = to_integer(explosionDamage);
			}

			auto explosionRadius = field("explosionRadius");
			if (is_number(explosionRadius)) {
				spec->explosionRadius = to_number(explosionRadius);
			}

			auto explosionRate = field("explosionRate");
			if (is_number(explosionRate)) {
				spec->explosionRate = to_number(explosionRate);
			}
		}

		auto enemy = field("enemy");
		if (is_bool(enemy)) {
			spec->enemy = to_bool(enemy);
		}

		auto missile = field("missile");
		if (is_bool(missile)) {
			spec->missile = to_bool(missile);
		}

		auto missileSpeed = field("missileSpeed");
		if (is_number(missileSpeed)) {
			spec->missileSpeed = to_number(missileSpeed);
		}

		auto flightPad = field("flightPad");
		if (is_bool(flightPad)) {
			spec->flightPad = to_bool(flightPad);
		}

		if (spec->flightPad) {
			auto flightPadDepot = field("flightPadDepot");
			if (is_bool(flightPadDepot)) {
				spec->flightPadDepot = to_bool(flightPadDepot);
			}

			auto flightPadSend = field("flightPadSend");
			if (is_bool(flightPadSend)) {
				spec->flightPadSend = to_bool(flightPadSend);
			}

			auto flightPadRecv = field("flightPadRecv");
			if (is_bool(flightPadRecv)) {
				spec->flightPadRecv = to_bool(flightPadRecv);
			}

			auto flightPadHome = field("flightPadHome");
			if (is_vector(flightPadHome)) {
				spec->flightPadHome = to_point(flightPadHome);
			}
		}

		auto teleporter = field("teleporter");
		if (is_bool(teleporter)) {
			spec->teleporter = to_bool(teleporter);
		}

		if (spec->teleporter) {
			auto teleporterSend = field("teleporterSend");
			if (is_bool(teleporterSend)) {
				spec->teleporterSend = to_bool(teleporterSend);
			}

			auto teleporterRecv = field("teleporterRecv");
			if (is_bool(teleporterRecv)) {
				spec->teleporterRecv = to_bool(teleporterRecv);
			}

			auto teleporterEnergyCycle = field("teleporterEnergyCycle");
			if (is_integer(teleporterEnergyCycle)) {
				spec->teleporterEnergyCycle = to_integer(teleporterEnergyCycle);
			}
		}

		auto source = field("source");
		if (is_bool(source)) {
			spec->source = to_bool(source);
		}

		auto sourceItem = field("sourceItem");
		if (is_string(sourceItem)) {
			scenario->specSourceItems[spec] = to_string(sourceItem);
		}

		auto sourceItemRate = field("sourceItemRate");
		if (is_integer(sourceItemRate)) {
			spec->sourceItemRate = to_integer(sourceItemRate);
		}

		auto sourceFluid = field("sourceFluid");
		if (is_string(sourceFluid)) {
			scenario->specSourceFluids[spec] = to_string(sourceFluid);
		}

		auto sourceFluidRate = field("sourceFluidRate");
		if (is_integer(sourceFluidRate)) {
			spec->sourceFluidRate = to_integer(sourceFluidRate);
		}

		auto iconD = field("iconD");
		if (is_number(iconD)) {
			spec->iconD = to_number(iconD);
		}

		auto iconV = field("iconV");
		if (is_number(iconV)) {
			spec->iconV = to_number(iconV);
		}

		auto conveyor = field("conveyor");
		if (is_bool(conveyor)) {
			spec->conveyor = to_bool(conveyor);
		}

		if (spec->conveyor) {
			auto conveyorInput = field("conveyorInput");
			if (is_vector(conveyorInput)) {
				spec->conveyorInput = to_point(conveyorInput);
			}

			auto conveyorOutput = field("conveyorOutput");
			if (is_vector(conveyorOutput)) {
				spec->conveyorOutput = to_point(conveyorOutput);
			}

			auto conveyorCentroid = field("conveyorCentroid");
			if (is_vector(conveyorCentroid)) {
				spec->conveyorCentroid = to_point(conveyorCentroid);
			}

			auto conveyorSlotsLeft = field("conveyorSlotsLeft");
			if (is_integer(conveyorSlotsLeft)) {
				spec->conveyorSlotsLeft = to_integer(conveyorSlotsLeft);
			}

			auto conveyorSlotsRight = field("conveyorSlotsRight");
			if (is_integer(conveyorSlotsRight)) {
				spec->conveyorSlotsRight = to_integer(conveyorSlotsRight);
			}

			auto conveyorEnergyDrain = field("conveyorEnergyDrain");
			if (is_integer(conveyorEnergyDrain)) {
				spec->conveyorEnergyDrain = to_integer(conveyorEnergyDrain);
			}

			auto conveyorTransformsLeft = field("conveyorTransformsLeft");
			if (is_vector(conveyorTransformsLeft)) {
				for (int i = 0, l = item_count(conveyorTransformsLeft); i < l; i++) {
					Mat4 m = *((Mat4*)to_data(vector_get(conveyorTransformsLeft, i)));
					spec->conveyorTransformsLeft.push_back(m);
				}
			}

			auto conveyorTransformsRight = field("conveyorTransformsRight");
			if (is_vector(conveyorTransformsRight)) {
				for (int i = 0, l = item_count(conveyorTransformsRight); i < l; i++) {
					Mat4 m = *((Mat4*)to_data(vector_get(conveyorTransformsRight, i)));
					spec->conveyorTransformsRight.push_back(m);
				}
			}
		}

		auto unveyor = field("unveyor");
		if (is_bool(unveyor)) {
			spec->unveyor = to_bool(unveyor);
		}

		if (spec->unveyor) {
			auto unveyorEntry = field("unveyorEntry");
			if (is_bool(unveyorEntry)) {
				spec->unveyorEntry = to_bool(unveyorEntry);
			}

			auto unveyorRange = field("unveyorRange");
			if (is_number(unveyorRange)) {
				spec->unveyorRange = to_number(unveyorRange);
			}

			auto unveyorPartner = field("unveyorPartner");
			if (is_string(unveyorPartner)) {
				scenario->specUnveyorPartners[spec] = to_string(unveyorPartner);
			}
		}

		auto loader = field("loader");
		if (is_bool(loader)) {
			spec->loader = to_bool(loader);
		}

		if (spec->loader) {
			auto loaderUnload = field("loaderUnload");
			if (is_bool(loaderUnload)) {
				spec->loaderUnload = to_bool(loaderUnload);
			}

			auto loaderPoint = field("loaderPoint");
			if (is_vector(loaderPoint)) {
				spec->loaderPoint = to_point(loaderPoint);
			}
		}

		auto balancer = field("balancer");
		if (is_bool(balancer)) {
			spec->balancer = to_bool(balancer);
		}

		auto tube = field("tube");
		if (is_bool(tube)) {
			spec->tube = to_bool(tube);
		}

		if (spec->tube) {
			auto tubeOrigin = field("tubeOrigin");
			if (is_number(tubeOrigin)) {
				spec->tubeOrigin = to_number(tubeOrigin);
			}

			auto tubeSpeed = field("tubeSpeed");
			if (is_number(tubeSpeed)) {
				spec->tubeSpeed = (uint)std::floor(to_number(tubeSpeed));
			}

			auto tubeSpan = field("tubeSpan");
			if (is_number(tubeSpan)) {
				spec->tubeSpan = (uint)std::floor(to_number(tubeSpan));
			}

			auto tubeRing = field("tubeRing");
			if (is_map(tubeRing)) {
				spec->tubeRing = read_part(tubeRing);
			}

			auto tubeGlass = field("tubeGlass");
			if (is_map(tubeGlass)) {
				spec->tubeGlass = read_part(tubeGlass);
			}

			auto tubeChevron = field("tubeChevron");
			if (is_map(tubeChevron)) {
				spec->tubeChevron = read_part(tubeChevron);
			}
		}

		auto monorail = field("monorail");
		if (is_bool(monorail)) {
			spec->monorail = to_bool(monorail);
		}

		if (spec->monorail) {
			auto monorailStop = field("monorailStop");
			if (is_bool(monorailStop)) {
				spec->monorailStop = to_bool(monorailStop);
			}

			auto monorailSpan = field("monorailSpan");
			if (is_number(monorailSpan)) {
				spec->monorailSpan = (uint)std::floor(to_number(monorailSpan));
			}

			auto monorailArrive = field("monorailArrive");
			if (is_vector(monorailArrive)) {
				spec->monorailArrive = to_point(monorailArrive);
			}

			auto monorailDepart = field("monorailDepart");
			if (is_vector(monorailDepart)) {
				spec->monorailDepart = to_point(monorailDepart);
			}

			auto monorailAngle = field("monorailAngle");
			if (is_number(monorailAngle)) {
				spec->monorailAngle = to_number(monorailAngle);
			}

			auto monorailStopUnload = field("monorailStopUnload");
			if (is_vector(monorailStopUnload)) {
				spec->monorailStopUnload = to_point(monorailStopUnload);
			}

			auto monorailStopEmpty = field("monorailStopEmpty");
			if (is_vector(monorailStopEmpty)) {
				spec->monorailStopEmpty = to_point(monorailStopEmpty);
			}

			auto monorailStopFill = field("monorailStopFill");
			if (is_vector(monorailStopFill)) {
				spec->monorailStopFill = to_point(monorailStopFill);
			}

			auto monorailStopLoad = field("monorailStopLoad");
			if (is_vector(monorailStopLoad)) {
				spec->monorailStopLoad = to_point(monorailStopLoad);
			}
		}

		auto monorailContainer = field("monorailContainer");
		if (is_bool(monorailContainer)) {
			spec->monorailContainer = to_bool(monorailContainer);
		}

		auto monocar = field("monocar");
		if (is_bool(monocar)) {
			spec->monocar = to_bool(monocar);
		}

		if (spec->monocar) {
			auto monocarContainer = field("monocarContainer");
			if (is_vector(monocarContainer)) {
				spec->monocarContainer = to_point(monocarContainer);
			}

			auto monocarBogieA = field("monocarBogieA");
			if (is_number(monocarBogieA)) {
				spec->monocarBogieA = to_number(monocarBogieA);
			}

			auto monocarBogieB = field("monocarBogieB");
			if (is_number(monocarBogieB)) {
				spec->monocarBogieB = to_number(monocarBogieB);
			}

			auto monocarRoute = field("monocarRoute");
			if (is_map(monocarRoute)) {
				spec->monocarRoute = read_part(monocarRoute);
			}

			auto monocarSpeed = field("monocarSpeed");
			if (is_number(monocarSpeed)) {
				spec->monocarSpeed = to_number(monocarSpeed);
			}
		}

		auto cartStop = field("cartStop");
		if (is_bool(cartStop)) {
			spec->cartStop = to_bool(cartStop);
		}

		auto cartWaypoint = field("cartWaypoint");
		if (is_bool(cartWaypoint)) {
			spec->cartWaypoint = to_bool(cartWaypoint);
		}

		for (auto part: spec->parts) {
			spec->coloredAuto = spec->coloredAuto || part->autoColor;
			spec->coloredCustom = spec->coloredCustom || part->customColor;
			spec->color = part->color;
		}

		spec->crafterMiner = spec->crafter && spec->recipeTags.count("mining") > 0;
		spec->crafterSmelter = spec->crafter && spec->recipeTags.count("smelting") > 0;
		spec->crafterChemistry = spec->crafter && spec->recipeTags.count("chemistry") > 0;

		collect();
	}

	void add_message() {
		auto data = stack_pop();

		//char tmp[1024];
		//notef("rela: message %s", to_text(data, tmp, sizeof(tmp)));

		auto field = [&](const char* name) {
			return map_get_named(data, name);
		};

		auto name = to_string(field("name"));
		auto text = to_string(field("text"));
		auto when = to_integer(field("when"));

		auto message = new Message(name);
		message->when = when;
		message->text = text;
	}

	void add_goal() {
		auto data = stack_pop();

		//char tmp[1024];
		//notef("rela: goal %s", to_text(data, tmp, sizeof(tmp)));

		auto field = [&](const char* name) {
			return map_get_named(data, name);
		};

		auto name = to_string(field("name"));
		auto title = to_string(field("title"));
		auto goal = new Goal(name);
		goal->title = title;

		auto hints = field("hints");
		if (is_vector(hints)) {
			for (int i = 0, l = item_count(hints); i < l; i++) {
				goal->hints.push_back(to_string(vector_get(hints, i)));
			}
		}

		auto chits = field("chits");
		if (is_number(chits)) {
			goal->reward = to_number(chits);
		}

		auto period = field("period");
		if (is_number(period)) {
			goal->period = to_number(period) * Goal::hour;
		}

		auto rates = field("rates");
		if (is_vector(rates)) {
			for (int i = 0, l = item_count(rates); i < l; i++) {
				auto rate = vector_get(rates, i);
				auto rateField = [&](auto name) {
					return map_get_named(rate, name);
				};
				goal->rates.push_back({
					.iid = Item::byName(to_string(rateField("item")))->id,
					.count = (uint)to_integer(rateField("count")),
				});
			}
		}

		auto construction = field("construction");
		if (is_map(construction)) {
			for (int i = 0, l = item_count(construction); i < l; i++) {
				auto key = map_key(construction, i);
				auto val = map_get(construction, key);
				auto specName = to_string(key);
				goal->construction[Spec::byName(specName)] = to_integer(val);
			}
		}

		auto production = field("production");
		if (is_map(production)) {
			for (int i = 0, l = item_count(production); i < l; i++) {
				auto key = map_key(production, i);
				auto val = map_get(production, key);
				auto itemName = to_string(key);
				if (Item::names.count(itemName))
					goal->productionItem[Item::byName(itemName)->id] = to_integer(val);
				if (Fluid::names.count(itemName))
					goal->productionFluid[Fluid::byName(itemName)->id] = to_integer(val);
			}
		}

		auto supplies = field("supplies");
		if (is_map(supplies)) {
			for (int i = 0, l = item_count(supplies); i < l; i++) {
				auto key = map_key(supplies, i);
				auto val = map_get(supplies, key);
				auto itemName = to_string(key);
				goal->supplies.push_back({Item::byName(itemName)->id, (uint)to_integer(val)});
			}
		}

		auto dependencies = field("dependencies");
		if (is_vector(dependencies)) {
			for (int i = 0, l = item_count(dependencies); i < l; i++) {
				auto val = vector_get(dependencies, i);
				auto goalName = to_string(val);
				goal->dependencies.insert(Goal::byName(goalName));
			}
		}

		auto licenseSpecs = field("specs");
		if (is_vector(licenseSpecs)) {
			for (int i = 0, l = item_count(licenseSpecs); i < l; i++) {
				auto name = to_string(vector_get(licenseSpecs, i));
				goal->license.specs.insert(Spec::byName(name));
			}
		}

		auto licenseRecipes = field("recipes");
		if (is_vector(licenseRecipes)) {
			for (int i = 0, l = item_count(licenseRecipes); i < l; i++) {
				auto name = to_string(vector_get(licenseRecipes, i));
				goal->license.recipes.insert(Recipe::byName(name));
			}
		}

		collect();
	};

	void point_result(Point p) {
		result(make_point(p));
	}

	void point_rotate() {
		auto point = to_point(argument(0));
		auto axis = to_point(argument(1));
		double degrees = to_number(argument(2));
		point_result(point.transform(Mat4::rotate(axis, glm::radians(degrees))));
	}

	void point_scale() {
		auto point = to_point(argument(0));
		double scale = to_number(argument(1));
		point_result(point * scale);
	}

	void point_add() {
		auto pointA = to_point(argument(0));
		auto pointB = to_point(argument(1));
		point_result(pointA + pointB);
	}

	void point_transform() {
		auto point = to_point(argument(0));
		Mat4 mat = *((Mat4*)to_data(argument(1)));
		point_result(point.transform(mat));
	}

	void point_meta() {
		const char* key = to_string(argument(0));
		if (key[0] == '+' && !key[1]) { result(make_function(funcs.point_add.id)); return; }
		if (key[0] == '*' && !key[1]) { result(make_function(funcs.point_scale.id)); return; }
		result(make_nil());
	}

	void matrix_result(Mat4* m) {
		auto data = make_data(m);
		meta_set(data, make_function(funcs.matrix_meta.id));
		results(1, &data);
	}

	void matrix_identity() {
		Mat4* mat = &scenario->matrices.emplace_back();
		*mat = Mat4::identity;
		matrix_result(mat);
	}

	void matrix_scale() {
		oitem argv[3]; int argc = arguments(3, argv);
		ensure(argc == 1 || argc == 3, "matrix_scale invalid arguments");
		Point p = {1,1,1};
		if (argc == 1 && is_vector(argv[0])) p = to_point(argv[0]);
		if (argc == 1 && is_number(argv[0])) p = {to_number(argv[0]),to_number(argv[0]),to_number(argv[0])};
		if (argc == 3) p = {to_number(argv[0]),to_number(argv[1]),to_number(argv[2])};
		Mat4* mat = &scenario->matrices.emplace_back();
		*mat = Mat4::scale(p.x, p.y, p.z);
		matrix_result(mat);
	}

	void matrix_rotate() {
		auto axis = to_point(argument(0));
		double degrees = to_number(argument(1));
		Mat4* mat = &scenario->matrices.emplace_back();
		*mat = Mat4::rotate(axis, glm::radians(degrees));
		matrix_result(mat);
	}

	void matrix_rotation() {
		Point p = to_point(argument(0));
		Mat4* mat = &scenario->matrices.emplace_back();
		*mat = p.rotation();
		matrix_result(mat);
	}

	void matrix_translate() {
		oitem argv[3]; int argc = arguments(3, argv);
		ensure(argc == 1 || argc == 3, "matrix_translate invalid arguments");
		Point p = Point::Zero;
		if (argc == 1) p = to_point(argv[0]);
		if (argc == 3) p = {to_number(argv[0]),to_number(argv[1]),to_number(argv[2])};
		Mat4* mat = &scenario->matrices.emplace_back();
		*mat = Mat4::translate(p.x, p.y, p.z);
		matrix_result(mat);
	}

	void matrix_translation() {
		Point p = to_point(argument(0));
		Mat4* mat = &scenario->matrices.emplace_back();
		*mat = p.translation();
		matrix_result(mat);
	}

	void matrix_multiply() {
		Mat4* mat = &scenario->matrices.emplace_back();
		*mat = Mat4::identity;
		for (int i = 0, l = stack_depth(); i < l; i++) {
			*mat = *mat * *((Mat4*)to_data(stack_pick(i)));
		}
		matrix_result(mat);
	}

	void matrix_invert() {
		Mat4 a = *((Mat4*)to_data(argument(0)));
		Mat4* b = &scenario->matrices.emplace_back();
		*b = a.invert();
		matrix_result(b);
	}

	void matrix_meta() {
		const char* key = to_string(argument(0));
		if (key[0] == '*' && !key[1]) { result(make_function(funcs.matrix_multiply.id)); return; }
		result(make_nil());
	}

	void world_elevation() {
		auto pos = to_point(stack_pop());
		stack_push(make_number(world.elevation(pos)));
	}

	void entity_create() {
		auto name = to_string(stack_pop());
		auto& en = Entity::create(Entity::next(), Spec::byName(name));
		stack_push(make_integer(en.id));
	}

	void entity_move() {
		auto dir = to_point(stack_pop());
		auto pos = to_point(stack_pop());
		uint id = (uint)to_integer(stack_pop());
		Entity::get(id).move(pos, dir);
	}

	void entity_materialize() {
		uint id = (uint)to_integer(stack_pop());
		Entity::get(id).materialize();
	}

	void entity_insert() {
		uint count = (uint)to_integer(stack_pop());
		uint iid = Item::byName(to_string(stack_pop()))->id;
		uint id = (uint)to_integer(stack_pop());
		auto stack = Entity::get(id).store().insert({iid,count});
		stack_push(make_string(Item::get(iid)->name.c_str()));
		stack_push(make_integer(stack.size));
	}

	void entity_level() {
		uint higher = 0;
		uint lower = 0;
		bool setting = false;
		if (stack_depth() > 2) {
			higher = (uint)to_integer(stack_pop());
			lower = (uint)to_integer(stack_pop());
			setting = true;
		}
		uint iid = Item::byName(to_string(stack_pop()))->id;
		uint id = (uint)to_integer(stack_pop());
		auto& store = Entity::get(id).store();
		if (setting) {
			store.levelSet(iid, lower, higher);
		}
		auto level = store.level(iid);
		if (level) {
			stack_push(make_integer(level->lower));
			stack_push(make_integer(level->upper));
		}
	}

	void entity_direct() {
		uint eid = (uint)to_integer(stack_pop());
		scene.directRevert = eid;
	}

	void toolbar_spec() {
		auto name = to_string(stack_pop());
		gui.toolbar->add(Spec::byName(name));
	}

	void license_spec() {
		auto name = to_string(stack_pop());
		Spec::byName(name)->licensed = true;
	}

	void license_recipe() {
		auto name = to_string(stack_pop());
		Recipe::byName(name)->licensed = true;
	}
};

ScenarioBase::ScenarioBase() {
	rela = new RelaMod();
}

Mesh* ScenarioBase::mesh(const char* name) {
	ensuref(meshes.count(name), "unknown mesh: %s", name);
	return meshes[name];
}

void ScenarioBase::generate() {
	int jobs = 0;
	channel<bool,-1> done;

	struct result {
		Spec* spec = nullptr;
		World::XY at = {0,0};
	};

	channel<result> results;

	jobs++;
	crew.job([&]() {
		for (auto res: results) {
			auto spec = res.spec;
			auto x = res.at.x;
			auto y = res.at.y;
			auto e = world.elevation(res.at);
			auto p = Point((float)(x), e + spec->collision.h/2.0f, (float)(y));
			Entity::create(Entity::next(), spec)
				.move(p, Point::South.randomHorizontal())
				.materialize()
			;
		}
		done.send(true);
	});

	auto generateRocks = [&](Box box) {
		for (auto at: world.walk(box)) {
			if (world.isLand(at.centroid().box().grow(1))) {
				double n = Sim::noise2D(at.x + 1000000, at.y + 1000000, 8, 0.6, 0.015);
				if (n < 0.4 && Sim::random() < 0.04) {
					Spec *spec = rocks[Sim::choose(rocks.size())];
					results.send({.spec = spec, .at = at});
				}
			}
		}
	};

	std::vector<Spec*> trees;
	for (auto [name,spec]: Spec::all) {
		if (spec->junk && name.substr(0,4) == "tree") {
			trees.push_back(spec);
		}
	}

	auto generateTrees = [&](Box box) {
		for (auto at: world.walk(box)) {
			if (!world.isLake(at.centroid().box().grow(1))) {
				double n = Sim::noise2D(at.x + 2000000, at.y + 2000000, 8, 0.6, 0.015);
				if (n < 0.4 && Sim::random() < 0.04) {
					Spec *spec = trees[Sim::choose(trees.size())];
					results.send({.spec = spec, .at = at});
				}
			}
		}
	};

	uint size = world.size()/8;
	for (auto chunk: gridwalk(size, world.box())) {
		jobs++;
		crew.job([&,chunk]() {
			auto box = Point(
				(float)chunk.x * size + size/2,
				0,
				(float)chunk.y * size + size/2
			).box().grow(size/2);
			generateRocks(box);
			generateTrees(box);
			done.send(true);
		});
	}

	while (jobs > 1) { done.recv(); jobs--; }
	results.close();
	done.recv();
}

void ScenarioBase::init() {
	world.scenario.size = std::max(Config::mode.world, 4096);
}

void ScenarioBase::create() {
	generate();
	rela->run({rela->modules.common, rela->modules.create});
	matrices.clear();
}

void ScenarioBase::load() {
	for (auto [_,goal]: Goal::all) goal->load();
}

bool ScenarioBase::attack() {
	auto produced = Item::byName("ammo1")->produced;
	return Goal::byName("defence")->met && produced > 100;
}

float ScenarioBase::attackFactor() {
	auto count = Item::supplied[Item::byName("ammo1")->id];
	return ((float)count / 2000.0f) + 1.0f;
}

void ScenarioBase::tick() {
}

void ScenarioBase::items() {
	meshes["oreLD"] = new MeshSphere(0.6f);
	meshes["unitCube"] = scene.unit.mesh.cube;
	meshes["unitSphere"] = scene.unit.mesh.sphere;
	rela->run({rela->modules.common, rela->modules.items});
}

void ScenarioBase::fluids() {

	Fluid* fluid = Fluid::create(Fluid::next(), "water");
	fluid->title = "Water";
	fluid->color = 0x0000ffff;
	fluid->raw = true;

	fluid = Fluid::create(Fluid::next(), "steam");
	fluid->title = "Steam";
	fluid->thermal = Energy::kJ(20);
	fluid->color = 0xccccccff;

	fluid = Fluid::create(Fluid::next(), "oil");
	fluid->title = "Oil";
	fluid->color = 0x111111ff;
	Fluid::drilling[fluid->id] = 1.0f;
	fluid->raw = true;

	fluid = Fluid::create(Fluid::next(), "slurry");
	fluid->title = "Slurry";
	fluid->color = 0xb5651dff;

	fluid = Fluid::create(Fluid::next(), "lubricant");
	fluid->title = "Lubricant";
	fluid->color = 0x006400ff;

	fluid = Fluid::create(Fluid::next(), "naphtha");
	fluid->title = "Naphtha";
	fluid->color = 0x7B68EEff;

	fluid = Fluid::create(Fluid::next(), "fuel-oil");
	fluid->title = "Fuel Oil";
	fluid->color = 0xFFD700ff;

	fluid = Fluid::create(Fluid::next(), "sulfuric-acid");
	fluid->title = "Sulfuric Acid";
	fluid->color = 0xFFff00ff;

	fluid = Fluid::create(Fluid::next(), "hydrofluoric-acid");
	fluid->title = "Hydrofluoric Acid";
	fluid->color = 0x00ffffff;

	fluid = Fluid::create(Fluid::next(), "hydrogen");
	fluid->title = "Liquid Hydrogen";
	fluid->color = 0xee82eeff;

	fluid = Fluid::create(Fluid::next(), "oxygen");
	fluid->title = "Liquid Oxygen";
	fluid->color = 0x87ceebff;

	fluid = Fluid::create(Fluid::next(), "hydrazine");
	fluid->title = "Hydrazine";
	fluid->color = 0xee82eeff;
}

void ScenarioBase::goals() {
	rela->run({rela->modules.common, rela->modules.goals});
}

void ScenarioBase::messages() {
	rela->run({rela->modules.common, rela->modules.messages});
}

void ScenarioBase::recipes() {
	rela->run({rela->modules.common, rela->modules.recipes});
}

void ScenarioBase::specifications() {
	rela->run({rela->modules.common, rela->modules.specs});

	for (auto [spec,upgrade]: specUpgrades) {
		spec->upgrade = Spec::byName(upgrade);
	}

	for (auto [spec,upgrades]: specUpgradeCascades) {
		for (auto upgrade: upgrades) {
			spec->upgradeCascade.insert(Spec::byName(upgrade));
		}
	}

	for (auto [spec,cycle]: specCycles) {
		spec->cycle = Spec::byName(cycle);
	}

	for (auto [spec,pipette]: specPipettes) {
		spec->pipette = Spec::byName(pipette);
	}

	for (auto [spec,statsGroup]: specStatsGroups) {
		spec->statsGroup = Spec::byName(statsGroup);
	}

	for (auto [spec,name]: specSourceItems) {
		spec->sourceItem = Item::byName(name);
	}

	for (auto [spec,name]: specSourceFluids) {
		spec->sourceFluid = Fluid::byName(name);
	}

	for (auto [spec,name]: specUnveyorPartners) {
		spec->unveyorPartner = Spec::byName(name);
	}

	for (auto [spec,name]: specFollows) {
		spec->follow = Spec::byName(name);
	}

	for (auto [spec,name]: specFollowConveyors) {
		spec->followConveyor = Spec::byName(name);
	}

	for (auto [spec,name]: specUpwards) {
		spec->upward = Spec::byName(name);
	}

	for (auto [spec,name]: specDownwards) {
		spec->downward = Spec::byName(name);
	}

	Spec* spec;

	meshes["fan"] = new Mesh("models/fan-hd.stl");
	meshes["fanLD"] = new Mesh("models/fan-ld.stl");
	meshes["chevron"] = new Mesh("models/pipe-chevron-hd.stl");

	Spec::byName("container-provider")->upgrade = Spec::byName("container-buffer");

	meshes["assemblerPiston"] = new Mesh("models/assembler-piston-hd.stl");
	meshes["assemblerPistonLD"] = new Mesh("models/assembler-piston-ld.stl");

	meshes["assemblerChassis"] = new Mesh("models/assembler-chassis-hd.stl");
	meshes["assemblerChassisLD"] = new Mesh("models/assembler-chassis-ld.stl");
	meshes["assemblerChassisVLD"] = new Mesh("models/assembler-chassis-vld.stl");

	meshes["windTurbineTower"] = new Mesh("models/wind-turbine-tower-hd.stl");
	meshes["windTurbineTowerLD"] = new Mesh("models/wind-turbine-tower-ld.stl");
	meshes["windTurbineTowerVLD"] = new Mesh("models/wind-turbine-tower-vld.stl");
	meshes["windTurbineBlade"] = new Mesh("models/wind-turbine-blade-hd.stl");
	meshes["windTurbineBladeLD"] = new Mesh("models/wind-turbine-blade-ld.stl");
	meshes["windTurbineBladeVLD"] = new Mesh("models/wind-turbine-blade-vld.stl");

	spec = new Spec("wind-turbine");
	spec->title = "Wind Turbine";
	spec->collision = {0, 0, 0, 10, 15, 10};
	spec->selection = spec->collision;
	spec->iconD = 8;
	spec->iconV = 0;
	spec->place = Spec::Land | Spec::Hill;
	spec->placeOnHill = true;
	spec->placeOnHillPlatform = 3.0f;
	spec->windTurbine = true;
	spec->generateElectricity = true;
	spec->energyGenerate = Energy::kW(20);
	spec->health = 150;
	spec->enemyTarget = true;

	spec->materials = {
		{ Item::byName("steel-sheet")->id, 5 },
		{ Item::byName("brick")->id, 5 },
	};

	spec->parts = {
		(new Part(0xB0C4DEff))
			->lod(mesh("windTurbineTower"), Part::HD, Part::SHADOW)
			->lod(mesh("windTurbineTowerLD"), Part::MD, Part::SHADOW)
			->lod(mesh("windTurbineTowerVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-7.5f,0)),
		(new Part(0xFFFAFAff))
			->lod(mesh("windTurbineBlade"), Part::HD, Part::SHADOW)
			->lod(mesh("windTurbineBladeLD"), Part::MD, Part::SHADOW)
			->lod(mesh("windTurbineBladeVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-7.5f,0)),
		(new Part(0xFFFAFAff))
			->lod(mesh("windTurbineBlade"), Part::HD, Part::SHADOW)
			->lod(mesh("windTurbineBladeLD"), Part::MD, Part::SHADOW)
			->lod(mesh("windTurbineBladeVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-7.5f,0)),
		(new Part(0xFFFAFAff))
			->lod(mesh("windTurbineBlade"), Part::HD, Part::SHADOW)
			->lod(mesh("windTurbineBladeLD"), Part::MD, Part::SHADOW)
			->lod(mesh("windTurbineBladeVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-7.5f,0)),
	};

	{
		Mat4 state0 = Mat4::identity;

		for (int i = 0; i < 360; i++) {
			Mat4 state1 = Mat4::rotateY(glm::radians((float)i));
			Mat4 state2 = Mat4::rotateY(glm::radians((float)(i+120)));
			Mat4 state3 = Mat4::rotateY(glm::radians((float)(i+240)));

			spec->states.push_back({
				state0,
				state1,
				state2,
				state3,
			});
		}
	}

	meshes["fluidTankSmall"] = new Mesh("models/fluid-tank-small-hd.stl");
	meshes["fluidTankSmallLD"] = new Mesh("models/fluid-tank-small-ld.stl");
	meshes["fluidTankSmallVLD"] = new Mesh("models/fluid-tank-small-vld.stl");

	meshes["pipe"] = new Mesh("models/pipe-straight-hd.stl");
	meshes["pipeLD"] = new Mesh("models/pipe-straight-ld.stl");
	meshes["pipeVLD"] = new Mesh("models/pipe-straight-vld.stl");

	meshes["pipeGround"] = new Mesh("models/pipe-ground-hd.stl");
	meshes["pipeGroundLD"] = new Mesh("models/pipe-ground-ld.stl");
	meshes["pipeGroundVLD"] = new Mesh("models/pipe-ground-vld.stl");

//	spec = new Spec("pipe-underground");
//	spec->title = "Pipe (Underground)";
//	spec->pipe = true;
//	spec->pipeCapacity = Liquid::l(500);
//	spec->pipeUnderground = true;
//	spec->pipeUndergroundRange = 10.0f;
//	spec->collision = {0, 0, 0, 1, 1, 1};
//	spec->selection = spec->collision;
//	spec->rotateGhost = true;
//	spec->rotateExtant = true;
//	spec->pipeConnections = {Point::North*0.5f};
//	spec->health = 150;
//
//	spec->parts = {
//		(new Part(0xffa500ff))
//			->lod(mesh("pipeGround"), Part::HD, Part::SHADOW)
//			->lod(mesh("pipeGroundLD"), Part::MD, Part::SHADOW)
//			->lod(mesh("pipeGroundVLD"), Part::VLD, Part::NOSHADOW)
//			->transform(Mat4::rotate(Point::Up, glm::radians(-90.0f))),
//	};
//
//	spec->materials = {
//		{ Item::byName("pipe")->id, 5 },
//	};

	auto rockVLD = new Mesh("models/rock-vld.stl");

	for (int i = 1; i < 4; i++) {
		auto name = "rock" + std::to_string(i);
		auto hd = "models/" + name + "-hd.stl";
		auto ld = "models/" + name + "-ld.stl";

		spec = new Spec(name);
		spec->title = "Rock";
		spec->build = false;
		spec->collision = {0, 0, 0, 2, 1, 2};
		spec->selection = spec->collision;
		spec->health = 500;
		spec->junk = true;
		spec->parts = {
			(new Part(0x888888ff))
				->lod(new Mesh(hd), Part::HD, Part::SHADOW)
				->lod(new Mesh(ld), Part::MD, Part::NOSHADOW)
				->lod(rockVLD, Part::VLD, Part::NOSHADOW),
		};
		spec->materials = {
			{Item::byName("stone")->id, 1},
		};

		rocks.push_back(spec);
	}

	meshes["droneChassis"] = new Mesh("models/drone-chassis-hd.stl");
	meshes["droneChassisLD"] = new Mesh("models/drone-chassis-ld.stl");
	meshes["droneSpars"] = new Mesh("models/drone-spars-hd.stl");
	meshes["droneSparsLD"] = new Mesh("models/drone-spars-ld.stl");
	meshes["droneRotor"] = new Mesh("models/drone-rotor-hd.stl");
	meshes["droneRotorLD"] = new Mesh("models/drone-rotor-ld.stl");

	spec = new Spec("drone");
	spec->title = "Drone";
	spec->select = false;
	spec->build = false;
	spec->collision = {0, 0, 0, 1, 1, 1};
	spec->selection = spec->collision;
	spec->parts = {
		(new Part(0x990000ff))
			->lod(mesh("droneChassis"), Part::HD, Part::SHADOW)
			->lod(mesh("droneChassis"), Part::MD, Part::NOSHADOW),
		(new Part(0x444444ff))
			->lod(mesh("droneSpars"), Part::HD, Part::SHADOW)
			->lod(mesh("droneSparsLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::rotate(Point::Up, glm::radians(45.0f))),
		(new PartSpinner(0x999999ff, Point::Up, 45))
			->lod(mesh("droneRotor"), Part::HD, Part::NOSHADOW)
			->lod(mesh("droneRotorLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0.3,0.02,0.3)),
		(new PartSpinner(0x999999ff, Point::Up, 45))
			->lod(mesh("droneRotor"), Part::HD, Part::NOSHADOW)
			->lod(mesh("droneRotorLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0.3,0.02,-0.3)),
		(new PartSpinner(0x999999ff, Point::Up, 45))
			->lod(mesh("droneRotor"), Part::HD, Part::NOSHADOW)
			->lod(mesh("droneRotorLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(-0.3,0.02,0.3)),
		(new PartSpinner(0x999999ff, Point::Up, 45))
			->lod(mesh("droneRotor"), Part::HD, Part::NOSHADOW)
			->lod(mesh("droneRotorLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(-0.3,0.02,-0.3)),
	};
	spec->align = false;
	spec->drone = true;
	spec->droneSpeed = 0.2f;
	spec->droneCargoSize = 5;
	spec->collideBuild = false;

//	Spec::byName("crawler")->depotDroneSpec = Spec::byName("drone");

	meshes["armBase"] = new Mesh("models/arm-base-hd.stl");
	meshes["armBaseLD"] = new Mesh("models/arm-base-ld.stl");
	meshes["armPillar"] = new Mesh("models/arm-pillar-hd.stl");
	meshes["armPillarLD"] = new Mesh("models/arm-pillar-ld.stl");
	meshes["armTelescope1"] = new Mesh("models/arm-telescope1-hd.stl");
	meshes["armTelescope1LD"] = new Mesh("models/arm-telescope1-ld.stl");
	meshes["armTelescope2"] = new Mesh("models/arm-telescope2-hd.stl");
	meshes["armTelescope2LD"] = new Mesh("models/arm-telescope2-ld.stl");
	meshes["armTelescope3"] = new Mesh("models/arm-telescope3-hd.stl");
	meshes["armTelescope3LD"] = new Mesh("models/arm-telescope3-ld.stl");
	meshes["armTelescope4"] = new Mesh("models/arm-telescope4-hd.stl");
	meshes["armTelescope4LD"] = new Mesh("models/arm-telescope4-ld.stl");
	meshes["armTelescope5"] = new Mesh("models/arm-telescope5-hd.stl");
	meshes["armTelescope5LD"] = new Mesh("models/arm-telescope5-ld.stl");
	meshes["armGrip"] = new Mesh("models/arm-grip-hd.stl");
	meshes["armGripLD"] = new Mesh("models/arm-grip-ld.stl");

	spec = new Spec("arm");
	spec->title = "Arm";
	spec->toolbar = "a-a";
	spec->health = 150;
	spec->collision = {0, 0, 0, 1, 2, 1};
	spec->selection = spec->collision;
	spec->iconD = 1.25;
	spec->iconV = 0.0;
	spec->enable = true;
	spec->arm = true;
	spec->armInput = Point::North + Point::Down*0.25f;
	spec->armOutput = Point::South + Point::Down*0.25f;
	spec->armSpeed = 1.0f/60.0f;
	spec->rotateGhost = true;
	spec->rotateExtant = true;
	spec->networker = true;
	spec->networkInterfaces = 1;
	spec->networkWifi = {0,-0.8,0};
	spec->status = true;
	spec->beacon = {0,-0.8,0};
	spec->parts = {
		(new Part(0x4169E1ff))
			->lod(mesh("armBase"), Part::HD, Part::SHADOW)
			->lod(mesh("armBaseLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0x4169E1ff))
			->lod(mesh("armPillar"), Part::HD, Part::SHADOW)
			->lod(mesh("armPillarLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0xC0C0C0ff))->gloss(8)
			->lod(mesh("armTelescope1"), Part::HD, Part::SHADOW)
			->lod(mesh("armTelescope1LD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0xC0C0C0ff))->gloss(8)
			->lod(mesh("armTelescope2"), Part::HD, Part::SHADOW)
			->lod(mesh("armTelescope2LD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0xC0C0C0ff))->gloss(8)
			->lod(mesh("armTelescope3"), Part::HD, Part::SHADOW)
			->lod(mesh("armTelescope3LD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0x4169E1ff))->gloss(8)
			->lod(mesh("armGrip"), Part::HD, Part::SHADOW)
			->lod(mesh("armGripLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
	};
	spec->consumeElectricity = true;
	spec->energyConsume = Energy::kW(10);
	spec->energyDrain = Energy::W(100);
	spec->materials = {
		{ Item::byName("steel-frame")->id, 1 },
		{ Item::byName("circuit-board")->id, 1 },
	};

	// Arm states:
	// 0-359: rotation
	// 360-n: parking

	{
		Mat4 state0 = Mat4::identity;

		for (uint i = 0; i < 360; i++) {
			Mat4 state = Mat4::rotateY(glm::radians((float)i));

			float theta = (float)i;

			float a = sin(glm::radians(theta))*1.0;
			float b = cos(glm::radians(theta))*0.4;
			float r = 1.0*0.4 / std::sqrt(a*a + b*b);

			Point t1 = Point::North * (0.0f*r) - Point::North * 0.2f;
			Point t2 = Point::North * (0.3f*r) - Point::North * 0.2f;
			Point t3 = Point::North * (0.6f*r) - Point::North * 0.2f;
			Point g  = Point::North * (0.6f*r) + Point::North * 0.2f;

			Mat4 extend1 = Mat4::translate(t1.x, t1.y, t1.z) * state;
			Mat4 extend2 = Mat4::translate(t2.x, t2.y, t2.z) * state;
			Mat4 extend3 = Mat4::translate(t3.x, t3.y, t3.z) * state;
			Mat4 extendG = Mat4::translate(g.x, g.y, g.z) * state;

			spec->states.push_back({
				state0,
				state,
				extend1,
				extend2,
				extend3,
				extendG,
			});
		}
	}

	{
		Mat4 state0 = Mat4::identity;

		for (uint i = 0; i < 90; i+=10) {
			Mat4 state = state0;

			float theta = (float)i;

			float a = sin(glm::radians(theta))*1.0;
			float b = cos(glm::radians(theta))*0.1;
			float r = 1.0*0.1 / std::sqrt(a*a + b*b);

			Point t1 = Point::North * (0.0f*r) - Point::North * 0.2f;
			Point t2 = Point::North * (0.3f*r) - Point::North * 0.2f;
			Point t3 = Point::North * (0.6f*r) - Point::North * 0.2f;
			Point g  = Point::North * (0.6f*r) + Point::North * 0.2f;

			Mat4 extend1 = Mat4::translate(t1.x, t1.y, t1.z) * state;
			Mat4 extend2 = Mat4::translate(t2.x, t2.y, t2.z) * state;
			Mat4 extend3 = Mat4::translate(t3.x, t3.y, t3.z) * state;
			Mat4 extendG = Mat4::translate(g.x, g.y, g.z) * state;

			spec->states.push_back({
				state0,
				state,
				extend1,
				extend2,
				extend3,
				extendG,
			});
		}
	}

	spec = new Spec("arm-long");
	spec->title = "Arm (Long)";
	spec->toolbar = "a-b";
	spec->collision = {0, 0, 0, 1, 2, 1};
	spec->selection = spec->collision;
	spec->iconD = 1.25;
	spec->iconV = 0.0;
	spec->enable = true;
	spec->arm = true;
	spec->armInput = Point::North*2.0f + Point::Down*0.25f;
	spec->armOutput = Point::South*2.0f + Point::Down*0.25f;
	spec->armSpeed = 1.0f/60.0f;
	spec->rotateGhost = true;
	spec->rotateExtant = true;
	spec->health = 150;
	spec->networker = true;
	spec->networkInterfaces = 1;
	spec->networkWifi = {0,-0.8,0};
	spec->status = true;
	spec->beacon = {0,-0.8,0};
	spec->parts = {
		(new Part(0xFF4500ff))
			->lod(mesh("armBase"), Part::HD, Part::SHADOW)
			->lod(mesh("armBaseLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0xFF4500ff))
			->lod(mesh("armPillar"), Part::HD, Part::SHADOW)
			->lod(mesh("armPillarLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0xC0C0C0ff))->gloss(8)
			->lod(mesh("armTelescope1"), Part::HD, Part::SHADOW)
			->lod(mesh("armTelescope1LD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0xC0C0C0ff))->gloss(8)
			->lod(mesh("armTelescope2"), Part::HD, Part::SHADOW)
			->lod(mesh("armTelescope2LD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0xC0C0C0ff))->gloss(8)
			->lod(mesh("armTelescope3"), Part::HD, Part::SHADOW)
			->lod(mesh("armTelescope3LD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0xC0C0C0ff))->gloss(8)
			->lod(mesh("armTelescope4"), Part::HD, Part::SHADOW)
			->lod(mesh("armTelescope4LD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0xC0C0C0ff))->gloss(8)
			->lod(mesh("armTelescope5"), Part::HD, Part::SHADOW)
			->lod(mesh("armTelescope5LD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
		(new Part(0xFF4500ff))->gloss(8)
			->lod(mesh("armGrip"), Part::HD, Part::SHADOW)
			->lod(mesh("armGripLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
	};
	spec->consumeElectricity = true;
	spec->energyConsume = Energy::kW(15);
	spec->energyDrain = Energy::W(300);
	spec->materials = {
		{ Item::byName("steel-frame")->id, 2 },
		{ Item::byName("circuit-board")->id, 1 },
	};

	// Arm states:
	// 0-359: rotation
	// 360-n: parking

	{
		Mat4 state0 = Mat4::identity;

		for (uint i = 0; i < 360; i++) {
			Mat4 state = Mat4::rotateY(glm::radians((float)i));

			float theta = (float)i;

			float a = sin(glm::radians(theta))*1.0;
			float b = cos(glm::radians(theta))*0.4;
			float r = 1.0*0.4 / std::sqrt(a*a + b*b);

			Point t1 = Point::North * (0.0f*r) - Point::North * 0.2f;
			Point t2 = Point::North * (0.4f*r) - Point::North * 0.2f;
			Point t3 = Point::North * (0.8f*r) - Point::North * 0.2f;
			Point t4 = Point::North * (1.2f*r) - Point::North * 0.2f;
			Point t5 = Point::North * (1.6f*r) - Point::North * 0.2f;
			Point g  = Point::North * (1.6f*r) + Point::North * 0.2f;

			Mat4 extend1 = Mat4::translate(t1.x, t1.y, t1.z) * state;
			Mat4 extend2 = Mat4::translate(t2.x, t2.y, t2.z) * state;
			Mat4 extend3 = Mat4::translate(t3.x, t3.y, t3.z) * state;
			Mat4 extend4 = Mat4::translate(t4.x, t4.y, t4.z) * state;
			Mat4 extend5 = Mat4::translate(t5.x, t5.y, t5.z) * state;
			Mat4 extendG = Mat4::translate(g.x, g.y, g.z) * state;

			spec->states.push_back({
				state0,
				state,
				extend1,
				extend2,
				extend3,
				extend4,
				extend5,
				extendG,
			});
		}
	}

	{
		Mat4 state0 = Mat4::identity;

		for (uint i = 0; i < 90; i+=10) {
			Mat4 state = state0;

			float theta = (float)i;

			float a = sin(glm::radians(theta))*1.0;
			float b = cos(glm::radians(theta))*0.1;
			float r = 1.0*0.1 / std::sqrt(a*a + b*b);

			Point t1 = Point::North * (0.0f*r) - Point::North * 0.2f;
			Point t2 = Point::North * (0.4f*r) - Point::North * 0.2f;
			Point t3 = Point::North * (0.8f*r) - Point::North * 0.2f;
			Point t4 = Point::North * (1.2f*r) - Point::North * 0.2f;
			Point t5 = Point::North * (1.6f*r) - Point::North * 0.2f;
			Point g  = Point::North * (1.6f*r) + Point::North * 0.2f;

			Mat4 extend1 = Mat4::translate(t1.x, t1.y, t1.z) * state;
			Mat4 extend2 = Mat4::translate(t2.x, t2.y, t2.z) * state;
			Mat4 extend3 = Mat4::translate(t3.x, t3.y, t3.z) * state;
			Mat4 extend4 = Mat4::translate(t4.x, t4.y, t4.z) * state;
			Mat4 extend5 = Mat4::translate(t5.x, t5.y, t5.z) * state;
			Mat4 extendG = Mat4::translate(g.x, g.y, g.z) * state;

			spec->states.push_back({
				state0,
				state,
				extend1,
				extend2,
				extend3,
				extend4,
				extend5,
				extendG,
			});
		}
	}

	Spec::byName("arm")->cycle = Spec::byName("arm-long");
	Spec::byName("arm-long")->cycle = Spec::byName("arm");

	spec = new Spec("nuclear-reactor");
	spec->title = "Nuclear Reactor";
	spec->enemyTarget = true;
	spec->collision = {0, 0, 0, 12, 14, 12};
	spec->selection = spec->collision;
	spec->iconD = 9;
	spec->iconV = 2.0;
	spec->pipe = true;
	spec->pipeCapacity = Liquid::l(10000);
	spec->pipeHints = true;
	spec->pipeConnections = {
		{ 3.5f, -6.5f,  6.0f},
		{ 3.5f, -6.5f, -6.0f},
		{-3.5f, -6.5f,  6.0f},
		{-3.5f, -6.5f, -6.0f},
	};
	spec->pipeOutputConnections = {
		{ 6.0f, -6.5f,  3.5f},
		{ 6.0f, -6.5f, -3.5f},
		{-6.0f, -6.5f,  3.5f},
		{-6.0f, -6.5f, -3.5f},
	};

	spec->health = 150;
	spec->align = true;
	spec->rotateGhost = true;
	spec->consumeFuel = true;
	spec->burnerState = true;
	spec->consumeFuelType = "nuclear";
	spec->energyConsume = Energy::MW(40);
	spec->energyDrain = Energy::MW(1);
	spec->crafter = true;
	spec->crafterRate = 1.0f;
	spec->crafterOnOff = true;
	spec->store = true;
	spec->priority = 10;
	spec->capacity = Mass::kg(10);
	spec->recipeTags = {"reacting"};
	spec->crafterRecipe = Recipe::byName("reacting");
	spec->materials = {
		{ Item::byName("brick")->id, 5 },
		{ Item::byName("copper-sheet")->id, 3 },
	};

	meshes["bwrBase"] = new Mesh("models/bwr-base-hd.stl");
	meshes["bwrBaseLD"] = new Mesh("models/bwr-base-ld.stl");
	meshes["bwrVessel"] = new Mesh("models/bwr-vessel-hd.stl");
	meshes["bwrVesselLD"] = new Mesh("models/bwr-vessel-ld.stl");
	meshes["bwrPool"] = new Mesh("models/bwr-pool-hd.stl");
	meshes["bwrPoolLD"] = new Mesh("models/bwr-pool-ld.stl");

	meshes["bwrPoolPipe"] = new Mesh("models/bwr-pool-pipe-hd.stl");
	meshes["bwrPoolPipeLD"] = new Mesh("models/bwr-pool-pipe-ld.stl");
	meshes["bwrFlowPipe"] = new Mesh("models/bwr-flow-pipe-hd.stl");
	meshes["bwrFlowPipeLD"] = new Mesh("models/bwr-flow-pipe-ld.stl");
	meshes["bwrPortal"] = new Mesh("models/bwr-portal-hd.stl");
	meshes["bwrPortalLD"] = new Mesh("models/bwr-portal-ld.stl");
	meshes["bwrPortalScreen"] = new Mesh("models/bwr-portal-screen-hd.stl");
	meshes["bwrPortalScreenLD"] = new Mesh("models/bwr-portal-screen-ld.stl");

	auto bwrPoolPipe = (new Part(0x444444ff))
		->lod(mesh("bwrPoolPipe"), Part::HD, Part::SHADOW)
		->lod(mesh("bwrPoolPipeLD"), Part::VLD, Part::SHADOW);

	auto bwrFlowPipe = (new Part(0xffa500ff))
		->lod(mesh("bwrFlowPipe"), Part::HD, Part::SHADOW)
		->lod(mesh("bwrFlowPipeLD"), Part::VLD, Part::SHADOW);

	auto bwrPortal = (new Part(0x444444ff))
		->lod(mesh("bwrPortal"), Part::HD, Part::SHADOW)
		->lod(mesh("bwrPortalLD"), Part::VLD, Part::NOSHADOW);

	auto bwrPortalScreenOff = (new Part(0x000000ff))->translucent()
		->lod(mesh("bwrPortalScreen"), Part::HD, Part::NOSHADOW)
		->lod(mesh("bwrPortalScreenLD"), Part::MD, Part::NOSHADOW);

	auto bwrPortalScreenOn = (new Part(0x00ff00ff))->translucent()
		->lod(mesh("bwrPortalScreen"), Part::HD, Part::NOSHADOW)
		->lod(mesh("bwrPortalScreenLD"), Part::MD, Part::NOSHADOW);

	spec->parts = {
		(new Part(0xB0C4DEff))
			->lod(mesh("bwrBase"), Part::HD, Part::SHADOW)
			->lod(mesh("bwrBaseLD"), Part::VLD, Part::SHADOW),

		(new Part(0xffffffff))
			->lod(mesh("bwrVessel"), Part::HD, Part::SHADOW)
			->lod(mesh("bwrVesselLD"), Part::VLD, Part::SHADOW),

		(new Part(0xffffffff))
			->lod(mesh("bwrPool"), Part::HD, Part::SHADOW)
			->lod(mesh("bwrPoolLD"), Part::VLD, Part::SHADOW),

		bwrPoolPipe,
		bwrPoolPipe,
		bwrPoolPipe,
		bwrPoolPipe,
		bwrPoolPipe,
		bwrPoolPipe,
		bwrPoolPipe,
		bwrPoolPipe,
		bwrFlowPipe,
		bwrFlowPipe,
		bwrFlowPipe,
		bwrFlowPipe,
		bwrPortal,
		bwrPortal,
		bwrPortal,
		bwrPortal,
		bwrPortalScreenOff,
		bwrPortalScreenOff,
		bwrPortalScreenOff,
		bwrPortalScreenOff,
		bwrPortalScreenOn,
		bwrPortalScreenOn,
		bwrPortalScreenOn,
		bwrPortalScreenOn,
	};

	spec->parts[0]->filter = 1;

	{
		auto center = Mat4::translate(0,-7,0);

		spec->states.push_back({
			center,
			center,
			center,

			// pool pipes
			Mat4::rotate(Point::Up, -glm::radians(  0.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians( 45.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians( 90.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(135.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(180.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(225.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(270.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(315.0)) * center,

			// flow pipes
			Mat4::rotate(Point::Up, -glm::radians( 45.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(135.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(225.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(315.0)) * center,

			// portals
			Mat4::rotate(Point::Up, -glm::radians(  0.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians( 90.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(180.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(270.0)) * center,

			// portal screens off
			Mat4::rotate(Point::Up, -glm::radians(  0.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians( 90.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(180.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(270.0)) * center,

			// portal screens on
			Mat4::rotate(Point::Up, -glm::radians(  0.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians( 90.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(180.0)) * center,
			Mat4::rotate(Point::Up, -glm::radians(270.0)) * center,
		});

		spec->states.push_back(spec->states.back());

		spec->statesShow.push_back({
			true,
			true,
			true,

			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,

			true,
			true,
			true,
			true,

			true,
			true,
			true,
			true,

			true,
			true,
			true,
			true,

			false,
			false,
			false,
			false,
		});

		spec->statesShow.push_back({
			true,
			true,
			true,

			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,

			true,
			true,
			true,
			true,

			true,
			true,
			true,
			true,

			false,
			false,
			false,
			false,

			true,
			true,
			true,
			true,
		});
	}

	meshes["steamEngineBoiler"] = new Mesh("models/steam-engine-boiler-hd.stl");
	meshes["steamEngineBoilerLD"] = new Mesh("models/steam-engine-boiler-ld.stl");
	meshes["steamEngineSaddle"] = new Mesh("models/steam-engine-saddle-hd.stl");
	meshes["steamEngineSaddleLD"] = new Mesh("models/steam-engine-saddle-ld.stl");
	meshes["steamEngineFoot"] = new Mesh("models/steam-engine-foot-hd.stl");
	meshes["steamEngineFootLD"] = new Mesh("models/steam-engine-foot-ld.stl");
	meshes["steamEngineAxel"] = new Mesh("models/steam-engine-axel-hd.stl");
	meshes["steamEngineAxelLD"] = new Mesh("models/steam-engine-axel-ld.stl");
	meshes["steamEngineWheel"] = new Mesh("models/steam-engine-wheel-hd.stl");
	meshes["steamEngineWheelLD"] = new Mesh("models/steam-engine-wheel-ld.stl");

	spec = new Spec("steam-engine");
	spec->title = "Steam Engine";
	spec->collision = {0, 0, 0, 4, 4, 5};
	spec->selection = spec->collision;
	spec->iconD = 3.5;
	spec->iconV = 0;
	spec->pipe = true;
	spec->pipeCapacity = Liquid::l(100);
	spec->pipeHints = true;
	spec->pipeConnections = {{-0.5f, -1.5f, 2.5f}, {0.5f, -1.5f, 2.5f}};
	spec->parts = {
		(new Part(0x708090ff))->gloss(16)
			->lod(mesh("steamEngineBoiler"), Part::HD, Part::SHADOW)
			->lod(mesh("steamEngineBoilerLD"), Part::VLD, Part::SHADOW)
			->transform(Mat4::translate(0,-2,0)),
		(new Part(0x004225ff))->gloss(16)
			->lod(mesh("steamEngineSaddle"), Part::HD, Part::SHADOW)
			->lod(mesh("steamEngineSaddleLD"), Part::VLD, Part::SHADOW)
			->transform(Mat4::translate(0,-2,0)),
		(new Part(0x004225ff))->gloss(16)
			->lod(mesh("steamEngineFoot"), Part::HD, Part::SHADOW)
			->lod(mesh("steamEngineFootLD"), Part::VLD, Part::SHADOW)
			->transform(Mat4::translate(0,-2,0)),
		(new Part(0x666666ff))->gloss(16)
			->lod(mesh("steamEngineAxel"), Part::HD, Part::SHADOW)
			->lod(mesh("steamEngineAxelLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,1,1)),
		(new Part(0x666666ff))->gloss(16)
			->lod(mesh("steamEngineWheel"), Part::HD, Part::SHADOW)
			->lod(mesh("steamEngineWheelLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::rotate(Point::South, glm::radians(90.0f)) * Mat4::translate(-1.75,1,1)),
		(new Part(0x666666ff))->gloss(16)
			->lod(mesh("steamEngineWheel"), Part::HD, Part::SHADOW)
			->lod(mesh("steamEngineWheelLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::rotate(Point::South, glm::radians(90.0f)) * Mat4::translate(1.75,1,1)),
	};
	spec->health = 150;
	spec->align = true;
	spec->rotateGhost = true;
	spec->enable = true;
	spec->consumeThermalFluid = true;
	spec->generateElectricity = true;
	spec->generatorState = true;
	spec->energyGenerate = Energy::MW(1);
	spec->enemyTarget = true;
	spec->materials = {
		{ Item::byName("steel-sheet")->id, 3 },
		{ Item::byName("copper-sheet")->id, 5 },
		{ Item::byName("gear-wheel")->id, 2 },
	};

	{
		Mat4 state0 = Mat4::identity;

		for (int i = 0; i < 720; i++) {
			Mat4 state1 = Mat4::rotateY(glm::radians((float)i*5.0f));

			spec->states.push_back({
				state0,
				state0,
				state0,
				state0,
				state1,
				state1,
			});
		}
	}

	meshes["computerRack"] = new Mesh("models/computer-rack-hd.stl");
	meshes["computerRackLD"] = new Mesh("models/computer-rack-ld.stl");

	spec = new Spec("computer");
	spec->title = "Computer";
	spec->health = 150;
	spec->rotateGhost = true;
	spec->rotateExtant = true;
	spec->computer = true;
	spec->computerDataStackSize = 8;
	spec->computerReturnStackSize = 8;
	spec->computerRAMSize = 32;
	spec->computerROMSize = 32;
	spec->computerCyclesPerTick = 60;
	spec->consumeElectricity = true;
	spec->energyDrain = Energy::W(100);
	spec->collision = {0, 0, 0, 1, 2, 1};
	spec->selection = spec->collision;
	spec->iconD = 1.5;
	spec->iconV = 0;
	spec->enable = true;
	spec->networker = true;
	spec->networkInterfaces = 2;
	spec->networkWifi = {0,1,0};
	spec->status = true;
	spec->beacon = {0,1,0};
	spec->parts = {
		(new Part(0x888888ff))
			->lod(mesh("computerRack"), Part::HD, Part::SHADOW)
			->lod(mesh("computerRackLD"), Part::MD, Part::SHADOW)
			->lod(mesh("computerRackLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-1,0)),
	};
	spec->materials = {
		{ Item::byName("steel-frame")->id, 1 },
		{ Item::byName("copper-wire")->id, 10 },
		{ Item::byName("solder")->id, 3 },
		{ Item::byName("circuit-board")->id, 10 },
		{ Item::byName("mother-board")->id, 1 },
		{ Item::byName("battery")->id, 1 },
	};

	meshes["oilRigBase"] = new Mesh("models/oil-rig-base-hd.stl");
	meshes["oilRigBaseLD"] = new Mesh("models/oil-rig-base-ld.stl");
	meshes["oilRigBaseVLD"] = new Mesh("models/oil-rig-base-vld.stl");
	meshes["oilRigTower"] = new Mesh("models/oil-rig-tower-hd.stl");
	meshes["oilRigTowerLD"] = new Mesh("models/oil-rig-tower-ld.stl");
	meshes["oilRigTowerVLD"] = new Mesh("models/oil-rig-tower-vld.stl");

	spec = new Spec("oil-rig");
	spec->title = "Oil Rig";
	spec->rotateGhost = true;
	spec->rotateExtant = true;
	spec->collision = {0, -0.5, 0, 9, 9, 9};
	spec->selection = spec->collision;
	spec->iconD = 8;
	spec->iconV = 1;
	spec->place = Spec::Footings;
	spec->footings = {
		{.place = Spec::Water, .point = Point( 30,-6,-30)},
		{.place = Spec::Water, .point = Point( 30,-6, 30)},
		{.place = Spec::Water, .point = Point(-30,-6,-30)},
		{.place = Spec::Water, .point = Point(-30,-6, 30)},
	};
	spec->crafter = true;
	spec->crafterRate = 1.0f;
	spec->crafterState = true;
	spec->enable = true;
	spec->recipeTags = {"drilling"};
	spec->consumeElectricity = true;
	spec->energyConsume = Energy::kW(200);
	spec->energyDrain = Energy::kW(6);
	spec->health = 150;
	spec->enemyTarget = true;
	spec->pipeHints = true;
	spec->pipeOutputConnections = {Point::South*4.5f + Point::East*2.0f + Point::Down*4.0f};
	spec->networker = true;
	spec->networkInterfaces = 1;
	spec->networkWifi = Point(-2.5,4.5,-2.5);
	spec->crafterTransmitResources = true;

	spec->parts = {
		(new Part(0xBEBEBEff))
			->lod(mesh("oilRigBase"), Part::HD, Part::SHADOW)
			->lod(mesh("oilRigBaseLD"), Part::MD, Part::SHADOW)
			->lod(mesh("oilRigBaseVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-4.5,0)),
		(new Part(0xB0C4DEff))
			->lod(mesh("oilRigTower"), Part::HD, Part::SHADOW)
			->lod(mesh("oilRigTowerLD"), Part::MD, Part::SHADOW)
			->lod(mesh("oilRigTowerVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(-2.5,-4.5,-2.5)),
		(new Part(0xFF8C00ff))
			->lod(mesh("fluidTankSmall"), Part::HD, Part::SHADOW)
			->lod(mesh("fluidTankSmallLD"), Part::MD, Part::SHADOW)
			->lod(mesh("fluidTankSmallVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(2,-4.5,2)),
	};
	spec->materials = {
		{ Item::byName("steel-frame")->id, 3 },
		{ Item::byName("brick")->id, 5 },
		{ Item::byName("circuit-board")->id, 1 },
		{ Item::byName("gear-wheel")->id, 2 },
	};

	meshes["clarifierBase"] = new Mesh("models/clarifier-base-hd.stl");
	meshes["clarifierBaseLD"] = new Mesh("models/clarifier-base-ld.stl");
	meshes["clarifierBaseVLD"] = new Mesh("models/clarifier-base-vld.stl");

	meshes["clarifierArm"] = new Mesh("models/clarifier-arm-hd.stl");
	meshes["clarifierArmLD"] = new Mesh("models/clarifier-arm-ld.stl");

	meshes["clarifierFluid"] = new Mesh("models/clarifier-fluid-hd.stl");
	meshes["clarifierFluidLD"] = new Mesh("models/clarifier-fluid-ld.stl");
	meshes["clarifierFluidVLD"] = new Mesh("models/clarifier-fluid-vld.stl");

	spec = new Spec("clarifier");
	spec->title = "Clarifier";
	spec->store = true;
	spec->capacity = Mass::kg(100);
	spec->priority = 10;
	spec->rotateGhost = true;
	spec->rotateExtant = true;
	spec->rotateExtant = true;
	spec->crafter = true;
	spec->crafterRate = 1.0f;
	spec->crafterState = true;
	spec->enable = true;
	spec->recipeTags = {"clarifying"};
	spec->consumeElectricity = true;
	spec->energyConsume = Energy::kW(300);
	spec->energyDrain = Energy::kW(9);
	spec->collision = {0, 0, 0, 8, 1, 8};
	spec->selection = spec->collision;
	spec->iconD = 5.5;
	spec->iconV = -0.5;
	spec->health = 150;
	spec->pipeHints = true;
	spec->pipeInputConnections = {
		Point(4.0f, 0.0f, 2.5f).transform(Mat4::rotateY(glm::radians(-0.0f))),
		Point(4.0f, 0.0f, 2.5f).transform(Mat4::rotateY(glm::radians(-90.0f))),
		Point(4.0f, 0.0f, 2.5f).transform(Mat4::rotateY(glm::radians(-180.0f))),
		Point(4.0f, 0.0f, 2.5f).transform(Mat4::rotateY(glm::radians(-270.0f))),
	};
	spec->pipeOutputConnections = {
		Point(4.0f, 0.0f, -2.5f).transform(Mat4::rotateY(glm::radians(-0.0f))),
		Point(4.0f, 0.0f, -2.5f).transform(Mat4::rotateY(glm::radians(-90.0f))),
		Point(4.0f, 0.0f, -2.5f).transform(Mat4::rotateY(glm::radians(-180.0f))),
		Point(4.0f, 0.0f, -2.5f).transform(Mat4::rotateY(glm::radians(-270.0f))),
	};
	spec->parts = {
		(new Part(0x999999ff))->gloss(32)
			->lod(mesh("clarifierBase"), Part::HD, Part::SHADOW)
			->lod(mesh("clarifierBaseLD"), Part::MD, Part::NOSHADOW)
			->lod(mesh("clarifierBaseVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-0.5,0)),
		(new Part(0x888888ff))->gloss(16)
			->lod(mesh("clarifierArm"), Part::HD, Part::NOSHADOW)
			->lod(mesh("clarifierArmLD"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-0.5,0)),
		(new Part(0x010190FF))->gloss(8)
			->lod(mesh("clarifierFluid"), Part::HD, Part::SHADOW)
			->lod(mesh("clarifierFluidLD"), Part::MD, Part::NOSHADOW)
			->lod(mesh("clarifierFluidVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(0,-0.6,0)),
	};
	spec->materials = {
		{ Item::byName("copper-sheet")->id, 5 },
		{ Item::byName("brick")->id, 5 },
	};

	{
		Mat4 state0 = Mat4::identity;

		for (int i = 0; i < 360; i++) {
			Mat4 state1 = Mat4::rotateY(glm::radians(-(float)i));

			spec->states.push_back({
				state0,
				state1,
				state0,
			});
		}
	}

	meshes["refineryStack"] = new Mesh("models/refinery-stack-hd.stl");
	meshes["refineryStackLD"] = new Mesh("models/refinery-stack-ld.stl");
	meshes["refineryStackVLD"] = new Mesh("models/refinery-stack-vld.stl");

	spec = new Spec("incinerator");
	spec->title = "Incinerator";
	spec->store = true;
	spec->capacity = Mass::kg(100);
	spec->priority = 10;
	spec->rotateGhost = true;
	spec->rotateExtant = true;
	spec->crafter = true;
	spec->crafterRate = 1.0f;
	spec->crafterProgress = true;
	spec->enable = true;
	spec->recipeTags = {"incineration"};
	spec->consumeFuel = true;
	spec->burnerState = true;
	spec->consumeFuelType = "chemical";
	spec->energyConsume = Energy::kW(100);
	spec->energyDrain = Energy::kW(3);
	spec->collision = {0, 0, 0, 2, 8, 2};
	spec->selection = spec->collision;
	spec->iconD = 4.0;
	spec->iconV = 0.5;
	spec->health = 150;
	spec->pipeHints = true;
	spec->pipeInputConnections = {
		Point(1.0f, -3.5f, 0.5f).transform(Mat4::rotateY(glm::radians(-0.0f))),
		Point(1.0f, -3.5f, 0.5f).transform(Mat4::rotateY(glm::radians(-90.0f))),
		Point(1.0f, -3.5f, 0.5f).transform(Mat4::rotateY(glm::radians(-180.0f))),
		Point(1.0f, -3.5f, 0.5f).transform(Mat4::rotateY(glm::radians(-270.0f))),
	};
	spec->parts = {
		(new Part(0xB0C4DEff))->gloss(16)
			->lod(mesh("refineryStack"), Part::HD, Part::SHADOW)
			->lod(mesh("refineryStackLD"), Part::MD, Part::SHADOW)
			->lod(mesh("refineryStackVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(-1.6,-4,1.6)),
		(new PartSmoke(0x111111ff, Point::Up, 1200, 10, 0.01, 0.25f, 0.01f, 0.005f, 0.05f, 0.99f, 60, 180))
			->transform(Mat4::translate(0,4,0)),
		(new PartFlame())
			->transform(Mat4::scale(1.1,1.5,1.1) * Mat4::translate(0,4.25,0)),
	};
	spec->materials = {
		{ Item::byName("copper-sheet")->id, 5 },
		{ Item::byName("brick")->id, 5 },
	};

	for (uint i = 0; i < 10; i++) {
		float fi = (float)i;
		spec->states.push_back({
			Mat4::identity,
			Mat4::scale(0.1f*fi, 0.1f*fi, 0.1f*fi),
			Mat4::identity,
		});
		spec->statesShow.push_back({
			true,
			i > 0,
			i > 0,
		});
	}

	for (uint i = 0; i < 80; i++) {
		spec->states.push_back({
			Mat4::identity,
			Mat4::identity,
			Mat4::identity,
		});
		spec->statesShow.push_back({
			true,
			true,
			true,
		});
	}

	for (uint i = 0; i < 10; i++) {
		float fi = (float)(9-i);
		spec->states.push_back({
			Mat4::identity,
			Mat4::scale(0.1f*fi, 0.1f*fi, 0.1f*fi),
			Mat4::identity,
		});
		spec->statesShow.push_back({
			true,
			true,
			true,
		});
	}

	spec = new Spec("flare-stack");
	spec->title = "Flare Stack";
	spec->rotateGhost = true;
	spec->rotateExtant = true;
	spec->venter = true;
	spec->venterRate = 1;
	spec->enable = true;
	spec->consumeElectricity = true;
	spec->energyConsume = Energy::kW(100);
	spec->energyDrain = Energy::kW(3);
	spec->collision = {0, 0, 0, 2, 8, 2};
	spec->selection = spec->collision;
	spec->iconD = 4.0;
	spec->iconV = 0.5;
	spec->health = 150;
	spec->pipe = true;
	spec->pipeCapacity = Liquid::l(100);
	spec->pipeConnections = {
		Point(1.0f, -3.5f, 0.5f).transform(Mat4::rotateY(glm::radians(-0.0f))),
		Point(1.0f, -3.5f, 0.5f).transform(Mat4::rotateY(glm::radians(-90.0f))),
		Point(1.0f, -3.5f, 0.5f).transform(Mat4::rotateY(glm::radians(-180.0f))),
		Point(1.0f, -3.5f, 0.5f).transform(Mat4::rotateY(glm::radians(-270.0f))),
	};
	spec->parts = {
		(new Part(0x90a4bEff))->gloss(16)
			->lod(mesh("refineryStack"), Part::HD, Part::SHADOW)
			->lod(mesh("refineryStackLD"), Part::MD, Part::SHADOW)
			->lod(mesh("refineryStackVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(-1.6,-4,1.6)),
		(new PartSmoke(0x111111ff, Point::Up, 1200, 10, 0.01, 0.25f, 0.01f, 0.005f, 0.075f, 0.99f, 60, 180))
			->transform(Mat4::translate(0,4,0)),
		(new PartFlame())
			->transform(Mat4::scale(1.1,1.5,1.1) * Mat4::translate(0,4.25,0)),
	};
	spec->materials = {
		{ Item::byName("copper-sheet")->id, 5 },
		{ Item::byName("brick")->id, 5 },
	};

	for (uint i = 0; i < 10; i++) {
		float fi = (float)i;
		spec->states.push_back({
			Mat4::identity,
			Mat4::scale(0.1f*fi, 0.1f*fi, 0.1f*fi),
			Mat4::identity,
		});
		spec->statesShow.push_back({
			true,
			i > 0,
			i > 0,
		});
	}

	for (uint i = 0; i < 80; i++) {
		spec->states.push_back({
			Mat4::identity,
			Mat4::identity,
			Mat4::identity,
		});
		spec->statesShow.push_back({
			true,
			true,
			true,
		});
	}

	for (uint i = 0; i < 10; i++) {
		float fi = (float)(9-i);
		spec->states.push_back({
			Mat4::identity,
			Mat4::scale(0.1f*fi, 0.1f*fi, 0.1f*fi),
			Mat4::identity,
		});
		spec->statesShow.push_back({
			true,
			true,
			true,
		});
	}

	cartTier(1);

	Spec::byName("cart-engineer1")->depotDroneSpec = Spec::byName("drone");

	truckTier(1);

	Spec::byName("truck-engineer1")->depotDroneSpec = Spec::byName("drone");

	meshes["block"] = new Mesh("models/block.stl");

	spec = new Spec("pile");
	spec->title = "Pile";
	spec->collision = {0, 0, 0, 1, 1, 1};
	spec->selection = spec->collision;
	spec->parts = {
		(new Part(0x999999ff))
			->lod(mesh("block"), Part::MD, Part::NOSHADOW)
			->transform(Mat4::translate(0,0.01,0)),
		(new Part(0x999999ff))
			->lod(mesh("pipe"), Part::HD, Part::NOSHADOW)
			->lod(mesh("pipeLD"), Part::MD, Part::NOSHADOW)
			->lod(mesh("pipeVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::rotate(Point::South, glm::radians(90.0f)) * Mat4::scale(1,5,1) * Mat4::translate(0,-2.5,0)),
	};
	spec->pile = true;
	spec->rotateGhost = true;
	spec->rotateExtant = true;
	spec->underground = true;
	spec->forceDelete = true;
	spec->place = Spec::Water;

	spec->materials = {
		{ Item::byName("brick")->id, 3 },
	};

	droneDepotTier(1);

	spec = new Spec("centrifuge1");
	spec->title = "Centrifuge";
	spec->store = true;
	spec->capacity = Mass::kg(5000);
	spec->priority = 10;
	spec->tipStorage = true;
	spec->rotateGhost = true;
	spec->rotateExtant = true;
	spec->crafter = true;
	spec->crafterRate = 1.0f;
	spec->crafterState = true;
	spec->enable = true;
	spec->recipeTags = {"centrifuging"};
	spec->consumeElectricity = true;
	spec->energyConsume = Energy::kW(500);
	spec->energyDrain = Energy::kW(10);
	spec->collision = {0, 0, 0, 5, 4, 5};
	spec->selection = spec->collision;
	spec->iconD = 4;
	spec->iconV = 0;
	spec->status = true;
	spec->beacon = Point::Up*2.0,
	spec->health = 150;
	spec->enemyTarget = true;
	spec->pipeHints = true;
	spec->pipeInputConnections = {
		Point( 2.5f, -1.5f, 0.0f),
		Point(-2.5f, -1.5f, 0.0f),
	};
	spec->pipeOutputConnections = {
		Point( 0.0f, -1.5f, 2.5f),
		Point( 0.0f, -1.5f,-2.5f),
	};

	spec->materials = {
		{ Item::byName("mother-board")->id, 3 },
		{ Item::byName("circuit-board")->id, 10 },
		{ Item::byName("steel-sheet")->id, 10 },
		{ Item::byName("steel-frame")->id, 5 },
		{ Item::byName("plastic-bar")->id, 10 },
	};

	meshes["centrifugeBase"] = new Mesh("models/centrifuge-base-hd.stl");
	meshes["centrifugeBaseLD"] = new Mesh("models/centrifuge-base-ld.stl");
	meshes["centrifugeCasing"] = new Mesh("models/centrifuge-casing-hd.stl");
	meshes["centrifugeCasingLD"] = new Mesh("models/centrifuge-casing-ld.stl");
	meshes["centrifugeCasingVLD"] = new Mesh("models/centrifuge-casing-vld.stl");
	meshes["centrifugeRotor"] = new Mesh("models/centrifuge-rotor-hd.stl");
	meshes["centrifugeHelix"] = new Mesh("models/centrifuge-helix-hd.stl");

	auto centrifugeBase = (new Part(0xB0C4DEff))
		->lod(mesh("centrifugeBase"), Part::HD, Part::SHADOW)
		->lod(mesh("centrifugeBaseLD"), Part::VLD, Part::NOSHADOW);

	auto centrifugeCasing = (new Part(0xffffffff))
		->lod(mesh("centrifugeCasing"), Part::HD, Part::SHADOW)
		->lod(mesh("centrifugeCasingLD"), Part::MD, Part::NOSHADOW)
		->lod(mesh("centrifugeCasingVLD"), Part::VLD, Part::NOSHADOW);

	auto centrifugeRotor1 = (new Part(0x00ff00ff))
		->lod(mesh("centrifugeRotor"), Part::HD, Part::NOSHADOW);

	auto centrifugeRotor2 = (new Part(0x444444ff))
		->lod(mesh("centrifugeRotor"), Part::HD, Part::NOSHADOW);

	auto centrifugeHelix = (new Part(0xccccccff))->gloss(2)
		->lod(mesh("centrifugeHelix"), Part::HD, Part::NOSHADOW);

	spec->parts = {centrifugeBase};

	for (int i = 0; i < 25; i++) spec->parts.push_back(centrifugeCasing);
	for (int i = 0; i < 25; i++) spec->parts.push_back(centrifugeRotor1);
	for (int i = 0; i < 25; i++) spec->parts.push_back(centrifugeRotor2);
	for (int i = 0; i < 25; i++) spec->parts.push_back(centrifugeHelix);

	{
		Mat4 center = Mat4::translate(0,-2,0);

		std::vector<Mat4> casings;
		for (int i = -2; i <= 2; i++) {
			for (int j = -2; j <= 2; j++) {
				casings.push_back(Mat4::translate(i,0,j) * center);
			}
		}

		std::vector<float> spinOffsets;
		for (int i = 0; i < 25; i++) {
			spinOffsets.push_back(Sim::random()*360.0f);
		}

		auto spin = [&](float d, int o) {
			return Mat4::rotate(Point::Up, -glm::radians(d*5 + spinOffsets[o]));
		};

		for (int i = 0; i < 360; i++) {
			std::vector<Mat4> state = {center};

			for (int j = 0; j < 25; j++) state.push_back(casings[j]);
			for (int j = 0; j < 25; j++) state.push_back(spin(i,j) * casings[j]);
			for (int j = 0; j < 25; j++) state.push_back(spin(i+90,j) * casings[j]);
			for (int j = 0; j < 25; j++) state.push_back(casings[j]);

			spec->states.push_back(state);
		}
	}

	spec = new Spec("starship");
	spec->title = "Starship";
	spec->starship = true;
	spec->health = 150;
	spec->store = true;
	spec->storeSetUpper = true;
	spec->storeAnything = true;
	spec->tipStorage = true;
	spec->capacity = Mass::kg(10000);
	spec->rotateGhost = true;
	spec->rotateExtant = true;
	spec->launcher = true;
	spec->launcherFuel = {
		{Fluid::byName("hydrogen")->id,10000},
		{Fluid::byName("oxygen")->id,5000},
	};
	spec->enable = true;
	spec->collision = {0, 0, 0, 20, 45, 20};
	spec->selection = spec->collision;
	spec->iconD = 15;
	spec->iconV = 18;
	spec->enemyTarget = true;
	spec->pipeHints = true;
	spec->pipeInputConnections = {
		Point(10.0f, -22.0f, 6.5f).transform(Mat4::rotateY(glm::radians(0.0f))),
		Point(10.0f, -22.0f, -6.5f).transform(Mat4::rotateY(glm::radians(0.0f))),
		Point(10.0f, -22.0f, 6.5f).transform(Mat4::rotateY(glm::radians(180.0f))),
		Point(10.0f, -22.0f, -6.5f).transform(Mat4::rotateY(glm::radians(180.0f))),
	};

	float starshipGloss = 4;

	spec->networker = true;
	spec->networkInterfaces = 1;
	spec->networkWifi = {9.5,-19.5,9.5};

	meshes["starshipNoseFin"] = new Mesh("models/starship-nosefin-hd.stl");
	meshes["starshipNoseFinLD"] = new Mesh("models/starship-nosefin-ld.stl");
	meshes["starshipTailFin"] = new Mesh("models/starship-tailfin-hd.stl");
	meshes["starshipTailFinLD"] = new Mesh("models/starship-tailfin-ld.stl");
	meshes["starshipRaptor"] = new Mesh("models/starship-raptor-hd.stl");
	meshes["starshipRaptorLD"] = new Mesh("models/starship-raptor-ld.stl");
	meshes["starshipPlume"] = new Mesh("models/starship-plume-hd.stl");
	meshes["starshipPlumeLD"] = new Mesh("models/starship-plume-ld.stl");
	meshes["starshipLaunchpad"] = new Mesh("models/starship-launchpad-hd.stl");
	meshes["starshipLaunchpadLD"] = new Mesh("models/starship-launchpad-ld.stl");
	meshes["starshipFrame"] = new Mesh("models/starship-frame-hd.stl");
	meshes["starshipFrameLD"] = new Mesh("models/starship-frame-ld.stl");
	meshes["starshipBody"] = new Mesh("models/starship-body-hd.stl");
	meshes["starshipBodyLD"] = new Mesh("models/starship-body-ld.stl");
	meshes["starshipNose"] = new Mesh("models/starship-nose-hd.stl");
	meshes["starshipNoseLD"] = new Mesh("models/starship-nose-ld.stl");

	auto starshipNoseFin = (new Part(0x111111ff))->gloss(starshipGloss)
		->lod(mesh("starshipNoseFin"), Part::MD, Part::SHADOW)
		->lod(mesh("starshipNoseFinLD"), Part::VLD, Part::NOSHADOW);

	auto starshipTailFin = (new Part(0x111111ff))->gloss(starshipGloss)
		->lod(mesh("starshipTailFin"), Part::MD, Part::SHADOW)
		->lod(mesh("starshipTailFinLD"), Part::VLD, Part::NOSHADOW);

	auto starshipRaptor = (new Part(0x111111ff))->gloss(starshipGloss)
		->lod(mesh("starshipRaptor"), Part::MD, Part::SHADOW)
		->lod(mesh("starshipRaptorLD"), Part::VLD, Part::NOSHADOW);

	auto starshipPlumeA = (new Part(0xFFA500ff))->translucent()
		->lod(mesh("starshipPlume"), Part::MD, Part::SHADOW)
		->lod(mesh("starshipPlumeLD"), Part::VLD, Part::NOSHADOW);

	auto starshipPlumeB = (new Part(0xffff00ff))->translucent()
		->lod(mesh("starshipPlume"), Part::MD, Part::SHADOW)
		->lod(mesh("starshipPlumeLD"), Part::VLD, Part::NOSHADOW);

	spec->parts = {
		(new Part(0x708090ff))->gloss(starshipGloss)
			->lod(mesh("starshipLaunchpad"), Part::MD, Part::SHADOW)
			->lod(mesh("starshipLaunchpadLD"), Part::VLD, Part::NOSHADOW),
		(new Part(0x111111ff))->gloss(starshipGloss)
			->lod(mesh("starshipFrame"), Part::MD, Part::SHADOW)
			->lod(mesh("starshipFrameLD"), Part::VLD, Part::NOSHADOW),
		(new Part(0xA3C1DAff))->gloss(starshipGloss)
			->lod(mesh("starshipBody"), Part::MD, Part::SHADOW)
			->lod(mesh("starshipBodyLD"), Part::VLD, Part::NOSHADOW),
		(new Part(0xA3C1DAff))->gloss(starshipGloss)
			->lod(mesh("starshipNose"), Part::MD, Part::SHADOW)
			->lod(mesh("starshipNoseLD"), Part::VLD, Part::NOSHADOW),
		starshipRaptor,
		starshipRaptor,
		starshipRaptor,
		starshipNoseFin,
		starshipNoseFin,
		starshipTailFin,
		starshipTailFin,
		starshipPlumeB,
		starshipPlumeB,
		starshipPlumeB,
		starshipPlumeA,
		starshipPlumeA,
		starshipPlumeA,
	};

	spec->parts[0]->filter = 1;

	spec->highLOD = 10;

	spec->materials = {
		{ Item::byName("brick")->id, 100 },
		{ Item::byName("steel-sheet")->id, 50 },
		{ Item::byName("steel-frame")->id, 25 },
		{ Item::byName("copper-sheet")->id, 50 },
		{ Item::byName("circuit-board")->id, 50 },
		{ Item::byName("pipe")->id, 50 },
	};

	{
		Mat4 center = Mat4::translate(0,-22.5,0);

		// @todo: make it belly-flop down :-)

		auto fly = [&](float h, bool flame) {
			int i = h;
			h /= 100;
			Mat4 up = Mat4::translate(0,h*h*h,0);
			Mat4 spin = Mat4::rotateY(-glm::radians(h*h*h));

			auto plumeScaleA = Mat4::scale((i%2 ? 0.9: 1.0), (i%2 ? 1.2: 1.3), (i%2 ? 0.9: 1.0));
			auto plumeScaleB = Mat4::scale(0.75, (i%2 ? 1.1: 1.0), 0.75);

			spec->states.push_back({
				center,
				center * Mat4::translate(0,1,0),
				center * Mat4::translate(0,5,0) * up * spin,
				center * Mat4::translate(0,35,0) * up * spin,
				center * Mat4::translate(2,3.25,0) * Mat4::rotateY(glm::radians(-0.0f)) * up * spin,
				center * Mat4::translate(2,3.25,0) * Mat4::rotateY(glm::radians(-120.0f)) * up * spin,
				center * Mat4::translate(2,3.25,0) * Mat4::rotateY(glm::radians(-240.0f)) * up * spin,
				center * Mat4::rotateY(glm::radians(  -0.0f)) * Mat4::translate( 5,35,0) * up * spin,
				center * Mat4::rotateY(glm::radians(-180.0f)) * Mat4::translate(-5,35,0) * up * spin,
				center * Mat4::rotateY(glm::radians(  -0.0f)) * Mat4::translate( 5, 5,0) * up * spin,
				center * Mat4::rotateY(glm::radians(-180.0f)) * Mat4::translate(-5, 5,0) * up * spin,
				plumeScaleB * center * Mat4::translate(2,0,0) * Mat4::rotateY(glm::radians(  -0.0f)) * up * spin,
				plumeScaleB * center * Mat4::translate(2,0,0) * Mat4::rotateY(glm::radians(-120.0f)) * up * spin,
				plumeScaleB * center * Mat4::translate(2,0,0) * Mat4::rotateY(glm::radians(-240.0f)) * up * spin,
				plumeScaleA * center * Mat4::translate(2,0,0) * Mat4::rotateY(glm::radians(  -0.0f)) * up * spin,
				plumeScaleA * center * Mat4::translate(2,0,0) * Mat4::rotateY(glm::radians(-120.0f)) * up * spin,
				plumeScaleA * center * Mat4::translate(2,0,0) * Mat4::rotateY(glm::radians(-240.0f)) * up * spin,
			});

			spec->statesShow.push_back({
				true,
				true,
				true,
				true,
				true,
				true,
				true,
				true,
				true,
				true,
				true,
				flame,
				flame,
				flame,
				flame,
				flame,
				flame,
			});
		};

		for (uint i = 0; i < 200; i++) {
			fly(0, i > 150);
		}

		for (uint i = 0; i < 1000; i++) {
			fly(i, true);
		}

		for (uint i = 1000; i > 0; i--) {
			fly(i, true);
		}

		for (uint i = 0; i < 200; i++) {
			fly(0, i < 50);
		}
	}

	meshes["falconLaunchpad"] = new Mesh("models/falcon-launchpad-hd.stl");
	meshes["falconLaunchpadLD"] = new Mesh("models/falcon-launchpad-ld.stl");
	meshes["falconBody"] = new Mesh("models/falcon-body-hd.stl");
	meshes["falconBodyLD"] = new Mesh("models/falcon-body-ld.stl");
	meshes["falconNose"] = new Mesh("models/falcon-nose-hd.stl");
	meshes["falconNoseLD"] = new Mesh("models/falcon-nose-ld.stl");
	meshes["falconLeg"] = new Mesh("models/falcon-leg-hd.stl");
	meshes["falconLegLD"] = new Mesh("models/falcon-leg-ld.stl");
	meshes["falconBase"] = new Mesh("models/falcon-base-hd.stl");
	meshes["falconBaseLD"] = new Mesh("models/falcon-base-ld.stl");

	spec = new Spec("falcon");
	spec->title = "Rocket";
	spec->starship = true;
	spec->health = 150;
	spec->store = true;
	spec->storeSetUpper = true;
	spec->storeAnything = true;
	spec->tipStorage = true;
	spec->capacity = Mass::kg(1000);
	spec->rotateGhost = true;
	spec->rotateExtant = true;
	spec->launcher = true;
	spec->launcherFuel = {
		{Fluid::byName("hydrazine")->id,5000},
	};
	spec->enable = true;
	spec->collision = {0, 0, 0, 12, 28, 12};
	spec->selection = spec->collision;
	spec->iconD = 10;
	spec->iconV = 9;

	spec->pipe = true;
	spec->pipeCapacity = Liquid::l(5000);
	spec->pipeHints = true;
	spec->pipeConnections = {
		Point(6.0f, -13.5f, 4.5f).transform(Mat4::rotateY(glm::radians(0.0f))),
		Point(6.0f, -13.5f, -4.5f).transform(Mat4::rotateY(glm::radians(0.0f))),
		Point(6.0f, -13.5f, 4.5f).transform(Mat4::rotateY(glm::radians(180.0f))),
		Point(6.0f, -13.5f, -4.5f).transform(Mat4::rotateY(glm::radians(180.0f))),
	};

	spec->materials = {
		{ Item::byName("brick")->id, 25 },
		{ Item::byName("steel-sheet")->id, 10 },
		{ Item::byName("steel-frame")->id, 5 },
		{ Item::byName("copper-sheet")->id, 10 },
		{ Item::byName("circuit-board")->id, 10 },
		{ Item::byName("pipe")->id, 10 },
	};

	auto falconLeg = (new Part(0x222222ff))->gloss(starshipGloss)
		->lod(mesh("falconLeg"), Part::MD, Part::SHADOW)
		->lod(mesh("falconLegLD"), Part::VLD, Part::NOSHADOW);

	auto falconRaptor = (new Part(0x111111ff))->gloss(starshipGloss)
		->lod(mesh("starshipRaptor"), Part::MD, Part::SHADOW)
		->lod(mesh("starshipRaptorLD"), Part::VLD, Part::NOSHADOW);

	auto falconPlumeA = (new Part(0xFFA500ff))->translucent()
		->lod(mesh("starshipPlume"), Part::MD, Part::SHADOW)
		->lod(mesh("starshipPlumeLD"), Part::VLD, Part::NOSHADOW);

	auto falconPlumeB = (new Part(0xffff00ff))->translucent()
		->lod(mesh("starshipPlume"), Part::MD, Part::SHADOW)
		->lod(mesh("starshipPlumeLD"), Part::VLD, Part::NOSHADOW);

	spec->parts = {
		(new Part(0x708090ff))->gloss(starshipGloss)
			->lod(mesh("falconLaunchpad"), Part::MD, Part::SHADOW)
			->lod(mesh("falconLaunchpadLD"), Part::VLD, Part::NOSHADOW),
		(new Part(0xffffffff))->gloss(starshipGloss)
			->lod(mesh("falconBody"), Part::MD, Part::SHADOW)
			->lod(mesh("falconBodyLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(0,5,0)),
		(new Part(0x333333ff))->gloss(starshipGloss)
			->lod(mesh("falconBase"), Part::MD, Part::SHADOW)
			->lod(mesh("falconBaseLD"), Part::VLD, Part::NOSHADOW),
		falconLeg,
		falconLeg,
		falconLeg,
		falconLeg,
		falconRaptor,
		falconPlumeB,
		falconPlumeA,
	};

	spec->parts[0]->filter = 1;

	spec->highLOD = 10;

	{
		Mat4 center = Mat4::translate(0,-spec->collision.h/2,0);
		Mat4 rocket = Mat4::translate(0,1,0);

		auto fly = [&](float h, bool flame) {
			int i = h;
			h /= 100;
			Mat4 up = Mat4::translate(0,h*h*h,0);
			Mat4 spin = Mat4::rotateY(-glm::radians(h*h*h));

			int retractLeg = 150;
			float legAngle = -130.0f;
			if (i >= retractLeg) legAngle += std::min((float)i-retractLeg, 130.0f);

			auto legRotate = Mat4::rotate(Point::South, glm::radians(legAngle));

			auto plumeScaleA = Mat4::scale((i%2 ? 0.9: 1.0), (i%2 ? 1.2: 1.3), (i%2 ? 0.9: 1.0));
			auto plumeScaleB = Mat4::scale(0.75, (i%2 ? 1.1: 1.0), 0.75);

			spec->states.push_back({
				center,
				center * rocket * up * spin,
				Mat4::translate(0,5,0) * center * rocket * up * spin,
				legRotate * Mat4::translate(1.8,5.25,0) * Mat4::rotateY(glm::radians(  0.0f)) * center * rocket * up * spin,
				legRotate * Mat4::translate(1.8,5.25,0) * Mat4::rotateY(glm::radians( 90.0f)) * center * rocket * up * spin,
				legRotate * Mat4::translate(1.8,5.25,0) * Mat4::rotateY(glm::radians(180.0f)) * center * rocket * up * spin,
				legRotate * Mat4::translate(1.8,5.25,0) * Mat4::rotateY(glm::radians(270.0f)) * center * rocket * up * spin,
				Mat4::translate(0,3,0) * center * rocket * up * spin,
				Mat4::translate(0,-0.5,0) * plumeScaleB * center * rocket * up * spin,
				Mat4::translate(0,-0.5,0) * plumeScaleA * center * rocket * up * spin,
			});

			spec->statesShow.push_back({
				true,
				true,
				true,
				true,
				true,
				true,
				true,
				true,
				flame,
				flame,
			});
		};

		for (uint i = 0; i < 200; i++) {
			fly(0, i > 150);
		}

		for (uint i = 0; i < 1000; i++) {
			fly(i, true);
		}

		for (uint i = 1000; i > 0; i--) {
			fly(i, true);
		}

		for (uint i = 0; i < 200; i++) {
			fly(0, i < 50);
		}

		spec->launcherInitialState = 0;//spec->states.size()/2;
	}

	meshes["zeppelinBody"] = new Mesh("models/zeppelin-body-hd.stl");
	meshes["zeppelinBody"]->smooth();

	meshes["zeppelinBodyLD"] = new Mesh("models/zeppelin-body-ld.stl");
	meshes["zeppelinBodyLD"]->smooth();

	meshes["zeppelinGondola"] = new Mesh("models/zeppelin-gondola-hd.stl");
	meshes["zeppelinGondolaLD"] = new Mesh("models/zeppelin-gondola-ld.stl");
	meshes["zeppelinFin"] = new Mesh("models/zeppelin-fin-hd.stl");
	meshes["zeppelinFinLD"] = new Mesh("models/zeppelin-fin-ld.stl");
	meshes["zeppelinEngine"] = new Mesh("models/zeppelin-engine-hd.stl");
	meshes["zeppelinEngineLD"] = new Mesh("models/zeppelin-engine-ld.stl");
	meshes["zeppelinSpar"] = new Mesh("models/zeppelin-spar-hd.stl");
	meshes["zeppelinSparLD"] = new Mesh("models/zeppelin-spar-ld.stl");
	meshes["zeppelinProp"] = new Mesh("models/zeppelin-prop-hd.stl");
	meshes["zeppelinPropLD"] = new Mesh("models/zeppelin-prop-ld.stl");

	auto zeppelinGloss = 8;

	auto zeppelinBody = (new Part(0xffffffff))
		->lod(mesh("zeppelinBody"), Part::MD, Part::SHADOW)
		->lod(mesh("zeppelinBodyLD"), Part::VLD, Part::SHADOW)
		->gloss(zeppelinGloss);

	auto zeppelinGondola = (new Part(0x666666ff))
		->lod(mesh("zeppelinGondola"), Part::MD, Part::SHADOW)
		->lod(mesh("zeppelinGondolaLD"), Part::VLD, Part::NOSHADOW)
		->gloss(zeppelinGloss);

	auto zeppelinFin = (new Part(0x666666ff))
		->lod(mesh("zeppelinFin"), Part::MD, Part::SHADOW)
		->lod(mesh("zeppelinFinLD"), Part::VLD, Part::NOSHADOW)
		->gloss(zeppelinGloss);

	auto zeppelinEngine = (new Part(0xffffffff))
		->lod(mesh("zeppelinEngine"), Part::HD, Part::SHADOW)
		->lod(mesh("zeppelinEngineLD"), Part::MD, Part::NOSHADOW)
		->gloss(zeppelinGloss);

	auto zeppelinSpar = (new Part(0x999999ff))
		->lod(mesh("zeppelinSpar"), Part::HD, Part::SHADOW)
		->lod(mesh("zeppelinSparLD"), Part::MD, Part::NOSHADOW)
		->gloss(4);

	auto zeppelinProp = (new Part(0x999999ff))
		->lod(mesh("zeppelinProp"), Part::HD, Part::SHADOW)
		->lod(mesh("zeppelinPropLD"), Part::MD, Part::NOSHADOW)
		->gloss(4);

	{
		spec = new Spec("zeppelin");
		spec->title = "Zeppelin";
		spec->named = true;
		spec->collision = {0, 0, 0, 12, 12, 50};
		spec->selection = spec->collision;
		spec->iconD = 20;
		spec->iconV = 0;
		spec->place = Spec::Land|Spec::Water;

		//spec->health = 500;
		spec->align = false;
		spec->zeppelin = true;
		spec->zeppelinAltitude = 50.0f;
		spec->zeppelinSpeed = 0.5f;
		spec->energyConsume = Energy::kW(150);
		spec->consumeMagic = true;
		spec->store = true;
		spec->tipStorage = true;
		spec->capacity = Mass::kg(2500);
		spec->logistic = true;
		spec->storeSetLower = true;
		spec->storeSetUpper = true;
		spec->generateElectricity = true;
		spec->energyGenerate = Energy::MW(3);
		spec->rotateGhost = true;
		spec->plan = false;
		spec->clone = false;

		//spec->forceDelete = true;
		spec->deconstructable = false;

		spec->depot = true;
		spec->depotRange = 150.0f;
		spec->depotDrones = 100;
		spec->depotDroneSpec = Spec::byName("drone");
		spec->depotAssist = true;
		spec->dronePoint = Point::Down*(spec->collision.h/2.0f);
		spec->dronePointRadius = 3.0;

		spec->materials = {
			{ Item::byName("aluminium-frame")->id, 100 },
			{ Item::byName("plastic-bar")->id, 100 },
			{ Item::byName("electric-motor")->id, 100 },
			{ Item::byName("battery")->id, 100 },
			{ Item::byName("mother-board")->id, 5 },
		};

		spec->highLOD = 10;

		spec->parts = {
			zeppelinBody,
			zeppelinGondola,
			zeppelinFin,
			zeppelinFin,
			zeppelinFin,
			zeppelinFin,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
		};

		{
			for (uint i = 0; i < 36; i++) {
				Mat4 center = Mat4::rotate(Point::Up, glm::radians(-90.0f));

				Mat4 gondola = Mat4::translate(12,-4,0);

				Mat4 tailFin1 = Mat4::rotate(Point::East, glm::radians(  0.0f)) * Mat4::translate(-15,0,0);
				Mat4 tailFin2 = Mat4::rotate(Point::East, glm::radians( 90.0f)) * Mat4::translate(-15,0,0);
				Mat4 tailFin3 = Mat4::rotate(Point::East, glm::radians(180.0f)) * Mat4::translate(-15,0,0);
				Mat4 tailFin4 = Mat4::rotate(Point::East, glm::radians(270.0f)) * Mat4::translate(-15,0,0);

				Mat4 prop = Mat4::translate(-0.25,0,0) * Mat4::rotate(Point::East, glm::radians((float)(i*10)));

				Mat4 engine1 = Mat4::translate(10,-6,0) * Mat4::rotate(Point::East, glm::radians((float)( 50)));
				Mat4 engine2 = Mat4::translate(10,-6,0) * Mat4::rotate(Point::East, glm::radians((float)(-50)));

				Mat4 engine3 = Mat4::translate(0,-6,0) * Mat4::rotate(Point::East, glm::radians((float)( 40)));
				Mat4 engine4 = Mat4::translate(0,-6,0) * Mat4::rotate(Point::East, glm::radians((float)(-40)));

				Mat4 engine5 = Mat4::translate(-10,-6,0) * Mat4::rotate(Point::East, glm::radians((float)( 30)));
				Mat4 engine6 = Mat4::translate(-10,-6,0) * Mat4::rotate(Point::East, glm::radians((float)(-30)));

				Mat4 spar1 = Mat4::translate(0,2.5,0) * Mat4::rotate(Point::East, glm::radians((float)(-35)));
				Mat4 spar2 = Mat4::translate(0,2.5,0) * Mat4::rotate(Point::East, glm::radians((float)(35)));
				Mat4 spar3 = Mat4::translate(0,2.5,0) * Mat4::rotate(Point::South, glm::radians((float)(-45)));

				spec->states.push_back({
					center,
					gondola * center,
					tailFin1 * center,
					tailFin2 * center,
					tailFin3 * center,
					tailFin4 * center,
					engine1 * center,
					spar1 * engine1 * center,
					spar2 * engine1 * center,
					spar3 * engine1 * center,
					prop * engine1 * center,
					engine2 * center,
					spar1 * engine2 * center,
					spar2 * engine2 * center,
					spar3 * engine2 * center,
					prop * engine2 * center,
					engine3 * center,
					spar1 * engine3 * center,
					spar2 * engine3 * center,
					spar3 * engine3 * center,
					prop * engine3 * center,
					engine4 * center,
					spar1 * engine4 * center,
					spar2 * engine4 * center,
					spar3 * engine4 * center,
					prop * engine4 * center,
					engine5 * center,
					spar1 * engine5 * center,
					spar2 * engine5 * center,
					spar3 * engine5 * center,
					prop * engine5 * center,
					engine6 * center,
					spar1 * engine6 * center,
					spar2 * engine6 * center,
					spar3 * engine6 * center,
					prop * engine6 * center,
				});
			}
		}
	}

	{
		auto zeppelinBody = (new Part(0xffa500ff))
			->lod(mesh("zeppelinBody"), Part::MD, Part::SHADOW)
			->lod(mesh("zeppelinBodyLD"), Part::VLD, Part::SHADOW)
			->gloss(zeppelinGloss);

		spec = new Spec("zeppelin-support");
		spec->title = "Zeppelin (Support)";
		spec->named = true;
		spec->collision = {0, 0, 0, 12, 12, 50};
		spec->selection = spec->collision;
		spec->place = Spec::Land|Spec::Water;
		spec->iconD = 20;
		spec->iconV = 0;

		//spec->health = 500;
		spec->align = false;
		spec->zeppelin = true;
		spec->zeppelinAltitude = 50.0f;
		spec->zeppelinSpeed = 0.5f;
		spec->energyConsume = Energy::kW(150);
		spec->consumeFuel = true;
		spec->consumeFuelType = "chemical";
		spec->store = true;
		spec->tipStorage = true;
		spec->capacity = Mass::kg(5000);
		spec->logistic = true;
		spec->storeSetLower = true;
		spec->storeSetUpper = true;
		spec->rotateGhost = true;
		spec->plan = false;
		spec->clone = true;

		spec->forceDelete = true;
		//spec->deconstructable = false;

		spec->depot = true;
		spec->depotRange = 150.0f;
		spec->depotDrones = 100;
		spec->depotDroneSpec = Spec::byName("drone");
//		spec->depotAssist = true;
		spec->dronePoint = Point::Down*(spec->collision.h/2.0f);
		spec->dronePointRadius = 3.0;
		spec->depotDispatchEnergy = Energy::kJ(100);

		spec->materials = {
			{ Item::byName("aluminium-frame")->id, 100 },
			{ Item::byName("plastic-bar")->id, 100 },
			{ Item::byName("electric-motor")->id, 100 },
			{ Item::byName("battery")->id, 100 },
			{ Item::byName("mother-board")->id, 5 },
		};

		spec->highLOD = 10;

		spec->parts = {
			zeppelinBody,
			zeppelinGondola,
			zeppelinFin,
			zeppelinFin,
			zeppelinFin,
			zeppelinFin,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
			zeppelinEngine,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinSpar,
			zeppelinProp,
		};

		{
			for (uint i = 0; i < 36; i++) {
				Mat4 center = Mat4::rotate(Point::Up, glm::radians(-90.0f));

				Mat4 gondola = Mat4::translate(12,-4,0);

				Mat4 tailFin1 = Mat4::rotate(Point::East, glm::radians(  0.0f)) * Mat4::translate(-15,0,0);
				Mat4 tailFin2 = Mat4::rotate(Point::East, glm::radians( 90.0f)) * Mat4::translate(-15,0,0);
				Mat4 tailFin3 = Mat4::rotate(Point::East, glm::radians(180.0f)) * Mat4::translate(-15,0,0);
				Mat4 tailFin4 = Mat4::rotate(Point::East, glm::radians(270.0f)) * Mat4::translate(-15,0,0);

				Mat4 prop = Mat4::translate(-0.25,0,0) * Mat4::rotate(Point::East, glm::radians((float)(i*10)));

				Mat4 engine1 = Mat4::translate(10,-6,0) * Mat4::rotate(Point::East, glm::radians((float)( 50)));
				Mat4 engine2 = Mat4::translate(10,-6,0) * Mat4::rotate(Point::East, glm::radians((float)(-50)));

				Mat4 engine3 = Mat4::translate(0,-6,0) * Mat4::rotate(Point::East, glm::radians((float)( 40)));
				Mat4 engine4 = Mat4::translate(0,-6,0) * Mat4::rotate(Point::East, glm::radians((float)(-40)));

				Mat4 engine5 = Mat4::translate(-10,-6,0) * Mat4::rotate(Point::East, glm::radians((float)( 30)));
				Mat4 engine6 = Mat4::translate(-10,-6,0) * Mat4::rotate(Point::East, glm::radians((float)(-30)));

				Mat4 spar1 = Mat4::translate(0,2.5,0) * Mat4::rotate(Point::East, glm::radians((float)(-35)));
				Mat4 spar2 = Mat4::translate(0,2.5,0) * Mat4::rotate(Point::East, glm::radians((float)(35)));
				Mat4 spar3 = Mat4::translate(0,2.5,0) * Mat4::rotate(Point::South, glm::radians((float)(-45)));

				spec->states.push_back({
					center,
					gondola * center,
					tailFin1 * center,
					tailFin2 * center,
					tailFin3 * center,
					tailFin4 * center,
					engine1 * center,
					spar1 * engine1 * center,
					spar2 * engine1 * center,
					spar3 * engine1 * center,
					prop * engine1 * center,
					engine2 * center,
					spar1 * engine2 * center,
					spar2 * engine2 * center,
					spar3 * engine2 * center,
					prop * engine2 * center,
					engine3 * center,
					spar1 * engine3 * center,
					spar2 * engine3 * center,
					spar3 * engine3 * center,
					prop * engine3 * center,
					engine4 * center,
					spar1 * engine4 * center,
					spar2 * engine4 * center,
					spar3 * engine4 * center,
					prop * engine4 * center,
					engine5 * center,
					spar1 * engine5 * center,
					spar2 * engine5 * center,
					spar3 * engine5 * center,
					prop * engine5 * center,
					engine6 * center,
					spar1 * engine6 * center,
					spar2 * engine6 * center,
					spar3 * engine6 * center,
					prop * engine6 * center,
				});
			}
		}
	}

	blimpTier(1);
//
//	meshes["heavylifterBody"] = new Mesh("models/heavylifter-body-hd.stl");
//	meshes["heavylifterBodyLD"] = new Mesh("models/heavylifter-body-ld.stl");
//	meshes["heavylifterBodyVLD"] = new Mesh("models/heavylifter-body-vld.stl");
//	meshes["heavylifterArm"] = new Mesh("models/heavylifter-arm-hd.stl");
//	meshes["heavylifterEngineCowling"] = new Mesh("models/heavylifter-engine-cowling-hd.stl");
//	meshes["heavylifterEngineCowlingLD"] = new Mesh("models/heavylifter-engine-cowling-ld.stl");
//	meshes["heavylifterEngineShaft"] = new Mesh("models/heavylifter-engine-shaft-hd.stl");
//	meshes["heavylifterFrame"] = new Mesh("models/heavylifter-frame-hd.stl");
//	meshes["heavylifterMainWing"] = new Mesh("models/heavylifter-mainwing-hd.stl");
//	meshes["heavylifterMainWingLD"] = new Mesh("models/heavylifter-mainwing-ld.stl");
//	meshes["heavylifterTailWing"] = new Mesh("models/heavylifter-tailwing-hd.stl");
//	meshes["heavylifterTailWingLD"] = new Mesh("models/heavylifter-tailwing-ld.stl");
//	meshes["heavylifterTailFin"] = new Mesh("models/heavylifter-tailfin-hd.stl");
//	meshes["heavylifterTailFinLD"] = new Mesh("models/heavylifter-tailfin-ld.stl");
//
//	spec = new Spec("heavylifter");
//	spec->title = "Heavy Lifter";
//	spec->collision = {0, 0, 0, 3, 3, 7};
//	spec->selection = spec->collision;
//	spec->place = Spec::Land;
//
//	spec->health = 500;
//	spec->align = false;
//	spec->forceDelete = true;
//	spec->rotateGhost = true;
//	spec->plan = false;
//	spec->flightPath = true;
//	spec->flightPathClearance = 20.0f;
//	spec->flightPathSpeed = 0.5f;
//	spec->flightLogistic = true;
//
//	spec->energyConsume = Energy::kW(250);
//	spec->consumeCharge = true;
//	spec->consumeChargeBuffer = Energy::MJ(10);
//	spec->consumeChargeRate = Energy::kW(250);
//	spec->store = true;
//	spec->storeAnything = true;
//	spec->tipStorage = true;
//	spec->capacity = Mass::kg(5000);
//	spec->forceDelete = true;
//	spec->rotateGhost = true;
//	spec->plan = false;
//
//	float up = 2.1;
//
//	spec->parts = {
//		(new Part(0x444444ff))
//			->lod(mesh("models/container-small-hd.stl"), Part::HD, Part::SHADOW)
//			->lod(mesh("models/container-small-ld.stl"), Part::MD, Part::SHADOW)
//			->lod(mesh("models/container-small-vld.stl"), Part::VLD, Part::NOSHADOW),
//
//		(new Part(0xffffffff))
//			->lod(mesh("heavylifterBody"), Part::HD, Part::SHADOW)
//			->lod(mesh("heavylifterBodyLD"), Part::MD, Part::SHADOW)
//			->lod(mesh("heavylifterBodyVLD"), Part::VLD, Part::SHADOW)
//			->transform(Mat4::translate(0,up,0)),
//
//		(new Part(0x444444ff))
//			->lod(mesh("heavylifterMainWing"), Part::HD, Part::SHADOW)
//			->lod(mesh("heavylifterMainWingLD"), Part::MD, Part::SHADOW)
//			->lod(mesh("heavylifterMainWingLD"), Part::VLD, Part::NOSHADOW)
//			->transform(Mat4::translate(0,up+0.9,0)),
//
//		(new Part(0x444444ff))
// 			->lod(mesh("heavylifterTailWing"), Part::HD, Part::SHADOW)
//			->lod(mesh("heavylifterTailWingLD"), Part::MD, Part::SHADOW)
//			->lod(mesh("heavylifterTailWingLD"), Part::VLD, Part::NOSHADOW)
//			->transform(Mat4::translate(0,up+0.3,-4)),
//
//		(new Part(0x444444ff))
//			->lod(mesh("heavylifterTailFin"), Part::HD, Part::SHADOW)
//			->lod(mesh("heavylifterTailFinLD"), Part::MD, Part::SHADOW)
//			->lod(mesh("heavylifterTailFinLD"), Part::VLD, Part::NOSHADOW)
//			->transform(Mat4::translate(0,up+0.3,-1.75)),
//
//		(new Part(0x888888ff))->gloss(2)
//			->lod(mesh("heavylifterEngineCowling"), Part::HD, Part::SHADOW)
//			->lod(mesh("heavylifterEngineCowlingLD"), Part::MD, Part::SHADOW)
//			->lod(mesh("heavylifterEngineCowlingLD"), Part::VLD, Part::NOSHADOW)
//			->transform(Mat4::translate(3,up+0.4,0)),
//
//		(new PartSpinner(0xccccccff))->gloss(2)
//			->lod(mesh("heavylifterEngineShaft"), Part::HD, Part::SHADOW)
//			->transform(Mat4::translate(3,up+0.4,0)),
//
//		(new Part(0x888888ff))->gloss(2)
//			->lod(mesh("heavylifterEngineCowling"), Part::HD, Part::SHADOW)
//			->lod(mesh("heavylifterEngineCowlingLD"), Part::MD, Part::SHADOW)
//			->lod(mesh("heavylifterEngineCowlingLD"), Part::VLD, Part::NOSHADOW)
//			->transform(Mat4::translate(-3,up+0.4,0)),
//
//		(new PartSpinner(0xccccccff))->gloss(2)
//			->lod(mesh("heavylifterEngineShaft"), Part::HD, Part::SHADOW)
//			->transform(Mat4::translate(-3,up+0.4,0)),
//
//		(new Part(0x888888ff))->gloss(2)
//			->lod(mesh("heavylifterEngineCowling"), Part::HD, Part::SHADOW)
//			->lod(mesh("heavylifterEngineCowlingLD"), Part::MD, Part::SHADOW)
//			->lod(mesh("heavylifterEngineCowlingLD"), Part::VLD, Part::NOSHADOW)
//			->transform(Mat4::rotate(Point::East, glm::radians(90.0f)) * Mat4::translate(-1.5,up+0.3,-4)),
//
//		(new PartSpinner(0xccccccff))->gloss(2)
//			->lod(mesh("heavylifterEngineShaft"), Part::HD, Part::SHADOW)
//			->transform(Mat4::rotate(Point::East, glm::radians(90.0f)) * Mat4::translate(-1.5,up+0.3,-4)),
//
//		(new Part(0x888888ff))->gloss(2)
//			->lod(mesh("heavylifterEngineCowling"), Part::HD, Part::SHADOW)
//			->lod(mesh("heavylifterEngineCowlingLD"), Part::MD, Part::SHADOW)
//			->lod(mesh("heavylifterEngineCowlingLD"), Part::VLD, Part::NOSHADOW)
//			->transform(Mat4::rotate(Point::East, glm::radians(90.0f)) * Mat4::translate(1.5,up+0.3,-4)),
//
//		(new PartSpinner(0xccccccff))->gloss(2)
//			->lod(mesh("heavylifterEngineShaft"), Part::HD, Part::SHADOW)
//			->transform(Mat4::rotate(Point::East, glm::radians(90.0f)) * Mat4::translate(1.5,up+0.3,-4)),
//
//		(new Part(0x444444ff))
//			->lod(mesh("heavylifterFrame"), Part::MD, Part::SHADOW)
//			->transform(Mat4::translate(0,up-0.55,0)),
//
//		(new Part(0x444444ff))
//			->lod(mesh("heavylifterArm"), Part::MD, Part::SHADOW)
//			->transform(Mat4::rotate(Point::Up, glm::radians(0.0f)) * Mat4::translate(1.6,up-0.55,-2)),
//
//		(new Part(0x444444ff))
//			->lod(mesh("heavylifterArm"), Part::MD, Part::SHADOW)
//			->transform(Mat4::rotate(Point::Up, glm::radians(180.0f)) * Mat4::translate(-1.6,up-0.55,-2)),
//
//		(new Part(0x444444ff))
//			->lod(mesh("heavylifterArm"), Part::MD, Part::SHADOW)
//			->transform(Mat4::rotate(Point::Up, glm::radians(0.0f)) * Mat4::translate(1.6,up-0.55,2)),
//
//		(new Part(0x444444ff))
//			->lod(mesh("heavylifterArm"), Part::MD, Part::SHADOW)
//			->transform(Mat4::rotate(Point::Up, glm::radians(180.0f)) * Mat4::translate(-1.6,up-0.55,2)),
//	};
}

void ScenarioBase::blimpTier(int tier) {

	auto blimpGloss = 8;

	auto logisticBlimp = fmt("blimp%d", tier);
	auto constructionBlimp = fmt("blimp-engineer%d", tier);

	if (!meshes.count("blimpBody")) {
		meshes["blimpBody"] = new Mesh("models/blimp-body-hd.stl");
		meshes["blimpBodyLD"] = new Mesh("models/blimp-body-ld.stl");
		meshes["blimpBodyVLD"] = new Mesh("models/blimp-body-vld.stl");
		meshes["blimpGondola"] = new Mesh("models/blimp-gondola-hd.stl");
		meshes["blimpGondolaLD"] = new Mesh("models/blimp-gondola-ld.stl");
		meshes["blimpFin"] = new Mesh("models/blimp-fin-hd.stl");
		meshes["blimpFinLD"] = new Mesh("models/blimp-fin-ld.stl");
		meshes["blimpFinVLD"] = new Mesh("models/blimp-fin-vld.stl");
		meshes["blimpEngine"] = new Mesh("models/blimp-engine-hd.stl");
		meshes["blimpEngineLD"] = new Mesh("models/blimp-engine-ld.stl");
		meshes["blimpSpar"] = new Mesh("models/blimp-spar-hd.stl");
		meshes["blimpSparLD"] = new Mesh("models/blimp-spar-ld.stl");
		meshes["blimpProp"] = new Mesh("models/blimp-prop-hd.stl");
		meshes["blimpPropLD"] = new Mesh("models/blimp-prop-ld.stl");

		meshes["blimpBody"]->smooth();
		meshes["blimpBodyLD"]->smooth();
		meshes["blimpBodyVLD"]->smooth();
	}

	auto blimpGondola = (new Part(0x666666ff))->gloss(blimpGloss)
		->lod(mesh("blimpGondola"), Part::HD, Part::SHADOW)
		->lod(mesh("blimpGondolaLD"), Part::MD, Part::SHADOW);

	auto blimpFin = (new Part(0x666666ff))->gloss(blimpGloss)
		->lod(mesh("blimpFin"), Part::HD, Part::SHADOW)
		->lod(mesh("blimpFinLD"), Part::MD, Part::SHADOW)
		->lod(mesh("blimpFinVLD"), Part::VLD, Part::SHADOW);

	auto blimpSpar = (new Part(0x999999ff))->gloss(4)
		->lod(mesh("blimpSpar"), Part::HD, Part::SHADOW)
		->lod(mesh("blimpSparLD"), Part::MD, Part::SHADOW);

	auto blimpProp = (new Part(0x999999ff))->gloss(4)
		->lod(mesh("blimpProp"), Part::HD, Part::SHADOW)
		->lod(mesh("blimpPropLD"), Part::MD, Part::SHADOW);

	{
		auto blimpBody = (new Part(0xffffffff))->gloss(blimpGloss)
			->lod(mesh("blimpBody"), Part::HD, Part::SHADOW)
			->lod(mesh("blimpBodyLD"), Part::MD, Part::SHADOW)
			->lod(mesh("blimpBodyVLD"), Part::VLD, Part::SHADOW);

		auto blimpEngine = (new Part(0xffffffff))->gloss(blimpGloss)
			->lod(mesh("blimpEngine"), Part::HD, Part::SHADOW)
			->lod(mesh("blimpEngineLD"), Part::MD, Part::SHADOW);

		Spec* spec = new Spec(logisticBlimp);
		spec->title = "Blimp";
		spec->collision = {0, 0, 0, 5, 5, 12};
		spec->selection = spec->collision;
		spec->iconD = 4.5;
		spec->iconV = 0;
		spec->place = Spec::Land|Spec::Water;

		spec->align = false;
		spec->flightPath = true;
		spec->flightPathClearance = 20.0f;
		spec->flightPathSpeed = 0.3f + (0.1f*tier);
		spec->flightLogistic = true;
		spec->flightLogisticRate = 1;
		spec->energyConsume = Energy::kW(50);
		spec->consumeCharge = true;
		spec->consumeChargeBuffer = Energy::kJ(50*tier);
		spec->consumeChargeRate = Energy::kW(50*tier);
		spec->store = true;
		spec->storeAnything = true;
		spec->tipStorage = true;
		spec->capacity = Mass::kg(300*tier);
		spec->forceDelete = true;
		spec->rotateGhost = true;
		spec->plan = false;
		spec->clone = true;

		spec->dronePoint = Point::Down*(spec->collision.h/2.0f);

		spec->materials = {
			{ Item::byName("battery")->id, 8 },
			{ Item::byName("aluminium-frame")->id, 50 },
			{ Item::byName("electric-motor")->id, 6 },
			{ Item::byName("mother-board")->id, 1 },
		};

		spec->highLOD = 10;

		spec->parts = {
			blimpBody,
			blimpGondola,
			blimpFin,
			blimpFin,
			blimpFin,
			blimpFin,
			blimpFin,
			blimpFin,
			blimpEngine,
			blimpSpar,
			blimpSpar,
			blimpSpar,
			blimpProp,
			blimpEngine,
			blimpSpar,
			blimpSpar,
			blimpSpar,
			blimpProp,
		};

		{
			for (uint i = 0; i < 36; i++) {
				Mat4 center = Mat4::rotate(Point::Up, glm::radians(-90.0f));

				Mat4 gondola = Mat4::translate(2.5,-1.6,0);

				Mat4 tailFin1 = Mat4::rotate(Point::East, -glm::radians(  0.0f)) * Mat4::translate(-2.6,0,0);
				Mat4 tailFin2 = Mat4::rotate(Point::East, -glm::radians( 90.0f)) * Mat4::translate(-2.6,0,0);
				Mat4 tailFin3 = Mat4::rotate(Point::East, -glm::radians(180.0f)) * Mat4::translate(-2.6,0,0);
				Mat4 tailFin4 = Mat4::rotate(Point::East, -glm::radians(270.0f)) * Mat4::translate(-2.6,0,0);

				Mat4 noseFin1 = Mat4::rotate(Point::East, -glm::radians( 90.0f)) * Mat4::translate(5,0,0.6);
				Mat4 noseFin2 = Mat4::rotate(Point::East, -glm::radians(270.0f)) * Mat4::translate(5,0,-0.6);

				Mat4 prop = Mat4::translate(-0.3,0,0) * Mat4::rotate(Point::East, -glm::radians((float)i*10));

				Mat4 engine1 = Mat4::translate(-3,-2.2,0) * Mat4::rotate(Point::East, -glm::radians(50.0f));
				Mat4 engine2 = Mat4::translate(-3,-2.2,0) * Mat4::rotate(Point::East,  glm::radians(50.0f));

				Mat4 spar1 = Mat4::translate(0,0.8,0) * Mat4::rotate(Point::East, glm::radians(35.0f));
				Mat4 spar2 = Mat4::translate(0,0.8,0) * Mat4::rotate(Point::East, glm::radians(-35.0f));
				Mat4 spar3 = Mat4::translate(0,0.8,0) * Mat4::rotate(Point::South, glm::radians(45.0f));

				spec->states.push_back({
					center,
					gondola * center,
					tailFin1 * center,
					tailFin2 * center,
					tailFin3 * center,
					tailFin4 * center,
					noseFin1 * center,
					noseFin2 * center,
					engine1 * center,
					spar1 * engine1 * center,
					spar2 * engine1 * center,
					spar3 * engine1 * center,
					prop * engine1 * center,
					engine2 * center,
					spar1 * engine2 * center,
					spar2 * engine2 * center,
					spar3 * engine2 * center,
					prop * engine2 * center,
				});
			}
		}
	}
}

void ScenarioBase::cartTier(int tier) {
	if (!meshes.count("cartWheel")) {
		meshes["cartWheel"] = new Mesh("models/cart-wheel-hd.stl");
		meshes["cartWheelLD"] = new Mesh("models/cart-wheel-ld.stl");
		meshes["cartChassis"] = new Mesh("models/cart-chassis-hd.stl");
		meshes["cartChassisLD"] = new Mesh("models/cart-chassis-ld.stl");
		meshes["cartStripeEngineer"] = new Mesh("models/cart-stripe-engineer-hd.stl");
		meshes["cartStripeEngineerLD"] = new Mesh("models/cart-stripe-engineer-ld.stl");
		meshes["cartCab"] = new Mesh("models/cart-cab-hd.stl");
		meshes["cartCabLD"] = new Mesh("models/cart-cab-ld.stl");
	}

	Spec* spec = new Spec(fmt("cart%d", tier));
	spec->title = "Cart";
	spec->collision = {0, 0, 0, 1.6, 1, 2};
	spec->selection = spec->collision;
	spec->iconD = 1.5;
	spec->iconV = 0.0;

	spec->parts = {
		(new Part(0xffffffff))
			->gloss(64)
			->lod(mesh("cartChassis"), Part::HD, Part::SHADOW)
			->lod(mesh("cartChassisLD"), Part::LD, Part::SHADOW)
			->transform(Mat4::rotateY(-glm::radians(180.0f)) * Mat4::translate(0,-0.5,0)),
		(new Part(0x444444ff))
			->lod(mesh("cartWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(0.6,-0.25,0.6)),
		(new Part(0x444444ff))
			->lod(mesh("cartWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(0.6,-0.25,-0.6)),
		(new Part(0x444444ff))
			->lod(mesh("cartWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(-0.6,-0.25,0.6)),
		(new Part(0x444444ff))
			->lod(mesh("cartWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(-0.6,-0.25,-0.6)),
	};

	spec->cartRoute =
		(new Part(0xffffffff))
			->gloss(64)
			->lod(mesh("cartCab"), Part::HD, Part::SHADOW)
			->lod(mesh("cartCabLD"), Part::LD, Part::SHADOW)
			->transform(Mat4::rotateY(-glm::radians(180.0f)) * Mat4::translate(0,-0.5,0))
	;

	spec->health = 200;
	spec->align = false;
	spec->enable = true;
	spec->status = true;
	spec->plan = false;
	spec->clone = true;
	spec->beacon = Point::Up*0.6 + Point::South*0.55;
	spec->cart = true;
	spec->cartSpeed = 10000.0/(float)(60*60*60);
	spec->cartWait = 120;
	spec->cartItem = Point::Up*0.25f + Point::North*0.35f;
	spec->forceDelete = true;
	spec->energyConsume = Energy::kW(50*tier);
	spec->consumeCharge = true;
	spec->consumeChargeBuffer = Energy::kJ(50*tier);
	spec->consumeChargeRate = Energy::kW(50*tier);
	spec->store = true;
	spec->tipStorage = true;
	spec->storeAnything = true;
	spec->storeSetUpper = true;
	spec->capacity = Mass::kg(300*tier);

	spec->materials = {
		{ Item::byName("electric-motor")->id, 1 },
		{ Item::byName("steel-sheet")->id, 1 },
		{ Item::byName("gear-wheel")->id, 1 },
		{ Item::byName("circuit-board")->id, 1 },
	};

	if (tier > 1) {
		spec->materials.push_back({ Item::byName("plastic-bar")->id, 1 });
		spec->materials.push_back({ Item::byName("battery")->id, 1 });
	}

	if (tier > 2) {
		spec->materials.push_back({ Item::byName("mother-board")->id, 1 });
	}

	spec = new Spec(fmt("cart-engineer%d", tier));
	spec->title = "Cart (Engineer)";
	spec->collision = {0, 0, 0, 1.6, 1, 2};
	spec->selection = spec->collision;
	spec->iconD = 1.5;
	spec->iconV = 0.0;

	spec->parts = {
		(new Part(0xffffffff))
			->gloss(64)
			->lod(mesh("cartChassis"), Part::HD, Part::SHADOW)
			->lod(mesh("cartChassisLD"), Part::LD, Part::SHADOW)
			->transform(Mat4::rotateY(-glm::radians(180.0f)) * Mat4::translate(0,-0.5,0)),
		(new Part(0xffa500ff))
			->gloss(64)
			->lod(mesh("cartStripeEngineer"), Part::HD, Part::NOSHADOW)
			->lod(mesh("cartStripeEngineerLD"), Part::LD, Part::NOSHADOW)
			->transform(Mat4::rotateY(-glm::radians(180.0f)) * Mat4::translate(0,-0.5,0)),
		(new Part(0x444444ff))
			->lod(mesh("cartWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(0.6,-0.25,0.6)),
		(new Part(0x444444ff))
			->lod(mesh("cartWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(0.6,-0.25,-0.6)),
		(new Part(0x444444ff))
			->lod(mesh("cartWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(-0.6,-0.25,0.6)),
		(new Part(0x444444ff))
			->lod(mesh("cartWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(-0.6,-0.25,-0.6)),
	};

	spec->cartRoute =
		(new Part(0xffffffff))
			->gloss(64)
			->lod(mesh("cartCab"), Part::HD, Part::SHADOW)
			->lod(mesh("cartCabLD"), Part::LD, Part::SHADOW)
			->transform(Mat4::rotateY(-glm::radians(180.0f)) * Mat4::translate(0,-0.5,0))
	;

	spec->health = 200;
	spec->align = false;
	spec->enable = true;
	spec->status = true;
	spec->plan = false;
	spec->clone = true;
	spec->beacon = Point::Up*0.6 + Point::South*0.55;
	spec->cart = true;
	spec->cartSpeed = 10000.0/(float)(60*60*60);
	spec->cartWait = 120;
	spec->cartItem = Point::Up*0.25f + Point::North*0.35f;
	spec->forceDelete = true;
	spec->energyConsume = Energy::kW(50*tier);
	spec->consumeCharge = true;
	spec->consumeChargeBuffer = Energy::MJ(1);
	spec->consumeChargeRate = Energy::kW(100*tier);
	spec->store = true;
	spec->tipStorage = true;
	spec->capacity = Mass::kg(200*tier);
	spec->storeSetUpper = true;
	spec->logistic = true;

	spec->depot = true;
	spec->depotRange = 50.0f;
	spec->depotDrones = 5;
	spec->dronePoint = Point::Up*(spec->collision.h/2.0f) + Point::Up*0.5f;

	spec->materials = {
		{ Item::byName("electric-motor")->id, 2 },
		{ Item::byName("steel-sheet")->id, 1 },
		{ Item::byName("gear-wheel")->id, 1 },
		{ Item::byName("circuit-board")->id, 2 },
	};

	if (tier > 1) {
		spec->materials.push_back({ Item::byName("plastic-bar")->id, 1 });
		spec->materials.push_back({ Item::byName("battery")->id, 1 });
	}

	if (tier > 2) {
		spec->materials.push_back({ Item::byName("mother-board")->id, 1 });
	}

	Spec::byName(fmt("cart%d", tier))->cycle = Spec::byName(fmt("cart-engineer%d", tier));
	Spec::byName(fmt("cart-engineer%d", tier))->cycle = Spec::byName(fmt("cart%d", tier));
}

void ScenarioBase::truckTier(int tier) {

	if (!meshes.count("truckChassisEngineer")) {
		meshes["truckChassisEngineer"] = new Mesh("models/truck-chassis-engineer-hd.stl");
		meshes["truckChassisEngineerLD"] = new Mesh("models/truck-chassis-engineer-ld.stl");
		meshes["truckChassisEngineerVLD"] = new Mesh("models/truck-chassis-engineer-vld.stl");
		meshes["truckStripeEngineer"] = new Mesh("models/truck-stripe-engineer-hd.stl");
		meshes["truckStripeEngineerLD"] = new Mesh("models/truck-stripe-engineer-ld.stl");
		meshes["truckChassisHauler"] = new Mesh("models/truck-chassis-hauler-hd.stl");
		meshes["truckChassisHaulerLD"] = new Mesh("models/truck-chassis-hauler-ld.stl");
		meshes["truckChassisHaulerVLD"] = new Mesh("models/truck-chassis-hauler-vld.stl");
		meshes["truckWheel"] = new Mesh("models/truck-wheel.stl");
		meshes["truckCab"] = new Mesh("models/truck-cab-hd.stl");
		meshes["truckCabLD"] = new Mesh("models/truck-cab-ld.stl");
	}

	Spec* spec = new Spec(fmt("truck%d", tier));
	spec->title = "Truck";

	spec->collision = {0, 0, 0, 2, 2, 3};
	spec->selection = spec->collision;
	spec->iconD = 2.0;
	spec->iconV = 0.0;

	spec->parts = {
		(new Part(0xffffffff))
			->lod(mesh("truckChassisHauler"), Part::HD, Part::SHADOW)
			->lod(mesh("truckChassisHaulerLD"), Part::MD, Part::SHADOW)
			->lod(mesh("truckChassisHaulerVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(0,0.3,0)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(-0.8,-0.75,-1)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(-0.8,-0.75,0)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(-0.8,-0.75,1)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(0.8,-0.75,-1)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(0.8,-0.75,0)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(0.8,-0.75,1)),
	};

	spec->cartRoute =
		(new Part(0xffffffff))
			->gloss(8)
			->lod(mesh("truckCab"), Part::HD, Part::SHADOW)
			->lod(mesh("truckCabLD"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(0,0.3,0))
	;

	spec->health = 500;
	spec->align = false;
	spec->enable = true;
	spec->status = true;
	spec->beacon = Point::Up + Point::South;
	spec->plan = false;
	spec->clone = true;
	spec->cart = true;
	spec->cartSpeed = 20000.0/(float)(60*60*60);
	spec->cartWait = 120;
	spec->cartItem = Point::Up*0.6 + Point::North*0.45;
	spec->forceDelete = true;
	spec->energyConsume = Energy::kW(150*tier);
	spec->consumeCharge = true;
	spec->consumeChargeBuffer = Energy::kJ(150*tier);
	spec->consumeChargeRate = Energy::kW(150*tier);
	spec->store = true;
	spec->tipStorage = true;
	spec->capacity = Mass::kg(1000*tier);
	spec->storeSetLower = true;
	spec->storeSetUpper = true;
	spec->storeAnything = true;

	spec->materials = {
		{ Item::byName("electric-motor")->id, 6 },
		{ Item::byName("steel-sheet")->id, 10 },
		{ Item::byName("gear-wheel")->id, 10 },
		{ Item::byName("circuit-board")->id, 10 },
	};

	if (tier > 1) {
		spec->materials.push_back({ Item::byName("plastic-bar")->id, 10 });
		spec->materials.push_back({ Item::byName("battery")->id, 10 });
	}

	if (tier > 2) {
		spec->materials.push_back({ Item::byName("mother-board")->id, 10 });
	}

	spec = new Spec(fmt("truck-engineer%d", tier));
	spec->title = "Truck (Engineer)";

	spec->collision = {0, 0, 0, 2, 2, 3};
	spec->selection = spec->collision;
	spec->iconD = 2.0;
	spec->iconV = 0.0;

	spec->parts = {
		(new Part(0xffffffff))
			->lod(mesh("truckChassisHauler"), Part::HD, Part::SHADOW)
			->lod(mesh("truckChassisHaulerLD"), Part::MD, Part::SHADOW)
			->lod(mesh("truckChassisHaulerVLD"), Part::VLD, Part::NOSHADOW)
			->transform(Mat4::translate(0,0.3,0)),
		(new Part(0xffa500ff))
			->gloss(16)
			->lod(mesh("truckStripeEngineer"), Part::HD, Part::NOSHADOW)
			->lod(mesh("truckStripeEngineerLD"), Part::LD, Part::NOSHADOW)
			->transform(Mat4::translate(0,0.3,0)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD)
			->transform(Mat4::translate(-0.8,-0.75,-1)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD)
			->transform(Mat4::translate(-0.8,-0.75,0)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD)
			->transform(Mat4::translate(-0.8,-0.75,1)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD)
			->transform(Mat4::translate(0.8,-0.75,-1)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD)
			->transform(Mat4::translate(0.8,-0.75,0)),
		(new Part(0x444444ff))
			->lod(mesh("truckWheel"), Part::MD)
			->transform(Mat4::translate(0.8,-0.75,1)),
	};

	spec->cartRoute =
		(new Part(0xffffffff))
			->gloss(8)
			->lod(mesh("truckCab"), Part::HD, Part::SHADOW)
			->lod(mesh("truckCabLD"), Part::MD, Part::SHADOW)
			->transform(Mat4::translate(0,0.3,0))
	;

	spec->health = 500;
	spec->align = false;
	spec->enable = true;
	spec->status = true;
	spec->plan = false;
	spec->clone = true;
	spec->beacon = Point::Up + Point::South;
	spec->cart = true;
	spec->cartSpeed = 20000.0/(float)(60*60*60);
	spec->cartWait = 120;
	spec->cartItem = Point::Up*0.6 + Point::North*0.45;
	spec->forceDelete = true;
	spec->energyConsume = Energy::kW(150*tier);
	spec->consumeCharge = true;
	spec->consumeChargeBuffer = Energy::kJ(150*tier);
	spec->consumeChargeRate = Energy::kW(150*tier);
	spec->store = true;
	spec->capacity = Mass::kg(800*tier);
	spec->tipStorage = true;
	spec->storeSetUpper = true;
	spec->storeSetLower = true;
	spec->logistic = true;

	spec->depot = true;
	spec->depotRange = 100.0f;
	spec->depotDrones = 10;
	spec->dronePoint = Point::Up*(spec->collision.h/2.0f) + Point::Up*0.5f;

	spec->materials = {
		{ Item::byName("electric-motor")->id, 20 },
		{ Item::byName("steel-sheet")->id, 10 },
		{ Item::byName("gear-wheel")->id, 10 },
		{ Item::byName("circuit-board")->id, 20 },
	};

	if (tier > 1) {
		spec->materials.push_back({ Item::byName("plastic-bar")->id, 10 });
		spec->materials.push_back({ Item::byName("battery")->id, 10 });
	}

	if (tier > 2) {
		spec->materials.push_back({ Item::byName("mother-board")->id, 10 });
	}
}

void ScenarioBase::droneDepotTier(int tier) {

	if (!meshes.count("depotBase")) {
		meshes["depotBase"] = new Mesh("models/depot-base-hd.stl");
		meshes["depotBaseLD"] = new Mesh("models/depot-base-ld.stl");
		meshes["depotBaseVLD"] = new Mesh("models/depot-base-vld.stl");
		meshes["depotSlat"] = new Mesh("models/depot-slat-hd.stl");
		meshes["depotSlatLD"] = new Mesh("models/depot-slat-ld.stl");
	}

	auto depot = fmt("drone-depot%d", tier);

	auto spec = new Spec(depot);
	spec->title = "Drone Port";
	spec->collision = {0, 0, 0, 3, 3, 3};
	spec->selection = spec->collision;
	spec->rotateGhost = true;
	spec->parts = {
		(new Part(0xFFD700ff))->gloss(16)
			->lod(mesh("depotBase"), Part::HD, Part::SHADOW)
			->lod(mesh("depotBaseLD"), Part::MD, Part::SHADOW)
			->lod(mesh("depotBaseVLD"), Part::VLD, Part::SHADOW)
			->transform(Mat4::translate(0,-1.5,0))
	};
	{
		std::vector<Mat4> states = {Mat4::identity};
		for (int i = 0; i < 12; i++) {
			spec->parts.push_back(
				(new Part(0x778899ff))->gloss(16)
					->lod(mesh("depotSlat"), Part::HD, Part::NOSHADOW)
					->lod(mesh("depotSlatLD"), Part::MD, Part::NOSHADOW)
					->transform(Mat4::translate(1.4,-1.2,0) * Mat4::rotateY((float)i*30.0f))
			);
		}
	}

	spec->networker = true;
	spec->networkInterfaces = tier;
	spec->networkWifi = {-1,1.5,0};
	spec->health = 150;

	spec->consumeCharge = true;
	spec->consumeChargeEffect = true;
	spec->consumeChargeBuffer = Energy::MJ(50) * (float)tier;
	spec->consumeChargeRate = Energy::kW(500) * (float)tier;

	spec->materials = {
		{Item::byName("steel-sheet")->id, 5},
		{Item::byName("plastic-bar")->id, 30*(uint)tier},
		{Item::byName("mother-board")->id, 1},
		{Item::byName("circuit-board")->id, 25*(uint)tier},
		{Item::byName("electric-motor")->id, 25*(uint)tier},
		{Item::byName("battery")->id, 25*(uint)tier + 25},
	};
	spec->depot = true;
	spec->depotAssist = true;
	spec->depotFixed = true;
	spec->depotRange = 100.0f + (10.0f*tier);
	spec->depotDrones = 25*tier;
	spec->depotDroneSpec = Spec::byName("drone");
	spec->dronePoint = Point::Up*(spec->collision.h/2.0f) + Point::Up*0.5f;
	spec->depotDispatchEnergy = Energy::MJ(1);
}

