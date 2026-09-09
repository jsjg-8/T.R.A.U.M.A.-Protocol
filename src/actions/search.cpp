#include "../action.h"
#include "../agent.h"

class SearchAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		return ctx.self && ctx.self->get_is_alive();
	}

	float score(const ActionContext &ctx) const override {
		if (!ctx.self) {
			return 0.0f;
		}
		// Score higher when hearing event but no visual contact
		const PerceptionState &perc = ctx.self->get_perception_state();
		if (perc.has_hearing_event && perc.known_targets.size() == 0) {
			return 0.7f;
		}
		if (perc.has_hearing_event) {
			return 0.4f;
		}
		return 0.05f;
	}

	float cost(const ActionContext &ctx) const override {
		return 0.3f;
	}

	void execute(Agent *agent, const ActionContext &ctx) const override {
		agent->set_goal(AgentGoal::INVESTIGATE);
		// Move to hearing event origin
		const PerceptionState &perc = agent->get_perception_state();
		if (perc.has_hearing_event) {
			agent->set_move_target(perc.hearing_event_origin);
		}
	}

	const char *get_name() const override { return "Search"; }
};

Action *create_search_action() {
	return new SearchAction();
}
