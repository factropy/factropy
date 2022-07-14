#include "common.h"
#include "signal.h"
#include "catenate.h"
#include "sim.h"
#include <type_traits>

// Wifi network packets

minimap<Signal,&Signal::key> Signal::NoSignals;

uint Signal::addLabel(const std::string& str) {
	for (auto& lbl: labels) {
		if (lbl.name == str) {
			lbl.drop = false;
			return lbl.id;
		}
	}
	labels.push_back({.id = ++sequence, .name = str, .used = 0, .drop = false});
	return sequence;
}

Signal::Label* Signal::findLabel(uint id) {
	for (auto& lbl: labels) if (lbl.id == id) return &lbl;
	return nullptr;
}

Signal::Label* Signal::findLabel(const std::string& str) {
	for (auto& lbl: labels) if (lbl.name == str) return &lbl;
	return nullptr;
}

bool Signal::modLabel(uint id, const std::string& str) {
	auto labelA = findLabel(id);
	auto labelB = findLabel(str);
	if (labelA && !labelB) {
		labelA->name = str;
		return true;
	}
	return false;
}

void Signal::dropLabel(uint id) {
	auto label = findLabel(id);
	if (label) label->drop = true;
}

void Signal::useLabel(uint id) {
	auto label = findLabel(id);
	if (label) label->used = Sim::tick;
}

void Signal::gcLabels() {
//	for (auto it = labels.begin(); it != labels.end(); ) {
//		it = it->drop && it->used < Sim::tick+60*60 ? labels.erase(it): ++it;
//	}
}

void Signal::clear() {
	key.clear();
	value = 0;
}

Signal::Signal() {
	clear();
}

Signal::Signal(const Key& k, int v) {
	key = k;
	value = v;
}

Signal::Signal(const Stack& stack) {
	key = Key(Item::get(stack.iid));
	value = (int)stack.size;
}

Signal::Signal(const Amount& amount) {
	key = Key(Fluid::get(amount.fid));
	value = (int)amount.size;
}

Signal::Signal(Letter letter, int val) {
	key = Key(letter);
	value = val;
}

Signal::Signal(Meta meta, int val) {
	key = Key(meta);
	value = val;
}

Signal::Signal(const std::string& label, int val) {
	key = Key(label);
	value = val;
}

std::string Signal::Key::name() const {
	switch (type) {
		case Type::Stack: return item()->name;
		case Type::Amount: return fluid()->name;
		case Type::Virtual: {
			switch (letter()) {
				case Letter::NONE: return "_";
				case Letter::A: return "A";
				case Letter::B: return "B";
				case Letter::C: return "C";
				case Letter::D: return "D";
				case Letter::E: return "E";
				case Letter::F: return "F";
				case Letter::G: return "G";
				case Letter::H: return "H";
				case Letter::I: return "I";
				case Letter::J: return "J";
				case Letter::K: return "K";
				case Letter::L: return "L";
				case Letter::M: return "M";
				case Letter::N: return "N";
				case Letter::O: return "O";
				case Letter::P: return "P";
				case Letter::Q: return "Q";
				case Letter::R: return "R";
				case Letter::S: return "S";
				case Letter::T: return "T";
				case Letter::U: return "U";
				case Letter::V: return "V";
				case Letter::W: return "W";
				case Letter::X: return "X";
				case Letter::Y: return "Y";
				case Letter::Z: return "Z";
			}
		}
		case Type::Special: {
			switch (meta()) {
				case Meta::NONE: return "_";
				case Meta::Any: return "Any";
				case Meta::All: return "All";
				case Meta::Now: return "Now";
			}
		}
		case Type::Label: {
			return id ? Signal::findLabel(id)->name: "(none)";
		}
	}
	ensure(false);
	return "wtf";
}

std::string Signal::Key::title() const {
	switch (type) {
		case Type::Stack: return item()->title;
		case Type::Amount: return fluid()->title;
		case Type::Virtual: {
			switch (letter()) {
				case Letter::NONE: return "_";
				case Letter::A: return "A";
				case Letter::B: return "B";
				case Letter::C: return "C";
				case Letter::D: return "D";
				case Letter::E: return "E";
				case Letter::F: return "F";
				case Letter::G: return "G";
				case Letter::H: return "H";
				case Letter::I: return "I";
				case Letter::J: return "J";
				case Letter::K: return "K";
				case Letter::L: return "L";
				case Letter::M: return "M";
				case Letter::N: return "N";
				case Letter::O: return "O";
				case Letter::P: return "P";
				case Letter::Q: return "Q";
				case Letter::R: return "R";
				case Letter::S: return "S";
				case Letter::T: return "T";
				case Letter::U: return "U";
				case Letter::V: return "V";
				case Letter::W: return "W";
				case Letter::X: return "X";
				case Letter::Y: return "Y";
				case Letter::Z: return "Z";
			}
		}
		case Type::Special: {
			switch (meta()) {
				case Meta::NONE: return "_";
				case Meta::Any: return "Any";
				case Meta::All: return "All";
				case Meta::Now: return "Now";
			}
		}
		case Type::Label: {
			return id ? fmt("Custom: %s", Signal::findLabel(id)->name): "(none)";
		}
	}
	ensure(false);
	return "wtf";
}

Item* Signal::Key::item() const {
	return type == Type::Stack ? Item::get(id): nullptr;
}

Stack Signal::stack() const {
	return {key.item()->id, (uint)std::max(0, value)};
}

Fluid* Signal::Key::fluid() const {
	return type == Type::Amount ? Fluid::get(id): nullptr;
}

Amount Signal::amount() const {
	return {key.fluid()->id, (uint)std::max(0, value)};
}

Signal::Letter Signal::Key::letter() const {
	return type == Type::Virtual ? static_cast<Letter>(id): Letter::NONE;
}

Signal::Meta Signal::Key::meta() const {
	return type == Type::Special ? static_cast<Meta>(id): Meta::NONE;
}

std::string Signal::Key::label() const {
	return type == Type::Label ? name(): std::string();
}

void Signal::Key::clear() {
	type = Type::Virtual;
	id = 0;
}

Signal::Key::Key() {
	clear();
}

Signal::Key::Key(Type t, uint i) {
	type = t;
	id = i;

	if (type == Type::Label && id) useLabel(id);
}

Signal::Key::Key(Item* item) {
	type = Type::Stack;
	id = (uint)item->id;
}

Signal::Key::Key(Fluid* fluid) {
	type = Type::Amount;
	id = (uint)fluid->id;
}

Signal::Key::Key(Letter letter) {
	type = Type::Virtual;
	id = (uint)static_cast<std::underlying_type_t<Letter>>(letter);
}

Signal::Key::Key(Meta meta) {
	type = Type::Special;
	id = (uint)static_cast<std::underlying_type_t<Meta>>(meta);
}

Signal::Key::Key(const std::string& label) {
	if (!label.size() || label == "(none)") {
		type = Type::Label;
		id = 0;
	}
	else {
		type = Type::Label;
		id = addLabel(label);
	}

	useLabel(id);
}

std::string Signal::Key::serialize() {
	switch (type) {
		case Type::Stack: return fmt("stack/%s", Item::get(id)->name);
		case Type::Amount: return fmt("amount/%s", Fluid::get(id)->name);
		case Type::Virtual: return fmt("virtual/%u", id);
		case Type::Special: return fmt("special/%u", id);
		case Type::Label: return fmt("label/%s", name());
	}
	ensure(false);
	return "wtf";
}

Signal::Key Signal::Key::unserialize(std::string str) {
	if (str.find("stack/") == 0) {
		return Key(Item::byName(str.substr(6, std::string::npos)));
	}
	if (str.find("amount/") == 0) {
		return Key(Fluid::byName(str.substr(7, std::string::npos)));
	}
	if (str.find("virtual/") == 0) {
		auto letter = std::stoi(str.substr(8, std::string::npos));
		return Key(Type::Virtual, letter);
	}
	if (str.find("special/") == 0) {
		auto meta = std::stoi(str.substr(8, std::string::npos));
		return Key(Type::Special, meta);
	}
	if (str.find("label/") == 0) {
		auto label = str.substr(6, std::string::npos);
		return Key(label);
	}
	ensure(false);
	return Signal::Key();
}

bool Signal::Key::operator==(const Signal::Key& o) const {
	return type == o.type && id == o.id;
}

bool Signal::Key::operator!=(const Signal::Key& o) const {
	return type != o.type || id != o.id;
}

bool Signal::Key::valid() const {
	switch (type) {
		case Signal::Type::Stack: return id > 0;
		case Signal::Type::Amount: return id > 0;
		case Signal::Type::Virtual: return id != (uint)static_cast<std::underlying_type_t<Letter>>(Signal::Letter::NONE);
		case Signal::Type::Special: return id != (uint)static_cast<std::underlying_type_t<Meta>>(Signal::Meta::NONE);
		case Signal::Type::Label: return id > 0;
	}
	return false;
}

bool Signal::Key::operator<(const Signal::Key& o) const {
	auto t = static_cast<std::underlying_type_t<Type>>(type);
	auto ot = static_cast<std::underlying_type_t<Type>>(o.type);
	return t < ot || (t == ot && id < o.id);
}

std::string Signal::name() const {
	return key.name();
}

std::string Signal::title() const {
	return key.title();
}

bool Signal::valid() const {
	return key.valid();
}

std::string Signal::serialize() {
	return fmt("%s %d", key.serialize(), value);
}

Signal Signal::unserialize(std::string str) {
	auto split = discatenate(str, " ");
	std::vector<std::string> parts = {split.begin(), split.end()};
	ensure(parts.size() == 2);

	return Signal(
		Signal::Key::unserialize(parts[0]),
		std::stoi(parts[1])
	);
}

Signal::Condition::Condition(Signal::Key k, Signal::Operator o, int c) {
	key = k;
	op = o;
	cmp = c;
}

Signal::Condition::Condition() {
	clear();
}

void Signal::Condition::clear() {
	key.clear();
	op = Signal::Operator::Eq;
	cmp = 0;
}

bool Signal::Condition::valid() const {
	return key.valid();
}

bool Signal::Condition::evaluateNotNull(const Signal& signal) {
	if (!valid()) return false;

	auto eval = [&](int64_t val) {
		switch (op) {
			case Signal::Operator::Eq:
				return val == cmp;
			case Signal::Operator::Ne:
				return val != cmp;
			case Signal::Operator::Lt:
				return val < cmp;
			case Signal::Operator::Lte:
				return val <= cmp;
			case Signal::Operator::Gt:
				return val > cmp;
			case Signal::Operator::Gte:
				return val >= cmp;
			case Signal::Operator::NMod:
				return cmp ? ((val % cmp) == 0): false;
		}
		ensure(false);
		return false;
	};

	if (key.type == Signal::Type::Special) {
		switch (key.meta()) {
			case Signal::Meta::NONE: return false;
			case Signal::Meta::All: {
				if (!eval(signal.value)) return false;
				return true;
			}
			case Signal::Meta::Any: {
				if (eval(signal.value)) return true;
				return false;
			}
			case Signal::Meta::Now: {
				if (eval((int64_t)(Sim::tick/60U))) return true;
				return false;
			}
		}
	}

	if (key.type == Signal::Type::Label) useLabel(key.id);

	return signal.key == key ? eval(signal.value): false;
}

bool Signal::Condition::evaluate(const minimap<Signal,&Signal::key>& signals) {
	if (!valid()) return false;

	// Now applies even where there are no signals
	if (key.type == Signal::Type::Special && key.meta() == Signal::Meta::Now) {
		return evaluateNotNull(Signal(Signal::Meta::Now, 0));
	}

	// All must be true, else false
	if (key.type == Signal::Type::Special && key.meta() == Signal::Meta::All) {
		for (auto& signal: signals) if (!evaluateNotNull(signal)) return false;
		return true;
	}

	// Any can be true, else false
	if (key.type == Signal::Type::Special && key.meta() == Signal::Meta::Any) {
		for (auto& signal: signals) if (evaluateNotNull(signal)) return true;
		return false;
	}

	// special case, looking for a null signal
	if (!signals.has(key) && (key.type == Signal::Type::Stack || key.type == Signal::Type::Amount)) {
		if (evaluateNotNull(Signal(key, 0))) return true;
	}

	for (auto& signal: signals) if (evaluateNotNull(signal)) return true;
	return false;
}

const char* Signal::opstr(Operator op) {
	switch (op) {
		case Signal::Operator::Eq:  return "=";
		case Signal::Operator::Ne:  return "!=";
		case Signal::Operator::Lt:  return "<";
		case Signal::Operator::Lte: return "<=";
		case Signal::Operator::Gt:  return ">";
		case Signal::Operator::Gte: return ">=";
		case Signal::Operator::NMod: return "!%";
	}
	ensure(false);
	return "";
}

std::string Signal::Condition::readable() {
	return fmt("%s %s %d",
		key.title(), Signal::opstr(op), cmp
	);
}

std::string Signal::Condition::serialize() {
	return fmt("%s %s %d",
		key.serialize(), Signal::opstr(op), cmp
	);
}

Signal::Condition Signal::Condition::unserialize(std::string str) {
	auto split = discatenate(str, " ");
	std::vector<std::string> parts = {split.begin(), split.end()};
	ensure(parts.size() == 3);

	auto op = [&](std::string s) {
		if (s == "=")  return Signal::Operator::Eq;
		if (s == "!=") return Signal::Operator::Ne;
		if (s == "<")  return Signal::Operator::Lt;
		if (s == "<=") return Signal::Operator::Lte;
		if (s == ">")  return Signal::Operator::Gt;
		if (s == ">=") return Signal::Operator::Gte;
		if (s == "!%") return Signal::Operator::NMod;
		ensure(false);
		return Signal::Operator::Eq;
	};

	return Condition(
		Signal::Key::unserialize(parts[0]),
		op(parts[1]),
		std::stoi(parts[2])
	);
}
