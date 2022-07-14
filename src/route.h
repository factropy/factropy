#pragma once

// A* (like) path-finder.

#include <map>
#include <deque>
#include <vector>
#include <atomic>

template <class CELL>
struct Route {
	CELL origin;
	CELL target;

	virtual std::vector<CELL> getNeighbours(CELL) = 0;
	virtual double calcCost(CELL,CELL) = 0;
	virtual double calcHeuristic(CELL) = 0;
	virtual bool rayCast(CELL,CELL) = 0;
	virtual ~Route() {};

	Route() = default;

	void init(CELL o, CELL t) {
		origin = o;
		target = t;
		auto node = getNode(origin);
		node->gScore = 0;
//		node->fScore = calcCost(origin, target);
		node->fScore = calcHeuristic(origin);
		toOpenSet(node);
	}

	std::vector<CELL> result;

	struct Node {
		CELL cell;
		double gScore = 0.0;
		double fScore = 0.0;
		Node* cameFrom = nullptr;
		int set = 0;
	};

	std::deque<Node> pool;

	Node* candidate = nullptr;
	std::map<CELL,Node*> nodes;
	std::map<CELL,Node*> opens;

	bool success = false;
	bool done = false;

	Node* getNode(CELL cell) {
		double inf = std::numeric_limits<double>::infinity();

		if (nodes.count(cell) == 0) {
			Node* node = &pool.emplace_back();
			node->cell = cell;
			node->gScore = inf;
			node->fScore = inf;
			node->cameFrom = nullptr;
			nodes[cell] = node;
		}

		return nodes[cell];
	}

	bool inOpenSet(Node* node) {
		return node->set == 1;
	}

	void toOpenSet(Node* node) {
		node->set = 1;
		opens[node->cell] = node;
	}

	bool inClosedSet(Node* node) {
		return node->set == 2;
	}

	void toClosedSet(Node* node) {
		opens.erase(node->cell);
		node->set = 2;
	}

	void update() {
		Node* current = candidate;
		candidate = nullptr;

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
			result.clear();
			success = false;
			done = true;
			return;
		}

		// arrived?
		if (current->cell == target) {

			std::vector<CELL> selected;
			while (current->cameFrom) {
				selected.push_back(current->cell);
				current = current->cameFrom;
			}

			// reverse cell order
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
					CELL a = result[i];
					CELL b = result[i+2];
					if (rayCast(a, b)) {
						// if so, remove the first follower
						result.erase(result.begin()+i+1);
						i--;
						l--;
					}
				}
			}

			success = true;
			done = true;
			return;
		}

		toClosedSet(current);

		for (CELL cell: getNeighbours(current->cell)) {
			Node* neighbour = getNode(cell);

			if (!inClosedSet(neighbour)) {
				double relCost = calcCost(current->cell, neighbour->cell);
				double gScoreTentative = current->gScore + relCost;

				if (!inOpenSet(neighbour) || gScoreTentative < neighbour->gScore) {
					neighbour->cameFrom = current;

					// http://theory.stanford.edu/~amitp/GameProgramming/Variations.html#theta
					// Short distance smoothing on the fly. Since the rayCast callback (in the
					// case of vehicles) needs to do a spatial query for entities, it's cheap
					// but not *that* cheap. Since theta smoothing runs on every node added to
					// the open set keep the ray casting to a short distance.
					while (neighbour->cameFrom->cameFrom != nullptr
						&& rayCast(neighbour->cell, neighbour->cameFrom->cameFrom->cell)) {
						neighbour->cameFrom = neighbour->cameFrom->cameFrom;
					}

					neighbour->gScore = gScoreTentative;
					double endCost = calcHeuristic(neighbour->cell);
					neighbour->fScore = gScoreTentative + endCost;
					toOpenSet(neighbour);

					// `current` had the lowest fScore this pass. Therefore any neighbour that
					// has a lower fScore is probably a good candidate for the next pass. Each
					// time we cache a candidate we avoid an extra O(n) pass over the open set.
					if ((candidate == nullptr && neighbour->fScore < current->fScore)
						|| (candidate != nullptr && neighbour->fScore < candidate->fScore)) {
						candidate = neighbour;
					}
				}
			}
		}
	}
};

