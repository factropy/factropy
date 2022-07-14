#include "monocar.h"

// Monorail car components navigate Monorail towers carying shipping containers.

void Monocar::reset() {
	all.clear();
}

void Monocar::tick() {
	for (auto& monocar: all) monocar.update();
}

Monocar& Monocar::create(uint id) {
	ensure(!all.has(id));
	Monocar& monocar = all[id];
	monocar.id = id;
	monocar.en = &Entity::get(id);
	monocar.tower = 0;
	monocar.speed = 0;
	monocar.bogieA.arrive = Point::Zero;
	monocar.dirArrive = Point::Zero;
	monocar.bogieB.arrive = Point::Zero;
	monocar.dirDepart = Point::Zero;
	monocar.bogieA.pos = Point::Zero;
	monocar.bogieA.dir = Point::Zero;
	monocar.bogieA.step = 0;
	monocar.bogieB.pos = Point::Zero;
	monocar.bogieB.dir = Point::Zero;
	monocar.bogieB.step = 0;
	monocar.state = State::Start;
	monocar.line = Monorail::Red;
	monocar.blocked = false;
	return monocar;
}

Monocar& Monocar::get(uint id) {
	return all.refer(id);
}

void Monocar::destroy() {
	all.erase(id);
}

Monorail* Monocar::getTower(Point pos) {
	Box box = pos.box(); box.h = 1000.0;
	for (auto em: Entity::intersecting(box)) {
		if (!em->spec->monorail) continue;
		return &em->monorail();
	}
	return nullptr;
}

void Monocar::start() {
	tower = 0;
	speed = 0;
	bogieA.arrive = Point::Zero;
	dirArrive = Point::Zero;
	bogieB.arrive = Point::Zero;
	dirDepart = Point::Zero;
	steps.clear();
	bogieA.step = 0;
	bogieB.step = 0;

	Monorail* monorail = getTower(en->pos());
	if (!monorail) return;
	monorail->claim(id);
	tower = monorail->id;

	state = State::Acquire;
	acquire();
}

void Monocar::acquire() {
	if (!tower || !Monorail::all.has(tower)) {
		state = State::Start;
		return;
	}

	Monorail* monorailB = &Monorail::get(tower);
	monorailB->claim(id);

	Monorail* monorailA = monorailB->next(&line, signals());

	if (!monorailA || monorailA->en->isGhost()) {
		speed = 0;
		Sim::alerts.monocarsBlocked++;
		blocked = true;
		return;
	}

	if (!monorailA->claim(id)) {
		speed = 0;
		return;
	}

	tower = monorailA->id;
	rail = monorailB->railTo(monorailA);

	steps = rail.steps(1.0);
	ensuref(steps.size(), "invalid rail path");

	dirArrive = (monorailA->depart() - monorailA->origin()).normalize();
	dirDepart = (monorailB->depart() - monorailB->origin()).normalize();

	bogieA.pos = monorailB->origin() + (dirDepart * en->spec->monocarBogieA);
	bogieA.dir = dirDepart;
	bogieB.pos = monorailB->origin() + (dirDepart * en->spec->monocarBogieB);
	bogieB.dir = dirDepart;

	bogieA.step = 0;
	bogieB.step = 0;

	bogieA.arrive = monorailA->origin() + (dirArrive * en->spec->monocarBogieA);
	bogieB.arrive = monorailA->origin() + (dirArrive * en->spec->monocarBogieB);

	steps.push_back(bogieA.arrive);

	state = State::Travel;
	travel();
}

void Monocar::travel() {
	if (!tower || !Monorail::all.has(tower)) {
		state = State::Start;
		return;
	}

	float fueled = en->consumeRate(en->spec->energyConsume);

	Monorail* monorail = &Monorail::get(tower);
	monorail->claim(id);

	if (flags.stop) return;

	int lineLast = line;
	Monorail* next = monorail->next(&lineLast, signals());
	if (next && next->en->isGhost()) next = nullptr;

	bool stopping = fueled < 0.5 || monorail->en->spec->monorailStop || (next && next->occupied());

	if (stopping && bogieA.arrive.distance(bogieA.pos) < en->spec->collision.d) {
		speed = std::max(0.01f, speed*0.95f);
	}
	else
	if (speed < en->spec->monocarSpeed) {
		speed = std::max(0.01f, speed*1.1f);
	}

	if (steps[bogieA.step].distance(bogieA.pos) < speed) {
		bogieA.step++;
	}

	if (bogieA.step >= steps.size()) {
		bogieA.pos = bogieA.arrive;
		bogieA.dir = dirArrive;
		steps.clear();
	}
	else {
		Point p = bogieA.pos;
		Point v = steps[bogieA.step] - p;
		Point n = v.normalize();
		Point s = n * speed;
		bogieA.pos = p + s;
		bogieA.dir = n;
	}

	if (!steps.size()) {
		bogieB.pos = bogieB.arrive;
		bogieB.dir = dirDepart;
	}
	else {
		if (steps[bogieB.step].distance(bogieB.pos) < speed) {
			bogieB.step++;
		}

		if (bogieB.step >= steps.size()) {
			bogieB.pos = bogieB.arrive;
			bogieB.dir = dirDepart;
		}
		else {
			Point p = bogieB.pos;
			Point v = steps[bogieB.step] - p;
			Point n = v.normalize();
			Point s = n * speed;
			bogieB.pos = p + s;
			bogieB.dir = n;
		}
	}

	{
		auto deck = bogieA.pos - bogieB.pos;
		auto rotate = deck.rotation();
		auto raise = Point::Up.transform(rotate) * (en->spec->collision.h/2.0);

		Point pos = (bogieA.pos + raise) - (deck*0.5);
		Point dir = deck.normalize();
		en->move(pos, dir);
	}

	if (container) Entity::get(container).bump(positionCargo(), en->dir());

	if (!steps.size()) state = State::Stop;
}

void Monocar::stop() {
	if (!tower || !Monorail::all.has(tower)) {
		state = State::Start;
		return;
	}

	Monorail* monorail = &Monorail::get(tower);
	monorail->claim(id);

	if (monorail->en->spec->monorailStop) {
		speed = 0;
		state = container ? State::Unload: State::Load;
		return;
	}

	state = State::Acquire;
	acquire();
}

void Monocar::unload() {
	if (!tower || !Monorail::all.has(tower)) {
		state = State::Start;
		return;
	}

	if (flags.move) {
		state = State::Acquire;
		return;
	}

	Monorail* monorail = &Monorail::get(tower);
	monorail->claim(id);

	ensure(monorail->en->spec->monorailStop);
	if (monorail->canUnload()) state = State::Unloading;
}

void Monocar::unloading() {
	if (!tower || !Monorail::all.has(tower)) {
		state = State::Start;
		return;
	}

	Monorail* monorail = &Monorail::get(tower);
	monorail->claim(id);

	if (!container) {
		state = State::Stop;
		return;
	}

	ensure(monorail->en->spec->monorailStop);
	auto ec = &Entity::get(container);

	if (ec->pos().distance(monorail->positionUnload()) > 0.02) {
		Point dir = (monorail->positionUnload() - ec->pos()).normalize();
		ec->bump(ec->pos() + (dir * 0.02), en->dir());
	}
	else {
		ec->bump(monorail->positionUnload(), en->dir());
		monorail->unload(container);
		container = 0;
		state = State::Load;
	}
}

void Monocar::load() {
	if (!tower || !Monorail::all.has(tower)) {
		state = State::Start;
		return;
	}

	if (flags.move) {
		state = State::Acquire;
		return;
	}

	Monorail* monorail = &Monorail::get(tower);
	monorail->claim(id);

	ensure(monorail->en->spec->monorailStop);

	container = monorail->load();
	if (container) state = State::Loading;
}

void Monocar::loading() {
	if (!tower || !Monorail::all.has(tower)) {
		state = State::Start;
		return;
	}

	Monorail* monorail = &Monorail::get(tower);
	monorail->claim(id);

	if (!container) {
		state = State::Stop;
		return;
	}

	ensure(monorail->en->spec->monorailStop);
	auto ec = &Entity::get(container);

	if (ec->pos().distance(positionCargo()) > 0.02) {
		Point dir = (positionCargo() - ec->pos()).normalize();
		ec->bump(ec->pos() + (dir * 0.02), en->dir());
	}
	else {
		ec->bump(positionCargo(), en->dir());
		state = State::Acquire;
	}
}

void Monocar::update() {
	if (en->isGhost()) return;

	if (container && !Entity::exists(container)) container = 0;

	iid = 0;

	if (!iid && container) {
		auto& store = Store::get(container);
		if (store.stacks.size()) iid = store.stacks.front().iid;
	}

	if (!iid && constants.size()) {
		for (auto& signal: constants) {
			if (signal.valid() && signal.key.type == Signal::Type::Stack) {
				iid = signal.key.item()->id;
				break;
			}
		}
	}

	blocked = false;

	switch (state) {
		case State::Start: start(); break;
		case State::Acquire: acquire(); break;
		case State::Travel: travel(); break;
		case State::Stop: stop(); break;
		case State::Unload: unload(); break;
		case State::Unloading: unloading(); break;
		case State::Load: load(); break;
		case State::Loading: loading(); break;
	}

	flags.move = false;
}

Point Monocar::positionCargo() {
	return en->spec->monocarContainer.transform(en->dir().rotation()) + en->pos();
}

minimap<Signal,&Signal::key> Monocar::signals() {
	minimap<Signal,&Signal::key> sigs;
	if (container) {
		sigs = Store::get(container).signals();
	}
	for (auto& signal: constants) {
		if (signal.valid()) sigs.insert(signal);
	}
	return sigs;
}
