#include "common.h"
#include "computer.h"
#include "entity.h"
#include "catenate.h"
#include <cstdio>

// Computer components are small virtual machines emulating a MISC instruction set
// programmable by the player using a Forth dialect

void Computer::reset() {
	all.clear();
}

void Computer::tick() {
	for (auto& computer: all) computer.update();
}

Computer& Computer::create(uint id) {
	ensure(!all.has(id));
	Computer& computer = all[id];
	computer.id = id;
	auto& en = Entity::get(id);
	computer.ram.resize(en.spec->computerRAMSize);
	computer.rom.resize(en.spec->computerROMSize);
	computer.ip = 0;
	computer.err = Ok;
	computer.debug = false;
	return computer;
}

Computer& Computer::get(uint id) {
	return all.refer(id);
}

void Computer::destroy() {
	all.erase(id);
}

void Computer::update() {
	Entity& en = Entity::get(id);
	if (en.isGhost()) return;
	if (!en.isEnabled()) return;
	execute();
}

void Computer::reboot() {
	err = Ok;
	log = "";
	ip = 0;
	ds.clear();
	rs.clear();
	code.clear();
	for (uint i = 0; i < ram.size(); i++) ram[i] = 0;
	for (uint i = 0; i < rom.size(); i++) rom[i] = 0;
	compile();
}

void Computer::compile() {
	if (err) return;
	auto& en = Entity::get(id);

	minivec<int> cs;
	int rp = 0; // ROM pointer

	const int ramAddr = 0;
	const int romAddr = ramAddr + (int)ram.size();

	std::map<std::string,Opcode> forth = {
		{ "rom", Rom },
		{ "ram", Ram },
		{ "push", Push },
		{ "pop", Pop },
		{ "dup", Dup },
		{ "drop", Drop },
		{ "over", Over },
		{ "swap", Swap },
		{ "@", Fetch },
		{ "!", Store },
		{ "+", Add },
		{ "-", Sub },
		{ "*", Mul },
		{ "/", Div },
		{ "%", Mod },
		{ "&", And },
		{ "|", Or },
		{ "^", Xor },
		{ "~", Inv },
		{ "=", Eq },
		{ "!=", Ne },
		{ "<", Lt },
		{ "<=", Lt },
		{ ">", Gt },
		{ ">=", Gt },
		{ "max", Max },
		{ "min", Min },
		{ "print", Print },
		{ ".", DumpTop },
		{ ".s", DumpStack },
		{ "send", Send },
		{ "recv", Recv },
		{ "sniff", Sniff },
		{ "now", Now },
	};

	std::map<std::string,int> words = {
	};

	std::map<std::string,int> constants = {
	};

	constants["A"] = Signal::Key(Signal::Letter::A).id + 2000u;
	constants["B"] = Signal::Key(Signal::Letter::B).id + 2000u;
	constants["C"] = Signal::Key(Signal::Letter::C).id + 2000u;
	constants["D"] = Signal::Key(Signal::Letter::D).id + 2000u;
	constants["E"] = Signal::Key(Signal::Letter::E).id + 2000u;
	constants["F"] = Signal::Key(Signal::Letter::F).id + 2000u;
	constants["G"] = Signal::Key(Signal::Letter::G).id + 2000u;
	constants["H"] = Signal::Key(Signal::Letter::H).id + 2000u;
	constants["I"] = Signal::Key(Signal::Letter::I).id + 2000u;
	constants["J"] = Signal::Key(Signal::Letter::J).id + 2000u;
	constants["K"] = Signal::Key(Signal::Letter::K).id + 2000u;
	constants["L"] = Signal::Key(Signal::Letter::L).id + 2000u;
	constants["M"] = Signal::Key(Signal::Letter::M).id + 2000u;
	constants["N"] = Signal::Key(Signal::Letter::N).id + 2000u;
	constants["O"] = Signal::Key(Signal::Letter::O).id + 2000u;
	constants["P"] = Signal::Key(Signal::Letter::P).id + 2000u;
	constants["Q"] = Signal::Key(Signal::Letter::Q).id + 2000u;
	constants["R"] = Signal::Key(Signal::Letter::R).id + 2000u;
	constants["S"] = Signal::Key(Signal::Letter::S).id + 2000u;
	constants["T"] = Signal::Key(Signal::Letter::T).id + 2000u;
	constants["U"] = Signal::Key(Signal::Letter::U).id + 2000u;
	constants["V"] = Signal::Key(Signal::Letter::V).id + 2000u;
	constants["W"] = Signal::Key(Signal::Letter::W).id + 2000u;
	constants["X"] = Signal::Key(Signal::Letter::X).id + 2000u;
	constants["Y"] = Signal::Key(Signal::Letter::Y).id + 2000u;
	constants["Z"] = Signal::Key(Signal::Letter::Z).id + 2000u;

	for (auto& [name,item]: Item::names) {
		constants[name] = item->id;
	}

	for (auto& [name,fluid]: Fluid::names) {
		constants[name] = 1000u + fluid->id;
	}

	if (en.spec->networker) {
		auto& nw = Networker::get(id);
		for (uint i = 0; i < nw.interfaces.size(); i++) {
			constants[fmt("nic%d", i)] = i;
		}
	}

	std::map<std::string,std::function<void(void)>> macro;

	auto mark = [&]() {
		cs.push((int)code.size());
	};

	auto resolve = [&](int a) {
		int m = cs.pop();
		code[m].value = a;
	};

	auto swap = [&]() {
		int b = cs.pop();
		int a = cs.pop();
		cs.push(b);
		cs.push(a);
	};

	macro["if"] = [&]() {
		mark();
		code.push(Instruction(Jz));
	};

	macro["else"] = [&]() {
		mark();
		code.push(Instruction(Jmp));
		swap();
		resolve(code.size());
	};

	macro["then"] = [&]() {
		resolve(code.size());
	};

	macro["begin"] = [&]() {
		mark();
	};

	macro["while"] = [&]() {
		mark();
		code.push(Instruction(Jz));
	};

	macro["repeat"] = [&]() {
		swap();
		mark();
		swap();
		code.push(Instruction(Jmp));
		resolve(cs.pop()); // begin
		resolve(code.size()); // while
	};

	const char* src = source.c_str();

	auto white = [](int c) {
		return c == ' ' || c == '\t' || c == '\r' || c == '\n';
	};

	auto quote = [](int c) {
		return c == '"';
	};

	auto skip = [&](std::function<int(int)> cb) {
		while (*src && cb(*src)) ++src;
	};

	auto scan = [&](std::function<int(int)> cb) {
		while (*src && !cb(*src)) ++src;
	};

	auto romStore = [&](int n) {
		if (err) return;
		if (rp >= (int)rom.size()) {
			err = OutOfBounds;
			log = "ROM overflow";
		} else {
			rom[rp++] = n;
		}
	};

	macro[":"] = [&]() {
		skip(white);
		if (!*src) {
			err = Syntax;
			log = fmt("expected name at");
			return;
		}

		mark();
		code.push(Instruction(Jmp));

		const char* a = src;
		scan(white);
		const char* b = src;

		auto name = std::string(a, b-a);
		words[name] = (int)code.size();
	};

	macro[";"] = [&]() {
		code.push(Instruction(Ret));
		resolve(code.size());
	};

	macro["("] = [&]() {
		scan([&](int c) { return c == ')'; });
		if (*src) ++src;
	};

	// read environment into ram
	for (auto& signal: env) {
		switch (signal.key.type) {
			case Signal::Type::Stack: {
				romStore(signal.key.id);
				romStore(signal.value);
				break;
			}
			case Signal::Type::Amount: {
				romStore(signal.key.id + 1000);
				romStore(signal.value);
				break;
			}
			case Signal::Type::Virtual: {
				romStore(signal.key.id + 2000);
				romStore(signal.value);
				break;
			}
			case Signal::Type::Special: {
				romStore(signal.key.id + 3000);
				romStore(signal.value);
				break;
			}
			case Signal::Type::Label: {
				romStore(signal.key.id + 4000);
				romStore(signal.value);
				break;
			}
		}
	}

	while (!err) {
		skip(white);
		if (!*src) break;

		if (quote(*src)) {
			[&](){
				int address = rp+romAddr;

				++src;

				int counter = rp;
				romStore(0);
				if (err) return;

				while (*src && !quote(*src)) {
					romStore(*src++);
					if (err) return;
				}

				if (quote(*src)) ++src;
				rom[counter] = rp-counter-1;

				++src;

				code.push(Instruction(Lit, address));
			}();
			continue;
		}

		const char* a = src;
		scan(white);
		const char* b = src;

		auto token = std::string(a, b-a);

		if (macro.count(token)) {
			macro[token]();
			continue;
		}

		if (forth.count(token)) {
			code.push(Instruction(forth[token]));
			continue;
		}

		if (constants.count(token)) {
			code.push(Instruction(Lit, constants[token]));
			continue;
		}

		if (words.count(token)) {
			code.push(Instruction(Call, words[token]));
			continue;
		}

		int n = 0;
		if (1 == std::sscanf(token.c_str(), "%d", &n)) {
			code.push(Instruction(Lit, n));
			continue;
		}

		err = What;
		log = fmt("what? %s", token);
		break;
	}

	code.push(Instruction(Jmp, 0));
}

void Computer::execute() {
	auto& en = Entity::get(id);
	int cycles = (int)en.spec->computerCyclesPerTick;

	const int ramAddr = 0;
	const int romAddr = ramAddr + (int)ram.size();
	const int limAddr = romAddr + (int)rom.size();

	auto dsPop = [&]() {
		if (err) return 0;
		if (!ds.size()) {
			err = StackUnderflow;
			log = "data stack underflow";
		}
		if (!err) return ds.pop();
		return 0;
	};

	auto dsPush = [&](int n) {
		if (err) return;
		if (ds.size() == en.spec->computerDataStackSize) {
			err = StackOverflow;
			log = "data stack overflow";
		}
		if (!err) ds.push(n);
	};

	auto rsPop = [&]() {
		if (err) return 0;
		if (!rs.size()) {
			err = StackUnderflow;
			log = "return stack underflow";
		}
		if (!err) return rs.pop();
		return 0;
	};

	auto rsPush = [&](int n) {
		if (err) return;
		if (rs.size() == en.spec->computerReturnStackSize) {
			err = StackOverflow;
			log = "return stack overflow";
		}
		if (!err) rs.push(n);
	};

	auto memWrite = [&](int a) {
		if (err) return;
		if (a >= ramAddr && a < romAddr) {
			ram[a] = dsPop();
			return;
		}
		err = OutOfBounds;
		log = fmt("memory write out of bounds: %d", a);
	};

	auto memRead = [&](int a) {
		if (err) return;
		if (a >= ramAddr && a < romAddr) {
			dsPush(ram[a]);
			return;
		}
		if (a >= romAddr && a < limAddr) {
			dsPush(rom[a-romAddr]);
			return;
		}
		err = OutOfBounds;
		log = fmt("memory read out of bounds: %d", a);
	};

	std::vector<std::vector<int>> sniffed;

	auto sniff = [&]() {
		if (sniffed.size()) return;
		if (!en.spec->networker) return;
		auto& nw = Networker::get(id);

		for (auto& interface: nw.interfaces) {
			sniffed.resize(sniffed.size()+1);
			for (auto& signal: interface.signals) {
				switch (signal.key.type) {
					case Signal::Type::Stack: {
						sniffed.back().push_back(signal.key.id);
						break;
					}
					case Signal::Type::Amount: {
						sniffed.back().push_back(signal.key.id + 1000);
						break;
					}
					case Signal::Type::Virtual: {
						sniffed.back().push_back(signal.key.id + 2000);
						break;
					}
					case Signal::Type::Special: {
						break;
					}
					case Signal::Type::Label: {
						break;
					}
				}
			}
		}
	};

	bool once = false;
	for (int cycle = 0; cycle < cycles && !err; ) {

		// looped back to start
		if (once && ip == 0) break;

		once = true;

		if (ip < 0 || ip >= (int)code.size()) {
			err = OutOfBounds;
			log = fmt("invalid instruction pointer %d", ip);
			break;
		}

		Instruction in = code[ip++];

		switch (in.opcode) {
			case Nop: {
				break;
			}

			case Rom: {
				cycle++;
				dsPush(romAddr);
				break;
			}

			case Ram: {
				cycle++;
				dsPush(ramAddr);
				break;
			}

			case Jz: {
				cycle++;
				if (dsPop() == 0)
					ip = in.value;
				break;
			}

			case Jmp: {
				cycle++;
				ip = in.value;
				break;
			}

			case Call: {
				rsPush(ip);
				ip = in.value;
				break;
			}

			case Ret: {
				ip = rsPop();
				break;
			}

			case Push: {
				cycle++;
				rsPush(dsPop());
				break;
			}

			case Pop: {
				cycle++;
				dsPush(rsPop());
				break;
			}

			case Dup: {
				cycle++;
				int n = dsPop();
				dsPush(n);
				dsPush(n);
				break;
			}

			case Drop: {
				cycle++;
				dsPop();
				break;
			}

			case Over: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a);
				dsPush(b);
				dsPush(a);
				break;
			}

			case Swap: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(b);
				dsPush(a);
				break;
			}

			case Fetch: {
				cycle++;
				memRead(dsPop());
				break;
			}

			case Store: {
				cycle++;
				memWrite(dsPop());
				break;
			}

			case Lit: {
				cycle++;
				dsPush(in.value);
				break;
			}

			case Add: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a + b);
				break;
			}

			case Sub: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a - b);
				break;
			}

			case Mul: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a * b);
				break;
			}

			case Div: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a / b);
				break;
			}

			case Mod: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a % b);
				break;
			}

			case And: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a & b);
				break;
			}

			case Or: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a | b);
				break;
			}

			case Xor: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a ^ b);
				break;
			}

			case Inv: {
				cycle++;
				dsPush(~dsPop());
				break;
			}

			case Eq: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a == b);
				break;
			}

			case Ne: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a != b);
				break;
			}

			case Lt: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a < b);
				break;
			}

			case Lte: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a <= b);
				break;
			}

			case Gt: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a > b);
				break;
			}

			case Gte: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(a >= b);
				break;
			}

			case Max: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(std::max(a,b));
				break;
			}

			case Min: {
				cycle++;
				int b = dsPop();
				int a = dsPop();
				dsPush(std::min(a,b));
				break;
			}

			case Print: {
				cycle++;
				int address = dsPop();
				memRead(address++);
				if (err) break;

				int length = dsPop();
				if (length < 0 || address+length > (int)(ram.size() + rom.size())) {
					err = OutOfBounds;
					log = fmt("invalid string length %d at %d", length, address);
					break;
				}

				char str[length+1];
				for (int i = 0; !err && i < length; i++) {
					memRead(address++);
					str[i] = (char)dsPop();
				}
				str[length] = 0;

				log = std::string(str);
				break;
			}

			case DumpTop: {
				int n = dsPop();
				dsPush(n);
				log = fmt("%d", n);
				break;
			}

			case DumpStack: {
				log = "stack:";
				for (auto n: ds) log += fmt(" %d", n);
				break;
			}

			case Send: {
				cycle++;
				int key = dsPop();
				int nic = dsPop();
				int val = dsPop();

				if (!en.spec->networker) break;
				auto& nw = Networker::get(id);

				if (nic < 0 || nic >= (int)nw.interfaces.size()) {
					err = OutOfBounds;
					log = fmt("invalid network interface %d", nic);
					break;
				}

				if (key > 0 && key < 1000 && Item::all.has((uint)key)) {
					if (!debug) nw.interfaces[nic].write((Stack){(uint)key,(uint)val});
					break;
				}

				if (key >= 1000 && key < 2000 && Fluid::all.has((uint)(key-1000))) {
					if (!debug) nw.interfaces[nic].write((Amount){(uint)(key-1000),(uint)val});
					break;
				}

				if (key >= 2001 && key <= 2026) {
					auto virt = Signal::Key(Signal::Type::Virtual, key-2000);
					if (!debug) nw.interfaces[nic].write(Signal(virt, val));
					break;
				}

				if (key == 3001) {
					if (!debug) nw.interfaces[nic].write(Signal(Signal::Meta::Any, val));
					break;
				}

				if (key == 3002) {
					if (!debug) nw.interfaces[nic].write(Signal(Signal::Meta::All, val));
					break;
				}

				if (key >= 4000) {
					auto label = Signal::findLabel(key-4000);
					if (!debug) if (label) nw.interfaces[nic].write(Signal(label->name, val));
					break;
				}

				err = OutOfBounds;
				log = fmt("unknown signal key %d", key);
				break;
			}

			case Recv: {
				cycle++;
				int key = dsPop();
				int nic = dsPop();

				if (!en.spec->networker) break;
				auto& nw = Networker::get(id);

				if (nic < 0 || nic >= (int)nw.interfaces.size()) {
					err = OutOfBounds;
					log = fmt("invalid network interface %d", nic);
					break;
				}

				if (!nw.interfaces[nic].network) {
					dsPush(0);
					break;
				}

				if (key > 0 && key < 1000 && Item::all.has((uint)key)) {
					dsPush(nw.interfaces[nic].network->read(Signal::Key(Item::get(key))));
					break;
				}

				if (key >= 1000 && key < 2000 && Fluid::all.has((uint)(key-1000))) {
					dsPush(nw.interfaces[nic].network->read(Signal::Key(Fluid::get(key-1000))));
					break;
				}

				if (key >= 2001 && key <= 2026) {
					auto virt = Signal::Key(Signal::Type::Virtual, key-2000);
					dsPush(nw.interfaces[nic].network->read(virt));
					break;
				}

				if (key == 3001) {
					dsPush(nw.interfaces[nic].network->read(Signal::Key(Signal::Meta::Any)));
					break;
				}

				if (key == 3002) {
					dsPush(nw.interfaces[nic].network->read(Signal::Key(Signal::Meta::All)));
					break;
				}

				if (key >= 4000) {
					auto label = Signal::findLabel(key-4000);
					if (label) {
						dsPush(nw.interfaces[nic].network->read(Signal::Key(label->name)));
						break;
					}
				}

				err = OutOfBounds;
				log = fmt("unknown signal key %d", key);
				break;
			}

			case Sniff: {
				cycle++;
				int nic = dsPop();

				sniff();

				if (nic < 0 || nic >= (int)sniffed.size()) {
					err = OutOfBounds;
					log = fmt("invalid network interface %d", nic);
					break;
				}

				if (!sniffed[nic].size()) {
					dsPush(0);
					break;
				}

				dsPush(sniffed[nic].back());
				sniffed[nic].pop_back();
				break;
			}

			case Now: {
				cycle++;
				dsPush(Sim::tick/60U);
				break;
			}
		}
	}
}
