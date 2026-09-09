#include "../action.h"
#include "../agent.h"

class SuppressAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		return ctx.self && ctx.self->get_is_alive();
	}

	float score(const ActionContext &ctx) const override {
		if (!ctx.self) {
			return 0.0f;
		}
		// Score higher when enemy is low health (pinned)
		const PerceptionState &perc = ctx.self->get_perception_state();
		if (perc.known_targets.size() > 0) {
			// Check if any enemy in world state has low health
			if (ctx.world) {
				for (int i = 0; i < ctx.world->agents.size(); i++) {
					if (ctx.world->agents[i].id == perc.known_targets[0].target_id) {
						float enemy_hp = ctx.world->agents[i].health / ctx.world->agents[i].max_health;
						if (enemy_hp < 0.3f) {
							return 0.8f;
						}
					}
				}
			}
			return 0.4f;
		}
		return 0.0f;
	}

	float cost(const ActionContext &ctx) const override {
		return 0.35f;
	}

	void execute(Agent *agent, const ActionContext &ctx) const override {
		agent->set_goal(AgentGoal::DEFEND);
		// Stay in cover and suppress — POC: just hold position
		agent->clear_move_target();
	}

	const char *get_name() const override { return "Suppress"; }
};

Action *create_suppress_action() {
	return new SuppressAction();
}
