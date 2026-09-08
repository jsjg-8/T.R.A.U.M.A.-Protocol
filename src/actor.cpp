#include "actor.h"
#include "agent.h"
#include <godot_cpp/classes/navigation_agent3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void Actor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("bind_agent", "agent"), &Actor::bind_agent);
	ClassDB::bind_method(D_METHOD("get_bound_agent"), &Actor::get_bound_agent);
	ClassDB::bind_method(D_METHOD("navigate_to", "target"), &Actor::navigate_to);
	ClassDB::bind_method(D_METHOD("stop_navigation"), &Actor::stop_navigation);
	ClassDB::bind_method(D_METHOD("get_is_navigating"), &Actor::get_is_navigating);
	ClassDB::bind_method(D_METHOD("set_arrival_threshold", "threshold"), &Actor::set_arrival_threshold);
	ClassDB::bind_method(D_METHOD("get_arrival_threshold"), &Actor::get_arrival_threshold);
	ClassDB::bind_method(D_METHOD("set_agent_id", "id"), &Actor::set_agent_id);
	ClassDB::bind_method(D_METHOD("get_agent_id"), &Actor::get_agent_id);
	ClassDB::bind_method(D_METHOD("sync_from_agent"), &Actor::sync_from_agent);

	ClassDB::add_property("Actor", PropertyInfo(Variant::FLOAT, "arrival_threshold", PROPERTY_HINT_RANGE, "0.1,5.0,0.1"), "set_arrival_threshold", "get_arrival_threshold");
}

Actor::Actor() {}

Actor::~Actor() {}

void Actor::_ready() {
	// Find NavigationAgent3D child
	for (int i = 0; i < get_child_count(); i++) {
		Node *child = get_child(i);
		NavigationAgent3D *nav = Object::cast_to<NavigationAgent3D>(child);
		if (nav) {
			nav_agent = nav;
			break;
		}
	}
}

void Actor::_physics_process(double delta) {
	if (!nav_agent || !is_navigating) {
		return;
	}

	if (nav_agent->is_navigation_finished()) {
		is_navigating = false;
		return;
	}

	Vector3 next_pos = nav_agent->get_next_path_position();
	Vector3 direction = next_pos - get_global_position();
	direction.y = 0.0f; // stay on horizontal plane

	if (direction.length() < arrival_threshold) {
		is_navigating = false;
		return;
	}

	direction.normalize();
	// Move toward next path point
	Vector3 new_pos = get_global_position() + direction * nav_agent->get_max_speed() * delta;
	new_pos.y = get_global_position().y; // preserve Y
	set_global_position(new_pos);
}

void Actor::bind_agent(Agent *agent) {
	bound_agent = agent;
	if (agent) {
		agent_id = agent->get_agent_id();
	}
}

Agent *Actor::get_bound_agent() const {
	return bound_agent;
}

void Actor::navigate_to(const Vector3 &target) {
	if (!nav_agent) {
		return;
	}
	nav_agent->set_target_position(target);
	is_navigating = true;
}

void Actor::stop_navigation() {
	is_navigating = false;
	if (nav_agent) {
		nav_agent->set_target_position(get_global_position());
	}
}

bool Actor::get_is_navigating() const {
	return is_navigating;
}

void Actor::set_arrival_threshold(float threshold) {
	arrival_threshold = threshold;
}

float Actor::get_arrival_threshold() const {
	return arrival_threshold;
}

void Actor::set_agent_id(AgentId id) {
	agent_id = id;
}

AgentId Actor::get_agent_id() const {
	return agent_id;
}

void Actor::sync_from_agent() {
	if (bound_agent) {
		Vector3 pos = bound_agent->get_position();
		pos.y = get_global_position().y; // preserve Y from scene
		set_global_position(pos);
	}
}
