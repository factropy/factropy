#include "common.h"
#include "path.h"
#include <limits>

// DEPRECATED see route.h
// A* (like) path-finder.

void Path::tick() {
	for (Path* job: queue.recv_all()) {
		jobs.push_back(job);
	}

	for (int i = 0; jobs.size() && (i < 100 || !stop); i++) {
		Path* job = jobs.front();

		if (job->cancel) {
			jobs.pop_front();
			delete job;
			continue;
		}

		job->update();

		if (job->done) {
			jobs.pop_front();
		}
	}
}

Path::Path() {
}

Path::~Path() {
}

void Path::submit() {
	Node* node = getNode(origin);
	node->gScore = 0;
	node->fScore = calcCost(origin, target);
	toOpenSet(node);
	queue.send(this);
}

Path::Node* Path::getNode(Point point) {
	double inf = std::numeric_limits<double>::infinity();

	if (nodes.count(point) == 0) {
		Node* node = &pool.request();
		node->point = point;
		node->gScore = inf;
		node->fScore = inf;
		node->cameFrom = NULL;
		nodes[point] = node;
	}

	return nodes[point];
}

bool Path::inOpenSet(Path::Node* node) {
	return node->set == 1;
}

void Path::toOpenSet(Path::Node* node) {
	node->set = 1;
	opens[node->point] = node;
}

bool Path::inClosedSet(Path::Node* node) {
	return node->set == 2;
}

void Path::toClosedSet(Path::Node* node) {
	opens.erase(node->point);
	node->set = 2;
}

void Path::update() {
	Node* current = candidate;
	candidate = NULL;

	if (!current) {
		for (auto pair: opens) {
			Node *check = pair.second;
			if (!current || check->fScore < current->fScore) {
				candidate = current;
				current = check;
			}
		}
	}

	// not possible?
	if (!current) {
		done = true;
		success = false;
		result.clear();
		return;
	}

	// arrived?
	if (current->point == target) {
		done = true;
		success = true;

		std::vector<Point> selected;
		while (current->cameFrom) {
			selected.push_back(current->point);
			current = current->cameFrom;
		}

		// reverse point order
		for (size_t i = 0, l = selected.size(); i < l; i++) {
			result.push_back(selected.back());
			selected.pop_back();
		}

		if (result.size() > 2) {
			// http://theory.stanford.edu/~amitp/GameProgramming/MapRepresentations.html#path-smoothing
			// Longer distance smoothing at the end. Unlike theta smoothing below, this
			// smoothing runs only on nodes in the final chosen path, so we can use the
			// ray caster over longer distances without a huge imapct.
			for (size_t i = 1, l = result.size()-2; i < l; i++) {
				// check if the second follower is in sight
				Point a = result[i];
				Point b = result[i+2];
				if (rayCast(a, b)) {
					// if so, remove the first follower
					result.erase(result.begin()+i+1);
					i--;
					l--;
				}
			}
		}

		return;
	}

	toClosedSet(current);

	for (Point point: getNeighbours(current->point)) {
		Node* neighbour = getNode(point);

		if (!inClosedSet(neighbour)) {
			double relCost = calcCost(current->point, neighbour->point);
			double gScoreTentative = current->gScore + relCost;

			if (!inOpenSet(neighbour) || gScoreTentative < neighbour->gScore) {
				neighbour->cameFrom = current;

				// http://theory.stanford.edu/~amitp/GameProgramming/Variations.html#theta
				// Short distance smoothing on the fly. Since the rayCast callback (in the
				// case of vehicles) needs to do a spatial query for entities, it's cheap
				// but not *that* cheap. Since theta smoothing runs on every node added to
				// the open set keep the ray casting to a short distance.
				while (neighbour->cameFrom->cameFrom != NULL
					&& neighbour->point.distance(neighbour->cameFrom->cameFrom->point) < 8
					&& rayCast(neighbour->point, neighbour->cameFrom->cameFrom->point)) {

					neighbour->cameFrom = neighbour->cameFrom->cameFrom;
				}

				neighbour->gScore = gScoreTentative;
				double endCost = calcHeuristic(neighbour->point);
				neighbour->fScore = gScoreTentative + endCost;
				toOpenSet(neighbour);

				// `current` had the lowest fScore this pass. Therefore any neighbour that
				// has a lower fScore is probably a good candidate for the next pass. Each
				// time we cache a candidate we avoid an extra O(n) pass over the open set.
				if ((candidate == NULL && neighbour->fScore < current->fScore)
					|| (candidate != NULL && neighbour->fScore < candidate->fScore)) {

					candidate = neighbour;
				}
			}
		}
	}
}