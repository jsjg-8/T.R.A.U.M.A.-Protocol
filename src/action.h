#pragma once

#include "world_state.h"

using namespace godot;

class Agent;

// Context passed to action scoring
struct ActionContext {
	const WorldState *world = nullptr;
	const Agent *self = nullptr;
	float delta = 0.0f;
};

// Base class for all tactical actions
class Action {
public:
	virtual ~Action() = default;

	// Can this action be executed right now?
	virtual bool can_execute(const ActionContext &ctx) const = 0;

	// How desirable is this action? (0-1)
	virtual float score(const ActionContext &ctx) const = 0;

	// Cost of this action (0-1) — higher = more resource-intensive
	virtual float cost(const ActionContext &ctx) const = 0;

	// Execute the action — mutate agent state
	virtual void execute(Agent *agent, const ActionContext &ctx) const = 0;

	// Name for debug
	virtual const char *get_name() const = 0;

	// Utility: score - cost
	float utility(const ActionContext &ctx) const {
		return score(ctx) - cost(ctx);
	}
};

// Factory functions for concrete actions
Action *create_take_cover_action();
Action *create_advance_action();
Action *create_retreat_action();
Action *create_flank_action();
Action *create_search_action();
Action *create_suppress_action();
