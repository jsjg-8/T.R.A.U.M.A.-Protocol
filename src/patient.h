#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node.hpp>

using namespace godot;

enum class PatientState : uint8_t {
	CRITICAL, // needs immediate stabilization
	STABILIZED, // treated, stable
	TRANSPORTABLE, // can be moved to extraction
	EXTRACTED, // at extraction point
	DEAD,
};

class Patient : public Node {
	GDCLASS(Patient, Node)

private:
	AgentId patient_agent_id = INVALID_AGENT_ID;
	PatientState state = PatientState::CRITICAL;
	AgentId assigned_medic_id = INVALID_AGENT_ID;

	float stabilization_progress = 0.0f; // 0-1
	float stabilization_required = 1.0f; // time needed to stabilize

protected:
	static void _bind_methods();

public:
	Patient();
	~Patient();

	void _ready() override;
	void _process(double delta) override;

	void set_patient_agent_id(AgentId p_id);
	AgentId get_patient_agent_id() const;

	void set_state(PatientState p_state);
	PatientState get_state() const;
	// GDScript-visible int variants (enum is internal, not Variant-compatible).
	void set_state_id(int64_t p_state);
	int64_t get_state_id() const;

	void set_assigned_medic(AgentId p_id);
	AgentId get_assigned_medic() const;

	void set_stabilization_required(float p_time);
	float get_stabilization_required() const;
	float get_stabilization_progress() const;

	void start_stabilization(AgentId p_medic_id);
	void update_stabilization(float p_delta);
	void extract();
};
