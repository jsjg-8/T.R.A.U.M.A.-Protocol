#include "../action.h"
#include "../agent.h"

class FlankAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		return ctx.self && ctx.self->get_is_alive();
	}

	float score(const ActionContext &ctx) const override {
		if (!ctx.self || !ctx.world) {
			return 0.0f;
		}
		// Score higher when multiple squadmates are attacking
		int32_t attackers = 0;
		SquadId my_squad = ctx.self->get_squad_id();
		if (my_squad == INVALID_SQUAD_ID) {
			return 0.0f;
		}
		for (int i = 0; i < ctx.world->agents.size(); i++) {
			if (ctx.world->agents[i].squad_id == my_squad &&
				ctx.world->agents[i].tactical_state == TacticalState::ATTACK) {
				attackers++;
			}
		}
		if (attackers >= 2) {
			return 0.7f;
		}
		return 0.1f;
	}

	float cost(const ActionContext &ctx) const override {
		return 0.6f;
	}

	void execute(Agent *agent, const ActionContext &ctx) const override {
		agent->set_goal(AgentGoal::ATTACK);
		// Move to side of enemy (simplified: offset perpendicular to enemy direction)
		if (ctx.world) {
			const PerceptionState &perc = agent->get_perception_state();
			if (perc.known_targets.size() > 0) {
				Vector3 enemy_pos = perc.known_targets[0].last_known_position;
				Vector3 to_enemy = enemy_pos - agent->get_position();
				// Perpendicular offset
				Vector3 perpendicular = Vector3(-to_enemy.z, 0, to_enemy.x).normalized();
				agent->set_move_target(enemy_pos + perpendicular * 5.0f);
			}
		}
	}

	const char *get_name() const override { return "Flank"; }
};

Action *create_flank_action() {
	return new FlankAction();
}
