#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node.hpp>
#include <vector>

using namespace godot;

class Agent;

class PerceptionComponent : public Node {
	GDCLASS(PerceptionComponent, Node)

private:
	Agent *owner_agent = nullptr;

	// Perception parameters
	float visual_range = 30.0f;
	float visual_angle = 120.0f; // degrees, full cone
	float hearing_range = 20.0f;
	float confidence_decay_rate = 0.5f; // per second

	// Current perception state
	PerceptionState state;

protected:
	static void _bind_methods();

public:
	PerceptionComponent();
	~PerceptionComponent();

	void _ready() override;
	void _process(double delta) override;

	// Owner
	void set_owner_agent(Agent *agent);
	Agent *get_owner_agent() const;

	// Parameters
	void set_visual_range(float range);
	float get_visual_range() const;
	void set_visual_angle(float angle_degrees);
	float get_visual_angle() const;
	void set_hearing_range(float range);
	float get_hearing_range() const;
	void set_confidence_decay_rate(float rate);
	float get_confidence_decay_rate() const;

	// Perception update — called each tick
	void update_perception(double delta, const WorldState &world);

	// Event handling
	void on_hearing_event(const Vector3 &origin, float loudness);
	void on_visual_contact(AgentId target_id, const Vector3 &position, float confidence);

	// Query
	const PerceptionState &get_state() const;
	bool can_see_target(AgentId target_id) const;
	PerceptionRecord get_last_known(AgentId target_id) const;

	// Reset
	void clear();

	// Signals
	void _on_target_detected(AgentId target_id, float confidence);
	void _on_target_lost(AgentId target_id);
};
