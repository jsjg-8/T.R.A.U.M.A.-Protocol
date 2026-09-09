#include "../action.h"
#include "../agent.h"

class RetreatAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		return ctx.self && ctx.self->get_is_alive();
	}

	float score(const ActionContext &ctx) const override {
		if (!ctx.self) {
			return 0.0f;
		}
		float health_pct = ctx.self->get_health() / ctx.self->get_max_health();
		if (health_pct < 0.3f) {
			return 0.95f;
		}
		if (health_pct < 0.5f) {
			return 0.6f;
		}
		return 0.1f;
	}

	float cost(const ActionContext &ctx) const override {
		return 0.15f;
	}

	void execute(Agent *agent, const ActionContext &ctx) const override {
		agent->set_goal(AgentGoal::RETREAT);
		// Move away from nearest known enemy
		if (ctx.world) {
			const PerceptionState &perc = agent->get_perception_state();
			if (perc.known_targets.size() > 0) {
				Vector3 enemy_pos = perc.known_targets[0].last_known_position;
				Vector3 away = agent->get_position() - enemy_pos;
				away.normalize();
				agent->set_move_target(agent->get_position() + away * 10.0f);
			}
		}
	}

	const char *get_name() const override { return "Retreat"; }
};

Action *create_retreat_action() {
	return new RetreatAction();
}
