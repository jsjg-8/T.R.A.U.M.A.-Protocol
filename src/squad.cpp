#include "squad.h"
#include "agent.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void Squad::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_squad_id", "id"), &Squad::set_squad_id);
	ClassDB::bind_method(D_METHOD("get_squad_id"), &Squad::get_squad_id);

	ClassDB::bind_method(D_METHOD("set_faction_id", "id"), &Squad::set_faction_id);
	ClassDB::bind_method(D_METHOD("get_faction_id"), &Squad::get_faction_id);

	ClassDB::bind_method(D_METHOD("add_member", "id"), &Squad::add_member);
	ClassDB::bind_method(D_METHOD("remove_member", "id"), &Squad::remove_member);
	ClassDB::bind_method(D_METHOD("get_member_count"), &Squad::get_member_count);
	ClassDB::bind_method(D_METHOD("set_leader", "id"), &Squad::set_leader);
	ClassDB::bind_method(D_METHOD("get_leader_id"), &Squad::get_leader_id);

	ClassDB::bind_method(D_METHOD("set_objective_room", "room"), &Squad::set_objective_room);
	ClassDB::bind_method(D_METHOD("get_objective_room"), &Squad::get_objective_room);

	ClassDB::bind_method(D_METHOD("set_engagement_range", "range"), &Squad::set_engagement_range);
	ClassDB::bind_method(D_METHOD("get_engagement_range"), &Squad::get_engagement_range);
	ClassDB::bind_method(D_METHOD("set_retreat_health_threshold", "threshold"), &Squad::set_retreat_health_threshold);
	ClassDB::bind_method(D_METHOD("get_retreat_health_threshold"), &Squad::get_retreat_health_threshold);

	// aggregate_perceptions, evaluate_tactical_state, issue_orders, update — pure C++ internal

	ClassDB::add_property("Squad", PropertyInfo(Variant::INT, "squad_id"), "set_squad_id", "get_squad_id");
	ClassDB::add_property("Squad", PropertyInfo(Variant::INT, "faction_id"), "set_faction_id", "get_faction_id");
	ClassDB::add_property("Squad", PropertyInfo(Variant::FLOAT, "engagement_range", PROPERTY_HINT_RANGE, "5.0,100.0,1.0"), "set_engagement_range", "get_engagement_range");
}

Squad::Squad() {
}

Squad::~Squad() {
}

void Squad::_ready() {
}

void Squad::update(double delta, const WorldState &world) {
	aggregate_perceptions(world);
	evaluate_tactical_state(world);
	issue_orders(const_cast<WorldState &>(world));
}

void Squad::set_squad_id(SquadId id) { squad_id = id; }
SquadId Squad::get_squad_id() const { return squad_id; }

void Squad::set_faction_id(FactionId id) { faction_id = id; }
FactionId Squad::get_faction_id() const { return faction_id; }

void Squad::add_member(AgentId id) {
	for (int i = 0; i < member_ids.size(); i++) {
		if (member_ids[i] == id) {
			return;
		}
	}
	member_ids.push_back(id);
}

void Squad::remove_member(AgentId id) {
	for (int i = member_ids.size() - 1; i >= 0; i--) {
		if (member_ids[i] == id) {
			member_ids.erase(member_ids.begin() + i);
			return;
		}
	}
}

int32_t Squad::get_member_count() const {
	return member_ids.size();
}

const std::vector<AgentId> &Squad::get_member_ids() const {
	return member_ids;
}

void Squad::set_leader(AgentId id) {
	leader_id = id;
}

AgentId Squad::get_leader_id() const {
	return leader_id;
}

void Squad::set_tactical_state(TacticalState state) {
	tactical_state = state;
}

TacticalState Squad::get_tactical_state() const {
	return tactical_state;
}

void Squad::set_objective_room(RoomId room) {
	objective_room = room;
}

RoomId Squad::get_objective_room() const {
	return objective_room;
}

void Squad::set_engagement_range(float range) {
	engagement_range = range;
}

float Squad::get_engagement_range() const {
	return engagement_range;
}

void Squad::set_retreat_health_threshold(float threshold) {
	retreat_health_threshold = threshold;
}

float Squad::get_retreat_health_threshold() const {
	return retreat_health_threshold;
}

const std::vector<PerceptionRecord> &Squad::get_known_enemies() const {
	return known_enemies;
}

void Squad::aggregate_perceptions(const WorldState &world) {
	known_enemies.clear();

	// Merge perception from all members
	for (int m = 0; m < member_ids.size(); m++) {
		AgentId mid = member_ids[m];
		for (int i = 0; i < world.agents.size(); i++) {
			if (world.agents[i].id == mid) {
				// Use agent's perception state from world snapshot
				const PerceptionState &perc = world.agents[i].perception;
				for (int j = 0; j < perc.known_targets.size(); j++) {
					bool found = false;
					for (int k = 0; k < known_enemies.size(); k++) {
						if (known_enemies[k].target_id == perc.known_targets[j].target_id) {
							// Update with higher confidence
							if (perc.known_targets[j].confidence > known_enemies[k].confidence) {
								known_enemies[k] = perc.known_targets[j];
							}
							found = true;
							break;
						}
					}
					if (!found) {
						known_enemies.push_back(perc.known_targets[j]);
					}
				}
				break;
			}
		}
	}
}

void Squad::evaluate_tactical_state(const WorldState &world) {
	TacticalState old_state = tactical_state;

	if (known_enemies.size() == 0) {
		tactical_state = TacticalState::IDLE;
		return;
	}

	// Check if any member needs retreat (low health)
	for (int m = 0; m < member_ids.size(); m++) {
		AgentId mid = member_ids[m];
		for (int i = 0; i < world.agents.size(); i++) {
			if (world.agents[i].id == mid) {
				float health_pct = world.agents[i].health / world.agents[i].max_health;
				if (health_pct < retreat_health_threshold) {
					tactical_state = TacticalState::RETREAT;
					return;
				}
				break;
			}
		}
	}

	// Check if enemies are in engagement range
	bool enemies_in_range = false;
	for (int i = 0; i < known_enemies.size(); i++) {
		if (known_enemies[i].confidence > 0.5f) {
			enemies_in_range = true;
			break;
		}
	}

	if (enemies_in_range) {
		tactical_state = TacticalState::ATTACK;
	} else {
		tactical_state = TacticalState::SEARCH;
	}
}

void Squad::issue_orders(WorldState &world) const {
	for (int m = 0; m < member_ids.size(); m++) {
		AgentId mid = member_ids[m];
		for (int i = 0; i < world.agents.size(); i++) {
			if (world.agents[i].id == mid) {
				world.agents[i].tactical_state = tactical_state;

				// Set goal based on tactical state
				switch (tactical_state) {
					case TacticalState::ATTACK:
						world.agents[i].goal = AgentGoal::ATTACK;
						break;
					case TacticalState::RETREAT:
						world.agents[i].goal = AgentGoal::RETREAT;
						break;
					case TacticalState::SEARCH:
						world.agents[i].goal = AgentGoal::INVESTIGATE;
						break;
					case TacticalState::DEFEND:
						world.agents[i].goal = AgentGoal::DEFEND;
						break;
					default:
						world.agents[i].goal = AgentGoal::IDLE;
						break;
				}
				break;
			}
		}
	}
}
