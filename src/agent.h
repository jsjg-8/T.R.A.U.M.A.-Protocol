#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node.hpp>

using namespace godot;

class WorldSimulation;

class Agent : public Node {
	GDCLASS(Agent, Node)

private:
	AgentId agent_id = INVALID_AGENT_ID;
	FactionId faction_id = INVALID_FACTION_ID;
	RoomId room_id = INVALID_ROOM_ID;
	DistrictId district_id = 0;

	Vector3 position;
	float health = 100.0f;
	float max_health = 100.0f;
	AgentLifeState life_state = AgentLifeState::ALIVE;
	AgentGoal goal = AgentGoal::IDLE;
	TacticalState tactical_state = TacticalState::IDLE;
	SquadId squad_id = INVALID_SQUAD_ID;

	PerceptionState perception;

	float move_speed = 3.0f;
	Vector3 move_target;
	bool has_move_target = false;

	WorldSimulation *sim = nullptr;
	bool debug_verbose = false;

protected:
	static void _bind_methods();

public:
	Agent();
	~Agent();

	void _ready() override;

	void update(double delta, const WorldState &world);
	void update_perception(double delta, const WorldState &world);

	void set_agent_id(AgentId id);
	AgentId get_agent_id() const;

	void set_faction_id(FactionId id);
	FactionId get_faction_id() const;
	void set_room_id(RoomId id);
	RoomId get_room_id() const;
	void set_district_id(DistrictId id);
	DistrictId get_district_id() const;

	void set_position(const Vector3 &pos);
	Vector3 get_position() const;

	void set_health(float h);
	float get_health() const;
	void set_max_health(float h);
	float get_max_health() const;
	bool get_is_alive() const;
	void take_damage(float amount);
	void heal(float amount);

	void set_life_state(AgentLifeState state);
	AgentLifeState get_life_state() const;

	void set_goal(AgentGoal g);
	AgentGoal get_goal() const;
	void set_tactical_state(TacticalState s);
	TacticalState get_tactical_state() const;

	void set_squad_id(SquadId id);
	SquadId get_squad_id() const;

	void set_move_target(const Vector3 &target);
	Vector3 get_move_target() const;
	bool get_has_move_target() const;
	void clear_move_target();
	void set_move_speed(float speed);
	float get_move_speed() const;

	void set_debug_verbose(bool verbose);
	bool get_debug_verbose() const;

	const PerceptionState &get_perception_state() const;
	void add_perception_record(const PerceptionRecord &record);
	void clear_perception();

	void set_simulation(WorldSimulation *sim);
	WorldSimulation *get_simulation() const;

	// Navigation via semantic graph
	void navigate_to_room(RoomId target_room, const WorldSimulation *sim);
	RoomId get_current_room() const;
	bool can_reach_room(RoomId target, const WorldSimulation *sim) const;

	// Signals
	void _on_health_changed(float new_health, float old_health);
	void _on_death();
};
