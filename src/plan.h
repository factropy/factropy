#pragma once

struct Plan;

// A plan is a collection of entities including layout and configuration.
// A temporary base blueprint.

#include "gui-entity.h"

struct Plan {
	static inline uint sequence = 0;

	uint id = 0;
	Point position;
	std::vector<GuiFakeEntity*> entities;
	std::vector<Point> offsets;
	bool config;

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
	bool canRaise();
	void raise();
	bool canLower();
	void lower();
	Spec* canUpward();
	void upward();
	Spec* canDownward();
	void downward();
	bool fits();
	bool conforms();
	bool entityFits(Spec *spec, Point pos, Point dir);
	GuiFakeEntity* central();
};
