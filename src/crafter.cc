#include "common.h"
#include "crafter.h"

// Crafter components take input materials, do some work and output different
// materials. Almost everything that assembles or mines or pumps or teleports
// is at heart a Crafter. Recipes it what to do. Specs tell it how to behave.

void Crafter::reset() {
	hot.clear();
	cold.clear();
	all.clear();
}

void Crafter::tick() {
	hot.tick();
	cold.tick();
	for (auto crafter: hot) crafter->update();
	for (auto crafter: cold) crafter->update();
}

Crafter& Crafter::create(uint id) {
	ensure(!all.has(id));
	Crafter& crafter = all[id];
	crafter.id = id;
	crafter.en = &Entity::get(id);
	crafter.working = false;
	crafter.progress = 0.0f;
	crafter.efficiency = 0.0f;
	crafter.recipe = nullptr;
	crafter.changeRecipe = crafter.en->spec->crafterRecipe;
	crafter.energyUsed = 0;
	crafter.completed = 0;
	crafter.interval = false;
	crafter.transmit = false;
	crafter.updatedPipes = 0;
	cold.insert(&crafter);
	return crafter;
}

Crafter& Crafter::get(uint id) {
	return all.refer(id);
}

void Crafter::destroy() {
	hot.erase(this);
	cold.erase(this);
	all.erase(id);
}

CrafterSettings* Crafter::settings() {
	return new CrafterSettings(*this);
}

CrafterSettings::CrafterSettings(Crafter& crafter) {
	recipe = crafter.changeRecipe ? crafter.changeRecipe: crafter.recipe;
	transmit = crafter.transmit;
}

void Crafter::setup(CrafterSettings* settings) {
	changeRecipe = settings->recipe;
	transmit = settings->transmit;
}

Point Crafter::output() {
	return en->pos().floor(0.5f) + (en->dir() * (en->spec->collision.d/2.0f+0.5f));
}

float Crafter::inputsProgress() {
	float avg = 0.0f;
	float agg = 0.0f;
	if (en->spec->store && recipe) {
		for (auto [iid,count]: recipe->inputItems) {
			agg += std::min(1.0f, (float)en->store().count(iid)/float(count));
		}
		avg = agg/(float)recipe->inputItems.size();
	}
	return std::max(0.0f, std::min(1.0f, avg));
}

std::vector<Point> Crafter::pipeConnections() {
	return en->spec->relativePoints(en->spec->pipeConnections, en->dir().rotation(), en->pos());
}

std::vector<Point> Crafter::pipeInputConnections() {
	return en->spec->relativePoints(en->spec->pipeInputConnections, en->dir().rotation(), en->pos());
}

std::vector<Point> Crafter::pipeOutputConnections() {
	return en->spec->relativePoints(en->spec->pipeOutputConnections, en->dir().rotation(), en->pos());
}

void Crafter::updatePipes() {
	// Only go looking for adjacent pipes, which is a cheap query but expensive in aggregate, if
	// something in the surrounding area has changed. See Entity::index().
	// Originally this seemed like the first place in the game that would require a messaging
	// system so that crafters could "subscribe" to changes in their vicinity, but the gridagg
	// approach benchmarks very fast and is loosely coupled.
	uint64_t changeNearby = Entity::gridPipesChange.max(en->box().grow(1.0f), 0);
	if (updatedPipes && changeNearby < updatedPipes) return;

	updatedPipes = Sim::tick;

	inputPipes.clear();

	auto candidates = Entity::intersecting(en->box().grow(1.0f), Entity::gridPipes);
	discard_if(candidates, [](Entity* ec) { return ec->isGhost(); });

	for (auto point: pipeConnections()) {
		for (auto pid: Pipe::servicing(point.box(), candidates)) {
			inputPipes.push_back(pid);
		}
	}

	for (auto point: pipeInputConnections()) {
		for (auto pid: Pipe::servicing(point.box(), candidates)) {
			inputPipes.push_back(pid);
		}
	}

	deduplicate(inputPipes);

	outputPipes.clear();

	for (auto point: pipeOutputConnections()) {
		for (auto pid: Pipe::servicing(point.box(), candidates)) {
			outputPipes.push_back(pid);
		}
	}

	deduplicate(outputPipes);
}

bool Crafter::exporting() {
	return exportItems.size() > 0 || exportFluids.size() > 0;
}

minimap<Stack,&Stack::iid> Crafter::inputItemsState() {
	minimap<Stack,&Stack::iid> found;
	auto& store = en->store();
	for (auto [iid,_]: recipe->inputItems) {
		found[iid].size = store.count(iid);
	}
	return found;
}

minimap<Stack,&Stack::iid> Crafter::outputItemsState() {
	minimap<Stack,&Stack::iid> found;
	auto& store = en->store();

	if (exportItems.size()) {
		for (auto [iid,count]: exportItems) {
			found[iid].size = count + store.count(iid);
		}
	}
	else
	if (recipe->outputItems.size()) {
		for (auto [iid,_]: recipe->outputItems) {
			found[iid].size = store.count(iid);
		}
	}

	return found;
}

bool Crafter::inputItemsReady() {
	bool itemsReady = true;

	if (recipe->inputItems.size()) {
		auto& store = en->store();
		itemsReady = !store.isEmpty();

		for (auto [iid,count]: recipe->inputItems) {
			itemsReady = itemsReady && store.count(iid) >= count;
		}
	}

	return itemsReady;
}

minimap<Amount,&Amount::fid> Crafter::inputFluidsState() {
	minimap<Amount,&Amount::fid> found;

	if (recipe->inputFluids.size()) {
		updatePipes();
		for (auto [fid,count]: recipe->inputFluids) {
			found[fid].size = 0;
			for (uint pid: inputPipes) {
				Entity& pe = Entity::get(pid);
				if (pe.isGhost()) continue;
				Pipe& pipe = Pipe::get(pid);
				if (!pipe.network) continue;
				found[fid].size += pipe.network->count(fid);
			}
		}
	}

	return found;
}

minimap<Amount,&Amount::fid> Crafter::outputFluidsState() {
	minimap<Amount,&Amount::fid> found;

	if (exportFluids.size()) {
		for (auto& amount: exportFluids) {
			found[amount.fid].size = amount.size;
		}
	}
	else
	if (recipe->outputFluids.size()) {
		for (auto [fid,count]: recipe->outputFluids) {
			found[fid].size = 0;
		}
	}

	return found;
}

bool Crafter::inputFluidsReady() {
	bool fluidsReady = true;

	auto state = inputFluidsState();
	for (auto [fid,count]: recipe->inputFluids) {
		if (state[fid].size < count) fluidsReady = false;
	}

	return fluidsReady;
}

bool Crafter::inputMiningReady() {
	return !recipe->mine || world.canMine(en->miningBox(), recipe->mine);
}

bool Crafter::inputDrillingReady() {
	return !recipe->drill || world.canDrill(en->drillingBox(), recipe->drill);
}

bool Crafter::outputItemsReady() {
	if (!en->spec->crafterManageStore) return true;

	bool outputReady = true;

	if (recipe->outputItems.size()) {
		auto& store = en->store();
		for (auto& [iid,count]: recipe->outputItems) {
			uint have = store.count(iid);
			outputReady = outputReady && (have <= count || have == 1);
		}
	}
	return outputReady;
}

bool Crafter::insufficientResources() {
	if (!recipe) return false;
	return !inputItemsReady() || !inputFluidsReady() || !inputMiningReady() || !inputDrillingReady();
}

bool Crafter::excessProducts() {
	if (!recipe) return false;
	if (recipe->mine) return !working && exporting() && en->store().isFull();
	if (recipe->drill) return !working && exporting();
	return !outputItemsReady() || (!working && exporting() && !interval && recipe && recipe->licensed);
}

bool Crafter::craftable(Recipe* r) {
	for (auto& tag: en->spec->recipeTags) {
		if (r->tags.count(tag)) {
			return true;
		}
	}
	return false;
}

void Crafter::craft(Recipe* r) {
	changeRecipe = r;
}

void Crafter::retool(Recipe* r) {
	recipe = r;

	ensure(!recipe || recipe->energyUsage > Energy(0));

	if (recipe) {
		exportItems.clear();
		exportFluids.clear();
	}

	if (en->spec->store && en->spec->crafterManageStore) {
		en->store().levels.clear();
		en->store().stacks.clear();
	}

	if (recipe && en->spec->crafterManageStore) {
		for (auto [iid,count]: recipe->inputItems) {
			en->store().levelSet(iid, count*2, count*2);
		}
		for (auto [iid,_]: recipe->outputItems) {
			en->store().levelSet(iid, 0, 0);
		}
		if (recipe->mine) {
			en->store().levelSet(recipe->mine, 0, 1);
		}
	}

	working = false;
	energyUsed = 0;
	progress = 0.0f;
	efficiency = 0.0f;
}

void Crafter::update() {
	if (en->isGhost()) {
		cold.insert(this);
		return;
	}

	interval = false;
	bool enabled = en->isEnabled();

	if (en->spec->crafterProgress) {
		en->state = (uint)std::floor(progress * (float)en->spec->states.size());
	}
	else
	if (en->spec->crafterOnOff) {
		en->state = en->isEnabled() && working && efficiency > 0.01f ? 1:0;
	}
	else
	if (en->spec->crafterState && en->isEnabled() && working) {
		en->state = en->state + (efficiency > 0.5 ? 2: 1);
		if (en->state >= en->spec->states.size()) en->state = 0;
	}

	bool transmitting = false;

	if (recipe && recipe->mine && en->spec->networker && en->spec->crafterTransmitResources && transmit) {
		transmitting = true;
		auto& networker = en->networker();
		for (auto& interface: networker.interfaces) {
			if (interface.network)
				interface.write((Stack){recipe->mine, world.countMine(en->box(), recipe->mine)});
		}
	}

	if (recipe && recipe->drill && en->spec->networker && en->spec->crafterTransmitResources && transmit) {
		transmitting = true;
		auto& networker = en->networker();
		for (auto& interface: networker.interfaces) {
			if (interface.network)
				interface.write((Amount){recipe->drill, world.countDrill(en->box(), recipe->drill)});
		}
	}

	if (exporting()) {

		for (auto& stack: exportItems) {
			if (!stack.size) continue;
			stack = en->store().insert(stack);
		}

		bool empty = true;

		for (auto& stack: exportItems) {
			if (stack.size) {
				empty = false;
				break;
			}
		}

		if (empty) {
			exportItems.clear();
		}

		if (exportFluids.size()) {
			updatePipes();

			for (auto& amount: exportFluids) {
				if (!amount.size) continue;

				for (uint pid: outputPipes) {
					Entity& pe = Entity::get(pid);
					if (pe.isGhost()) continue;
					Pipe& pipe = Pipe::get(pid);
					if (!pipe.network) continue;

					amount = pipe.network->inject(amount);
				}
			}

			bool empty = true;

			for (auto& amount: exportFluids) {
				if (amount.size) {
					empty = false;
					break;
				}
			}

			if (empty) {
				exportFluids.clear();
			}
		}
	}

	if (changeRecipe) {
		if (changeRecipe != recipe) {
			retool(changeRecipe);
		}
		changeRecipe = NULL;
	}

	if (enabled && !working && !exporting() && recipe && recipe->licensed) {

		if (inputItemsReady() && inputFluidsReady() && inputMiningReady() && inputDrillingReady() && outputItemsReady()) {

			for (auto [iid,count]: recipe->inputItems) {
				en->store().remove({iid,count});
				Item::get(iid)->consume(count);
			}

			if (recipe->inputFluids.size()) {
				std::map<uint,int> slurp;
				for (auto [fid,count]: recipe->inputFluids) {
					slurp[fid] = (int)count;
				}
				for (uint pid: inputPipes) {
					Entity& pe = Entity::get(pid);
					if (pe.isGhost()) continue;
					Pipe& pipe = Pipe::get(pid);
					if (!pipe.network) continue;
					uint fid = pipe.network->fid;
					if (slurp[fid] > 0) {
						Amount amount = pipe.network->extract({fid,(uint)slurp[fid]});
						slurp[fid] -= (int)amount.size;
						Fluid::get(fid)->consume(amount.size);
					}
				}
			}

			working = true;
			energyUsed = 0;
			progress = 0.0f;
			efficiency = 0.0f;
		}
	}

	en->setBlocked(!working);

	if (enabled && working) {
		energyUsed += en->consume(consumption() * speed());
		efficiency = energyUsed.portion(consumption());
		progress = energyUsed.portion(recipe->energyUsage);

		if (progress > 0.999) {
			for (auto [iid,count]: recipe->outputItems) {
				exportItems.push_back({iid, count});
				Item::get(iid)->produce(count);
			}

			for (auto [fid,count]: recipe->outputFluids) {
				exportFluids.push_back({fid, count});
				Fluid::get(fid)->produce(count);
			}

			if (recipe->mine) {
				Stack stack = world.mine(en->miningBox(), recipe->mine);
				if (stack.size) {
					exportItems.push_back(stack);
					Item::get(stack.iid)->produce(stack.size);
				}
			}

			if (recipe->drill) {
				Amount amount = world.drill(en->drillingBox(), recipe->drill);
				if (amount.size) {
					exportFluids.push_back(amount);
					Fluid::get(amount.fid)->produce(amount.size);
				}
			}

			if (recipe->delivery) {
				for (auto [iid,count]: recipe->inputItems) {
					Item::get(iid)->supply(count);
				}
			}

			working = false;
			energyUsed = 0;
			progress = 0.0f;
			efficiency = 0.0f;
			interval = true;
			completed++;
		}
	}

	if (enabled && en->spec->crafterOutput) {
		auto& store = en->store();
		uint iid = 0;
		for (auto& stack: store.stacks) {
			if (store.countActiveProviding(stack.iid)) {
				iid = stack.iid;
				break;
			}
		}
		if (iid) {
			auto eo = Entity::at(en->pos() + en->spec->crafterOutputPos.transform(en->dir().rotation()));
			if (eo && !eo->isGhost() && eo->spec->conveyor) {
				auto& conveyor = eo->conveyor();
				if (conveyor.insertAnyBack(iid) || conveyor.insertAnyFront(iid)) {
					store.remove({iid,1});
				}
			}
			else
			if (eo && !eo->isGhost() && eo->spec->consumeFuel && eo->burner().store.isAccepting(iid)) {
				eo->burner().store.insert({iid,1});
				store.remove({iid,1});
			}
			else
			if (eo && !eo->isGhost() && eo->spec->store && eo->store().isAccepting(iid)) {
				eo->store().insert({iid,1});
				store.remove({iid,1});
			}
			else
			if (eo && !eo->isGhost() && eo->spec->store && !eo->store().strict() && !eo->store().level(iid) && eo->store().countSpace(iid)) {
				eo->store().insert({iid,1});
				store.remove({iid,1});
			}
		}
	}

	if (working || interval || transmitting) hot.insert(this); else cold.insert(this);
}

float Crafter::speed() {
	return en->spec->crafterRate * (recipe ? recipe->rate(en->spec): 1.0f) * Effector::speed(id);
}

Energy Crafter::consumption() {
	return std::max(en->spec->crafterEnergyConsume, en->spec->energyConsume);
}
