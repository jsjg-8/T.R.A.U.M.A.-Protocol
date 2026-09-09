#include "mission.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void Mission::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mission_id", "id"), &Mission::set_mission_id);
	ClassDB::bind_method(D_METHOD("get_mission_id"), &Mission::get_mission_id);

	ClassDB::bind_method(D_METHOD("start_mission"), &Mission::start_mission);
	ClassDB::bind_method(D_METHOD("complete_current_objective"), &Mission::complete_current_objective);
	ClassDB::bind_method(D_METHOD("fail_mission"), &Mission::fail_mission);
	ClassDB::bind_method(D_METHOD("cancel_mission"), &Mission::cancel_mission);

	// add_objective — takes ObjectiveType (internal enum), use from C++ only.
	// GDScript uses add_objective_with_ids with plain ints instead.
	ClassDB::bind_method(D_METHOD("add_objective_with_ids", "type", "room", "agent"), &Mission::add_objective_with_ids);
	ClassDB::bind_method(D_METHOD("get_status_id"), &Mission::get_status_id);
	ClassDB::bind_method(D_METHOD("get_current_objective_type_id"), &Mission::get_current_objective_type_id);
	ClassDB::bind_method(D_METHOD("get_objective_count"), &Mission::get_objective_count);
	ClassDB::bind_method(D_METHOD("get_current_objective_index"), &Mission::get_current_objective_index);
	ClassDB::bind_method(D_METHOD("has_objectives"), &Mission::has_objectives);
	ClassDB::bind_method(D_METHOD("all_objectives_completed"), &Mission::all_objectives_completed);

	ClassDB::bind_method(D_METHOD("set_insertion_room", "room"), &Mission::set_insertion_room);
	ClassDB::bind_method(D_METHOD("get_insertion_room"), &Mission::get_insertion_room);
	ClassDB::bind_method(D_METHOD("set_extraction_room", "room"), &Mission::set_extraction_room);
	ClassDB::bind_method(D_METHOD("get_extraction_room"), &Mission::get_extraction_room);

	ClassDB::bind_method(D_METHOD("set_time_limit", "limit"), &Mission::set_time_limit);
	ClassDB::bind_method(D_METHOD("get_time_limit"), &Mission::get_time_limit);
	ClassDB::bind_method(D_METHOD("get_elapsed_time"), &Mission::get_elapsed_time);

	ADD_SIGNAL(MethodInfo("mission_started"));
	ADD_SIGNAL(MethodInfo("mission_completed", PropertyInfo(Variant::BOOL, "success")));
	ADD_SIGNAL(MethodInfo("objective_completed", PropertyInfo(Variant::INT, "index")));
}

Mission::Mission() {
}

Mission::~Mission() {
}

void Mission::_ready() {
}

void Mission::_process(double delta) {
	if (status != MissionStatus::IN_PROGRESS) {
		return;
	}
	elapsed_time += delta;
	if (time_limit > 0.0f && elapsed_time >= time_limit) {
		fail_mission();
	}
}

void Mission::start_mission() {
	if (status != MissionStatus::PENDING) {
		return;
	}
	status = MissionStatus::IN_PROGRESS;
	elapsed_time = 0.0f;
	emit_signal("mission_started");
}

void Mission::complete_current_objective() {
	if (status != MissionStatus::IN_PROGRESS) {
		return;
	}
	if (current_objective_index < objectives.size()) {
		objectives[current_objective_index].completed = true;
		emit_signal("objective_completed", current_objective_index);
		current_objective_index++;

		if (all_objectives_completed()) {
			status = MissionStatus::COMPLETED_SUCCESS;
			emit_signal("mission_completed", true);
		}
	}
}

void Mission::fail_mission() {
	if (status == MissionStatus::IN_PROGRESS) {
		status = MissionStatus::COMPLETED_FAILURE;
		emit_signal("mission_completed", false);
	}
}

void Mission::cancel_mission() {
	if (status == MissionStatus::PENDING || status == MissionStatus::IN_PROGRESS) {
		status = MissionStatus::CANCELLED;
		emit_signal("mission_completed", false);
	}
}

void Mission::set_mission_id(MissionId p_id) { mission_id = p_id; }
MissionId Mission::get_mission_id() const { return mission_id; }
MissionStatus Mission::get_status() const { return status; }

void Mission::add_objective(ObjectiveType p_type, RoomId p_room, AgentId p_agent) {
	MissionObjective obj;
	obj.type = p_type;
	obj.target_room = p_room;
	obj.target_agent = p_agent;
	obj.completed = false;
	objectives.push_back(obj);
}

void Mission::add_objective_with_ids(int64_t p_type, int64_t p_room, int64_t p_agent) {
	add_objective(static_cast<ObjectiveType>(p_type), static_cast<RoomId>(p_room), static_cast<AgentId>(p_agent));
}

int64_t Mission::get_status_id() const {
	return static_cast<int64_t>(status);
}

int64_t Mission::get_current_objective_type_id() const {
	return static_cast<int64_t>(get_current_objective_type());
}

int32_t Mission::get_objective_count() const {
	return objectives.size();
}

int32_t Mission::get_current_objective_index() const {
	return current_objective_index;
}

ObjectiveType Mission::get_current_objective_type() const {
	if (current_objective_index < objectives.size()) {
		return objectives[current_objective_index].type;
	}
	return ObjectiveType::LOCATE_VIP;
}

bool Mission::is_current_objective_completed() const {
	if (current_objective_index < objectives.size()) {
		return objectives[current_objective_index].completed;
	}
	return false;
}

bool Mission::has_objectives() const {
	return objectives.size() > 0;
}

bool Mission::all_objectives_completed() const {
	for (int i = 0; i < objectives.size(); i++) {
		if (!objectives[i].completed) {
			return false;
		}
	}
	return objectives.size() > 0;
}

void Mission::set_insertion_room(RoomId p_room) { insertion_room = p_room; }
RoomId Mission::get_insertion_room() const { return insertion_room; }
void Mission::set_extraction_room(RoomId p_room) { extraction_room = p_room; }
RoomId Mission::get_extraction_room() const { return extraction_room; }
void Mission::set_time_limit(float p_limit) { time_limit = p_limit; }
float Mission::get_time_limit() const { return time_limit; }
float Mission::get_elapsed_time() const { return elapsed_time; }
