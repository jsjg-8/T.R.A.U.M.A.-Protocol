#include "../action.h"
#include "../agent.h"

class TakeCoverAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		return ctx.self && ctx.self->get_is_alive();
	}

	float score(const ActionContext &ctx) const override {
		if (!ctx.self) {
			return 0.0f;
		}
		float health_pct = ctx.self->get_health() / ctx.self->get_max_health();
		if (health_pct < 0.5f) {
			return 0.9f;
		}
		if (health_pct < 0.8f) {
			return 0.5f;
		}
		return 0.2f;
	}

	float cost(const ActionContext &ctx) const override {
		return 0.1f;
	}

	void execute(Agent *agent, const ActionContext &ctx) const override {
		agent->set_goal(AgentGoal::DEFEND);
		agent->clear_move_target();
	}

	const char *get_name() const override { return "TakeCover"; }
};

Action *create_take_cover_action() {
	return new TakeCoverAction();
}
