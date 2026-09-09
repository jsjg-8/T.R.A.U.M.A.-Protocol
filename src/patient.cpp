#include "patient.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void Patient::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_patient_agent_id", "id"), &Patient::set_patient_agent_id);
	ClassDB::bind_method(D_METHOD("get_patient_agent_id"), &Patient::get_patient_agent_id);

	ClassDB::bind_method(D_METHOD("set_assigned_medic", "id"), &Patient::set_assigned_medic);
	ClassDB::bind_method(D_METHOD("get_assigned_medic"), &Patient::get_assigned_medic);

	ClassDB::bind_method(D_METHOD("set_stabilization_required", "time"), &Patient::set_stabilization_required);
	ClassDB::bind_method(D_METHOD("get_stabilization_required"), &Patient::get_stabilization_required);
	ClassDB::bind_method(D_METHOD("get_stabilization_progress"), &Patient::get_stabilization_progress);

	ClassDB::bind_method(D_METHOD("start_stabilization", "medic_id"), &Patient::start_stabilization);
	ClassDB::bind_method(D_METHOD("update_stabilization", "delta"), &Patient::update_stabilization);
	ClassDB::bind_method(D_METHOD("extract"), &Patient::extract);

	ADD_SIGNAL(MethodInfo("state_changed", PropertyInfo(Variant::INT, "old_state"), PropertyInfo(Variant::INT, "new_state")));
	ADD_SIGNAL(MethodInfo("stabilized"));
	ADD_SIGNAL(MethodInfo("extracted"));
}

Patient::Patient() {
}

Patient::~Patient() {
}

void Patient::_ready() {
}

void Patient::_process(double delta) {
	if (state == PatientState::CRITICAL && assigned_medic_id != INVALID_AGENT_ID) {
		update_stabilization(delta);
	}
}

void Patient::set_patient_agent_id(AgentId p_id) { patient_agent_id = p_id; }
AgentId Patient::get_patient_agent_id() const { return patient_agent_id; }

void Patient::set_state(PatientState p_state) {
	PatientState old_state = state;
	state = p_state;
	if (old_state != p_state) {
		emit_signal("state_changed", (int)old_state, (int)p_state);
	}
}

PatientState Patient::get_state() const { return state; }

void Patient::set_assigned_medic(AgentId p_id) { assigned_medic_id = p_id; }
AgentId Patient::get_assigned_medic() const { return assigned_medic_id; }

void Patient::set_stabilization_required(float p_time) { stabilization_required = MAX(0.1f, p_time); }
float Patient::get_stabilization_required() const { return stabilization_required; }
float Patient::get_stabilization_progress() const { return stabilization_progress; }

void Patient::start_stabilization(AgentId p_medic_id) {
	if (state != PatientState::CRITICAL) {
		return;
	}
	assigned_medic_id = p_medic_id;
	stabilization_progress = 0.0f;
}

void Patient::update_stabilization(float p_delta) {
	if (state != PatientState::CRITICAL || assigned_medic_id == INVALID_AGENT_ID) {
		return;
	}
	stabilization_progress += p_delta / stabilization_required;
	if (stabilization_progress >= 1.0f) {
		stabilization_progress = 1.0f;
		state = PatientState::STABILIZED;
		emit_signal("stabilized");
		emit_signal("state_changed", (int)PatientState::CRITICAL, (int)PatientState::STABILIZED);
	}
}

void Patient::extract() {
	if (state == PatientState::STABILIZED || state == PatientState::TRANSPORTABLE) {
		PatientState old_state = state;
		state = PatientState::EXTRACTED;
		emit_signal("extracted");
		emit_signal("state_changed", (int)old_state, (int)PatientState::EXTRACTED);
	}
}
