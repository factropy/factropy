#pragma once

// DEPRECATED see route.h
// A* (like) path-finder.

#include "point.h"
#include "slabpool.h"
#include "channel.h"
#include <map>
#include <list>
#include <vector>
#include <atomic>

struct Path {
	// pathing requests
	static inline channel<Path*,-1> queue;
	static inline std::list<Path*> jobs;
	static inline std::atomic<bool> stop = {false};

	// job updates per tick
	static void tick();

	struct Node {
		Point point = {0,0,0};
		double gScore = 0.0;
		double fScore = 0.0;
		Node* cameFrom = nullptr;
		int set = 0;
	};

	slabpool<Node> pool;

	Point origin = {0,0,0};
	Point target = {0,0,0};
	Node* candidate = nullptr;
	std::map<Point,Node*> nodes;
	std::map<Point,Node*> opens;

	bool done = false;
	bool success = false;
	bool cancel = false;
	std::vector<Point> result;

	Path();
	virtual ~Path();
	void submit();
	Node* getNode(Point point);
	void update();

	bool inOpenSet(Node*);
	void toOpenSet(Node*);
	bool inClosedSet(Node*);
	void toClosedSet(Node*);

	virtual std::vector<Point> getNeighbours(Point) = 0;
	virtual double calcCost(Point,Point) = 0;
	virtual double calcHeuristic(Point) = 0;
	virtual bool rayCast(Point,Point) = 0;
};
