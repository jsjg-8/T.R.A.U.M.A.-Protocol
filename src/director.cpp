#include "director.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void Director::_bind_methods() {
	ClassDB::bind_method(D_METHOD("clear_pending_events"), &Director::clear_pending_events);
	ClassDB::bind_method(D_METHOD("emit_event_with_id", "type", "origin", "radius", "description"), &Director::emit_event_with_id);
	ClassDB::bind_method(D_METHOD("get_history_count"), &Director::get_history_count);
	ClassDB::bind_method(D_METHOD("get_history_event_type"), &Director::get_history_event_type);

	ClassDB::bind_method(D_METHOD("set_mission_time_threshold", "threshold"), &Director::set_mission_time_threshold);
	ClassDB::bind_method(D_METHOD("get_mission_time_threshold"), &Director::get_mission_time_threshold);
	ClassDB::bind_method(D_METHOD("set_casualty_threshold", "threshold"), &Director::set_casualty_threshold);
	ClassDB::bind_method(D_METHOD("get_casualty_threshold"), &Director::get_casualty_threshold);
	ClassDB::bind_method(D_METHOD("set_alertness_threshold", "threshold"), &Director::set_alertness_threshold);
	ClassDB::bind_method(D_METHOD("get_alertness_threshold"), &Director::get_alertness_threshold);

	ClassDB::bind_method(D_METHOD("reset"), &Director::reset);

	ADD_SIGNAL(MethodInfo("director_event", PropertyInfo(Variant::INT, "event_type"), PropertyInfo(Variant::FLOAT, "timestamp")));
}

Director::Director() {
}

Director::~Director() {
}

void Director::_ready() {
}

void Director::_process(double delta) {
}

void Director::observe(const WorldState &p_world, double p_delta) {
	mission_elapsed_time += p_delta;

	// Count casualties
	total_casualties = 0;
	for (int i = 0; i < p_world.agents.size(); i++) {
		if (p_world.agents[i].life_state == AgentLifeState::DEAD ||
			p_world.agents[i].life_state == AgentLifeState::INCAPACITATED) {
			total_casualties++;
		}
	}

	evaluate_conditions(p_world);
}

void Director::evaluate_conditions(const WorldState &p_world) {
	// Time-based events
	if (mission_elapsed_time >= mission_time_threshold && event_history.size() == 0) {
		emit_event(DirectorEventType::REINFORCEMENT, Vector3(), 50.0f, "Enemy reinforcements arriving");
	}

	// Casualty-based events
	if (total_casualties >= casualty_threshold) {
		bool found = false;
		for (int i = 0; i < event_history.size(); i++) {
			if (event_history[i].type == DirectorEventType::AMBUSH) {
				found = true;
				break;
			}
		}
		if (!found) {
			emit_event(DirectorEventType::AMBUSH, Vector3(), 30.0f, "Hostile ambush triggered");
		}
	}

	// Alertness-based events
	for (int i = 0; i < p_world.factions.size(); i++) {
		if (p_world.factions[i].alertness >= alertness_threshold) {
			bool found = false;
			for (int j = 0; j < event_history.size(); j++) {
				if (event_history[j].type == DirectorEventType::LOCKDOWN) {
					found = true;
					break;
				}
			}
			if (!found) {
				emit_event(DirectorEventType::LOCKDOWN, Vector3(), 100.0f, "Sector lockdown initiated");
				break;
			}
		}
	}
}

void Director::emit_event(DirectorEventType p_type, const Vector3 &p_origin, float p_radius, const String &p_desc) {
	DirectorEvent event;
	event.type = p_type;
	event.timestamp = mission_elapsed_time;
	event.origin = p_origin;
	event.radius = p_radius;
	event.description = p_desc;
	pending_events.push_back(event);
	event_history.push_back(event);
	emit_signal("director_event", (int)p_type, event.timestamp);
}

const std::vector<DirectorEvent> &Director::get_pending_events() const {
	return pending_events;
}

void Director::clear_pending_events() {
	pending_events.clear();
}

const std::vector<DirectorEvent> &Director::get_event_history() const {
	return event_history;
}

void Director::emit_event_with_id(int64_t p_type, const Vector3 &p_origin, float p_radius, const String &p_desc) {
	emit_event(static_cast<DirectorEventType>(p_type), p_origin, p_radius, p_desc);
}

int64_t Director::get_history_count() const {
	return event_history.size();
}

int64_t Director::get_history_event_type(int64_t p_index) const {
	if (p_index < 0 || p_index >= event_history.size()) {
		return -1;
	}
	return static_cast<int64_t>(event_history[p_index].type);
}

void Director::set_mission_time_threshold(float p_threshold) { mission_time_threshold = p_threshold; }
float Director::get_mission_time_threshold() const { return mission_time_threshold; }
void Director::set_casualty_threshold(int32_t p_threshold) { casualty_threshold = p_threshold; }
int32_t Director::get_casualty_threshold() const { return casualty_threshold; }
void Director::set_alertness_threshold(float p_threshold) { alertness_threshold = p_threshold; }
float Director::get_alertness_threshold() const { return alertness_threshold; }

void Director::reset() {
	pending_events.clear();
	event_history.clear();
	mission_elapsed_time = 0.0f;
	total_casualties = 0;
}
