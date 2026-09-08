#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/navigation_agent3d.hpp>

using namespace godot;

class Agent;

class Actor : public Node3D {
	GDCLASS(Actor, Node3D)

private:
	AgentId agent_id = INVALID_AGENT_ID;
	Agent *bound_agent = nullptr;
	NavigationAgent3D *nav_agent = nullptr;

	float arrival_threshold = 0.5f;
	bool is_navigating = false;

protected:
	static void _bind_methods();

public:
	Actor();
	~Actor();

	void _ready() override;
	void _physics_process(double delta) override;

	// Binding
	void bind_agent(Agent *agent);
	Agent *get_bound_agent() const;

	// Navigation
	void navigate_to(const Vector3 &target);
	void stop_navigation();
	bool get_is_navigating() const;

	// Arrival
	void set_arrival_threshold(float threshold);
	float get_arrival_threshold() const;

	// Agent ID
	void set_agent_id(AgentId id);
	AgentId get_agent_id() const;

	// Sync — called by WorldSimulation to update actor position from agent
	void sync_from_agent();
};
