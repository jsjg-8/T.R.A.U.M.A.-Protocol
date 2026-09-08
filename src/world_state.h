#pragma once

#include <cstdint>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <vector>

using namespace godot;

// ID types — strongly typed integers prevent accidental cross-system confusion
using AgentId = uint32_t;
using SquadId = uint32_t;
using FactionId = uint32_t;
using RoomId = uint32_t;
using DistrictId = uint32_t;
using CoverPointId = uint32_t;
using DoorId = uint32_t;
using MissionId = uint32_t;

constexpr AgentId INVALID_AGENT_ID = 0;
constexpr SquadId INVALID_SQUAD_ID = 0;
constexpr FactionId INVALID_FACTION_ID = 0;
constexpr RoomId INVALID_ROOM_ID = 0;

// Agent goal states — what the agent is trying to do at the highest level
enum class AgentGoal : int64_t {
	IDLE,
	PATROL,
	ATTACK,
	DEFEND,
	RETREAT,
	INVESTIGATE,
	PROTECT_VIP,
	EXTRACT,
	STABILIZE_PATIENT,
};

// Agent life state
enum class AgentLifeState : int64_t {
	ALIVE,
	STABILIZED,
	INCAPACITATED,
	DEAD,
};

// Tactical state — set by Squad, read by Agent
enum class TacticalState : int64_t {
	IDLE,
	ATTACK,
	DEFEND,
	SEARCH,
	RETREAT,
	REGROUP,
	PROTECT_VIP,
};

// Perception state — what the agent knows about a target
struct PerceptionRecord {
	AgentId target_id = INVALID_AGENT_ID;
	Vector3 last_known_position;
	float confidence = 0.0f;
	float last_seen_time = 0.0f;
	bool is_hostile = false;
};

// Perception state — full per-agent knowledge
struct PerceptionState {
	std::vector<PerceptionRecord> known_targets;
	float hearing_event_time = 0.0f;
	Vector3 hearing_event_origin;
	bool has_hearing_event = false;
};

// Snapshot of a single agent — what WorldState stores
struct AgentState {
	AgentId id = INVALID_AGENT_ID;
	FactionId faction = INVALID_FACTION_ID;
	RoomId room = INVALID_ROOM_ID;
	DistrictId district = 0;
	Vector3 position;
	float health = 100.0f;
	float max_health = 100.0f;
	AgentLifeState life_state = AgentLifeState::ALIVE;
	AgentGoal goal = AgentGoal::IDLE;
	TacticalState tactical_state = TacticalState::IDLE;
	SquadId squad_id = INVALID_SQUAD_ID;
	PerceptionState perception;
};

// Snapshot of a squad
struct SquadState {
	SquadId id = INVALID_SQUAD_ID;
	FactionId faction = INVALID_FACTION_ID;
	AgentId leader_id = INVALID_AGENT_ID;
	std::vector<AgentId> member_ids;
	TacticalState tactical_state = TacticalState::IDLE;
	RoomId objective_room = INVALID_ROOM_ID;
};

// Snapshot of a faction
struct FactionState {
	FactionId id = INVALID_FACTION_ID;
	String name;
	float alertness = 0.0f;
	int32_t strength = 100;
	std::vector<FactionId> allies;
	std::vector<FactionId> enemies;
};

// World snapshot — read by Director, tests, debug
struct WorldState {
	std::vector<AgentState> agents;
	std::vector<SquadState> squads;
	std::vector<FactionState> factions;
	float elapsed_time = 0.0f;
	uint64_t tick_count = 0;
};

// Note: AgentGoal / AgentLifeState / TacticalState are pure C++ simulation enums.
// Do NOT use VARIANT_ENUM_CAST here — enum class has no implicit int64_t
// conversion, and these enums are not exposed as Godot Variant properties in POC.
// If you later need to expose them (e.g., as @export), change to plain `enum`
// or add explicit Variant conversion.
