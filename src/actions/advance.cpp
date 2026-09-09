#include "../action.h"
#include "../agent.h"

class AdvanceAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		return ctx.self && ctx.self->get_is_alive();
	}

	float score(const ActionContext &ctx) const override {
		if (!ctx.self) {
			return 0.0f;
		}
		// Score higher when tactical state is ATTACK
		if (ctx.self->get_tactical_state() == TacticalState::ATTACK) {
			return 0.8f;
		}
		return 0.3f;
	}

	float cost(const ActionContext &ctx) const override {
		return 0.4f;
	}

	void execute(Agent *agent, const ActionContext &ctx) const override {
		agent->set_goal(AgentGoal::ATTACK);
		// Move toward nearest known enemy
		if (ctx.world) {
			const PerceptionState &perc = agent->get_perception_state();
			if (perc.known_targets.size() > 0) {
				agent->set_move_target(perc.known_targets[0].last_known_position);
			}
		}
	}

	const char *get_name() const override { return "Advance"; }
};

Action *create_advance_action() {
	return new AdvanceAction();
}
