#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node.hpp>
#include <vector>

using namespace godot;

class Agent;

class Squad : public Node {
	GDCLASS(Squad, Node)

private:
	SquadId squad_id = INVALID_SQUAD_ID;
	FactionId faction_id = INVALID_FACTION_ID;
	AgentId leader_id = INVALID_AGENT_ID;
	std::vector<AgentId> member_ids;

	TacticalState tactical_state = TacticalState::IDLE;
	RoomId objective_room = INVALID_ROOM_ID;

	// Known enemies — aggregated from member perceptions
	std::vector<PerceptionRecord> known_enemies;

	// Parameters
	float engagement_range = 30.0f;
	float retreat_health_threshold = 0.3f;

protected:
	static void _bind_methods();

public:
	Squad();
	~Squad();

	void _ready() override;

	// Tick — called by WorldSimulation
	void update(double delta, const WorldState &world);

	// ID
	void set_squad_id(SquadId id);
	SquadId get_squad_id() const;

	// Faction
	void set_faction_id(FactionId id);
	FactionId get_faction_id() const;

	// Members
	void add_member(AgentId id);
	void remove_member(AgentId id);
	int32_t get_member_count() const;
	const std::vector<AgentId> &get_member_ids() const;
	void set_leader(AgentId id);
	AgentId get_leader_id() const;

	// Tactical state
	void set_tactical_state(TacticalState state);
	TacticalState get_tactical_state() const;

	// Objective
	void set_objective_room(RoomId room);
	RoomId get_objective_room() const;

	// Parameters
	void set_engagement_range(float range);
	float get_engagement_range() const;
	void set_retreat_health_threshold(float threshold);
	float get_retreat_health_threshold() const;

	// Known enemies
	const std::vector<PerceptionRecord> &get_known_enemies() const;
	void aggregate_perceptions(const WorldState &world);

	// Decision making
	void evaluate_tactical_state(const WorldState &world);
	void issue_orders(WorldState &world) const;
};
