
#include "entity.h"
#include "electricity.h"

void ElectricityNetwork::add(Entity* en) {
	if (en->spec->generateElectricity) {
		producers[en] = ElectricityProducer(en, this);
	}
	if (en->spec->consumeElectricity) {
		consumers[en] = ElectricityConsumer(en, this);
	}
}

void ElectricityNetwork::drop(Entity* en) {
	if (en->spec->generateElectricity) {
		producers.erase(en);
	}
	if (en->spec->consumeElectricity) {
		consumers.erase(en);
	}
}

ElectricityNode::ElectricityNode(Entity* ent, ElectricityNetwork* net) {
	en = ent;
	network = net;
}

ElectricityProducer::ElectricityProducer(Entity* ent, ElectricityNetwork* net) : ElectricityNode(ent, net) {
}

ElectricityConsumer::ElectricityConsumer(Entity* ent, ElectricityNetwork* net) : ElectricityNode(ent, net) {
}

bool ElectricityNode::connected() {
	return network != nullptr;
}

void ElectricityProducer::produce() {
	if (!connected()) return;
	if (!en->isEnabled()) return;
	if (!en->isGenerating()) return;
	if (en->isGhost()) return;

	auto spec = en->spec;

	// At the start of the game when a single generator exists, or when the electricity network
	// fuel supply crashes and producers need to restart, load must always be slightly > 0.
	Energy energy = std::max(Energy::J(1), spec->energyGenerate * network->load);

	if (spec->generateElectricity && spec->consumeFuel) {
		auto& burn = en->burner();

		Energy supplied = burn.consume(energy);
		spec->statsGroup->energyGeneration.add(Sim::tick, supplied);
		network->supply += supplied;

		if (burn.energy) network->capacityReady += spec->energyGenerate;
		network->capacity += spec->energyGenerate;

		if (spec->burnerState) {
			// Burner state is a smooth progression between 0 and #spec->states
			en->state = std::floor((float)burn.energy.value/(float)burn.buffer.value * (float)spec->states.size());
			en->state = std::min((uint16_t)spec->states.size(), std::max((uint16_t)1, en->state)) - 1;
		}
		return;
	}

	if (spec->generateElectricity && spec->consumeThermalFluid) {
		auto& gen = en->generator();

		Energy supplied = gen.consume(energy);
		spec->statsGroup->energyGeneration.add(Sim::tick, supplied);
		network->supply += supplied;

		if (gen.supplying) network->capacityReady += spec->energyGenerate;
		network->capacity += spec->energyGenerate;

		// The state for producers is currently stopped, slow or fast. The steam-engine entity
		// uses state to spin its flywheel albeit jerkily. This should be done in a smoother
		// fashion someday
		if (spec->generatorState && supplied) {
			en->state += supplied > (spec->energyGenerate * 0.5f) ? 2: 1;
			if (en->state >= spec->states.size()) en->state -= spec->states.size();
		}

		return;
	}

	if (spec->windTurbine) {
		auto pos = en->pos();
		float wind = Sim::windSpeed({pos.x, pos.y-(spec->collision.h/2.0f), pos.z});

		Energy supplied = spec->energyGenerate * wind;
		spec->statsGroup->energyGeneration.add(Sim::tick, supplied);
		network->supply += supplied;

		if (wind > 0.0f) {
			en->state += std::ceil(wind/10.0f);
			if (en->state >= spec->states.size()) en->state -= spec->states.size();

			network->capacityReady += supplied;
			network->capacity += supplied;
		}

		return;
	}

	if (spec->generateElectricity && spec->consumeMagic) {
		Energy supplied = energy;
		spec->statsGroup->energyGeneration.add(Sim::tick, supplied);
		network->supply += supplied;
		network->capacityReady += spec->energyGenerate;
		network->capacity += spec->energyGenerate;

		// The state for producers is currently stopped, slow or fast. The steam-engine entity
		// uses state to spin its flywheel albeit jerkily. This should be done in a smoother
		// fashion someday
		if (spec->generatorState && supplied) {
			en->state += supplied > (spec->energyGenerate * 0.5f) ? 2: 1;
			if (en->state >= spec->states.size()) en->state -= spec->states.size();
		}
		return;
	}
}

Energy ElectricityConsumer::consume(Energy e) {
	if (!connected()) return 0;
	if (!en->isEnabled()) return 0;
	if (en->isGhost()) return 0;
	network->demand += e;
	return e * network->satisfaction;
}
