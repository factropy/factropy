#pragma once

// Wifi network packets

struct Signal;

#include "item.h"
#include "fluid.h"
#include "common.h"
#include "minimap.h"
#include <mutex>

struct Signal {
	enum class Type {
		Stack,
		Amount,
		Virtual,
		Special,
		Label,
	};

	struct Label {
		uint id = 0;
		std::string name;
		uint64_t used = 0;
		bool drop = false;
	};

	static inline uint sequence = 0;
	static inline std::vector<Label> labels;

	static uint addLabel(const std::string& str);
	static void dropLabel(uint id);
	static void useLabel(uint id);
	static Label* findLabel(uint id);
	static Label* findLabel(const std::string& str);
	static bool modLabel(uint id, const std::string& str);
	static void gcLabels();

	enum class Letter {
		NONE = 0,
		A,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
	};

	enum class Meta {
		NONE = 0,
		Any,
		All,
		Now,
	};

	struct Key {
		Type type = Type::Virtual;
		uint id = 0;

		operator bool() const {
			return !(type == Type::Virtual && id == 0);
		}

		bool operator==(const Signal::Key& o) const;
		bool operator!=(const Signal::Key& o) const;
		bool operator<(const Signal::Key& o) const;

		Key();
		Key(Type t, uint i);
		Key(Item* item);
		Key(Fluid* fluid);
		Key(Letter letter);
		Key(Meta meta);
		Key(const std::string& label);

		Item* item() const;
		Fluid* fluid() const;
		Letter letter() const;
		Meta meta() const;
		std::string label() const;

		std::string serialize();
		static Key unserialize(std::string str);
		std::string name() const;
		std::string title() const;
		bool valid() const;
		void clear();
	};

	Key key;
	int value = 0;
	static minimap<Signal,&Signal::key> NoSignals;

	Signal();
	Signal(const Key& key, int value);
	Signal(const Stack& stack);
	Signal(const Amount& amount);
	Signal(const Letter letter, int value);
	Signal(const Meta meta, int value);
	Signal(const std::string& label, int value);
	std::string name() const;
	std::string title() const;
	bool valid() const;
	void clear();

	operator const std::string() const {
		return fmt("{%s,%d}", title(), value);
	};

	std::string serialize();
	static Signal unserialize(std::string str);

	Stack stack() const;
	Amount amount() const;

	enum class Operator {
		Eq = 1,
		Ne,
		Lt,
		Lte,
		Gt,
		Gte,
		NMod,
	};

	static const char* opstr(Operator op);

	struct Condition {
		Key key;
		Operator op = Operator::Eq;
		int cmp = 0;

		Condition();
		Condition(Key k, Operator o, int c);
		bool valid() const;
		void clear();
		bool evaluateNotNull(const Signal& signal);
		bool evaluate(const minimap<Signal,&Signal::key>& signals);
		std::string readable();
		std::string serialize();
		static Condition unserialize(std::string str);
	};
};
