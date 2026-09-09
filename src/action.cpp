#include "action.h"
#include "agent.h"

#include <godot_cpp/variant/vector3.hpp>

using namespace godot;

// Helpers — all internal, operate on public Agent API only.

static bool has_known_hostile(const Agent *self) {
	const PerceptionState &perc = self->get_perception_state();
	for (int i = 0; i < perc.known_targets.size(); i++) {
		if (perc.known_targets[i].is_hostile && perc.known_targets[i].confidence > 0.0f) {
			return true;
		}
	}
	return false;
}

static const PerceptionRecord *best_target(const Agent *self) {
	const PerceptionState &perc = self->get_perception_state();
	const PerceptionRecord *best = nullptr;
	for (int i = 0; i < perc.known_targets.size(); i++) {
		const PerceptionRecord &rec = perc.known_targets[i];
		if (rec.is_hostile && (!best || rec.confidence > best->confidence)) {
			best = &rec;
		}
	}
	return best;
}

static float health_fraction(const Agent *self) {
	float max_h = self->get_max_health();
	if (max_h <= 0.0f) {
		return 0.0f;
	}
	return self->get_health() / max_h;
}

// TakeCover — hold position, go defensive. Always available, desirable under threat.
class TakeCoverAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		return ctx.self != nullptr && ctx.self->get_is_alive();
	}
	float score(const ActionContext &ctx) const override {
		if (has_known_hostile(ctx.self)) {
			return 0.7f;
		}
		return 0.1f;
	}
	float cost(const ActionContext &ctx) const override {
		(void)ctx;
		return 0.1f;
	}
	void execute(Agent *agent, const ActionContext &ctx) const override {
		(void)ctx;
		agent->clear_move_target();
		agent->set_goal(AgentGoal::DEFEND);
	}
	const char *get_name() const override { return "TakeCover"; }
};

// Advance — push toward best known threat.
class AdvanceAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		return ctx.self != nullptr && ctx.self->get_is_alive() && best_target(ctx.self) != nullptr;
	}
	float score(const ActionContext &ctx) const override {
		if (ctx.self->get_tactical_state() == TacticalState::ATTACK) {
			return 0.6f;
		}
		return 0.3f;
	}
	float cost(const ActionContext &ctx) const override {
		(void)ctx;
		return 0.3f;
	}
	void execute(Agent *agent, const ActionContext &ctx) const override {
		(void)ctx;
		const PerceptionRecord *tgt = best_target(agent);
		if (tgt) {
			agent->set_move_target(tgt->last_known_position);
			agent->set_goal(AgentGoal::ATTACK);
		}
	}
	const char *get_name() const override { return "Advance"; }
};

// Retreat — break contact when hurt.
class RetreatAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		return ctx.self != nullptr && ctx.self->get_is_alive() && health_fraction(ctx.self) < 0.5f;
	}
	float score(const ActionContext &ctx) const override {
		return 1.0f - health_fraction(ctx.self);
	}
	float cost(const ActionContext &ctx) const override {
		(void)ctx;
		return 0.2f;
	}
	void execute(Agent *agent, const ActionContext &ctx) const override {
		(void)ctx;
		Vector3 away = agent->get_position();
		const PerceptionRecord *tgt = best_target(agent);
		if (tgt) {
			Vector3 dir = agent->get_position() - tgt->last_known_position;
			if (dir.length() > 0.01f) {
				dir.normalize();
				away = agent->get_position() + dir * 10.0f;
			}
		}
		agent->set_move_target(away);
		agent->set_goal(AgentGoal::RETREAT);
	}
	const char *get_name() const override { return "Retreat"; }
};

// Flank — wide route to best known threat. Preferred while squad attacks.
class FlankAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		const PerceptionRecord *tgt = ctx.self ? best_target(ctx.self) : nullptr;
		return ctx.self != nullptr && ctx.self->get_is_alive() && tgt != nullptr && tgt->confidence > 0.5f;
	}
	float score(const ActionContext &ctx) const override {
		if (ctx.self->get_tactical_state() == TacticalState::ATTACK) {
			return 0.8f;
		}
		return 0.3f;
	}
	float cost(const ActionContext &ctx) const override {
		(void)ctx;
		return 0.5f;
	}
	void execute(Agent *agent, const ActionContext &ctx) const override {
		(void)ctx;
		const PerceptionRecord *tgt = best_target(agent);
		if (tgt) {
			Vector3 to_target = tgt->last_known_position - agent->get_position();
			Vector3 side = Vector3(-to_target.z, 0.0f, to_target.x);
			if (side.length() > 0.01f) {
				side.normalize();
				agent->set_move_target(tgt->last_known_position + side * 6.0f);
			} else {
				agent->set_move_target(tgt->last_known_position);
			}
			agent->set_goal(AgentGoal::ATTACK);
		}
	}
	const char *get_name() const override { return "Flank"; }
};

// Search — investigate hearing event or stale contact.
class SearchAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		if (ctx.self == nullptr || !ctx.self->get_is_alive()) {
			return false;
		}
		return ctx.self->get_perception_state().has_hearing_event || best_target(ctx.self) != nullptr;
	}
	float score(const ActionContext &ctx) const override {
		if (ctx.self->get_perception_state().has_hearing_event) {
			return 0.7f;
		}
		return 0.2f;
	}
	float cost(const ActionContext &ctx) const override {
		(void)ctx;
		return 0.3f;
	}
	void execute(Agent *agent, const ActionContext &ctx) const override {
		(void)ctx;
		const PerceptionState &perc = agent->get_perception_state();
		if (perc.has_hearing_event) {
			agent->set_move_target(perc.hearing_event_origin);
		} else {
			const PerceptionRecord *tgt = best_target(agent);
			if (tgt) {
				agent->set_move_target(tgt->last_known_position);
			}
		}
		agent->set_goal(AgentGoal::INVESTIGATE);
	}
	const char *get_name() const override { return "Search"; }
};

// Suppress — hold and engage a solid contact.
class SuppressAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		const PerceptionRecord *tgt = ctx.self ? best_target(ctx.self) : nullptr;
		return ctx.self != nullptr && ctx.self->get_is_alive() && tgt != nullptr && tgt->confidence > 0.7f;
	}
	float score(const ActionContext &ctx) const override {
		(void)ctx;
		return 0.6f;
	}
	float cost(const ActionContext &ctx) const override {
		(void)ctx;
		return 0.4f;
	}
	void execute(Agent *agent, const ActionContext &ctx) const override {
		(void)ctx;
		agent->clear_move_target();
		agent->set_goal(AgentGoal::ATTACK);
	}
	const char *get_name() const override { return "Suppress"; }
};

// Factories — function-local statics, never deleted, no lifecycle issues.
Action *create_take_cover_action() {
	static TakeCoverAction instance;
	return &instance;
}

Action *create_advance_action() {
	static AdvanceAction instance;
	return &instance;
}

Action *create_retreat_action() {
	static RetreatAction instance;
	return &instance;
}

Action *create_flank_action() {
	static FlankAction instance;
	return &instance;
}

Action *create_search_action() {
	static SearchAction instance;
	return &instance;
}

Action *create_suppress_action() {
	static SuppressAction instance;
	return &instance;
}
