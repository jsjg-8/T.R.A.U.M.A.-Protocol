#include "perception_component.h"
#include "agent.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void PerceptionComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_owner_agent", "agent"), &PerceptionComponent::set_owner_agent);
	ClassDB::bind_method(D_METHOD("get_owner_agent"), &PerceptionComponent::get_owner_agent);

	ClassDB::bind_method(D_METHOD("set_visual_range", "range"), &PerceptionComponent::set_visual_range);
	ClassDB::bind_method(D_METHOD("get_visual_range"), &PerceptionComponent::get_visual_range);
	ClassDB::bind_method(D_METHOD("set_visual_angle", "angle_degrees"), &PerceptionComponent::set_visual_angle);
	ClassDB::bind_method(D_METHOD("get_visual_angle"), &PerceptionComponent::get_visual_angle);
	ClassDB::bind_method(D_METHOD("set_hearing_range", "range"), &PerceptionComponent::set_hearing_range);
	ClassDB::bind_method(D_METHOD("get_hearing_range"), &PerceptionComponent::get_hearing_range);
	ClassDB::bind_method(D_METHOD("set_confidence_decay_rate", "rate"), &PerceptionComponent::set_confidence_decay_rate);
	ClassDB::bind_method(D_METHOD("get_confidence_decay_rate"), &PerceptionComponent::get_confidence_decay_rate);

	ClassDB::bind_method(D_METHOD("update_perception", "delta", "world"), &PerceptionComponent::update_perception);
	ClassDB::bind_method(D_METHOD("on_hearing_event", "origin", "loudness"), &PerceptionComponent::on_hearing_event);
	ClassDB::bind_method(D_METHOD("on_visual_contact", "target_id", "position", "confidence"), &PerceptionComponent::on_visual_contact);
	ClassDB::bind_method(D_METHOD("can_see_target", "target_id"), &PerceptionComponent::can_see_target);
	ClassDB::bind_method(D_METHOD("get_last_known", "target_id"), &PerceptionComponent::get_last_known);
	ClassDB::bind_method(D_METHOD("clear"), &PerceptionComponent::clear);

	ClassDB::add_property("PerceptionComponent", PropertyInfo(Variant::FLOAT, "visual_range", PROPERTY_HINT_RANGE, "5.0,100.0,1.0"), "set_visual_range", "get_visual_range");
	ClassDB::add_property("PerceptionComponent", PropertyInfo(Variant::FLOAT, "visual_angle", PROPERTY_HINT_RANGE, "10.0,360.0,5.0"), "set_visual_angle", "get_visual_angle");
	ClassDB::add_property("PerceptionComponent", PropertyInfo(Variant::FLOAT, "hearing_range", PROPERTY_HINT_RANGE, "1.0,100.0,1.0"), "set_hearing_range", "get_hearing_range");

	ADD_SIGNAL(MethodInfo("target_detected", PropertyInfo(Variant::INT, "target_id"), PropertyInfo(Variant::FLOAT, "confidence")));
	ADD_SIGNAL(MethodInfo("target_lost", PropertyInfo(Variant::INT, "target_id")));
}

PerceptionComponent::PerceptionComponent() {
}

PerceptionComponent::~PerceptionComponent() {
}

void PerceptionComponent::_ready() {
	// Auto-attach to parent Agent if in scene tree
	Node *parent = get_parent();
	if (parent) {
		Agent *agent = Object::cast_to<Agent>(parent);
		if (agent) {
			owner_agent = agent;
		}
	}
}

void PerceptionComponent::_process(double delta) {
	// Perception can also be ticked via _process if not driven by WorldSimulation
	if (owner_agent) {
		WorldState empty_state;
		update_perception(delta, empty_state);
	}
}

void PerceptionComponent::set_owner_agent(Agent *agent) {
	owner_agent = agent;
}

Agent *PerceptionComponent::get_owner_agent() const {
	return owner_agent;
}

void PerceptionComponent::set_visual_range(float range) {
	visual_range = range;
}

float PerceptionComponent::get_visual_range() const {
	return visual_range;
}

void PerceptionComponent::set_visual_angle(float angle_degrees) {
	visual_angle = angle_degrees;
}

float PerceptionComponent::get_visual_angle() const {
	return visual_angle;
}

void PerceptionComponent::set_hearing_range(float range) {
	hearing_range = range;
}

float PerceptionComponent::get_hearing_range() const {
	return hearing_range;
}

void PerceptionComponent::set_confidence_decay_rate(float rate) {
	confidence_decay_rate = rate;
}

float PerceptionComponent::get_confidence_decay_rate() const {
	return confidence_decay_rate;
}

void PerceptionComponent::update_perception(double delta, const WorldState &world) {
	if (!owner_agent) {
		return;
	}

	Vector3 self_pos = owner_agent->get_position();
	FactionId self_faction = owner_agent->get_faction_id();

	// Decay existing perception records
	for (int i = state.known_targets.size() - 1; i >= 0; i--) {
		state.known_targets[i].confidence -= confidence_decay_rate * delta;
		if (state.known_targets[i].confidence <= 0.0f) {
			AgentId lost_id = state.known_targets[i].target_id;
			state.known_targets.erase(state.known_targets.begin() + i);
			emit_signal("target_lost", (int64_t)lost_id);
		}
	}

	// Check each agent in world state
	for (int i = 0; i < world.agents.size(); i++) {
		const AgentState &other = world.agents[i];

		// Skip self and same faction
		if (other.id == owner_agent->get_agent_id()) {
			continue;
		}
		if (other.faction == self_faction) {
			continue;
		}
		if (other.life_state == AgentLifeState::DEAD) {
			continue;
		}

		// Distance check
		Vector3 diff = other.position - self_pos;
		float distance = diff.length();

		if (distance > visual_range) {
			continue;
		}

		// Angle check (simplified — always in front for POC)
		bool in_visual_cone = true;
		if (distance > 0.1f) {
			// Use dot product for cone check
			Vector3 forward = Vector3(0, 0, -1); // default forward
			Vector3 to_target = diff.normalized();
			float dot = forward.dot(to_target);
			float half_angle_cos = Math::cos(Math::deg_to_rad(visual_angle * 0.5f));
			in_visual_cone = dot >= half_angle_cos;
		}

		if (!in_visual_cone) {
			continue;
		}

		// Calculate confidence based on distance
		float dist_factor = 1.0f - (distance / visual_range);
		float confidence = dist_factor * 0.8f + 0.2f; // 0.2-1.0 range

		// Update or add perception record
		bool found = false;
		for (int j = 0; j < state.known_targets.size(); j++) {
			if (state.known_targets[j].target_id == other.id) {
				state.known_targets[j].last_known_position = other.position;
				state.known_targets[j].confidence = confidence;
				state.known_targets[j].last_seen_time = world.elapsed_time;
				state.known_targets[j].is_hostile = true;
				found = true;
				break;
			}
		}

		if (!found) {
			PerceptionRecord record;
			record.target_id = other.id;
			record.last_known_position = other.position;
			record.confidence = confidence;
			record.last_seen_time = world.elapsed_time;
			record.is_hostile = true;
			state.known_targets.push_back(record);
			emit_signal("target_detected", (int64_t)other.id, confidence);
		}
	}

	// Sync perception to owner agent
	if (owner_agent) {
		owner_agent->clear_perception();
		for (int i = 0; i < state.known_targets.size(); i++) {
			owner_agent->add_perception_record(state.known_targets[i]);
		}
	}
}

void PerceptionComponent::on_hearing_event(const Vector3 &origin, float loudness) {
	float distance = (origin - (owner_agent ? owner_agent->get_position() : Vector3())).length();
	if (distance <= hearing_range * loudness) {
		state.has_hearing_event = true;
		state.hearing_event_origin = origin;
		state.hearing_event_time = 0.0f; // will be set by world time
	}
}

void PerceptionComponent::on_visual_contact(AgentId target_id, const Vector3 &position, float confidence) {
	for (int i = 0; i < state.known_targets.size(); i++) {
		if (state.known_targets[i].target_id == target_id) {
			state.known_targets[i].last_known_position = position;
			state.known_targets[i].confidence = confidence;
			return;
		}
	}

	PerceptionRecord record;
	record.target_id = target_id;
	record.last_known_position = position;
	record.confidence = confidence;
	record.is_hostile = true;
	state.known_targets.push_back(record);
	emit_signal("target_detected", (int64_t)target_id, confidence);
}

const PerceptionState &PerceptionComponent::get_state() const {
	return state;
}

bool PerceptionComponent::can_see_target(AgentId target_id) const {
	for (int i = 0; i < state.known_targets.size(); i++) {
		if (state.known_targets[i].target_id == target_id && state.known_targets[i].confidence > 0.3f) {
			return true;
		}
	}
	return false;
}

PerceptionRecord PerceptionComponent::get_last_known(AgentId target_id) const {
	for (int i = 0; i < state.known_targets.size(); i++) {
		if (state.known_targets[i].target_id == target_id) {
			return state.known_targets[i];
		}
	}
	PerceptionRecord empty;
	return empty;
}

void PerceptionComponent::clear() {
	state.known_targets.clear();
	state.has_hearing_event = false;
	state.hearing_event_time = 0.0f;
}
