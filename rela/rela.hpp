// Rela, MIT License
//
// Copyright (c) 2021 Sean Pringle <sean.pringle@gmail.com> github:seanpringle
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include <array>
#include <string>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <new>
#include <algorithm>
#include <cassert>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

#ifndef NDEBUG
#include <signal.h>
#endif

#ifdef PCRE
#include <pcre.h>
#endif

class Rela {

	enum opcode_t {
		OP_STOP=0, OP_JMP, OP_FOR, OP_ENTER, OP_LIT, OP_MARK, OP_LIMIT, OP_CLEAN, OP_RETURN,
		OP_LGET, OPP_LCALL, OPP_FNAME, OPP_CFUNC, OPP_ASSIGNL, OPP_ASSIGNP,
		OPP_MUL_LIT, OPP_ADD_LIT, OPP_GNAME, OPP_COPIES, OPP_UPDATE, OPP_MARK2, OPP_LIMIT2,
		OP_PRINT, OP_COROUTINE, OP_RESUME, OP_YIELD, OP_CALL, OP_GLOBAL, OP_MAP, OP_VECTOR, OP_VPUSH,
		OP_META_SET, OP_META_GET, OP_UNMAP, OP_LOOP, OP_UNLOOP, OP_BREAK, OP_CONTINUE, OP_JFALSE,
		OP_JTRUE, OP_NIL, OP_COPY, OP_SHUNT, OP_SHIFT, OP_TRUE, OP_FALSE, OP_ASSIGN, OP_AND, OP_OR,
		OP_FIND, OP_SET, OP_GET, OP_COUNT, OP_DROP, OP_ADD, OP_NEG, OP_SUB, OP_MUL, OP_DIV,
		OP_MOD, OP_NOT, OP_EQ, OP_NE, OP_LT, OP_GT, OP_LTE, OP_GTE, OP_CONCAT, OP_MATCH, OP_SORT,
		OP_ASSERT, OP_SIN, OP_COS, OP_TAN, OP_ASIN, OP_ACOS, OP_ATAN, OP_SINH, OP_COSH, OP_TANH,
		OP_CEIL, OP_FLOOR, OP_SQRT, OP_ABS, OP_ATAN2, OP_LOG, OP_LOG10, OP_POW, OP_MIN, OP_MAX, OP_TYPE,
		OP_UNPACK, OP_GC,
	};

	enum type_t {
		NIL = 0, INTEGER, FLOAT, STRING, BOOLEAN, VECTOR, MAP, SUBROUTINE, COROUTINE, OPERATION,
		EXECUTE, USERDATA, TYPES
	};

	const char* type_names[TYPES] = {
		[NIL] = "nil",
		[INTEGER] = "integer",
		[FLOAT] = "number",
		[STRING] = "string",
		[BOOLEAN] = "boolean",
		[VECTOR] = "vector",
		[MAP] = "map",
		[SUBROUTINE] = "subroutine",
		[COROUTINE] = "coroutine",
		[OPERATION] = "operation",
		[EXECUTE] = "callback",
		[USERDATA] = "userdata",
	};

	enum {
		NODE_MULTI=1, NODE_NAME, NODE_LITERAL, NODE_OPCODE, NODE_IF, NODE_WHILE, NODE_FUNCTION, NODE_RETURN,
		NODE_VEC, NODE_MAP, NODE_FOR, NODE_CALL_CHAIN, NODE_OPERATOR
	};

	static const int STRTMP = 100;
	static const int STRBUF = 1000;

	char emsg[STRTMP];

	#ifdef NDEBUG
	void explode() { throw std::runtime_error(emsg); }
	#else
	void explode() { raise(SIGUSR1); }
	#endif

	struct vec_t;
	struct map_t;
	struct cor_t;
	struct data_t;

	struct item_t {
		enum type_t type = NIL;
		union {
			bool flag;
			int sub;
			int64_t inum;
			double fnum;
			const char* str;
			vec_t* vec;
			map_t* map;
			cor_t* cor;
			data_t* data;
			enum opcode_t opcode;
			int function;
		};
	};

	struct vec_t {
		item_t meta;
		std::vector<item_t> items;
	};

	struct map_t {
		item_t meta;
		vec_t keys;
		vec_t vals;
	};

	struct data_t {
		item_t meta;
		void* ptr = nullptr;
	}; // userdata

	// powers of 2
	static const uint8_t STACK = 32u;
	static const uint8_t LOCALS = 32u;
	static const uint8_t PATH = 8u;

	template <class T, size_t L = STACK>
	struct tstack {
		T cells[L];
		int depth = 0;

		void push(const T& t) {
			assert(depth < (int)L);
			cells[depth++] = t;
		}

		T pop() {
			assert(depth > 0);
			return cells[--depth];
		}

		T& top() {
			assert(depth > 0);
			return cells[depth-1];
		}

		T& operator[](int i) {
			assert(i >= 0 && i < depth);
			return cells[i];
		}

		size_t size() {
			return depth;
		}
	};

	struct frame_t {
		int loops = 0;
		int marks = 0;
		int ip = 0;
		int scope = 0;
		tstack<item_t,LOCALS> locals;
		item_t map;
	};

	struct cor_t {
		int ip = 0;
		int state = 0;
		tstack<item_t> stack;
		tstack<item_t> other;
		tstack<frame_t> frames;
		tstack<int> marks;
		tstack<int> loops;
		item_t map;
	}; // coroutine

	struct code_t {
		enum opcode_t op = OP_STOP;
		item_t item;
	}; // compiled "bytecode"

	struct node_t {
		int type = 0;
		enum opcode_t opcode = OP_STOP;
		int call = 0;
		item_t item;
		node_t *args = nullptr;
		node_t *chain = nullptr;
		std::vector<node_t*> keys;
		std::vector<node_t*> vals;
		std::vector<item_t> fkeys;
		int results = 0;
		struct {
			int id = 0;
			tstack<int> ids;
		} fpath;
		bool index = false;
		bool field = false;
		bool method = false;
		bool control = false;
		bool single = false;
	}; // AST

	struct keyword_t {
		const char *name = nullptr;
		enum opcode_t opcode = OP_STOP;
	};

	keyword_t keywords[4] = {
		{ .name = "global", .opcode = OP_GLOBAL },
		{ .name = "true",   .opcode = OP_TRUE   },
		{ .name = "false",  .opcode = OP_FALSE  },
		{ .name = "nil",    .opcode = OP_NIL    },
	};

	struct operator_t {
		const char *name = nullptr;
		int precedence = 0;
		enum opcode_t opcode = OP_STOP;
		int argc = 0;
		bool single = false; // single result
	};

	// order is important; eg longer >= should be matched before >
	operator_t operators[16] = {
		{ .name = "||", .precedence = 0, .opcode = OP_OR,    .argc = 2, .single = true },
		{ .name = "or", .precedence = 0, .opcode = OP_OR,    .argc = 2, .single = true },
		{ .name = "&&", .precedence = 1, .opcode = OP_AND,   .argc = 2, .single = true },
		{ .name = "==", .precedence = 2, .opcode = OP_EQ,    .argc = 2, .single = true },
		{ .name = "!=", .precedence = 2, .opcode = OP_NE,    .argc = 2, .single = true },
		{ .name = ">=", .precedence = 2, .opcode = OP_GTE,   .argc = 2, .single = true },
		{ .name = ">",  .precedence = 2, .opcode = OP_GT,    .argc = 2, .single = true },
		{ .name = "<=", .precedence = 2, .opcode = OP_LTE,   .argc = 2, .single = true },
		{ .name = "<",  .precedence = 2, .opcode = OP_LT,    .argc = 2, .single = true },
		{ .name = "~",  .precedence = 2, .opcode = OP_MATCH, .argc = 2, .single = true },
		{ .name = "+",  .precedence = 3, .opcode = OP_ADD,   .argc = 2, .single = true },
		{ .name = "-",  .precedence = 3, .opcode = OP_SUB,   .argc = 2, .single = true },
		{ .name = "*",  .precedence = 4, .opcode = OP_MUL,   .argc = 2, .single = true },
		{ .name = "/",  .precedence = 4, .opcode = OP_DIV,   .argc = 2, .single = true },
		{ .name = "%",  .precedence = 4, .opcode = OP_MOD,   .argc = 2, .single = true },
		{ .name = "...",.precedence = 4, .opcode = OP_UNPACK,.argc = 1, .single = false },
	};

	typedef keyword_t modifier_t;

	modifier_t modifiers[3] = {
		{ .name = "#", .opcode = OP_COUNT },
		{ .name = "-", .opcode = OP_NEG   },
		{ .name = "!", .opcode = OP_NOT   },
	};

	template <class T>
	struct pool_t {
		struct cell {
			T data;
			bool used = false;
			bool mark = false;
		};

		struct pair {
			T* key;
			int val;
		};

		std::deque<cell> cells;
		std::vector<pair> lookup;
		std::vector<int> recycle;

		decltype(lookup.begin()) lower_bound(T* ptr) {
			return std::lower_bound(lookup.begin(), lookup.end(), ptr, [](const pair& a, const T* b) { return a.key < b; });
		}

		int index(T* ptr) {
			auto it = lower_bound(ptr);
			return it != lookup.end() && it->key == ptr ? it->val: -1;
		}

		T* alloc() {
			int i = -1;
			if (recycle.size()) {
				i = recycle.back();
				recycle.pop_back();
			}
			else {
				i = cells.size();
				cells.emplace_back();
			}

			auto it = lower_bound(&cells[i].data);
			assert(it == lookup.end() || it->key != &cells[i].data);
			lookup.insert(it, {.key = &cells[i].data, .val = i});

			cells[i].used = true;
			return &cells[i].data;
		}

		void clear() {
			cells.clear();
			lookup.clear();
			recycle.clear();
		}

		void mark(int i) {
			cells[i].mark = true;
		}

		void purge() {
			recycle.clear();
			lookup.clear();
			for (int i = 0, l = cells.size(); i < l; i++) {
				auto& cell = cells[i];
				if (!cell.mark && cell.used) {
					cell.data = T(); // empty one
					cell.used = false;
				}
				cell.mark = false;
				if (!cell.used) {
					recycle.push_back(i);
				}
				else {
					auto it = lower_bound(&cell.data);
					lookup.insert(it, {.key = &cell.data, .val = i});
				}
			}
		}
	};

	struct string_pool {
		struct cell {
			char* data = nullptr;
			bool mark = false;
		};

		std::vector<cell> cells;

		~string_pool() {
			clear();
		}

		void clear() {
			for (auto& cell: cells) free(cell.data);
			cells.clear();
		}

		decltype(cells.begin()) expect(const char* key) {
			return std::lower_bound(cells.begin(), cells.end(), key, [](const cell& a, const char* b) {
				return strcmp(a.data, b) < 0;
			});
		}

		int index(const char* key) {
			auto it = expect(key);
			return it == cells.end() || strcmp(it->data, key) ? -1: std::distance(cells.begin(), it);
		}

		const char* insert(const char* key) {
			auto it = expect(key);
			if (it != cells.end() && !strcmp(it->data, key)) return it->data;
			it = cells.emplace(it);
			it->data = strdup(key);
			return it->data;
		}

		void mark(int i) {
			assert(i >= 0 && i < (int)cells.size());
			cells[i].mark = true;
		}

		void purge() {
			for (auto it = cells.begin(); it != cells.end(); ) {
				if (it->mark) {
					it->mark = false;
					++it;
					continue;
				}
				free(it->data);
				it = cells.erase(it);
			}
		}

		void merge(string_pool& other) {
			for (auto& cell: other.cells) {
				auto it = expect(cell.data);
				assert(it == cells.end() || strcmp(it->data, cell.data));
				it = cells.emplace(it);
				it->data = cell.data;
			}
			other.cells.clear();
		}
	};

	// routines[0] == main coroutine, always set at run-time
	// routines[1...n] resume/yield chain
	vec_t routines;
	cor_t* routine = nullptr;
	map_t* scope_core = nullptr;
	map_t* scope_global = nullptr;

	std::deque<node_t> nodes;

	pool_t<map_t> maps;
	pool_t<vec_t> vecs;
	pool_t<cor_t> cors;
	pool_t<data_t> data;

	// compiled "bytecode"
	std::vector<code_t> code;
	size_t peephole = 0;

	std::vector<int> modules;
	int functions = 0;

	// interned strings
	string_pool stringsA; // young
	string_pool stringsB; // old

	// compile-time scope tree
	struct {
		int id = 0;
		tstack<int,PATH> ids;
	} fpath;

	struct scope {
		int id = 0;
		std::vector<int> up;
		std::vector<const char*> locals;
	};

	std::deque<scope> scopes;

	node_t* parsed = nullptr;

	typedef int (*strcb)(int);

	#define must(c,...) if (!(c)) { snprintf(emsg, sizeof(emsg), __VA_ARGS__); explode(); }

	static const int RESULTS_DISCARD = 0;
	static const int RESULTS_FIRST = 1;
	static const int RESULTS_ALL = -1;

	static const int PARSE_UNGREEDY = 0;
	// parse() consumes multiple nodes: a[,b]
	static const int PARSE_COMMA = 1<<0;
	// parse() consumes and/or nodes: x and y or z
	static const int PARSE_ANDOR = 1<<2;

	static const int PROCESS_ASSIGN = (1<<0);
	static const int PROCESS_CHAIN = (1<<1);

	static const int COR_SUSPENDED = 0;
	static const int COR_RUNNING = 1;
	static const int COR_DEAD = 2;

	void gc_mark_item(item_t item) {
		if (item.type == STRING) gc_mark_str(item.str);
		if (item.type == VECTOR) gc_mark_vec(item.vec);
		if (item.type == MAP) gc_mark_map(item.map);
		if (item.type == COROUTINE) gc_mark_cor(item.cor);
		if (item.type == USERDATA && item.data->meta.type != NIL) gc_mark_item(item.data->meta);
		if (item.type == USERDATA) gc_mark_data(item.data);
	}

	void gc_mark_str(const char* str) {
		int index = stringsA.index(str);
		if (index >= 0) stringsA.mark(index);
	}

	void gc_mark_vec(vec_t* vec) {
		if (!vec) return;
		gc_mark_item(vec->meta);
		int index = vecs.index(vec);
		if (index >= 0) vecs.mark(index);

		for (int i = 0, l = vec_size(vec); i < l; i++) {
			gc_mark_item(vec_get(vec, i));
		}
	}

	void gc_mark_map(map_t* map) {
		if (!map) return;
		gc_mark_item(map->meta);
		int index = maps.index(map);
		if (index >= 0) maps.mark(index);

		gc_mark_vec(&map->keys);
		gc_mark_vec(&map->vals);
	}

	void gc_mark_cor(cor_t* cor) {
		if (!cor) return;

		int index = cors.index(cor);
		if (index >= 0) cors.mark(index);

		for (int i = 0, l = cor->stack.depth; i < l; i++)
			gc_mark_item(cor->stack.cells[i]);

		for (int i = 0, l = cor->other.depth; i < l; i++)
			gc_mark_item(cor->other.cells[i]);

		gc_mark_item(cor->map);

		for (int i = 0, l = cor->frames.depth; i < l; i++) {
			frame_t* frame = &cor->frames.cells[i];
			for (int j = 0; j < frame->locals.depth; j++) {
				gc_mark_item(frame->locals[j]);
			}
		}
	}

	void gc_mark_data(data_t* datum) {
		if (!datum) return;
		int index = data.index(datum);
		if (index >= 0) data.mark(index);
	}

	// A naive mark-and-sweep collector that is never called implicitly
	// at run-time. Can be explicitly triggered with "collect()" via
	// script or with rela_collect() via callback.
	void gc() {
		gc_mark_map(scope_core);
		gc_mark_map(scope_global);

		for (int i = 0, l = vec_size(&routines); i < l; i++) {
			gc_mark_cor(vec_get(&routines, i).cor);
		}

		for (int i = 0, l = code.size(); i < l; i++) {
			gc_mark_item(code[i].item);
		}

		for (auto& scope: scopes) {
			for (auto& name: scope.locals) gc_mark_str(name);
		}

		vecs.purge();
		maps.purge();
		cors.purge();
		data.purge();
		stringsA.purge();
	}

	vec_t* vec_allot() {
		return vecs.alloc();
	}

	map_t* map_allot() {
		return maps.alloc();
	}

	data_t* data_allot() {
		return data.alloc();
	}

	cor_t* cor_allot() {
		return cors.alloc();
	}

	size_t vec_size(vec_t* vec) {
		return vec ? vec->items.size(): 0;
	}

	item_t* vec_ins(vec_t* vec, int index) {
		assert(index >= 0 && index <= (int)vec_size(vec));
		item_t item;
		vec->items.insert(vec->items.begin()+index, item);
		return &vec->items[index];
	}

	void vec_del(vec_t* vec, int index) {
		assert(index >= 0 && index < (int)vec_size(vec));
		vec->items.erase(vec->items.begin()+index);
	}

	void vec_push(vec_t* vec, item_t item) {
		vec->items.push_back(item);
	}

	item_t vec_pop(vec_t* vec) {
		item_t item = vec->items.back();
		vec->items.pop_back();
		return item;
	}

	item_t vec_top(vec_t* vec) {
		return vec->items.back();
	}

	item_t* vec_cell(vec_t* vec, int index) {
		if (index < 0) index = (int)vec_size(vec) + index;
		must(index >= 0 && index < (int)vec_size(vec), "vec_cell out of bounds");
		return &vec->items[index];
	}

	item_t vec_get(vec_t* vec, int index) {
		return vec_cell(vec, index)[0];
	}

	int vec_lower_bound(vec_t* vec, item_t key) {
		auto it = std::lower_bound(vec->items.begin(), vec->items.end(), key, [&](const item_t& a, const item_t& b) {
			return less(a, b);
		});
		return it - vec->items.begin();
	}

	void vec_sort(vec_t* vec) {
		std::sort(vec->items.begin(), vec->items.end(), [&](const item_t& a, const item_t& b) {
			return less(a, b);
		});
	}

	int map_lower_bound(map_t* map, item_t key) {
		return vec_lower_bound(&map->keys, key);
	}

	item_t* map_ref(map_t* map, item_t key) {
		int i = map_lower_bound(map, key);
		return (i < (int)vec_size(&map->keys) && equal(vec_get(&map->keys, i), key))
			? vec_cell(&map->vals, i): nullptr;
	}

	bool map_get(map_t* map, item_t key, item_t* val) {
		item_t* item = map_ref(map, key);
		if (!item) return false;
		assert(item->type != NIL);
		*val = *item;
		return true;
	}

	void map_clr(map_t* map, item_t key) {
		int i = map_lower_bound(map, key);
		if (i < (int)vec_size(&map->keys) && equal(vec_get(&map->keys, i), key)) {
			vec_del(&map->keys, i);
			vec_del(&map->vals, i);
		}
	}

	void map_set(map_t* map, item_t key, item_t val) {
		if (val.type == NIL) {
			map_clr(map, key);
			return;
		}
		int i = map_lower_bound(map, key);
		if (i < (int)vec_size(&map->keys) && equal(vec_get(&map->keys, i), key)) {
			vec_cell(&map->vals, i)[0] = val;
		}
		else {
			vec_ins(&map->keys, i)[0] = key;
			vec_ins(&map->vals, i)[0] = val;
		}
	}

	const char* strintern(const char* str) {
		int index = stringsB.index(str);
		return (index >= 0) ? stringsB.cells[index].data: stringsA.insert(str);
	}

	const char* substr(const char *start, int off, int len) {
		char buf[STRBUF];
		must(len < STRBUF, "substr max len exceeded (%d bytes)", STRBUF-1);
		memcpy(buf, start+off, len);
		buf[len] = 0;
		return strintern(buf);
	}

	const char* strliteral(const char *str, char **err) {
		char res[STRBUF];
		char *rp = res, *sp = (char*)str+1, *ep = res + STRBUF;

		while (sp && *sp) {
			int c = *sp++;
			if (c == '"') break;

			if (c == '\\') {
				c = *sp++;
					 if (c == 'a') c = '\a';
				else if (c == 'b') c = '\b';
				else if (c == 'f') c = '\f';
				else if (c == 'n') c = '\n';
				else if (c == 'r') c = '\r';
				else if (c == 't') c = '\t';
				else if (c == 'v') c = '\v';
			}
			*rp++ = c;
			if (rp >= ep) rp = ep-1;
		}
		*rp = 0;
		must(rp <= ep, "strliteral max length exceeded (%d bytes)", STRBUF-1);

		if (err) *err = sp;
		return strintern(res);
	}

	int str_skip(const char *source, strcb cb) {
		int offset = 0;
		while (source[offset] && cb(source[offset])) offset++;
		return offset;
	}

	int str_scan(const char *source, strcb cb) {
		int offset = 0;
		while (source[offset] && !cb(source[offset])) offset++;
		return offset;
	}

	void reset() {
		scope_global = nullptr;
		routines.items.clear();
		routine = nullptr;
		gc();
	}

	item_t nil() {
		return (item_t){.type = NIL, .str = nullptr};
	}

	item_t integer(int64_t i) {
		return (item_t){.type = INTEGER, .inum = i};
	}

	item_t number(double d) {
		return (item_t){.type = FLOAT, .fnum = d};
	}

	item_t string(const char* s) {
		return (item_t){.type = STRING, .str = strintern(s) };
	}

	item_t operation(enum opcode_t opcode) {
		return (item_t){.type = OPERATION, .opcode = opcode};
	}

	bool truth(item_t a) {
		if (a.type == INTEGER) return a.inum != 0;
		if (a.type == FLOAT) return a.fnum > 0+DBL_EPSILON || a.fnum < 0-DBL_EPSILON;
		if (a.type == STRING) return a.str && a.str[0];
		if (a.type == BOOLEAN) return a.flag;
		if (a.type == VECTOR) return vec_size(a.vec) > 0;
		if (a.type == MAP) return vec_size(&a.map->keys) > 0;
		if (a.type == SUBROUTINE) return true;
		if (a.type == COROUTINE) return true;
		if (a.type == OPERATION) return true;
		if (a.type == EXECUTE) return true;
		if (a.type == USERDATA) return a.data != nullptr;
		return false;
	}

	bool meta_get(item_t meta, const char* name, item_t *func) {
		if (meta.type == MAP) return map_get(meta.map, string(name), func);
		if (meta.type == SUBROUTINE || meta.type == EXECUTE) {
			item_t key = string(name);
			method(meta, 1, &key, 1, func);
			return func->type != NIL;
		}
		return false;
	}

	bool equal(item_t a, item_t b) {
		item_t func;
		item_t argv[2] = {a,b};
		item_t retv[1];

		if (a.type == b.type) {
			if (a.type == INTEGER) return a.inum == b.inum;
			if (a.type == FLOAT) return fabs(a.fnum - b.fnum) < DBL_EPSILON*10;
			if (a.type == STRING) return a.str == b.str; // .str must use strintern
			if (a.type == BOOLEAN) return a.flag == b.flag;
			if (a.type == VECTOR && meta_get(a.vec->meta, "==", &func)) {
				method(func, 2, argv, 1, retv);
				return truth(retv[0]);
			}
			if (a.type == VECTOR && a.vec == b.vec) return true;
			if (a.type == VECTOR && vec_size(a.vec) == vec_size(b.vec)) {
				for (int i = 0, l = vec_size(a.vec); i < l; i++) {
					if (!equal(vec_get(a.vec, i), vec_get(b.vec, i))) return false;
				}
				return true;
			}
			if (a.type == MAP && meta_get(a.map->meta, "==", &func)) {
				method(func, 2, argv, 1, retv);
				return truth(retv[0]);
			}
			if (a.type == MAP && a.map == b.map) return true;
			if (a.type == MAP && vec_size(&a.map->keys) == vec_size(&b.map->keys)) {
				for (int i = 0, l = vec_size(&a.map->keys); i < l; i++) {
					if (!equal(vec_get(&a.map->keys, i), vec_get(&b.map->keys, i))) return false;
					if (!equal(vec_get(&a.map->vals, i), vec_get(&b.map->vals, i))) return false;
				}
				return true;
			}
			if (a.type == SUBROUTINE) return a.sub == b.sub;
			if (a.type == COROUTINE) return a.cor == b.cor;
			if (a.type == USERDATA && meta_get(a.data->meta, "==", &func)) {
				method(func, 2, argv, 1, retv);
				return truth(retv[0]);
			}
			if (a.type == USERDATA) return a.data == b.data;
			if (a.type == NIL) return true;
		}
		return false;
	}

	bool less(item_t a, item_t b) {
		if (a.type == b.type) {
			if (a.type == INTEGER) return a.inum < b.inum;
			if (a.type == FLOAT) return a.fnum < b.fnum;
			if (a.type == STRING) return a.str != b.str && strcmp(a.str, b.str) < 0;

			item_t func;
			item_t argv[2] = {a,b};
			item_t retv[1];

			if (a.type == VECTOR && meta_get(a.vec->meta, "<", &func)) {
				method(func, 2, argv, 1, retv);
				return truth(retv[0]);
			}
			if (a.type == VECTOR) return vec_size(a.vec) < vec_size(b.vec);
			if (a.type == MAP && meta_get(a.map->meta, "<", &func)) {
				method(func, 2, argv, 1, retv);
				return truth(retv[0]);
			}
			if (a.type == MAP) return vec_size(&a.map->keys) < vec_size(&b.map->keys);
			if (a.type == USERDATA && meta_get(a.data->meta, "<", &func)) {
				method(func, 2, argv, 1, retv);
				return truth(retv[0]);
			}
		}
		return false;
	}

	int count(item_t a) {
		item_t func;
		item_t argv[1] = {a};
		item_t retv[1];

		if (a.type == INTEGER) return a.inum;
		if (a.type == FLOAT) return floor(a.fnum);
		if (a.type == STRING) return strlen(a.str);
		if (a.type == VECTOR) return vec_size(a.vec);
		if (a.type == MAP) return vec_size(&a.map->keys);
		if (a.type == USERDATA && meta_get(a.data->meta, "#", &func)) {
			method(func, 1, argv, 1, retv);
			must(retv[0].type == INTEGER, "meta method # should return an integer");
			return retv[0].inum;
		}
		return 0;
	}

	item_t add(item_t a, item_t b) {
		if (a.type == INTEGER && b.type == INTEGER) return (item_t){.type = INTEGER, .inum = a.inum + b.inum};
		if (a.type == INTEGER && b.type == FLOAT) return (item_t){.type = INTEGER, .inum = a.inum + (int64_t)b.fnum};
		if (a.type == FLOAT && b.type == INTEGER) return (item_t){.type = FLOAT, .fnum = a.fnum + b.inum};
		if (a.type == FLOAT && b.type == FLOAT) return (item_t){.type = FLOAT, .fnum = a.fnum + b.fnum};

		item_t func;
		item_t argv[2] = {a,b};
		item_t retv[1];

		if (a.type == VECTOR && meta_get(a.vec->meta, "+", &func)) {
			method(func, 2, argv, 1, retv);
			return retv[0];
		}
		if (a.type == MAP && meta_get(a.map->meta, "+", &func)) {
			method(func, 2, argv, 1, retv);
			return retv[0];
		}
		if (a.type == USERDATA && meta_get(a.data->meta, "+", &func)) {
			method(func, 2, argv, 1, retv);
			return retv[0];
		}
		return nil();
	}

	item_t multiply(item_t a, item_t b) {
		if (a.type == INTEGER && b.type == INTEGER) return (item_t){.type = INTEGER, .inum = a.inum * b.inum};
		if (a.type == INTEGER && b.type == FLOAT) return (item_t){.type = INTEGER, .inum = a.inum * (int64_t)b.fnum};
		if (a.type == FLOAT && b.type == INTEGER) return (item_t){.type = FLOAT, .fnum = a.fnum * b.inum};
		if (a.type == FLOAT && b.type == FLOAT) return (item_t){.type = FLOAT, .fnum = a.fnum * b.fnum};

		item_t func;
		item_t argv[2] = {a,b};
		item_t retv[1];

		if (a.type == VECTOR && meta_get(a.vec->meta, "*", &func)) {
			method(func, 2, argv, 1, retv);
			return retv[0];
		}
		if (a.type == MAP && meta_get(a.map->meta, "*", &func)) {
			method(func, 2, argv, 1, retv);
			return retv[0];
		}
		if (a.type == USERDATA && meta_get(a.data->meta, "*", &func)) {
			method(func, 2, argv, 1, retv);
			return retv[0];
		}
		return nil();
	}

	item_t divide(item_t a, item_t b) {
		if (a.type == INTEGER && b.type == INTEGER) return (item_t){.type = INTEGER, .inum = a.inum / b.inum};
		if (a.type == INTEGER && b.type == FLOAT) return (item_t){.type = INTEGER, .inum = a.inum / (int64_t)b.fnum};
		if (a.type == FLOAT && b.type == INTEGER) return (item_t){.type = FLOAT, .fnum = a.fnum / b.inum};
		if (a.type == FLOAT && b.type == FLOAT) return (item_t){.type = FLOAT, .fnum = a.fnum / b.fnum};

		item_t func;
		item_t argv[2] = {a,b};
		item_t retv[1];

		if (a.type == VECTOR && meta_get(a.vec->meta, "/", &func)) {
			method(func, 2, argv, 1, retv);
			return retv[0];
		}
		if (a.type == MAP && meta_get(a.map->meta, "/", &func)) {
			method(func, 2, argv, 1, retv);
			return retv[0];
		}
		if (a.type == USERDATA && meta_get(a.data->meta, "/", &func)) {
			method(func, 2, argv, 1, retv);
			return retv[0];
		}
		return nil();
	}

	const char* tmptext(item_t a, char* tmp, int size) {
		if (a.type == STRING) return a.str;

		item_t func;
		item_t argv[1] = {a};
		item_t retv[1];

		assert(a.type >= 0 && a.type < TYPES);

		if (a.type == NIL) snprintf(tmp, size, "nil");
		if (a.type == INTEGER) snprintf(tmp, size, "%ld", a.inum);
		if (a.type == FLOAT) snprintf(tmp, size, "%f", a.fnum);
		if (a.type == BOOLEAN) snprintf(tmp, size, "%s", a.flag ? "true": "false");
		if (a.type == SUBROUTINE) snprintf(tmp, size, "%s(%d)", type_names[a.type], a.sub);
		if (a.type == COROUTINE) snprintf(tmp, size, "%s", type_names[a.type]);
		if (a.type == OPERATION) snprintf(tmp, size, "%s", operation_name(a.opcode));
		if (a.type == EXECUTE) snprintf(tmp, size, "%s", type_names[a.type]);

		if (a.type == USERDATA && meta_get(a.data->meta, "$", &func)) {
			method(func, 1, argv, 1, retv);
			must(retv[0].type == STRING, "$ should return a string");
			return retv[0].str;
		}

		if (a.type == USERDATA) snprintf(tmp, size, "%s", type_names[a.type]);

		char subtmpA[STRTMP];
		char subtmpB[STRTMP];

		if (a.type == VECTOR && meta_get(a.vec->meta, "$", &func)) {
			method(func, 1, argv, 1, retv);
			must(retv[0].type == STRING, "$ should return a string");
			return retv[0].str;
		}

		if (a.type == VECTOR) {
			int len = snprintf(tmp, size, "[");
			for (int i = 0, l = vec_size(a.vec); len < size && i < l; i++) {
				if (len < size) len += snprintf(tmp+len, size-len, "%s",
					tmptext(vec_get(a.vec, i), subtmpA, sizeof(subtmpA)));
				if (len < size && i < l-1) len += snprintf(tmp+len, size-len, ", ");
			}
			if (len < size) len += snprintf(tmp+len, size-len, "]");
		}

		if (a.type == MAP && meta_get(a.map->meta, "$", &func)) {
			method(func, 1, argv, 1, retv);
			must(retv[0].type == STRING, "$ should return a string");
			return retv[0].str;
		}

		if (a.type == MAP) {
			int len = snprintf(tmp, size, "{");
			for (int i = 0, l = vec_size(&a.map->keys); len < size && i < l; i++) {
				if (len < size) len += snprintf(tmp+len, size-len, "%s = %s",
					tmptext(vec_get(&a.map->keys, i), subtmpA, sizeof(subtmpA)),
					tmptext(vec_get(&a.map->vals, i), subtmpB, sizeof(subtmpB))
				);
				if (len < size && i < l-1) len += snprintf(tmp+len, size-len, ", ");
			}
			if (len < size) len += snprintf(tmp+len, size-len, "}");
		}

		assert(tmp[0]);
		return tmp;
	}

	code_t* compiled(int offset) {
		return &code[offset < 0 ? code.size()+offset: offset];
	}

	void compile_barrier() {
		peephole = code.size();
	}

	int compile(enum opcode_t op, item_t item) {
		// peephole
		if (code.size() > peephole) {
			code_t* back1 = compiled(-1);
			code_t* back2 = code.size() > peephole+1 ? compiled(-2): nullptr;
			code_t* back3 = code.size() > peephole+2 ? compiled(-3): nullptr;

			// remove implicit return block dead code
			if (op == OP_CLEAN && back1->op == OP_CLEAN) return code.size()-1;
			if (op == OP_CLEAN && back1->op == OP_RETURN) return code.size()-1;
			if (op == OP_RETURN && back1->op == OP_RETURN) return code.size()-1;

			// lit,find -> fname (duplicate vars, single lookup array[#array])
			if (op == OP_FIND && back1->op == OP_LIT) {
				for(;;) {
					// fname,copies=n (n+1 stack items: one original + n copies)
					if (back2 && back3 && back2->op == OPP_COPIES && back3->op == OPP_FNAME && equal(back1->item, back3->item)) {
						back2->item.inum++;
						code.pop_back();
						break;
					}
					// fname,copies=1 (two stack items: one original + one copy)
					if (back2 && back2->op == OPP_FNAME && equal(back1->item, back2->item)) {
						back1->op = OPP_COPIES;
						back1->item = integer(1);
						break;
					}
					back1->op = OPP_FNAME;
					break;
				}
				return code.size()-1;
			}

			// lit,get -> gname
			if (op == OP_GET && back1->op == OP_LIT) {
				back1->op = OPP_GNAME;
				return code.size()-1;
			}

			// fname,call -> cfunc
			if (op == OP_CALL && back1->op == OPP_FNAME) {
				back1->op = OPP_CFUNC;
				return code.size()-1;
			}

			// lget,call -> lcall
			if (op == OP_CALL && back1->op == OP_LGET) {
				back1->op = OPP_LCALL;
				return code.size()-1;
			}

			// fname[a],op,lit[a],assign0 -> update[a],op (var=var+n, var=var*n)
			if (op == OP_ASSIGN && item.type == INTEGER && item.inum == 0 && back1->op == OP_LIT && back2 && back3) {
				bool sameName = back3->op == OPP_FNAME && equal(back1->item, back3->item);
				bool simpleOp = back2->op == OPP_ADD_LIT || back2->op == OPP_MUL_LIT;
				if (sameName && simpleOp) {
					code[code.size()-3] = (code_t){.op = OPP_UPDATE, .item = back1->item};
					code.pop_back();
					return code.size()-1;
				}
			}

			// mark,update,op,limit -> update,op
			if (op == OP_LIMIT && item.type == INTEGER && item.inum == 0 && back2 && back3) {
				if (back3->op == OP_MARK && back2->op == OPP_UPDATE) {
					code[code.size()-3] = *back2;
					code[code.size()-2] = *back1;
					code.pop_back();
					return code.size()-1;
				}
			}

			// lit,assign0 -> assignl
			if (op == OP_ASSIGN && item.type == INTEGER && item.inum == 0 && back1->op == OP_LIT) {
				back1->op = OPP_ASSIGNL;
				return code.size()-1;
			}

			// mark,lit,assignl,limit0 -> lit,assignp (map { litkey = litval }, var = lit)
			if (op == OP_LIMIT && item.type == INTEGER && item.inum == 0 && back2 && back3) {
				if (back3->op == OP_MARK && back2->op == OP_LIT && back1->op == OPP_ASSIGNL) {
					item_t key = back1->item;
					code[code.size()-3] = code[code.size()-2];
					code[code.size()-2] = (code_t){.op = OPP_ASSIGNP, .item = key};
					code.pop_back();
					return code.size()-1;
				}
			}

			// lit,neg
			if (op == OP_NEG && back1->op == OP_LIT && back1->item.type == INTEGER) {
				back1->item.inum = -back1->item.inum;
				return code.size()-1;
			}

			// lit,neg
			if (op == OP_NEG && back1->op == OP_LIT && back1->item.type == FLOAT) {
				back1->item.fnum = -back1->item.fnum;
				return code.size()-1;
			}

			// lit,add
			if (op == OP_ADD && back1->op == OP_LIT) {
				back1->op = OPP_ADD_LIT;
				return code.size()-1;
			}

			// lit,mul
			if (op == OP_MUL && back1->op == OP_LIT) {
				back1->op = OPP_MUL_LIT;
				return code.size()-1;
			}

			// mark,lit,limit1
			if (op == OP_LIMIT && item.inum != 0 && back1->op == OP_LIT && back2 && back2->op == OP_MARK) {
				back2->op = back1->op;
				back2->item = back1->item;
				code.pop_back();
				return code.size()-1;
			}

			// mark,mark
			if (op == OP_MARK && back1->op == OP_MARK) {
				back1->op = OPP_MARK2;
				return code.size()-1;
			}

			// limit,limit
			if (op == OP_LIMIT && item.inum == 0 && back1->op == OP_LIMIT) {
				back1->op = OPP_LIMIT2;
				back1->item = item;
				return code.size()-1;
			}

			// enter,clean
			if (op == OP_CLEAN && back1->op == OP_ENTER) {
				return code.size()-1;
			}
		}

		code.push_back((code_t){.op = op, .item = item});
		return code.size()-1;
	}

	item_t* stack_ref(cor_t* routine, int index) {
		return &routine->stack.cells[index];
	}

	item_t* stack_cell(int index) {
		if (index < 0) index = routine->stack.depth + index;
		assert(index >= 0 && index < routine->stack.depth);
		return stack_ref(routine, index);
	}

	item_t* items() {
		cor_t* cor = routine;
		int base = cor->marks.depth ? cor->marks.cells[cor->marks.depth-1]: 0;
		return &cor->stack.cells[base];
	}

	int depth() {
		cor_t* cor = routine;
		int base = cor->marks.depth ? cor->marks.cells[cor->marks.depth-1]: 0;
		return cor->stack.depth - base;
	}

	item_t* item(int i) {
		int base = routine->marks.cells[routine->marks.depth-1];
		int index = i >= 0 ? base+i: i;
		return stack_ref(routine, index);
	}

	void push(item_t item) {
		assert(routine->stack.depth < (int)STACK);
		int index = routine->stack.depth++;
		*stack_ref(routine, index) = item;
	}

	item_t pop() {
		assert(routine->stack.depth > 0);
		int index = --routine->stack.depth;
		return *stack_ref(routine, index);
	}

	item_t top() {
		assert(routine->stack.depth > 0);
		int index = routine->stack.depth-1;
		return *stack_ref(routine, index);
	}

	void opush(item_t item) {
		assert(routine->other.depth < (int)STACK);
		routine->other.cells[routine->other.depth++] = item;
	}

	item_t opop() {
		assert(routine->other.depth > 0);
		return routine->other.cells[--routine->other.depth];
	}

	item_t otop() {
		assert(routine->other.depth > 0);
		return routine->other.cells[routine->other.depth-1];
	}

	item_t pop_type(int type) {
		item_t i = pop();
		if (type == FLOAT && i.type == INTEGER) return number(i.inum);
		must(i.type == type, "pop_type expected %s, found %s", type_names[type], type_names[i.type]);
		return i;
	}

	static int isnamefirst(int c) {
		return isalpha(c) || c == '_';
	}

	static int isname(int c) {
		return isnamefirst(c) || isdigit(c);
	}

	static int islf(int c) {
		return c == '\n';
	}

	int skip_gap(const char *source) {
		int offset = 0;
		while (source[offset]) {
			if (isspace(source[offset])) {
				offset += str_skip(&source[offset], isspace);
				continue;
			}
			if (source[offset] == '/' && source[offset+1] == '/') {
				while (source[offset] == '/' && source[offset+1] == '/') {
					offset += str_scan(&source[offset], islf);
					offset += str_skip(&source[offset], islf);
				}
				continue;
			}
			break;
		}
		return offset;
	}

	bool peek(const char* source, const char* name) {
		while (*source && *name && *source == *name) { ++source; ++name; }
		return *source && !*name && !isname(*source);
	}

	node_t* node_allot() {
		return &nodes.emplace_back();
	}

	int parse_block(const char *source, node_t *node) {
		int length = 0;
		int found_end = 0;
		int offset = skip_gap(source);

		// don't care about this, but Lua habits...
		if (peek(&source[offset], "do")) offset += 2;

		while (source[offset]) {
			if ((length = skip_gap(&source[offset])) > 0) {
				offset += length;
				continue;
			}
			if (peek(&source[offset], "end")) {
				offset += 3;
				found_end = 1;
				break;
			}

			offset += parse(&source[offset], RESULTS_DISCARD, PARSE_COMMA|PARSE_ANDOR);
			node->vals.push_back(parsed); parsed = nullptr;
		}

		must(found_end, "expected keyword 'end': %s", &source[offset]);
		return offset;
	}

	int parse_branch(const char *source, node_t *node) {
		int length = 0;
		int found_else = 0;
		int found_end = 0;
		int offset = skip_gap(source);

		// don't care about this, but Lua habits...
		if (peek(&source[offset], "then")) offset += 4;

		while (source[offset]) {
			if ((length = skip_gap(&source[offset])) > 0) {
				offset += length;
				continue;
			}

			if (peek(&source[offset], "else")) {
				offset += 4;
				found_else = 1;
				break;
			}

			if (peek(&source[offset], "end")) {
				offset += 3;
				found_end = 1;
				break;
			}

			offset += parse(&source[offset], RESULTS_DISCARD, PARSE_COMMA|PARSE_ANDOR);
			node->vals.push_back(parsed); parsed = nullptr;
		}

		if (found_else) {
			while (source[offset]) {
				if ((length = skip_gap(&source[offset])) > 0) {
					offset += length;
					continue;
				}

				if (peek(&source[offset], "end")) {
					offset += 3;
					found_end = 1;
					break;
				}

				offset += parse(&source[offset], RESULTS_DISCARD, PARSE_COMMA|PARSE_ANDOR);
				node->keys.push_back(parsed); parsed = nullptr;
			}
		}

		must(found_else || found_end, "expected keyword 'else' or 'end': %s", &source[offset]);

		// last NODE_MULTI in the block returns its value (ternary operator equiv)
		if (node->vals.size())
			node->vals.back()->results = RESULTS_FIRST;
		if (node->keys.size())
			node->keys.back()->results = RESULTS_FIRST;

		return offset;
	}

	int parse_arglist(const char *source) {
		parsed = nullptr;
		int offset = skip_gap(source);

		if (source[offset] == '(') {
			offset++;
			offset += skip_gap(&source[offset]);

			if (source[offset] != ')') {
				offset += parse(&source[offset], RESULTS_ALL, PARSE_COMMA|PARSE_ANDOR);
				offset += skip_gap(&source[offset]);
			}

			must(source[offset] == ')', "expected closing paren: %s", &source[offset]);
			offset++;
		}
		else {
			offset += parse(&source[offset], RESULTS_FIRST, PARSE_COMMA|PARSE_ANDOR);
		}

		return offset;
	}

	// Since anything can return values, including some control structures, there is no
	// real difference between statements and expressions. Just nodes in the parse tree.
	int parse_node(const char *source) {
		int length = 0;
		int offset = skip_gap(source);

		modifier_t* modifier = nullptr;

		for (int i = 0, l = sizeof(modifiers) / sizeof(modifier_t); i < l; i++) {
			modifier_t *mod = &modifiers[i];
			if (!strncmp(mod->name, &source[offset], strlen(mod->name))) {
				modifier = mod;
				break;
			}
		}

		if (modifier) {
			offset += strlen(modifier->name);
			offset += parse_node(&source[offset]);
			node_t* outer = node_allot();
			outer->type = NODE_OPCODE;
			outer->opcode = modifier->opcode;
			outer->single = true;
			outer->args = parsed;
			parsed = outer;
			return offset;
		}

		node_t *node = node_allot();

		if (isnamefirst(source[offset])) {
			node->type = NODE_NAME;
			length = str_skip(&source[offset], isname);

			int have_keyword = 0;

			for (int i = 0, l = sizeof(keywords) / sizeof(keyword_t); i < l && !have_keyword; i++) {
				keyword_t *keyword = &keywords[i];

				if (peek(&source[offset], keyword->name)) {
					node->type = NODE_OPCODE;
					node->opcode = keyword->opcode;
					node->control = true;
					have_keyword = 1;
					offset += length;
				}
			}

			if (!have_keyword) {
				if (peek(&source[offset], "if")) {
					offset += 2;
					node->type = NODE_IF;
					node->control = false; // true breaks ternary

					// conditions
					offset += parse(&source[offset], RESULTS_FIRST, PARSE_COMMA|PARSE_ANDOR);
					node->args = parsed; parsed = nullptr;

					// block, optional else
					offset += parse_branch(&source[offset], node);
				}
				else
				if (peek(&source[offset], "while")) {
					offset += 5;
					node->type = NODE_WHILE;
					node->control = true;

					// conditions
					offset += parse(&source[offset], RESULTS_FIRST, PARSE_COMMA|PARSE_ANDOR);
					node->args = parsed; parsed = nullptr;

					// block
					offset += parse_block(&source[offset], node);
				}
				else
				if (peek(&source[offset], "for")) {
					offset += 3;
					node->type = NODE_FOR;
					node->control = true;

					// [key[,val]] local variable names
					offset += skip_gap(&source[offset]);
					if (!peek(&source[offset], "in")) {
						must(isnamefirst(source[offset]), "expected for [<key>,]val in iterable: %s", &source[offset]);

						length = str_skip(&source[offset], isname);
						node->fkeys.push_back(string(substr(&source[offset], 0, length)));
						offset += length;

						offset += skip_gap(&source[offset]);
						if (source[offset] == ',') {
							offset++;
							length = str_skip(&source[offset], isname);
							node->fkeys.push_back(string(substr(&source[offset], 0, length)));
							offset += length;
						}
					}

					offset += skip_gap(&source[offset]);
					must(peek(&source[offset], "in"), "expected for [<key>,]val in iterable: %s", &source[offset]);
					offset += 2;

					// iterable
					offset += parse(&source[offset], RESULTS_FIRST, PARSE_COMMA|PARSE_ANDOR);
					node->args = parsed; parsed = nullptr;

					// block
					offset += parse_block(&source[offset], node);
				}
				else
				if (peek(&source[offset], "function")) {
					offset += 8;
					node->type = NODE_FUNCTION;
					node->control = true;

					must(fpath.ids.size() < (int)PATH, "reached function nest limit(%d)", PATH);
					for (int i = 0, l = fpath.ids.size(); i < l; i++)
						node->fpath.ids.push(fpath.ids[i]);
					node->fpath.id = ++fpath.id;
					fpath.ids.push(node->fpath.id);

					offset += skip_gap(&source[offset]);

					// optional function name
					if (isnamefirst(source[offset])) {
						length = str_skip(&source[offset], isname);
						node->item = string(substr(&source[offset], 0, length));
						offset += length;
					}

					offset += skip_gap(&source[offset]);

					// argument locals list
					if (source[offset] == '(') {
						offset++;

						while (source[offset]) {
							if ((length = skip_gap(&source[offset])) > 0) {
								offset += length;
								continue;
							}
							if (source[offset] == ',') {
								offset++;
								continue;
							}
							if (source[offset] == ')') {
								offset++;
								break;
							}

							must(isnamefirst(source[offset]), "expected parameter: %s", &source[offset]);
							length = str_skip(&source[offset], isname);

							node_t *param = node_allot();
							param->type = NODE_NAME;
							param->item = string(substr(&source[offset], 0, length));
							node->keys.push_back(param);

							offset += length;
						}
					}

					// block
					offset += parse_block(&source[offset], node);

					assert(fpath.ids.size() > 0);
					fpath.ids.pop();
				}
				else
				if (peek(&source[offset], "return")) {
					offset += 6;
					node->type = NODE_RETURN;
					node->control = true;

					offset += skip_gap(&source[offset]);

					if (!peek(&source[offset], "end")) {
						offset += parse(&source[offset], RESULTS_ALL, PARSE_COMMA|PARSE_ANDOR);
						node->args = parsed; parsed = nullptr;
					}
				}
				else
				if (peek(&source[offset], "break")) {
					offset += 5;
					node->type = NODE_OPCODE;
					node->opcode = OP_BREAK;
					node->control = true;
				}
				else
				if (peek(&source[offset], "continue")) {
					offset += 8;
					node->type = NODE_OPCODE;
					node->opcode = OP_CONTINUE;
					node->control = true;
				}
				else {
					node->item = string(substr(&source[offset], 0, length));
					offset += length;
				}
			}
		}
		else
		if (source[offset] == '"') {
			node->type = NODE_LITERAL;
			node->single = true;
			char *end = nullptr;
			node->item = string(strliteral(&source[offset], &end));
			offset += end - &source[offset];
		}
		else
		if (isdigit(source[offset])) {
			node->type = NODE_LITERAL;
			node->single = true;
			char *a = nullptr, *b = nullptr;
			int64_t i = strtoll(&source[offset], &a, 0);
			double f = strtod(&source[offset], &b);
			node->item = (b > a) ? number(f): integer(i);
			offset += ((b > a) ? b: a) - &source[offset];
		}
		else
		// a vector[n] or map[s] to be set/get
		if (source[offset] == '[') {
			offset++;
			node->type = NODE_VEC;
			node->single = true;
			while (source[offset] && source[offset] != ']') {
				if ((length = skip_gap(&source[offset])) > 0) {
					offset += length;
					continue;
				}
				if (source[offset] == ',') {
					offset++;
					continue;
				}
				offset += parse(&source[offset], RESULTS_ALL, PARSE_ANDOR);
				node->vals.push_back(parsed); parsed = nullptr;
			}
			must(source[offset] == ']', "expected closing bracket: %s", &source[offset]);
			offset++;
		}
		else
		if (source[offset] == '{') {
			offset++;
			node->type = NODE_MAP;
			node->single = true;

			while (source[offset] && source[offset] != '}') {
				if ((length = skip_gap(&source[offset])) > 0) {
					offset += length;
					continue;
				}
				if (source[offset] == ',') {
					offset++;
					continue;
				}
				const char* left = &source[offset];
				offset += parse(&source[offset], RESULTS_DISCARD, PARSE_UNGREEDY);
				node_t* pair = parsed; parsed = nullptr;
				must(pair->type == NODE_MULTI && pair->keys.size() == 1 && pair->vals.size() == 1, "expected key/val pair: %s", left);
				node->vals.push_back(pair);
			}
			must(source[offset] == '}', "expected closing brace: %s", &source[offset]);
			offset++;
		}
		else {
			must(0, "what: %s", &source[offset]);
		}

		node_t *prev = node;

		while (source[offset]) {
			if ((length = skip_gap(&source[offset])) > 0) {
				offset += length;
				continue;
			}

			// function/opcode call arguments
			if (source[offset] == '(') {
				offset += parse_arglist(&source[offset]);
				// Chaining calls ()()...() means the previous node may be
				// a nested vecmap[...] or already have its own arguments so
				// chain extra OP_CALLs.
				if (prev->index || prev->call || prev->args) {
					node_t* call = node_allot();
					call->type = NODE_CALL_CHAIN;
					call->args = parsed;
					parsed = nullptr;
					prev->chain = call;
					prev = call;
				} else {
					prev->call = 1;
					prev->args = parsed;
					parsed = nullptr;
				}
				break;
			}

			// chained vector[n] or map[k] expressions
			if (source[offset] == '[') {
				offset++;
//				offset += parse_node(&source[offset]);
				offset += parse(&source[offset], RESULTS_FIRST, PARSE_UNGREEDY);
				prev->chain = parsed;
				parsed = nullptr;
				prev = prev->chain;
				prev->index = true;
				offset += skip_gap(&source[offset]);
				must(source[offset] == ']', "expected closing bracket: %s", &source[offset]);
				offset++;
				continue;
			}

			// chained map.field.subfield expressions
			if (source[offset] == '.' && isnamefirst(source[offset+1])) {
				offset++;
				offset += parse_node(&source[offset]);
				prev->chain = parsed;
				parsed = nullptr;
				prev = prev->chain;
				prev->field = true;
				continue;
			}

			// chained map:field:subfield expressions
			if (source[offset] == ':' && isnamefirst(source[offset+1])) {
				offset++;
				offset += parse_node(&source[offset]);
				prev->chain = parsed;
				parsed = nullptr;
				prev = prev->chain;
				prev->field = true;
				prev->method = true;
				continue;
			}
			break;
		}

		parsed = node;
		return offset;
	}

	int parse(const char *source, int results, int mode) {
		int length = 0;
		int offset = skip_gap(source);

		node_t *node = node_allot();
		node->type = NODE_MULTI;
		node->results = results;

		while (source[offset]) {
			if ((length = skip_gap(&source[offset])) > 0){
				offset += length;
				continue;
			}

			// shunting yard
			operator_t *operations[STACK];
			node_t *arguments[STACK];

			int operation = 0;
			int argument = 0;

			while (source[offset]) {
				if ((length = skip_gap(&source[offset])) > 0) {
					offset += length;
					continue;
				}

				if (source[offset] == '(') {
					offset++;
					offset += parse(&source[offset], RESULTS_FIRST, PARSE_COMMA|PARSE_ANDOR);
					arguments[argument++] = parsed; parsed = nullptr;
					arguments[argument-1]->results = RESULTS_FIRST;
					offset += skip_gap(&source[offset]);
					must(source[offset] == ')', "expected closing paren: %s", &source[offset]);
					offset++;
				}
				else {
					offset += parse_node(&source[offset]);
					arguments[argument++] = parsed; parsed = nullptr;
				}

				offset += skip_gap(&source[offset]);
				operator_t* compare = nullptr;

				for (int i = 0, l = sizeof(operators) / sizeof(operator_t); i < l; i++) {
					operator_t *op = &operators[i];
					int oplen = strlen(op->name);
					if (!strncmp(op->name, &source[offset], oplen)) {
						// and/or needs a trailing space
						if (isalpha(op->name[oplen-1]) && !isspace(source[offset+oplen])) continue;
						compare = op;
						break;
					}
				}

				if (!compare) break;
				offset += strlen(compare->name);

				while (operation > 0 && operations[operation-1]->precedence >= compare->precedence) {
					operator_t *consume = operations[--operation];
					node_t *result = node_allot();
					result->type   = NODE_OPERATOR;
					result->opcode = consume->opcode;
					result->single = consume->single;
					must(argument >= consume->argc, "operator %s insufficient arguments", consume->name);
					for (int j = consume->argc; j > 0; --j) {
						result->vals.push_back(arguments[argument-j]);
					}
					argument -= consume->argc;
					arguments[argument++] = result;
				}

				operations[operation++] = compare;
				if (compare->argc == 1 && argument > 0) break;
			}

			while (operation && argument) {
				operator_t *consume = operations[--operation];
				node_t *result = node_allot();
				result->type   = NODE_OPERATOR;
				result->opcode = consume->opcode;
				result->single = consume->single;
				must(argument >= consume->argc, "operator %s insufficient arguments", consume->name);
				for (int j = consume->argc; j > 0; --j) {
					result->vals.push_back(arguments[argument-j]);
				}
				argument -= consume->argc;
				arguments[argument++] = result;
			}

			must(!operation && argument == 1, "unbalanced expression: %s", &source[offset]);
			node->vals.push_back(arguments[0]);

			offset += skip_gap(&source[offset]);

			if (source[offset] == '=') {
				must(node->vals.size() > 0, "missing assignment name: %s", &source[offset]);

				offset++;
				for (int i = 0, l = node->vals.size(); i < l; i++)
					node->keys.push_back(node->vals[i]);

				node->vals.clear();
				continue;
			}

			if (source[offset] == ',' && (mode & PARSE_COMMA)) {
				offset++;
				continue;
			}

			break;
		}

		must(node->vals.size() > 0, "missing assignment value: %s", &source[offset]);

		bool solo = !node->args && node->keys.size() == 0 && node->vals.size() == 1;

		// node guarantees to handle it's own result limits
		if (solo && node->vals.front()->control) {
			parsed = node->vals.front();
			return offset;
		}

		// node guarantees to return a single result
		if (solo && results != RESULTS_DISCARD && node->vals.front()->single) {
			parsed = node->vals.front();
			return offset;
		}

		parsed = node;
		return offset;
	}

	int scope_find(node_t* scope, item_t var) {
		if (scope) for (int i = 0, l = scope->keys.size(); i < l; i++) {
			node_t* key = scope->keys[i];
			if (equal(key->item, var)) return i;
		}
		return -1;
	}

	int compile_local(node_t* scope, node_t* node) {
		int local = scope_find(scope, node->item);
		bool exists = local >= 0;
		if (scope && !exists) {
			local = scope->keys.size();
			scope->keys.push_back(node);
			exists = true;
		}
		return local;
	}

	int compile_assign(node_t* scope, node_t* node, int index) {
		int local = compile_local(scope, node);
		compile(OP_LIT, local >= 0 ? integer(local): node->item);
		compile(OP_ASSIGN, integer(index));
		return local;
	}

	void compile_lookup(node_t* scope, node_t* node) {
		int local = scope_find(scope, node->item);
		if (local >= 0) {
			compile(OP_LGET, integer(local));
		}
		else {
			compile(OP_LIT, node->item);
			compile(OP_FIND, nil());
		}
	}

	void process(node_t* scope, node_t *node, int flags, int index, int limit) {
		int flag_assign = flags & PROCESS_ASSIGN ? 1:0;

		// if we're assigning with chained expressions, only OP_SET|OP_ASSIGN the last one
		bool assigning = flag_assign && !node->chain;

		char tmp[STRTMP];

		// a multi-part expression: a[,b...] = node[,node...]
		// this is the entry point for most non-control non-opcode statements
		if (node->type == NODE_MULTI) {
			assert(!node->args);
			assert(node->vals.size());

			bool wrap = node->results != RESULTS_ALL && !node->control;

			// substack frame
			if (wrap)
				compile(OP_MARK, nil());

			// stream the values onto the substack
			for (int i = 0, l = node->vals.size(); i < l; i++)
				process(scope, node->vals[i], 0, 0, -1);

			// OP_SET|OP_ASSIGN index values from the start of the current substack frame
			for (int i = 0, l = node->keys.size(); i < l; i++) {
				node_t* subnode = node->keys[i];
				process(scope, subnode, PROCESS_ASSIGN, i, -1);
			}

			// end substack frame
			if (wrap)
				compile(OP_LIMIT, integer(node->results));

			// [expr]
			if (node->index)
				compile(assigning ? OP_SET: OP_GET, nil());

			// [expr](...)
			if (node->chain)
				process(scope, node->chain, flag_assign ? PROCESS_ASSIGN: 0, 0, 1);
		}
		else
		if (node->type == NODE_NAME) {
			assert(!node->keys.size() && !node->vals.size());

			// function or function-like opcode call
			if (node->call) {
				assert(!assigning);

				// vecmap[fn()]
//				if (node->index) {
//					compile(OP_MARK, nil());
//						if (node->args)
//							process(scope, node->args, 0, 0, -1);
//						compile(OP_LIT, node->item);
//						compile(OP_FIND, nil());
//						compile(OP_CALL, nil());
//					compile(OP_LIMIT, integer(1));
//					compile(OP_GET, nil());
//				}

				// :fn()
				if (node->field && node->method) {
					compile(OP_COPY, nil());
					compile(OP_LIT, node->item);
					compile(OP_GET, nil());
					compile(OP_SHUNT, nil());
					compile(OP_SHUNT, nil());
					compile(OP_MARK, nil());
						compile(OP_SHIFT, nil());
						if (node->args)
							process(scope, node->args, 0, 0, -1);
						compile(OP_SHIFT, nil());
						compile(OP_CALL, nil());
					compile(OP_LIMIT, integer(limit));
				}

				// .fn()
				if (node->field && !node->method) {
					compile(OP_LIT, node->item);
					compile(OP_GET, nil());
					compile(OP_SHUNT, nil());
					compile(OP_MARK, nil());
						if (node->args)
							process(scope, node->args, 0, 0, -1);
						compile(OP_SHIFT, nil());
						compile(OP_CALL, nil());
					compile(OP_LIMIT, integer(limit));
				}

				// fn()
				if (!node->index && !node->field) {
					compile(OP_MARK, nil());
						if (node->args)
							process(scope, node->args, 0, 0, -1);
						compile_lookup(scope, node);
						compile(OP_CALL, nil());
					compile(OP_LIMIT, integer(limit));
				}
			}
			// variable reference
			else {
				if (assigning) {
					if (node->index) {
						compile(OP_LIT, node->item);
						compile(OP_FIND, nil());
						compile(OP_SET, nil());
					}

					if (node->field) {
						compile(OP_LIT, node->item);
						compile(OP_SET, nil());
					}

					if (!node->index && !node->field) {
						compile_assign(scope, node, index);
					}
				}
				else {
					if (node->index) {
						compile(OP_LIT, node->item);
						compile(OP_FIND, nil());
						compile(OP_GET, nil());
					}

					if (node->field) {
						compile(OP_LIT, node->item);
						compile(OP_GET, nil());
					}

					if (!node->index && !node->field) {
						compile_lookup(scope, node);
					}
				}
			}

			if (node->chain) {
				process(scope, node->chain, flag_assign ? PROCESS_ASSIGN: 0, 0, 1);
			}
		}
		else
		// function with optional name assignment
		if (node->type == NODE_FUNCTION) {
			assert(!node->args);

			compile(OP_MARK, nil());
			int entry = compile(OP_LIT, nil());

			if (node->item.type) {
				compile_assign(scope, node, 0);
			}

			int jump = compile(OP_JMP, nil());
			compiled(entry)->item = (item_t){.type = SUBROUTINE, .sub = (int)code.size()};

			scopes.resize(std::max((size_t)(node->fpath.id+1), scopes.size()));
			auto& fscope = scopes[node->fpath.id];

			fscope.id = node->fpath.id;
			for (int i = 0, l = node->fpath.ids.size(); i < l; i++) {
				fscope.up.push_back(node->fpath.ids[i]);
			}

			compile(OP_ENTER, integer(fscope.id));

			for (int i = 0, l = node->vals.size(); i < l; i++) {
				process(node, node->vals[i], 0, 0, 0);
			}

			must(node->keys.size() < LOCALS, "too many locals");

			for (auto& key: node->keys) {
				fscope.locals.push_back(key->item.str);
			}

			// if an explicit return expression is used, these instructions
			// will be dead code
			compile(OP_CLEAN, nil());
			compile(OP_RETURN, nil());
			compiled(jump)->item = integer(code.size());
			compile_barrier();

			// value only returns if not function name() form
			compile(OP_LIMIT, integer(node->item.type ? 0:1));

			// function() ... end()
			if (node->call) {
				compile(OP_SHUNT, nil());
				compile(OP_MARK, nil());
					if (node->args)
						process(scope, node->args, 0, 0, -1);
					compile(OP_SHIFT, nil());
					compile(OP_CALL, nil());
				compile(OP_LIMIT, integer(limit));
			}
		}
		else
		// function/opcode call
		if (node->type == NODE_CALL_CHAIN) {
			compile(OP_SHUNT, nil());
			compile(OP_MARK, nil());
				if (node->args)
					process(scope, node->args, 0, 0, -1);
				compile(OP_SHIFT, nil());
				for (int i = 0, l = node->vals.size(); i < l; i++)
					process(scope, node->vals[i], 0, 0, -1);
				compile(OP_CALL, nil());
			compile(OP_LIMIT, integer(limit));

			if (node->index) {
				compile(assigning ? OP_SET: OP_GET, nil());
			}

			if (node->chain) {
				process(scope, node->chain, flag_assign ? PROCESS_ASSIGN: 0, 0, 1);
			}
		}
		// inline opcode
		else
		if (node->type == NODE_OPCODE) {
			assert(node->opcode != OP_CALL);

			if (node->args)
				process(scope, node->args, 0, 0, -1);

			for (int i = 0, l = node->vals.size(); i < l; i++)
				process(scope, node->vals[i], 0, 0, -1);

			compile(node->opcode, nil());

			if (node->index) {
				compile(assigning ? OP_SET: OP_GET, nil());
			}

			if (node->chain) {
				process(scope, node->chain, flag_assign ? PROCESS_ASSIGN: 0, 0, 1);
			}
		}
		else
		if (node->type == NODE_OPERATOR && node->opcode == OP_AND) {
			assert(node->vals.size() == 2);
			process(scope, node->vals[0], 0, 0, 1);
			int jump = compile(OP_JFALSE, nil());
			compile(OP_DROP, nil());
			process(scope, node->vals[1], 0, 0, 1);
			compiled(jump)->item = integer(code.size());
			compile_barrier();
		}
		else
		if (node->type == NODE_OPERATOR && node->opcode == OP_OR) {
			assert(node->vals.size() == 2);
			process(scope, node->vals[0], 0, 0, 1);
			int jump = compile(OP_JTRUE, nil());
			compile(OP_DROP, nil());
			process(scope, node->vals[1], 0, 0, 1);
			compiled(jump)->item = integer(code.size());
			compile_barrier();
		}
		else
		if (node->type == NODE_OPERATOR) {
			for (int i = 0, l = node->vals.size(); i < l; i++)
				process(scope, node->vals[i], 0, 0, 1);

			compile(node->opcode, nil());

			if (node->index) {
				compile(assigning ? OP_SET: OP_GET, nil());
			}

			if (node->chain) {
				process(scope, node->chain, flag_assign ? PROCESS_ASSIGN: 0, 0, 1);
			}
		}
		else
		// string or number literal, optionally part of array chain a[b]["c"]
		if (node->type == NODE_LITERAL) {
			assert(!node->args && !node->keys.size() && !node->vals.size());

			const char *dollar = node->item.type == STRING ? strchr(node->item.str, '$'): nullptr;

			if (dollar && dollar < node->item.str + strlen(node->item.str) - 1) {
				assert(node->item.type == STRING);

				const char *str = node->item.str;
				const char *left = str;
				const char *right = str;

				bool started = false;

				while ((right = strchr(left, '$')) && right && *right) {
					const char *start = right+1;
					const char *finish = start;
					int length = 0;

					if (*start == '(') {
						start++;
						const char *rparen = strchr(start, ')');
						must(rparen, "string interpolation missing closing paren: %s", right);
						length = rparen-start;
						finish = rparen+1;
					}
					else {
						length = str_skip(start, isname);
						finish = &start[length];
					}

					if (right > left) {
						compile(OP_LIT, string(substr(left, 0, right-left+(length ? 0:1))));
						if (started) compile(OP_CONCAT, nil());
						started = true;
					}

					left = finish;

					if (length) {
						const char *sub = substr(start, 0, length);
						must(length == parse(sub, RESULTS_FIRST, PARSE_COMMA|PARSE_ANDOR), "string interpolation parsing failed");
						process(scope, parsed, 0, 0, -1);
						if (started) compile(OP_CONCAT, nil());
						started = true;
					}
				}

				if (strlen(left)) {
					compile(OP_LIT, string(substr(left, 0, strlen(left))));
					if (started) compile(OP_CONCAT, nil());
					started = true;
				}
			}
			else {
				compile(OP_LIT, node->item);
			}

			if (node->index) {
				compile(assigning ? OP_SET: OP_GET, nil());
			}

			if (node->chain) {
				process(scope, node->chain, flag_assign ? PROCESS_ASSIGN: 0, 0, 1);
			}

			must(!assigning || node->item.type == STRING, "cannot assign %s",
				tmptext(node->item, tmp, sizeof(tmp)));

			// special case allows: "complex-string" = value in map literals
			if (!node->index && assigning && node->item.type == STRING) {
				compile(OP_ASSIGN, integer(index));
			}
		}
		else
		// if expression ... [else ...] end
		// (returns a value for ternary style assignment)
		if (node->type == NODE_IF) {

			// conditions
			if (node->args)
				process(scope, node->args, 0, 0, -1);

			// if false, jump to else/end
			int jump = compile(OP_JFALSE, nil());
			compile(OP_DROP, nil());

			// success block
			for (int i = 0, l = node->vals.size(); i < l; i++)
				process(scope, node->vals[i], 0, 0, 0);

			// optional failure block
			if (node->keys.size()) {
				// jump success path past failure block
				int jump2 = compile(OP_JMP, nil());
				compiled(jump)->item = integer(code.size());
				compile_barrier();
				compile(OP_DROP, nil());

				// failure block
				for (int i = 0, l = node->keys.size(); i < l; i++)
					process(scope, node->keys[i], 0, 0, 0);

				compiled(jump2)->item = integer(code.size());
				compile_barrier();
			}
			else {
				compiled(jump)->item = integer(code.size());
				compile_barrier();
			}

			must(!assigning, "cannot assign to if block");
		}
		else
		// while expression ... end
		if (node->type == NODE_WHILE) {
			assert(node->vals.size());

			compile(OP_MARK, nil());
			int loop = compile(OP_LOOP, nil());
			int begin = code.size();

			// condition(s)
			if (node->args)
				process(scope, node->args, 0, 0, -1);

			// if false, jump to end
			int iter = compile(OP_JFALSE, nil());
			compile(OP_DROP, nil());

			// do ... end
			for (int i = 0, l = node->vals.size(); i < l; i++)
				process(scope, node->vals[i], 0, 0, 0);

			// clean up
			compile(OP_JMP, integer(begin));
			compiled(iter)->item = integer(code.size());
			compiled(loop)->item = integer(code.size());
			compile_barrier();
			compile(OP_UNLOOP, nil());
			compile(OP_LIMIT, integer(0));

			must(!assigning, "cannot assign to while block");
		}
		else
		// for k,v in container ... end
		if (node->type == NODE_FOR) {
			compile(OP_MARK, nil());

			// the iterable
			if (node->args)
				process(scope, node->args, 0, 0, -1);

			int loop = compile(OP_LOOP, nil());
			int begin = code.size();

			// OP_FOR expects a vector with key[,val] variable names

			vec_t* keys = vec_allot();

			if (scope) {
				for (auto& key: node->fkeys) {
					node_t* nkey = node_allot();
					nkey->item = key;
					int local = compile_local(scope, nkey);
					vec_push(keys, integer(local));
				}
			}
			else {
				for (auto& key: node->fkeys) {
					vec_push(keys, key);
				}
			}

			compile(OP_FOR, (item_t){.type = VECTOR, .vec = keys});

			// block
			for (int i = 0, l = node->vals.size(); i < l; i++)
				process(scope, node->vals[i], 0, 0, 0);

			// clean up
			compile(OP_JMP, integer(begin));
			compiled(loop)->item = integer(code.size());
			compile_barrier();
			compile(OP_UNLOOP, nil());
			compile(OP_LIMIT, integer(0));

			must(!assigning, "cannot assign to for block");
		}
		else
		// return 0 or more values
		if (node->type == NODE_RETURN) {
			compile(OP_CLEAN, nil());

			if (node->args)
				process(scope, node->args, 0, 0, -1);

			compile(OP_RETURN, nil());

			must(!assigning, "cannot assign to return");
		}
		else
		// literal vector [1,2,3]
		if (node->type == NODE_VEC) {
			assert(!node->args);
			compile(OP_VECTOR, nil());
			compile(OP_MARK, nil());

			for (int i = 0, l = node->vals.size(); i < l; i++) {
				process(scope, node->vals[i], 0, 0, -1);
				compile(OP_VPUSH, nil());
			}

			compile(OP_LIMIT, integer(0));
			compile(OP_SHIFT, nil());
		}
		else
		// literal map { a = 1, b = 2, c = nil }
		if (node->type == NODE_MAP) {
			compile(OP_MARK, nil());
			compile(OP_MAP, nil());
			assert(!node->args);

			for (int i = 0, l = node->vals.size(); i < l; i++)
				process(nullptr, node->vals[i], 0, 0, 0);

			compile(OP_UNMAP, nil());
			compile(OP_LIMIT, integer(1));
		}
		else {
			must(0, "unexpected expression type: %d", node->type);
		}
	}

	void source(const char *source) {
		int offset = skip_gap(source);

		while (source[offset]) {
			offset += parse(&source[offset], RESULTS_DISCARD, PARSE_COMMA|PARSE_ANDOR);
			process(nullptr, parsed, 0, 0, -1);
		}

		must(!depth(), "parse unbalanced");
	}

	item_t& literal() {
		return code[routine->ip-1].item;
	}

	int64_t literal_int() {
		item_t& lit = literal();
		return lit.type == INTEGER ? lit.inum: 0;
	}

	void op_print() {
		int items = depth();
		if (!items) return;

		item_t* item = stack_cell(-items);

		char tmp[STRBUF];
		for (int i = 0; i < items; i++) {
			const char *str = tmptext(*item++, tmp, sizeof(tmp));
			fprintf(stdout, "%s%s", i ? "\t": "", str);
		}
		fprintf(stdout, "\n");
		fflush(stdout);
	}

	void op_clean() {
		routine->stack.depth -= depth();
	}

	void meta_set(item_t obj, item_t meta) {
		if (obj.type == VECTOR) {
			obj.vec->meta = meta;
			return;
		}

		if (obj.type == MAP) {
			obj.map->meta = meta;
			return;
		}

		if (obj.type == USERDATA) {
			obj.data->meta = meta;
			return;
		}

		char tmp[STRTMP];
		must(false, "cannot set meta on %s", tmptext(obj, tmp, sizeof(tmp)));
	}

	void op_meta_set() {
		item_t meta = pop();
		item_t obj = pop();
		meta_set(obj, meta);
	}

	void op_meta_get() {
		item_t obj = pop();

		if (obj.type == VECTOR) {
			push(obj.vec->meta);
			return;
		}

		if (obj.type == MAP) {
			push(obj.map->meta);
			return;
		}

		if (obj.type == USERDATA) {
			push(obj.data->meta);
			return;
		}

		push(nil());
	}

	void op_map() {
		opush(routine->map);
		routine->map = (item_t){.type = MAP, .map = map_allot()};
	}

	void op_unmap() {
		push(routine->map);
		routine->map = opop();
	}

	void op_mark() {
		cor_t* cor = routine;
		assert(cor->marks.depth < (int)(sizeof(cor->marks.cells)/sizeof(int)));
		cor->marks.cells[cor->marks.depth++] = cor->stack.depth;
	}

	void op_mark2() {
		op_mark();
		op_mark();
	}

	void limit(int count) {
		cor_t* cor = routine;
		assert(cor->marks.depth > 0);
		int old_depth = cor->marks.cells[--cor->marks.depth];
		if (count >= 0) {
			int req_depth = old_depth + count;
			while (req_depth > cor->stack.depth) push(nil());
			cor->stack.depth = req_depth;
		}
	}

	void op_limit() {
		limit(literal_int());
	}

	void op_limit2() {
		limit(-1);
		limit(literal_int());
	}

	void arrive(int ip) {
		cor_t* cor = routine;

		assert(cor->frames.depth < (int)(sizeof(cor->frames.cells)/sizeof(frame_t)));
		frame_t* frame = &cor->frames.cells[cor->frames.depth++];

		frame->loops = cor->loops.depth;
		frame->marks = cor->marks.depth;
		frame->ip = cor->ip;

		frame->locals.depth = 0;
		frame->scope = 0;

		frame->map = cor->map;
		cor->map = nil();

		cor->ip = ip;
	}

	void depart() {
		cor_t* cor = routine;

		assert(cor->frames.depth > 0);
		frame_t* frame = &cor->frames.cells[--cor->frames.depth];

		cor->ip = frame->ip;
		cor->marks.depth = frame->marks;
		cor->loops.depth = frame->loops;

		cor->map = frame->map;
	}

	void op_coroutine() {
		cor_t *cor = cor_allot();

		must(depth() && item(0)->type == SUBROUTINE, "coroutine missing subroutine");

		int ip = item(0)->sub;

		vec_push(&routines, (item_t){.type = COROUTINE, .cor = cor});
		routine = cor;

		cor->state = COR_RUNNING;
		arrive(ip);
		op_mark();

		cor->state = COR_SUSPENDED;

		vec_pop(&routines);
		routine = vec_top(&routines).cor;

		op_clean();

		push((item_t){.type = COROUTINE, .cor = cor});
	}

	void op_resume() {
		must(depth() && item(0)->type == COROUTINE, "resume missing coroutine");
		cor_t *cor = item(0)->cor;

		int items = depth();
		cor_t* caller = routine;

		if (cor->state == COR_DEAD) {
			caller->stack.depth -= items;
			push(nil());
			return;
		}

		cor->state = COR_RUNNING;

		vec_push(&routines, (item_t){.type = COROUTINE, .cor = cor});
		routine = cor;

		for (int i = 1; i < items; i++) {
			int index = caller->stack.depth-items+i;
			push(*stack_ref(caller, index));
		}

		caller->stack.depth -= items;
	}

	void op_yield() {
		int items = depth();

		cor_t* caller = routine;
		caller->state = COR_SUSPENDED;

		vec_pop(&routines);
		routine = vec_top(&routines).cor;

		for (int i = 0; i < items; i++) {
			int index = caller->stack.depth-items+i;
			push(*stack_ref(caller, index));
		}

		caller->stack.depth -= items;
	}

	void op_global() {
		push((item_t){.type = MAP, .map = scope_global});
	}

	void call(item_t item) {
		if (item.type == OPERATION) {
			operation_call(item.opcode);
			return;
		}
		if (item.type == EXECUTE) {
			execute(item.function);
			return;
		}

		int args = depth();

		char tmp[STRTMP];
		must(item.type == SUBROUTINE, "invalid function: %s (ip: %u)", tmptext(item, tmp, sizeof(tmp)), routine->ip);

		arrive(item.sub);

		op_mark();
		// subroutines need to know the base of their subframe,
		// which is the same as the current depth of the outer
		// frame mark
		routine->marks.cells[routine->marks.depth-1] -= args;
	}

	void op_call() {
		call(pop());
	}

	void op_return() {
		cor_t* cor = routine;

		// subroutines leave only results in their subframe, which
		// migrate to the caller frame when depart() truncates the
		// marks stack
		depart();

		if (!cor->ip) {
			cor->state = COR_DEAD;
			op_yield();
			return;
		}
	}

	void op_drop() {
		pop();
	}

	void op_lit() {
		push(literal());
	}

	void op_loop() {
		assert(routine->loops.depth+2 < (int)(sizeof(routine->loops.cells)/sizeof(int)));
		routine->loops.cells[routine->loops.depth++] = routine->marks.depth;
		routine->loops.cells[routine->loops.depth++] = literal_int();
		routine->loops.cells[routine->loops.depth++] = 0;
	}

	void op_unloop() {
		assert(routine->loops.depth > 2);
		--routine->loops.depth;
		--routine->loops.depth;
		must(routine->loops.cells[--routine->loops.depth] == routine->marks.depth, "mark stack mismatch (unloop)");
	}

	void op_break() {
		routine->ip = routine->loops.cells[routine->loops.depth-2];
		routine->marks.depth = routine->loops.cells[routine->loops.depth-3];
		while (depth()) pop();
	}

	void op_continue() {
		routine->ip = routine->loops.cells[routine->loops.depth-2]-1;
		routine->marks.depth = routine->loops.cells[routine->loops.depth-3];
		while (depth()) pop();
	}

	void op_copy() {
		push(top());
	}

	void op_shunt() {
		opush(pop());
	}

	void op_shift() {
		push(opop());
	}

	void op_nil() {
		push(nil());
	}

	void op_true() {
		push((item_t){.type = BOOLEAN, .flag = 1});
	}

	void op_false() {
		push((item_t){.type = BOOLEAN, .flag = 0});
	}

	void op_jmp() {
		routine->ip = literal_int();
	}

	void op_jfalse() {
		if (!truth(top())) op_jmp();
	}

	void op_jtrue() {
		if (truth(top())) op_jmp();
	}

	void op_vector() {
		opush((item_t){.type = VECTOR, .vec = vec_allot()});
	}

	void op_vpush() {
		for (int i = 0, l = depth(); i < l; i++)
			vec_push(otop().vec, *item(i));
		op_clean();
	}

	void op_unpack() {
		vec_t* vec = pop_type(VECTOR).vec;

		for (int i = 0, l = vec_size(vec); i < l; i++)
			push(vec_get(vec, i));
	}

	void op_type() {
		item_t a = pop();
		push(string(type_names[a.type]));
	}

	void op_enter() {
		cor_t* cor = routine;
		frame_t* frame = &cor->frames.top();
		frame->scope = literal_int();
		auto& scope = scopes[frame->scope];
		int i = 0;
		int d = depth();
		int l = scope.locals.size();
		item_t* argv = items();
		while (i < l && i < d) {
			frame->locals.push(argv[i++]);
		}
		while (i++ < l) {
			frame->locals.push(nil());
		}
		op_clean();
	}

	// locate a local variable cell in the current frame
	item_t* local(const char* key) {
		cor_t* cor = routine;

		if (!cor->frames.depth) return nullptr;
		frame_t* frame = &cor->frames.top();

		auto& scope = scopes[frame->scope];
		for (int i = 0, l = scope.locals.size(); i < l; i++)
			if (scope.locals[i] == key) return &frame->locals[i];
		return nullptr;
	}

	// locate a local variable in-scope in an outer frame
	item_t* uplocal(const char* key) {
		cor_t* cor = routine;
		if (cor->frames.depth < 2) return nullptr;

		int index = cor->frames.depth;

		frame_t* lframe = &cor->frames.cells[--index];
		auto& lscope = scopes[lframe->scope];

		if (lscope.up.size()) {
			while (index > 0) {
				frame_t* uframe = &cor->frames.cells[--index];
				auto& uscope = scopes[uframe->scope];
				for (auto id: lscope.up) {
					// only check this call stack frame if it belongs to another
					// function from the current function's compile-time scope chain
					if (uscope.id == id && uscope.id != lscope.id) {
						for (int i = 0, l = uscope.locals.size(); i < l; i++)
							if (uscope.locals[i] == key) return &uframe->locals[i];
					}
				}
			}
		}
		return nullptr;
	}

	void op_lget() {
		cor_t* cor = routine;
		assert(cor->frames.depth);
		frame_t* frame = &cor->frames.top();
		push(frame->locals[literal_int()]);
	}

	void op_lcall() {
		cor_t* cor = routine;
		assert(cor->frames.depth);
		frame_t* frame = &cor->frames.top();
		call(frame->locals[literal_int()]);
	}

	void lset(int index, item_t val) {
		cor_t* cor = routine;
		assert(cor->frames.depth);
		frame_t* frame = &cor->frames.top();
		frame->locals[index] = val;
	}

	void assign(item_t key, item_t val) {
		cor_t* cor = routine;

		// OP_ASSIGN is used for too many things: local variables, map literal keys, global keys
		map_t* map = cor->map.type == MAP ? cor->map.map: nullptr;

		if (!map && cor->frames.depth) {
			if (key.type == INTEGER) {
				lset(key.inum, val);
				return;
			}
			item_t* cell = nullptr;
			if (key.type == STRING && (cell = local(key.str)) && cell) {
				*cell = val;
				return;
			}
		}

		map_set(map ? map: scope_global, key, val);
	}

	item_t* find(item_t key) {
		assert(key.type == STRING);
		item_t* cell = local(key.str);
		if (!cell) cell = uplocal(key.str);
		if (!cell) cell = map_ref(scope_global, key);
		if (!cell) cell = map_ref(scope_core, key);
		return cell;
	}

	void op_assign() {
		item_t key = pop();

		int index = literal_int();
		// indexed from the base of the current subframe
		item_t val = depth() > index ? *item(index): nil();

		assign(key, val);
	}

	void op_find() {
		item_t key = pop();
		item_t* val = find(key);

		char tmp[STRTMP];
		must(val, "unknown name: %s", tmptext(key, tmp, sizeof(tmp)));

		push(*val);
	}

	void op_for() {
		assert(literal().type == VECTOR);

		int var = 0;
		vec_t* vars = literal().vec;
		int varc = vec_size(vars);

		item_t iter = top();
		int step = routine->loops.cells[routine->loops.depth-1];

		if (iter.type == INTEGER) {
			if (step == iter.inum) {
				routine->ip = routine->loops.cells[routine->loops.depth-2];
			}
			else {
				if (varc > 1)
					assign(vars->items[var++], integer(step));
				if (varc > 0)
					assign(vars->items[var++], integer(step));
			}
		}
		else
		if (iter.type == VECTOR) {
			if (step >= (int)vec_size(iter.vec)) {
				routine->ip = routine->loops.cells[routine->loops.depth-2];
			}
			else {
				if (varc > 1)
					assign(vars->items[var++], integer(step));
				if (varc > 0)
					assign(vars->items[var++], vec_get(iter.vec, step));
			}
		}
		else
		if (iter.type == MAP) {
			if (step >= (int)vec_size(&iter.map->keys)) {
				routine->ip = routine->loops.cells[routine->loops.depth-2];
			}
			else {
				if (varc > 1)
					assign(vars->items[var++], vec_get(&iter.map->keys, step));
				if (varc > 0)
					assign(vars->items[var++], vec_get(&iter.map->vals, step));
			}
		}
		else
		if (iter.type == SUBROUTINE || iter.type == EXECUTE) {
			item_t argv[1] = {integer(step)};
			item_t retv[2] = {nil(), nil()};
			method(iter, 1, argv, 2, retv);

			if (retv[0].type == NIL) {
				routine->ip = routine->loops.cells[routine->loops.depth-2];
			}
			else {
				int idx = 0;
				if (varc > 1)
					assign(vars->items[var++], retv[idx++]);
				if (varc > 0)
					assign(vars->items[var++], retv[idx++]);
			}
		}
		else
		if (iter.type == COROUTINE) {
			op_mark();
			push(iter);
			push(integer(step));
			op_resume();
			while (routine == iter.cor && tick());

			if (!depth() || item(0)->type == NIL) {
				routine->ip = routine->loops.cells[routine->loops.depth-2];
			}
			else {
				int idx = 0;
				if (varc > 1)
					assign(vars->items[var++], depth() > idx ? *item(idx++): nil());
				if (varc > 0)
					assign(vars->items[var++], depth() > idx ? *item(idx++): nil());
			}

			limit(0);
		}
		else {
			routine->ip = routine->loops.cells[routine->loops.depth-1];
		}

		routine->loops.cells[routine->loops.depth-1] = step+1;
	}

	void set(item_t dst, item_t key, item_t val) {
		if (dst.type == VECTOR && key.type == INTEGER && key.inum == (int)vec_size(dst.vec)) {
			vec_push(dst.vec, val);
		}
		else
		if (dst.type == VECTOR && key.type == INTEGER) {
			vec_cell(dst.vec, key.inum)[0] = val;
		}
		else
		if (dst.type == MAP) {
			map_set(dst.map, key, val);
		}
		else {
			char tmpA[STRTMP];
			char tmpB[STRTMP];
			must(0, "cannot set %s (%s) in item %s (%s)",
				tmptext(key, tmpA, sizeof(tmpA)), type_names[key.type],
				tmptext(val, tmpB, sizeof(tmpB)), type_names[dst.type]
			);
		}
	}

	void op_set() {
		item_t key = pop();
		item_t dst = pop();

		int index = literal_int();
		item_t val = depth() ? *item(index): nil();
		set(dst, key, val);
	}

	item_t get(item_t src, item_t key) {
		if (src.type == VECTOR && key.type == INTEGER) {
			return vec_get(src.vec, key.inum);
		}
		else
		if (src.type == VECTOR && src.vec->meta.type != NIL && key.type == STRING) {
			item_t val = nil();
			meta_get(src.vec->meta, key.str, &val);
			return val;
		}
		if (src.type == MAP) {
			item_t val = nil();
			map_get(src.map, key, &val);
			if (val.type == NIL && src.map->meta.type != NIL && key.type == STRING) {
				meta_get(src.map->meta, key.str, &val);
			}
			return val;
		}
		else {
			char tmpA[STRTMP];
			char tmpB[STRTMP];
			must(0, "cannot get %s (%s) from item %s (%s)",
				tmptext(key, tmpA, sizeof(tmpA)), type_names[key.type],
				tmptext(src, tmpB, sizeof(tmpB)), type_names[src.type]
			);
		}
		return nil();
	}

	void op_get() {
		item_t* b = stack_cell(-1);
		item_t* a = stack_cell(-2);
		*a = get(*a, *b);
		op_drop();
	}

	void op_add() {
		item_t* b = stack_cell(-1);
		item_t* a = stack_cell(-2);
		*a = add(*a, *b);
		op_drop();
	}

	void op_add_lit() {
		item_t* a = stack_cell(-1);
		*a = add(*a, literal());
	}

	void op_neg() {
		if (top().type == INTEGER) {
			stack_cell(-1)->inum *= -1;
		}
		else
		if (top().type == FLOAT) {
			stack_cell(-1)->fnum *= -1;
		}
		else {
			char tmp[STRTMP];
			must(0, "cannot negate %s", tmptext(top(), tmp, sizeof(tmp)));
		}
	}

	void op_sub() {
		op_neg();
		op_add();
	}

	void op_mul() {
		item_t* b = stack_cell(-1);
		item_t* a = stack_cell(-2);
		*a = multiply(*a, *b);
		op_drop();
	}

	void op_mul_lit() {
		item_t* a = stack_cell(-1);
		*a = multiply(*a, literal());
	}

	void op_div() {
		item_t* b = stack_cell(-1);
		item_t* a = stack_cell(-2);
		*a = divide(*a, *b);
		op_drop();
	}

	void op_mod() {
		item_t* b = stack_cell(-1);
		item_t* a = stack_cell(-2);
		*a = (item_t){.type = INTEGER, .inum = a->inum % b->inum};
		op_drop();
	}

	void op_eq() {
		item_t* b = stack_cell(-1);
		item_t* a = stack_cell(-2);
		*a = (item_t){.type = BOOLEAN, .flag = equal(*a, *b)};
		op_drop();
	}

	void op_not() {
		push((item_t){.type = BOOLEAN, .flag = !truth(pop())});
	}

	void op_ne() {
		op_eq();
		op_not();
	}

	void op_lt() {
		item_t* b = stack_cell(-1);
		item_t* a = stack_cell(-2);
		*a = (item_t){.type = BOOLEAN, .flag = less(*a, *b)};
		op_drop();
	}

	void op_gt() {
		item_t* b = stack_cell(-1);
		item_t* a = stack_cell(-2);
		*a = (item_t){.type = BOOLEAN, .flag = !less(*a, *b) && !equal(*a, *b)};
		op_drop();
	}

	void op_lte() {
		item_t* b = stack_cell(-1);
		item_t* a = stack_cell(-2);
		*a = (item_t){.type = BOOLEAN, .flag = less(*a, *b) || equal(*a, *b)};
		op_drop();
	}

	void op_gte() {
		item_t* b = stack_cell(-1);
		item_t* a = stack_cell(-2);
		*a = (item_t){.type = BOOLEAN, .flag = !less(*a, *b)};
		op_drop();
	}

	void op_concat() {
		item_t b = pop();
		item_t a = pop();
		char tmpA[STRTMP];
		char tmpB[STRTMP];

		const char *as = tmptext(a, tmpA, sizeof(tmpA));
		const char *bs = tmptext(b, tmpB, sizeof(tmpB));
		int lenA = strlen(as);
		int lenB = strlen(bs);

		must(lenA+lenB < STRBUF, "op_concat max length exceeded (%d bytes)", STRBUF-1);

		char buf[STRBUF];
		char* ap = buf+0;
		char* bp = buf+lenA;
		memcpy(ap, as, lenA);
		memcpy(bp, bs, lenB);
		buf[lenA+lenB] = 0;

		push(string(buf));
	}

	void op_count() {
		item_t a = pop();
		push(integer(count(a)));
	}

	void op_acos() { item_t a = pop_type(FLOAT); a.fnum = acos(a.fnum); push(a); }
	void op_asin() { item_t a = pop_type(FLOAT); a.fnum = asin(a.fnum); push(a); }
	void op_atan() { item_t a = pop_type(FLOAT); a.fnum = atan(a.fnum); push(a); }
	void op_cos() { item_t a = pop_type(FLOAT); a.fnum = cos(a.fnum); push(a); }
	void op_sin() { item_t a = pop_type(FLOAT); a.fnum = sin(a.fnum); push(a); }
	void op_tan() { item_t a = pop_type(FLOAT); a.fnum = tan(a.fnum); push(a); }
	void op_cosh() { item_t a = pop_type(FLOAT); a.fnum = cosh(a.fnum); push(a); }
	void op_sinh() { item_t a = pop_type(FLOAT); a.fnum = sinh(a.fnum); push(a); }
	void op_tanh() { item_t a = pop_type(FLOAT); a.fnum = tanh(a.fnum); push(a); }
	void op_ceil() { item_t a = pop_type(FLOAT); a.fnum = ceil(a.fnum); push(a); }
	void op_floor() { item_t a = pop_type(FLOAT); a.fnum = floor(a.fnum); push(a); }
	void op_sqrt() { item_t a = pop_type(FLOAT); a.fnum = sqrt(a.fnum); push(a); }
	void op_log() { item_t a = pop_type(FLOAT); a.fnum = log(a.fnum); push(a); }
	void op_log10() { item_t a = pop_type(FLOAT); a.fnum = log10(a.fnum); push(a); }

	void op_abs() {
		item_t a = pop();
		if (a.type == INTEGER) a.inum = a.inum < 0 ? -a.inum: a.inum;
		else if (a.type == FLOAT) a.fnum = a.fnum < 0.0 ? -a.fnum: a.fnum;
		else must(0, "op_abs invalid type");
		push(a);
	}

	void op_atan2() {
		double y = pop_type(FLOAT).fnum;
		double x = pop_type(FLOAT).fnum;
		push(number(atan2(x,y)));
	}

	void op_pow() {
		double y = pop_type(FLOAT).fnum;
		double x = pop_type(FLOAT).fnum;
		push(number(pow(x,y)));
	}

	void op_min() {
		item_t a = pop();
		while (depth()) {
			item_t b = pop();
			must(a.type == b.type, "op_min mixed types");
			a = less(a, b) ? a: b;
		}
		push(a);
	}

	void op_max() {
		item_t a = pop();
		while (depth()) {
			item_t b = pop();
			must(a.type == b.type, "op_min mixed types");
			a = less(a, b) ? b: a;
		}
		push(a);
	}

	void op_match() {
	#ifdef PCRE
		item_t pattern = pop_type(STRING);
		item_t subject = pop_type(STRING);

		const char *error;
		int erroffset;
		int ovector[STRTMP];
		pcre_extra *extra = nullptr;

		pcre *re = pcre_compile(pattern.str, PCRE_DOTALL|PCRE_UTF8, &error, &erroffset, 0);
		must(re, "pcre_compile: %s", pattern.str);

	#ifdef PCRE_STUDY_JIT_COMPILE
		error = nullptr;
		extra = pcre_study(re, PCRE_STUDY_JIT_COMPILE, &error);
		must(extra && !error, "pcre_study: %s", pattern.str);
	#endif

		int matches = pcre_exec(re, extra, subject.str, strlen(subject.str), 0, 0, ovector, sizeof(ovector));

		if (matches < 0) {
			if (extra)
				pcre_free_study(extra);
			pcre_free(re);
			return;
		}

		if (matches == 0) {
			matches = sizeof(ovector)/3;
		}

		for (int i = 0; i < matches; i++) {
			int offset = ovector[2*i];
			int length = ovector[2*i+1] - offset;
			push(string(substr(subject.str, offset, length)));
		}

		if (extra)
			pcre_free_study(extra);
		pcre_free(re);

	#else
		must(0, "matching not enabled; rebuild with -DPCRE");
	#endif
	}

	void op_sort() {
		item_t a = pop_type(VECTOR);
		if (vec_size(a.vec) > 0) vec_sort(a.vec);
		push(a);
	}

	void op_assert() {
		must(depth() && truth(top()), "assert");
	}

	void op_fname() {
		item_t key = literal();
		item_t* val = find(key);

		char tmp[STRTMP];
		must(val, "unknown name: %s", tmptext(key, tmp, sizeof(tmp)));

		push(*val);
	}

	void op_gname() {
		item_t key = literal();
		item_t src = pop();
		push(get(src, key));
	}

	void op_cfunc() {
		item_t key = literal();
		item_t* val = find(key);

		char tmp[STRTMP];
		must(val, "unknown name: %s", tmptext(key, tmp, sizeof(tmp)));

		call(*val);
	}

	// compression of mark,lit,lit,assign,limit0
	void op_assignp() {
		assign(literal(), pop());
	}

	// lit,assign0
	void op_assignl() {
		// indexed from the base of the current subframe
		assign(literal(), *item(0));
	}

	// dups
	void op_copies() {
		for (int i = 0, l = literal_int(); i < l; i++) push(top());
	}

	// apply op direct to variable
	void op_update() {
		item_t* val = find(literal());

		char tmp[STRTMP];
		must(val, "unknown name: %s", tmptext(literal(), tmp, sizeof(tmp)));

		push(*val); tick(); *val = pop();
	}

	void nop() {
	}

	void operation_call(enum opcode_t opcode) {
		switch (opcode) {
			case OP_STOP:      nop();          return;
			case OP_AND:       nop();          return;
			case OP_OR:        nop();          return;
			case OP_JMP:       op_jmp();       return;
			case OP_FOR:       op_for();       return;
			case OP_ENTER:     op_enter();     return;
			case OP_LIT:       op_lit();       return;
			case OP_MARK:      op_mark();      return;
			case OP_LIMIT:     op_limit();     return;
			case OP_CLEAN:     op_clean();     return;
			case OP_RETURN:    op_return();    return;
			case OP_LGET:      op_lget();      return;
			case OPP_LCALL:    op_lcall();     return;
			case OPP_FNAME:    op_fname();     return;
			case OPP_CFUNC:    op_cfunc();     return;
			case OPP_ASSIGNL:  op_assignl();   return;
			case OPP_ASSIGNP:  op_assignp();   return;
			case OPP_MUL_LIT:  op_mul_lit();   return;
			case OPP_ADD_LIT:  op_add_lit();   return;
			case OPP_GNAME:    op_gname();     return;
			case OPP_COPIES:   op_copies();    return;
			case OPP_UPDATE:   op_update();    return;
			case OPP_MARK2:    op_mark2();     return;
			case OPP_LIMIT2:   op_limit2();    return;
			case OP_PRINT:     op_print();     return;
			case OP_COROUTINE: op_coroutine(); return;
			case OP_RESUME:    op_resume();    return;
			case OP_YIELD:     op_yield();     return;
			case OP_CALL:      op_call();      return;
			case OP_GLOBAL:    op_global();    return;
			case OP_MAP:       op_map();       return;
			case OP_VECTOR:    op_vector();    return;
			case OP_VPUSH:     op_vpush();     return;
			case OP_META_SET:  op_meta_set();  return;
			case OP_META_GET:  op_meta_get();  return;
			case OP_UNMAP:     op_unmap();     return;
			case OP_LOOP:      op_loop();      return;
			case OP_UNLOOP:    op_unloop();    return;
			case OP_BREAK:     op_break();     return;
			case OP_CONTINUE:  op_continue();  return;
			case OP_JFALSE:    op_jfalse();    return;
			case OP_JTRUE:     op_jtrue();     return;
			case OP_NIL:       op_nil();       return;
			case OP_COPY:      op_copy();      return;
			case OP_SHUNT:     op_shunt();     return;
			case OP_SHIFT:     op_shift();     return;
			case OP_TRUE:      op_true();      return;
			case OP_FALSE:     op_false();     return;
			case OP_ASSIGN:    op_assign();    return;
			case OP_FIND:      op_find();      return;
			case OP_SET:       op_set();       return;
			case OP_GET:       op_get();       return;
			case OP_COUNT:     op_count();     return;
			case OP_DROP:      op_drop();      return;
			case OP_ADD:       op_add();       return;
			case OP_NEG:       op_neg();       return;
			case OP_SUB:       op_sub();       return;
			case OP_MUL:       op_mul();       return;
			case OP_DIV:       op_div();       return;
			case OP_MOD:       op_mod();       return;
			case OP_NOT:       op_not();       return;
			case OP_EQ:        op_eq();        return;
			case OP_NE:        op_ne();        return;
			case OP_LT:        op_lt();        return;
			case OP_GT:        op_gt();        return;
			case OP_LTE:       op_lte();       return;
			case OP_GTE:       op_gte();       return;
			case OP_CONCAT:    op_concat();    return;
			case OP_MATCH:     op_match();     return;
			case OP_SORT:      op_sort();      return;
			case OP_ASSERT:    op_assert();    return;
			case OP_SIN:       op_sin();       return;
			case OP_COS:       op_cos();       return;
			case OP_TAN:       op_tan();       return;
			case OP_ASIN:      op_asin();      return;
			case OP_ACOS:      op_acos();      return;
			case OP_ATAN:      op_atan();      return;
			case OP_SINH:      op_sinh();      return;
			case OP_COSH:      op_cosh();      return;
			case OP_TANH:      op_tanh();      return;
			case OP_CEIL:      op_ceil();      return;
			case OP_FLOOR:     op_floor();     return;
			case OP_SQRT:      op_sqrt();      return;
			case OP_ABS:       op_abs();       return;
			case OP_ATAN2:     op_atan2();     return;
			case OP_LOG:       op_log();       return;
			case OP_LOG10:     op_log10();     return;
			case OP_POW:       op_pow();       return;
			case OP_MIN:       op_min();       return;
			case OP_MAX:       op_max();       return;
			case OP_TYPE:      op_type();      return;
			case OP_UNPACK:    op_unpack();    return;
			case OP_GC:        gc();           return;
		}
		must(false, "invalid operation");
	}

	const char* operation_name(enum opcode_t opcode) {
		switch (opcode) {
			case OP_STOP:      return "stop";
			case OP_AND:       return "and";
			case OP_OR:        return "or";
			case OP_JMP:       return "jmp";
			case OP_FOR:       return "for";
			case OP_ENTER:     return "enter";
			case OP_LIT:       return "lit";
			case OP_MARK:      return "mark";
			case OP_LIMIT:     return "limit";
			case OP_CLEAN:     return "clean";
			case OP_RETURN:    return "return";
			case OP_LGET:      return "lget";
			case OPP_LCALL:    return "lcall";
			case OPP_FNAME:    return "fname";
			case OPP_CFUNC:    return "cfunc";
			case OPP_ASSIGNL:  return "assignl";
			case OPP_ASSIGNP:  return "assignp";
			case OPP_MUL_LIT:  return "mul_lit";
			case OPP_ADD_LIT:  return "add_lit";
			case OPP_GNAME:    return "gname";
			case OPP_COPIES:   return "copies";
			case OPP_UPDATE:   return "update";
			case OPP_MARK2:    return "mark2";
			case OPP_LIMIT2:   return "limit2";
			case OP_PRINT:     return "print";
			case OP_COROUTINE: return "coroutine";
			case OP_RESUME:    return "resume";
			case OP_YIELD:     return "yield";
			case OP_CALL:      return "call";
			case OP_GLOBAL:    return "global";
			case OP_MAP:       return "map";
			case OP_VECTOR:    return "vector";
			case OP_VPUSH:     return "vpush";
			case OP_META_SET:  return "meta_set";
			case OP_META_GET:  return "meta_get";
			case OP_UNMAP:     return "unmap";
			case OP_LOOP:      return "loop";
			case OP_UNLOOP:    return "unloop";
			case OP_BREAK:     return "break";
			case OP_CONTINUE:  return "continue";
			case OP_JFALSE:    return "jfalse";
			case OP_JTRUE:     return "jtrue";
			case OP_NIL:       return "nil";
			case OP_COPY:      return "copy";
			case OP_SHUNT:     return "shunt";
			case OP_SHIFT:     return "shift";
			case OP_TRUE:      return "true";
			case OP_FALSE:     return "false";
			case OP_ASSIGN:    return "assign";
			case OP_FIND:      return "find";
			case OP_SET:       return "set";
			case OP_GET:       return "get";
			case OP_COUNT:     return "count";
			case OP_DROP:      return "drop";
			case OP_ADD:       return "add";
			case OP_NEG:       return "neg";
			case OP_SUB:       return "sub";
			case OP_MUL:       return "mul";
			case OP_DIV:       return "div";
			case OP_MOD:       return "mod";
			case OP_NOT:       return "not";
			case OP_EQ:        return "eq";
			case OP_NE:        return "ne";
			case OP_LT:        return "lt";
			case OP_GT:        return "gt";
			case OP_LTE:       return "lte";
			case OP_GTE:       return "gte";
			case OP_CONCAT:    return "concat";
			case OP_MATCH:     return "match";
			case OP_SORT:      return "sort";
			case OP_ASSERT:    return "assert";
			case OP_SIN:       return "sin";
			case OP_COS:       return "cos";
			case OP_TAN:       return "tan";
			case OP_ASIN:      return "asin";
			case OP_ACOS:      return "acos";
			case OP_ATAN:      return "atan";
			case OP_SINH:      return "sinh";
			case OP_COSH:      return "cosh";
			case OP_TANH:      return "tanh";
			case OP_CEIL:      return "ceil";
			case OP_FLOOR:     return "floor";
			case OP_SQRT:      return "sqrt";
			case OP_ABS:       return "abs";
			case OP_ATAN2:     return "atan2";
			case OP_LOG:       return "log";
			case OP_LOG10:     return "log10";
			case OP_POW:       return "pow";
			case OP_MIN:       return "min";
			case OP_MAX:       return "max";
			case OP_TYPE:      return "type";
			case OP_UNPACK:    return "unpack";
			case OP_GC:        return "gc";
			default:           return "(function)";
		}
	}

	void destroy() {
		code.clear();
		scope_core = nullptr;
		reset();

		code.clear();
		routines.items.clear();
		modules.clear();
		stringsA.clear();
		stringsB.clear();
		maps.clear();
		vecs.clear();
		cors.clear();
		data.clear();
	}

	bool tick() {
		int ip = routine->ip++;
		assert(ip >= 0 && ip < (int)code.size());
		auto opcode = code[ip].op;
		operation_call(opcode);
		return opcode != OP_STOP;
	}

	void tick_all() {
		for (;;) {
			int ip = routine->ip++;
			assert(ip >= 0 && ip < (int)code.size());
			auto opcode = code[ip].op;
			switch (opcode) {
				case OP_STOP: { return; }
				case OP_JMP: { op_jmp(); continue; }
				case OP_FOR: { op_for(); continue; }
				case OP_ENTER: { op_enter(); continue; }
				case OP_LIT: { op_lit(); continue; }
				case OP_MARK: { op_mark(); continue; }
				case OP_LIMIT: { op_limit(); continue; }
				case OPP_MARK2: { op_mark2(); continue; }
				case OPP_LIMIT2: { op_limit2(); continue; }
				case OP_CLEAN: { op_clean(); continue; }
				case OP_RETURN: { op_return(); continue; }
				case OP_LGET: { op_lget(); continue; }
				case OPP_LCALL: { op_lcall(); continue; }
				case OPP_CFUNC: { op_cfunc(); continue; }
				case OPP_ASSIGNL: { op_assignl(); continue; }
				case OPP_ASSIGNP: { op_assignp(); continue; }
				case OPP_MUL_LIT: { op_mul_lit(); continue; }
				case OPP_ADD_LIT: { op_add_lit(); continue; }
				default: { operation_call(opcode); continue; }
			}
		}
	}

	void method(item_t func, int argc, item_t* argv, int retc, item_t* retv) {
		must(func.type == SUBROUTINE || func.type == EXECUTE, "invalid method");

		cor_t* cor = routine;
		int frame = cor->frames.depth;

		op_mark();

		for (int i = 0; i < argc; i++) push(argv[i]);

		call(func);

		if (func.type == SUBROUTINE) {
			while (tick()) {
				if (routine != cor) continue;
				if (frame < cor->frames.depth) continue;
				break;
			}
		}

		for (int i = 0; i < retc; i++) {
			retv[i] = i < depth() ? *item(i): nil();
		}

		limit(0);
	}

public:
	Rela() {
		std::string msg;
		try {
			scope_core = map_allot();

			item_t lib = nil();
			if (!map_get(scope_core, string("lib"), &lib)) {
				lib = (item_t){.type = MAP, .map = map_allot()};
				map_set(scope_core, string("lib"), lib);
			}

			map_set(lib.map, string("print"), operation(OP_PRINT));
			map_set(lib.map, string("coroutine"), operation(OP_COROUTINE));
			map_set(lib.map, string("resume"), operation(OP_RESUME));
			map_set(lib.map, string("yield"), operation(OP_YIELD));
			map_set(lib.map, string("setmeta"), operation(OP_META_SET));
			map_set(lib.map, string("getmeta"), operation(OP_META_GET));
			map_set(lib.map, string("sort"), operation(OP_SORT));
			map_set(lib.map, string("assert"), operation(OP_ASSERT));
			map_set(lib.map, string("type"), operation(OP_TYPE));
			map_set(lib.map, string("gc"), operation(OP_GC));
			map_set(lib.map, string("sin"), operation(OP_SIN));
			map_set(lib.map, string("cos"), operation(OP_COS));
			map_set(lib.map, string("tan"), operation(OP_TAN));
			map_set(lib.map, string("asin"), operation(OP_ASIN));
			map_set(lib.map, string("acos"), operation(OP_ACOS));
			map_set(lib.map, string("atan"), operation(OP_ATAN));
			map_set(lib.map, string("cosh"), operation(OP_COSH));
			map_set(lib.map, string("sinh"), operation(OP_SINH));
			map_set(lib.map, string("tanh"), operation(OP_TANH));
			map_set(lib.map, string("ceil"), operation(OP_CEIL));
			map_set(lib.map, string("floor"), operation(OP_FLOOR));
			map_set(lib.map, string("sqrt"), operation(OP_SQRT));
			map_set(lib.map, string("abs"), operation(OP_ABS));
			map_set(lib.map, string("atan2"), operation(OP_ATAN2));
			map_set(lib.map, string("log"), operation(OP_LOG));
			map_set(lib.map, string("log10"), operation(OP_LOG10));
			map_set(lib.map, string("pow"), operation(OP_POW));
			map_set(lib.map, string("min"), operation(OP_MIN));
			map_set(lib.map, string("max"), operation(OP_MAX));

			map_set(scope_core, string("print"), operation(OP_PRINT));

			stringsB.merge(stringsA);
			gc();
		}
		catch (const std::exception& e) {
			msg = e.what();
			fprintf(stderr, "%s (", msg.c_str());
			fprintf(stderr, "ip %d", vec_size(&routines) ? routine->ip: -1);
			fprintf(stderr, ")\n");
		}
		catch (...) {
			msg = "unknown error";
			throw;
		}
	}

	int module(const char* src) {
		vec_push(&routines, (item_t){.type = COROUTINE, .cor = cor_allot()});
		routine = vec_top(&routines).cor;
		op_mark();

		int mod = modules.size();
		modules.push_back(code.size());
		source(src);
		assert(!routine->stack.depth);
		compile(OP_STOP, nil());

		limit(0);
		vec_pop(&routines);
		routine = nullptr;
		nodes.clear();

		stringsB.merge(stringsA);
		gc();

		return mod;
	}

	void decompile() {
		for (auto& c: code) {
			char tmp[STRTMP];
			const char *str = tmptext(c.item, tmp, sizeof(tmp));
			fprintf(stderr, "%04ld  %-10s  %s", &c - code.data(), operation_name(c.op), str);
			if (c.op == OP_ENTER) for (auto& local: scopes[c.item.inum].locals) fprintf(stderr, " l:%s", local);
			if (c.op == OP_ENTER) for (auto& id: scopes[c.item.inum].up) fprintf(stderr, " u:%d", id);
			fprintf(stderr, "\n");
			fflush(stderr);
		}
	}

	virtual ~Rela() {
		destroy();
	}

	int run() {
		return run(modules);
	}

	int run(const std::vector<int>& mods) {
		std::string msg;
		try {
			vec_push(&routines, (item_t){.type = COROUTINE, .cor = cor_allot()});
			routine = vec_top(&routines).cor;
			scope_global = map_allot();

			for (auto mod: mods) {
				must(!routine->frames.depth, "frame exists mod %d\n", mod);
				must(mod < (int)modules.size(), "invalid module %d", mod);
				routine->ip = modules[mod];
				tick_all();
			}
			reset();
			return 0;
		}
		catch (const std::exception& e) {
			msg = e.what();
		}
		catch (...) {
			msg = "unknown error";
		}
		fprintf(stderr, "%s (", msg.c_str());
		fprintf(stderr, "ip %d", vec_size(&routines) ? routine->ip: -1);
		fprintf(stderr, ")\n");
		reset();
		return 1;
	}

	// Opaque type, use functions to access
	typedef struct {
		unsigned char raw[32];
	} oitem;

	virtual void execute(int fid) {
		must(false, "invalid execute");
	}

	void collect() {
		gc();
	}

	int arguments(int limit, oitem* cells) {
		must(routine, "no routine");
		int d = depth();
		for (int i = 0; i < d && i < limit; i++) {
			*((item_t*)&cells[i]) = *item(i);
		}
		return d;
	}

	oitem argument(int index) {
		must(routine, "no routine");
		return stack_pick(index);
	}

	void results(int count, oitem* cells) {
		must(routine, "no routine");
		op_clean();
		for (int i = 0; i < count; i++) {
			push(*((item_t*)&cells[i]));
		}
	}

	void result(oitem res) {
		must(routine, "no routine");
		results(1, &res);
	}

	size_t stack_depth() {
		must(routine, "no routine");
		return depth();
	}

	void stack_push(oitem item) {
		must(routine, "no routine");
		push(*((item_t*)&item));
	}

	oitem stack_pop() {
		must(routine, "no routine");
		oitem opaque;
		*((item_t*)&opaque) = pop();
		return opaque;
	}

	oitem stack_top() {
		must(routine, "no routine");
		oitem opaque;
		*((item_t*)&opaque) = top();
		return opaque;
	}

	oitem stack_pick(int index) {
		must(routine, "no routine");
		oitem opaque;
		*((item_t*)&opaque) = *item(index);
		return opaque;
	}

	oitem smudge(item_t i) {
		oitem opaque;
		*((item_t*)&opaque) = i;
		return opaque;
	}

	item_t polish(oitem o) {
		return *((item_t*)&o);
	}

	oitem make_nil() {
		return smudge(nil());
	}

	oitem make_bool(bool flag) {
		return smudge((item_t){.type = BOOLEAN, .flag = flag});
	}

	oitem make_number(double val) {
		return smudge((item_t){.type = FLOAT, .fnum = val});
	}

	oitem make_integer(int64_t val) {
		return smudge(integer(val));
	}

	oitem make_string(const char* str) {
		return smudge(string(str));
	}

	oitem make_vector() {
		return smudge((item_t){.type = VECTOR, .vec = vec_allot()});
	}

	oitem make_map() {
		return smudge((item_t){.type = MAP, .map = map_allot()});
	}

	oitem make_data(void* ptr) {
		data_t* data = data_allot(); data->ptr = ptr;
		return smudge((item_t){.type = USERDATA, .data = data});
	}

	oitem make_function(int id) {
		return smudge((item_t){.type = EXECUTE, .function = id});
	}

	bool is_nil(oitem opaque) {
		item_t item = polish(opaque);
		return item.type == NIL;
	}

	bool is_bool(oitem opaque) {
		item_t item = polish(opaque);
		return item.type == BOOLEAN;
	}

	bool is_number(oitem opaque) {
		item_t item = polish(opaque);
		return item.type == FLOAT || item.type == INTEGER;
	}

	bool is_integer(oitem opaque) {
		item_t item = polish(opaque);
		return item.type == INTEGER;
	}

	bool is_string(oitem opaque) {
		item_t item = polish(opaque);
		return item.type == STRING;
	}

	bool is_data(oitem opaque) {
		item_t item = polish(opaque);
		return item.type == USERDATA;
	}

	bool is_vector(oitem opaque) {
		item_t item = polish(opaque);
		return item.type == VECTOR;
	}

	bool is_map(oitem opaque) {
		item_t item = polish(opaque);
		return item.type == MAP;
	}

	bool is_true(oitem opaque) {
		item_t item = polish(opaque);
		return truth(item);
	}

	size_t item_count(oitem opaque) {
		item_t item = polish(opaque);
		return count(item);
	}

	oitem vector_get(oitem opaque, int index) {
		item_t item = polish(opaque);
		char tmp[STRTMP];
		must(item.type == VECTOR, "not a vector: %s", tmptext(item, tmp, sizeof(tmp)));
		return smudge(get(item, integer(index)));
	}

	void vector_set(oitem opaque, int index, oitem oval) {
		item_t item = polish(opaque);
		char tmp[STRTMP];
		must(item.type == VECTOR, "not a vector: %s", tmptext(item, tmp, sizeof(tmp)));
		item_t val = *((item_t*)&oval);
		set(item, integer(index), val);
	}

	oitem map_get(oitem opaque, oitem okey) {
		item_t item = polish(opaque);
		char tmp[STRTMP];
		must(item.type == MAP, "not a map: %s", tmptext(item, tmp, sizeof(tmp)));
		item_t key = polish(okey);
		return smudge(get(item, key));
	}

	oitem map_get_named(oitem opaque, const char* field) {
		return map_get(opaque, make_string(field));
	}

	oitem map_key(oitem opaque, int index) {
		item_t item = polish(opaque);
		char tmp[STRTMP];
		must(item.type == MAP, "not a map: %s", tmptext(item, tmp, sizeof(tmp)));
		return smudge(vec_get(&item.map->keys, index));
	}

	void map_set(oitem opaque, oitem okey, oitem oval) {
		item_t item = polish(opaque);
		char tmp[STRTMP];
		must(item.type == MAP, "not a map: %s", tmptext(item, tmp, sizeof(tmp)));
		item_t key = polish(okey);
		item_t val = polish(oval);
		set(item, key, val);
	}

	oitem map_core() {
		return smudge((item_t){.type = MAP, .map = scope_core});
	}

	oitem map_lib() {
		return smudge((item_t){.type = MAP, .map = map_ref(scope_core, string("lib"))->map});
	}

	const char* to_text(oitem opaque, char* tmp, size_t size) {
		item_t item = polish(opaque);
		return tmptext(item, tmp, size);
	}

	bool to_bool(oitem opaque) {
		item_t item = polish(opaque);
		char tmp[STRTMP];
		must(item.type == BOOLEAN, "not a boolean: %s", tmptext(item, tmp, sizeof(tmp)));
		return item.flag;
	}

	double to_number(oitem opaque) {
		item_t item = polish(opaque);
		char tmp[STRTMP];
		must(item.type == FLOAT || item.type == INTEGER, "not a number: %s", tmptext(item, tmp, sizeof(tmp)));
		return item.type == FLOAT ? item.fnum: item.inum;
	}

	int64_t to_integer(oitem opaque) {
		item_t item = polish(opaque);
		char tmp[STRTMP];
		must(item.type == INTEGER, "not an integer: %s", tmptext(item, tmp, sizeof(tmp)));
		return item.inum;
	}

	const char* to_string(oitem opaque) {
		item_t item = polish(opaque);
		char tmp[STRTMP];
		must(item.type == STRING, "not a string: %s", tmptext(item, tmp, sizeof(tmp)));
		return item.str;
	}

	void* to_data(oitem opaque) {
		item_t item = polish(opaque);
		char tmp[STRTMP];
		must(item.type == USERDATA, "not user data: %s", tmptext(item, tmp, sizeof(tmp)));
		return item.data->ptr;
	}

	void meta_set(oitem data, oitem meta) {
		item_t ditem = polish(data);
		item_t mitem = polish(meta);
		meta_set(ditem, mitem);
	}

	#undef must
};
