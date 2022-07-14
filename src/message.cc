#include "message.h"
#include "sim.h"
#include "scene.h"

// On-screen messages visible to the player. Not a log file.

void Message::tick() {
	for (auto [_,message]: all) {
		if (!message->sent) message->check();
	}
}

Message* Message::byName(std::string name) {
	ensure(all.count(name));
	return all[name];
}

Message::Message(std::string nname) {
	ensuref(!all.count(name), "duplicate message %s", name);
	name = nname;
	all[name] = this;
}

void Message::check() {
	if (!active()) return;

	if (when > Sim::tick) return;

	complete();
}

void Message::complete() {
	sent = true;
	scene.print(text);
	notef("message: %s", text);
}

bool Message::active() {
	if (sent) return false;

	return true;
}


