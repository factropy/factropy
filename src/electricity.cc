
#include "entity.h"
#include "electricity.h"
#include "powerpole.h"

// Node

bool ElectricityNode::connected() {
	return network != nullptr;
}

// Producer

void ElectricityProducer::reset() {
	all.clear();
}

ElectricityProducer& ElectricityProducer::create(uint id) {
	ensure(!all.has(id));
	auto& producer = all[id];
	producer.id = id;
	producer.en = &Entity::get(id);
	producer.network = nullptr;
	return producer;
}

ElectricityProducer& ElectricityProducer::get(uint id) {
	return all.refer(id);
}

void ElectricityProducer::destroy() {
	if (network) network->drop(*this);
	all.erase(id);
}

void ElectricityProducer::connect() {
	if (en->isGhost()) ensure(!connected());
	if (en->isGhost()) return;
	if (connected()) return;
	auto pole = en->spec->powerpole ? &en->powerpole(): PowerPole::covering(en->box());
	if (pole && pole->network) pole->network->add(*this);
}

void ElectricityProducer::disconnect() {
	if (en->isGhost()) ensure(!connected());
	if (en->isGhost()) return;
	if (!connected()) return;
	if (network) network->drop(*this);
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
		network->production[spec->statsGroup].add(Sim::tick, supplied);
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
		network->production[spec->statsGroup].add(Sim::tick, supplied);
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
		network->production[spec->statsGroup].add(Sim::tick, supplied);
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
		network->production[spec->statsGroup].add(Sim::tick, supplied);
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

// Consumer

void ElectricityConsumer::reset() {
	all.clear();
}

ElectricityConsumer& ElectricityConsumer::create(uint id) {
	ensure(!all.has(id));
	auto& consumer = all[id];
	consumer.id = id;
	consumer.en = &Entity::get(id);
	consumer.network = nullptr;
	return consumer;
}

ElectricityConsumer& ElectricityConsumer::get(uint id) {
	return all.refer(id);
}

void ElectricityConsumer::destroy() {
	if (network) network->drop(*this);
	all.erase(id);
}

void ElectricityConsumer::connect() {
	if (en->isGhost()) ensure(!connected());
	if (en->isGhost()) return;
	if (connected()) return;
	auto pole = en->spec->powerpole ? &en->powerpole(): PowerPole::covering(en->box());
	if (pole && pole->network) pole->network->add(*this);
}

void ElectricityConsumer::disconnect() {
	if (en->isGhost()) ensure(!connected());
	if (en->isGhost()) return;
	if (!connected()) return;
	if (network) network->drop(*this);
}

Energy ElectricityConsumer::consume(Energy e) {
	if (en->isGhost()) ensure(!connected());
	if (!connected()) return 0;
	if (!en->isEnabled()) return 0;
	if (en->isGhost()) return 0;
	network->demand += e;
	e = e * network->satisfaction;
	network->consumption[en->spec->statsGroup].add(Sim::tick, e);
	return e;
}

// Buffer

void ElectricityBuffer::reset() {
	all.clear();
}

ElectricityBuffer& ElectricityBuffer::create(uint id) {
	ensure(!all.has(id));
	auto& buffer = all[id];
	buffer.id = id;
	buffer.en = &Entity::get(id);
	buffer.network = nullptr;
	return buffer;
}

ElectricityBuffer& ElectricityBuffer::get(uint id) {
	return all.refer(id);
}

void ElectricityBuffer::destroy() {
	if (network) network->drop(*this);
	all.erase(id);
}

void ElectricityBuffer::connect() {
	if (en->isGhost()) ensure(!connected());
	if (en->isGhost()) return;
	if (connected()) return;
	auto pole = en->spec->powerpole ? &en->powerpole(): PowerPole::covering(en->box());
	if (pole && pole->network) pole->network->add(*this);
}

void ElectricityBuffer::disconnect() {
	if (en->isGhost()) ensure(!connected());
	if (en->isGhost()) return;
	if (!connected()) return;
	if (network) network->drop(*this);
}

void ElectricityBuffer::discharge() {
	if (en->isGhost()) ensure(!connected());
	if (!connected()) return;
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;

	auto spec = en->spec;

	if (network->discharge > 0 && level > Energy(0)) {
		Energy transfer = std::min(level, spec->bufferDischargeRate * network->discharge);

		if (transfer) {
			level -= transfer;
			spec->statsGroup->energyGeneration.add(Sim::tick, transfer);
			network->production[spec->statsGroup].add(Sim::tick, transfer);
			network->supply += transfer;
		}
	}

	network->capacityBuffered += spec->bufferDischargeRate;
	network->capacityBufferedReady += std::min(level, spec->bufferDischargeRate);

	network->bufferedLevel += level;
	network->bufferedLimit += spec->bufferCapacity;
}

void ElectricityBuffer::charge() {
	if (en->isGhost()) ensure(!connected());
	if (!connected()) return;
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;

	auto spec = en->spec;

	if (network->charge > 0 && level < spec->bufferCapacity) {
		Energy transfer = std::min(spec->bufferCapacity-level, spec->bufferChargeRate * network->charge);

		if (transfer) {
			level += transfer;
			spec->statsGroup->energyConsumption.add(Sim::tick, transfer);
			network->consumption[spec->statsGroup].add(Sim::tick, transfer);
			network->demand += transfer;
		}
	}

	if (en->spec->bufferElectricityState && en->spec->states.size()) {
		float speed = level.portion(spec->bufferCapacity) * 50.0f;
		uint steps = (uint)std::round(speed);
		en->state += std::max(1u, steps);
		if (en->state >= en->spec->states.size()) {
			en->state = en->state - en->spec->states.size();
		}
	}
}

// Network

void ElectricityNetwork::reset() {
	all.clear();
}

ElectricityNetwork& ElectricityNetwork::create(uint id) {
	ensure(!all.has(id));
	auto& network = all[id];
	network.id = id;
	return network;
}

ElectricityNetwork& ElectricityNetwork::get(uint id) {
	return all.refer(id);
}

void ElectricityNetwork::tick() {
	if (rebuild) {
		rebuild = false;

		// disconnect non-poles
		for (auto& producer: ElectricityProducer::all) producer.disconnect();
		for (auto& consumer: ElectricityConsumer::all) consumer.disconnect();
		for (auto& buffer: ElectricityBuffer::all) buffer.disconnect();

		for (auto& network: ElectricityNetwork::all) {
			ensure(!network.producers.size());
			ensure(!network.consumers.size());
			ensure(!network.buffers.size());
		}

		struct Root {
			ElectricityNetwork* network = nullptr;
			PowerPole* pole = nullptr;
		};

		minivec<Root> roots;

		// choose a root pole for existing networks, and disconnect poles
		for (auto& pole: PowerPole::all) {
			if (pole.en->isGhost()) continue;
			if (pole.network) {
				auto network = pole.network;
				roots.push_back({network,&pole});
				for (auto& pid: network->poles) {
					PowerPole::get(pid).network = nullptr;
				}
				ensure(!pole.network);
				network->poles.clear();
			}
		}

		std::sort(roots.begin(), roots.end(), [&](const auto& a, const auto& b) {
			return a.network->id < b.network->id;
		});

		std::function<void(PowerPole* pole, ElectricityNetwork* network)> flood;

		flood = [&](PowerPole* pole, ElectricityNetwork* network) {
			if (pole->network) {
				ensure(pole->network == network);
				return;
			}
			pole->network = network;
			network->poles.insert(pole->id);
			for (auto lid: pole->links) {
				auto& sib = PowerPole::get(lid);
				if (sib.network == network) continue;
				ensure(!sib.network);
				flood(&sib, network);
			}
		};

		// rebuild existing networks from roots
		for (auto& root: roots) {
			ensure(root.pole);
			// networks have merged
			if (root.pole->network) continue;
			ensure(!root.network->poles.size());
			flood(root.pole, root.network);
		}

		// create any new networks required
		for (auto& pole: PowerPole::all) {
			if (pole.network) continue;
			if (pole.en->isGhost()) continue;
			flood(&pole, &create(++sequence));
		}

		// remove any empty networks
		minivec<uint> drop;
		for (auto& network: all) {
			if (!network.poles.size()) drop.push_back(network.id);
		}
		for (auto nid: drop) {
			all.erase(nid);
		}

		// reassign producers and consumers
		for (auto& producer: ElectricityProducer::all) producer.connect();
		for (auto& consumer: ElectricityConsumer::all) consumer.connect();
		for (auto& buffer: ElectricityBuffer::all) buffer.connect();

		for (auto& network: all) {
			notef("ElectricityNetwork %u %u %u %u %u",
				network.id,
				network.poles.size(),
				network.producers.size(),
				network.consumers.size(),
				network.buffers.size()
			);
		}
	}
}

void ElectricityNetwork::updatePre() {
	supply = 0;
	capacity = 0;
	capacityBuffered = 0;
	capacityReady = 0;
	capacityBufferedReady = 0;
	bufferedLevel = 0;
	bufferedLimit = 0;

	for (auto& [_,series]: production) series.set(Sim::tick, 0);
	for (auto& [_,series]: consumption) series.set(Sim::tick, 0);

	for (auto& pid: producers) ElectricityProducer::get(pid).produce();
	for (auto& bid: buffers) ElectricityBuffer::get(bid).discharge();

	float loadTarget = std::min(1.0f, demand.portion(capacityReady)+0.05f);
	if (loadTarget > load) load = std::min(1.0f, load+0.01f);
	if (loadTarget < load-0.011f && !(charge > 0)) load = std::max(0.05f, load-0.01f);
	if (loadTarget < load-0.051f && charge > 0) load = std::max(0.05f, load-0.01f);

	satisfaction = supply.portion(demand);
	demand = 0;

	if (load < 0.95f && bufferedLevel < bufferedLimit) {
		charge = std::min(1.0f, charge+0.01f);
	}

	if (load > 0.98f || bufferedLevel >= bufferedLimit) {
		charge = std::max(0.0f, charge-0.01f);
	}

	if (load > 0.99f && bufferedLevel > Energy(0)) {
		discharge = std::min(1.0f, discharge+0.01f);
	}

	if (load < 0.96f || bufferedLevel == Energy(0)) {
		discharge = std::max(0.0f, discharge-0.01f);
	}

	for (auto& bid: buffers) ElectricityBuffer::get(bid).charge();
}

void ElectricityNetwork::updatePost() {
	for (auto& [_,series]: production) series.update(Sim::tick);
	for (auto& [_,series]: consumption) series.update(Sim::tick);
}

void ElectricityNetwork::add(ElectricityProducer& producer) {
	producers.insert(producer.id);
	producer.network = this;
}

void ElectricityNetwork::add(ElectricityConsumer& consumer) {
	consumers.insert(consumer.id);
	consumer.network = this;
}

void ElectricityNetwork::add(ElectricityBuffer& buffer) {
	buffers.insert(buffer.id);
	buffer.network = this;
}

void ElectricityNetwork::drop(ElectricityProducer& producer) {
	producers.erase(producer.id);
	producer.network = nullptr;
}

void ElectricityNetwork::drop(ElectricityConsumer& consumer) {
	consumers.erase(consumer.id);
	consumer.network = nullptr;
}

void ElectricityNetwork::drop(ElectricityBuffer& buffer) {
	buffers.erase(buffer.id);
	buffer.network = nullptr;
}

Energy ElectricityNetwork::consume(Spec* spec, Energy e) {
	demand += e;
	e = e * satisfaction;
	consumption[spec->statsGroup].add(Sim::tick, e);
	spec->statsGroup->energyConsumption.add(Sim::tick, e);
	return e;
}

void ElectricityNetwork::consume(Spec* spec, Energy e, int count) {
	e = e * (float)count;
	demand += e;
	e = e * satisfaction;
	consumption[spec->statsGroup].add(Sim::tick, e);
	spec->statsGroup->energyConsumption.add(Sim::tick, e);
}

ElectricityNetworkState ElectricityNetwork::aggregate() {
	ElectricityNetworkState agg;
	for (auto& network: all) {
		agg.load = std::max(agg.load, network.load);
		agg.satisfaction = std::min(agg.satisfaction, network.satisfaction);
		agg.demand += network.demand;
		agg.supply += network.supply;
		agg.capacity += network.capacity;
		agg.capacityBuffered += network.capacityBuffered;
		agg.capacityReady += network.capacityReady;
		agg.capacityBufferedReady += network.capacityBufferedReady;
		agg.bufferedLevel += network.bufferedLevel;
		agg.bufferedLimit += network.bufferedLimit;
	}
	return agg;
}

bool ElectricityNetworkState::lowPower() {
	return load > 0.99f && satisfaction < 0.75f;
}

bool ElectricityNetworkState::brownOut() {
	return load > 0.99f && satisfaction < 0.95f;
}

bool ElectricityNetworkState::noCapacity() {
	return capacityReady == Energy(0);
}

ElectricityNetwork* ElectricityNetwork::primary() {
	for (auto& network: all) return &network;
	return nullptr;
}
