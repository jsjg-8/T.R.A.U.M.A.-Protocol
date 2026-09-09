#pragma once

#include "world_state.h"

#include <godot_cpp/classes/resource.hpp>
#include <vector>

using namespace godot;

class Faction : public Resource {
	GDCLASS(Faction, Resource)

private:
	FactionId faction_id = INVALID_FACTION_ID;
	String faction_name;
	float alertness = 0.0f;
	int32_t strength = 100;
	std::vector<FactionId> allies;
	std::vector<FactionId> enemies;

protected:
	static void _bind_methods();

public:
	Faction();
	~Faction();

	void set_faction_id(FactionId id);
	FactionId get_faction_id() const;

	void set_faction_name(const String &p_name);
	String get_faction_name() const;

	void set_alertness(float p_alertness);
	float get_alertness() const;

	void set_strength(int32_t p_strength);
	int32_t get_strength() const;

	void add_ally(FactionId id);
	void add_enemy(FactionId id);
	bool is_enemy(FactionId id) const;
	bool is_ally(FactionId id) const;

	void remove_ally(FactionId id);
	void remove_enemy(FactionId id);
};
