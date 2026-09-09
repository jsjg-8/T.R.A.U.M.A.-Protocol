#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node.hpp>
#include <vector>

using namespace godot;

enum class MissionStatus : uint8_t {
	PENDING,
	IN_PROGRESS,
	COMPLETED_SUCCESS,
	COMPLETED_FAILURE,
	CANCELLED,
};

enum class ObjectiveType : uint8_t {
	LOCATE_VIP,
	STABILIZE_PATIENT,
	ESCORT_TO_EXTRACTION,
	EXTRACT,
	DEFEND_POSITION,
	ELIMINATE_HOSTILES,
};

struct MissionObjective {
	ObjectiveType type;
	bool completed = false;
	RoomId target_room = INVALID_ROOM_ID;
	AgentId target_agent = INVALID_AGENT_ID;
};

class Mission : public Node {
	GDCLASS(Mission, Node)

private:
	MissionId mission_id = 0;
	MissionStatus status = MissionStatus::PENDING;
	std::vector<MissionObjective> objectives;
	int32_t current_objective_index = 0;

	RoomId insertion_room = INVALID_ROOM_ID;
	RoomId extraction_room = INVALID_ROOM_ID;

	float elapsed_time = 0.0f;
	float time_limit = -1.0f; // -1 = no limit

protected:
	static void _bind_methods();

public:
	Mission();
	~Mission();

	void _ready() override;
	void _process(double delta) override;

	void start_mission();
	void complete_current_objective();
	void fail_mission();
	void cancel_mission();

	void set_mission_id(MissionId p_id);
	MissionId get_mission_id() const;
	MissionStatus get_status() const;

	void add_objective(ObjectiveType p_type, RoomId p_room, AgentId p_agent);
	int32_t get_objective_count() const;
	int32_t get_current_objective_index() const;
	ObjectiveType get_current_objective_type() const;
	bool is_current_objective_completed() const;
	bool has_objectives() const;
	bool all_objectives_completed() const;

	void set_insertion_room(RoomId p_room);
	RoomId get_insertion_room() const;
	void set_extraction_room(RoomId p_room);
	RoomId get_extraction_room() const;

	void set_time_limit(float p_limit);
	float get_time_limit() const;
	float get_elapsed_time() const;
};
