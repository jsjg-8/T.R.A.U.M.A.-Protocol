#pragma once

#include "faction.h"

#include <godot_cpp/classes/node.hpp>
#include <vector>

using namespace godot;

class FactionRegistry : public Node {
	GDCLASS(FactionRegistry, Node)

private:
	std::vector<Ref<Faction>> factions;

protected:
	static void _bind_methods();

public:
	FactionRegistry();
	~FactionRegistry();

	void add_faction(const Ref<Faction> &p_faction);
	Ref<Faction> get_faction(FactionId p_id) const;
	Ref<Faction> get_faction_by_index(int32_t p_index) const;
	int32_t get_faction_count() const;

	bool are_enemies(FactionId p_a, FactionId p_b) const;
	bool are_allies(FactionId p_a, FactionId p_b) const;

	void update_factions(float p_delta, const std::vector<struct AgentState> &p_agents);
};
