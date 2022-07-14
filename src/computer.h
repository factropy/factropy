#pragma once

// Computer components are small virtual machines emulating a MISC instruction set
// programmable by the player using a Forth dialect

struct Computer;

#include "slabmap.h"
#include "minivec.h"
#include "minimap.h"
#include "signal.h"

struct Computer {
	uint id;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Computer,&Computer::id> all;
	static Computer& create(uint id);
	static Computer& get(uint id);

	void destroy();
	void update();

	minivec<int> ds; // data stack
	minivec<int> rs; // return stack
	minivec<int> ram; // memory
	minivec<int> rom; // memory

	minimap<Signal,&Signal::key> env; // environment settings
	Signal add;

	enum Opcode {
		Nop = 0,
		Rom,
		Ram,
		Jmp,
		Jz,
		Call,
		Ret,
		Push,
		Pop,
		Dup,
		Drop,
		Over,
		Swap,
		Fetch,
		Store,
		Lit,
		Add,
		Sub,
		Mul,
		Div,
		Mod,
		And,
		Or,
		Xor,
		Inv,
		Eq,
		Ne,
		Lt,
		Lte,
		Gt,
		Gte,
		Max,
		Min,
		Print,
		DumpTop,
		DumpStack,
		Send,
		Recv,
		Sniff,
		Now,
	};

	struct Instruction {
		Opcode opcode;
		int value = 0;
		Instruction(Opcode o, int v = 0) {
			opcode = o;
			value = v;
		}
	};

	minivec<Instruction> code;
	int ip; // inctruction pointer

	enum Error {
		Ok = 0,
		What,
		StackUnderflow,
		StackOverflow,
		OutOfBounds,
		Syntax,
	};

	Error err; // crash error code
	std::string log;
	std::string source;
	bool debug;

	void reboot();
	void compile();
	void execute();
};
