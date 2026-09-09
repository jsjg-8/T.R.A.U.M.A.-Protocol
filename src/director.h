#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node.hpp>
#include <vector>

using namespace godot;

enum class DirectorEventType : uint8_t {
	POWER_FAILURE,
	REINFORCEMENT,
	AMBUSH,
	LOCKDOWN,
	AV_DELAY,
	FIRE,
	CIVILIAN_PANIC,
	BREACH,
};

struct DirectorEvent {
	DirectorEventType type;
	float timestamp;
	Vector3 origin;
	float radius;
	String description;
};

class Director : public Node {
	GDCLASS(Director, Node)

private:
	std::vector<DirectorEvent> pending_events;
	std::vector<DirectorEvent> event_history;

	float mission_time_threshold = 30.0f;
	int32_t casualty_threshold = 2;
	float alertness_threshold = 0.7f;

	float mission_elapsed_time = 0.0f;
	int32_t total_casualties = 0;

protected:
	static void _bind_methods();

public:
	Director();
	~Director();

	void _ready() override;
	void _process(double delta) override;

	void observe(const WorldState &p_world, double p_delta);

	void emit_event(DirectorEventType p_type, const Vector3 &p_origin, float p_radius, const String &p_desc);

	// GDScript-visible int variants (enum/vector are internal, not Variant-compatible).
	void emit_event_with_id(int64_t p_type, const Vector3 &p_origin, float p_radius, const String &p_desc);
	int64_t get_history_count() const;
	int64_t get_history_event_type(int64_t p_index) const;

	const std::vector<DirectorEvent> &get_pending_events() const;
	void clear_pending_events();

	const std::vector<DirectorEvent> &get_event_history() const;

	void set_mission_time_threshold(float p_threshold);
	float get_mission_time_threshold() const;

	void set_casualty_threshold(int32_t p_threshold);
	int32_t get_casualty_threshold() const;

	void set_alertness_threshold(float p_threshold);
	float get_alertness_threshold() const;

	void reset();

private:
	void evaluate_conditions(const WorldState &p_world);
};
