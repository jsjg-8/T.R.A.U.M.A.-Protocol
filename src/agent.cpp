#include "agent.h"
#include "world_simulation.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void Agent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_agent_id", "id"), &Agent::set_agent_id);
	ClassDB::bind_method(D_METHOD("get_agent_id"), &Agent::get_agent_id);

	ClassDB::bind_method(D_METHOD("set_faction_id", "id"), &Agent::set_faction_id);
	ClassDB::bind_method(D_METHOD("get_faction_id"), &Agent::get_faction_id);

	ClassDB::bind_method(D_METHOD("set_health", "h"), &Agent::set_health);
	ClassDB::bind_method(D_METHOD("get_health"), &Agent::get_health);

	ClassDB::bind_method(D_METHOD("set_position", "pos"), &Agent::set_position);
	ClassDB::bind_method(D_METHOD("get_position"), &Agent::get_position);

	ClassDB::bind_method(D_METHOD("set_move_speed", "speed"), &Agent::set_move_speed);
	ClassDB::bind_method(D_METHOD("get_move_speed"), &Agent::get_move_speed);

	ClassDB::bind_method(D_METHOD("set_room_id", "id"), &Agent::set_room_id);
	ClassDB::bind_method(D_METHOD("get_room_id"), &Agent::get_room_id);
	ClassDB::bind_method(D_METHOD("set_district_id", "id"), &Agent::set_district_id);
	ClassDB::bind_method(D_METHOD("get_district_id"), &Agent::get_district_id);
	ClassDB::bind_method(D_METHOD("set_max_health", "h"), &Agent::set_max_health);
	ClassDB::bind_method(D_METHOD("get_max_health"), &Agent::get_max_health);
	// Life/goal/tactical are pure C++ simulation enums (enum class : int64_t).
	// Not exposed as Godot Variant properties in POC — no GetTypeInfo/VARIANT_ENUM_CAST
	// needed. Keep strong typing internally; expose as int if GDScript needs it later.
	// Was: bind_method("set_life_state"/"get_life_state"/"set_goal"/... ) — removed.
	ClassDB::bind_method(D_METHOD("set_squad_id", "id"), &Agent::set_squad_id);
	ClassDB::bind_method(D_METHOD("get_squad_id"), &Agent::get_squad_id);
	ClassDB::bind_method(D_METHOD("take_damage", "amount"), &Agent::take_damage);
	ClassDB::bind_method(D_METHOD("heal", "amount"), &Agent::heal);
	ClassDB::bind_method(D_METHOD("get_is_alive"), &Agent::get_is_alive);
	ClassDB::bind_method(D_METHOD("set_move_target", "target"), &Agent::set_move_target);
	ClassDB::bind_method(D_METHOD("get_move_target"), &Agent::get_move_target);
	ClassDB::bind_method(D_METHOD("get_has_move_target"), &Agent::get_has_move_target);
	ClassDB::bind_method(D_METHOD("clear_move_target"), &Agent::clear_move_target);
	ClassDB::bind_method(D_METHOD("set_debug_verbose", "verbose"), &Agent::set_debug_verbose);
	ClassDB::bind_method(D_METHOD("get_debug_verbose"), &Agent::get_debug_verbose);

	ADD_SIGNAL(MethodInfo("health_changed", PropertyInfo(Variant::FLOAT, "new_health"), PropertyInfo(Variant::FLOAT, "old_health")));
	ADD_SIGNAL(MethodInfo("death"));
}

Agent::Agent() {
}

Agent::~Agent() {
}

void Agent::_ready() {
	Node *parent = get_parent();
	while (parent) {
		WorldSimulation *ws = Object::cast_to<WorldSimulation>(parent);
		if (ws) {
			sim = ws;
			break;
		}
		parent = parent->get_parent();
	}
}

void Agent::update(double delta, const WorldState &world) {
	if (!get_is_alive()) {
		return;
	}

	if (has_move_target) {
		Vector3 dir = move_target - position;
		float dist = dir.length();
		if (dist < 0.1f) {
			position = move_target;
			has_move_target = false;
		} else {
			dir.normalize();
			float step = move_speed * delta;
			if (step > dist) {
				step = dist;
			}
			position += dir * step;
		}
	}
}

void Agent::update_perception(double delta, const WorldState &world) {
	// Phase 2: PerceptionComponent drives this
}

void Agent::set_agent_id(AgentId id) { agent_id = id; }
AgentId Agent::get_agent_id() const { return agent_id; }

void Agent::set_faction_id(FactionId id) { faction_id = id; }
FactionId Agent::get_faction_id() const { return faction_id; }

void Agent::set_room_id(RoomId id) { room_id = id; }
RoomId Agent::get_room_id() const { return room_id; }

void Agent::set_district_id(DistrictId id) { district_id = id; }
DistrictId Agent::get_district_id() const { return district_id; }

void Agent::set_position(const Vector3 &pos) { position = pos; }
Vector3 Agent::get_position() const { return position; }

void Agent::set_health(float h) {
	float old = health;
	health = Math::clamp(h, 0.0f, max_health);
	if (health != old) {
		emit_signal("health_changed", health, old);
	}
	if (health <= 0.0f && old > 0.0f) {
		life_state = AgentLifeState::DEAD;
		emit_signal("death");
	}
}
float Agent::get_health() const { return health; }

void Agent::set_max_health(float h) {
	if (h <= 0.0f) {
		UtilityFunctions::printerr("Agent: max_health must be > 0");
		return;
	}
	max_health = h;
	if (health > max_health) {
		health = max_health;
	}
}
float Agent::get_max_health() const { return max_health; }

bool Agent::get_is_alive() const { return life_state == AgentLifeState::ALIVE; }

void Agent::take_damage(float amount) {
	if (amount <= 0.0f || !get_is_alive()) {
		return;
	}
	set_health(health - amount);
}

void Agent::heal(float amount) {
	if (amount <= 0.0f || !get_is_alive()) {
		return;
	}
	set_health(health + amount);
}

void Agent::set_life_state(AgentLifeState state) { life_state = state; }
AgentLifeState Agent::get_life_state() const { return life_state; }

void Agent::set_goal(AgentGoal g) { goal = g; }
AgentGoal Agent::get_goal() const { return goal; }

void Agent::set_tactical_state(TacticalState s) { tactical_state = s; }
TacticalState Agent::get_tactical_state() const { return tactical_state; }

void Agent::set_squad_id(SquadId id) { squad_id = id; }
SquadId Agent::get_squad_id() const { return squad_id; }

void Agent::set_move_target(const Vector3 &target) {
	move_target = target;
	has_move_target = true;
}
Vector3 Agent::get_move_target() const { return move_target; }
bool Agent::get_has_move_target() const { return has_move_target; }
void Agent::clear_move_target() { has_move_target = false; }

void Agent::set_move_speed(float speed) { move_speed = speed; }
float Agent::get_move_speed() const { return move_speed; }

void Agent::set_debug_verbose(bool verbose) { debug_verbose = verbose; }
bool Agent::get_debug_verbose() const { return debug_verbose; }

const PerceptionState &Agent::get_perception_state() const { return perception; }
void Agent::add_perception_record(const PerceptionRecord &record) {
	perception.known_targets.push_back(record);
}
void Agent::clear_perception() { perception.known_targets.clear(); }

void Agent::set_simulation(WorldSimulation *w) { sim = w; }
WorldSimulation *Agent::get_simulation() const { return sim; }

void Agent::navigate_to_room(RoomId target_room, const WorldSimulation *sim) {
	room_id = target_room;
}

RoomId Agent::get_current_room() const {
	return room_id;
}

bool Agent::can_reach_room(RoomId target, const WorldSimulation *sim) const {
	return true;
}

void Agent::_on_health_changed(float new_health, float old_health) {
	if (debug_verbose) {
		UtilityFunctions::print("Agent ", agent_id, ": health ", old_health, " -> ", new_health);
	}
}

void Agent::_on_death() {
	if (debug_verbose) {
		UtilityFunctions::print("Agent ", agent_id, ": died");
	}
}
