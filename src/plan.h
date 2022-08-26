#pragma once

struct Plan;

// A plan is a blueprint of entities including layout and configuration.
// A single-entity pipette is a blueprint without configuration.

#include "gui-entity.h"
#include <set>
#include <string>
#include <vector>

struct Plan {
	static inline uint sequence = 0;

	static void reset();
	static void gc();
	static void saveAll();
	static void loadAll();

	static inline std::set<Plan*> all;

	uint id = 0;
	std::string title;
	Point position;
	std::vector<GuiFakeEntity*> entities;
	std::vector<Point> offsets;
	bool config; // pipette vs blueprint
	bool save; // user requests save

	static inline Plan* clipboard = nullptr;
	static Plan* find(uint id);
	std::set<std::string> tags;

	// last time this plan was pasted.
	// used to avoid double-up and do belt snapping
	struct {
		uint64_t stamp = 0;
		Spec* spec = nullptr;
		Point pos;
		Point dir;
	} last;

	Plan();
	Plan(Point p);
	~Plan();

	void add(GuiFakeEntity* ge);
	void move(Point p);
	void move(Monorail& monorail);
	bool canRotate();
	void rotate();
	bool canCycle();
	void cycle();
	Spec* canFollow();
	void follow();
	void placed(Spec* spec, Point pos, Point dir);
	void floor(float level);
	Spec* canUpward();
	void upward();
	Spec* canDownward();
	void downward();
	bool fits();
	bool conforms();
	bool entityFits(Spec *spec, Point pos, Point dir);
	GuiFakeEntity* central();
};
