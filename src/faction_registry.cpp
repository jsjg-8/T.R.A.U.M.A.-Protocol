#include "faction_registry.h"
#include "world_state.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void FactionRegistry::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_faction", "faction"), &FactionRegistry::add_faction);
	ClassDB::bind_method(D_METHOD("get_faction", "id"), &FactionRegistry::get_faction);
	ClassDB::bind_method(D_METHOD("get_faction_count"), &FactionRegistry::get_faction_count);
	ClassDB::bind_method(D_METHOD("are_enemies", "a", "b"), &FactionRegistry::are_enemies);
	ClassDB::bind_method(D_METHOD("are_allies", "a", "b"), &FactionRegistry::are_allies);
}

FactionRegistry::FactionRegistry() {
}

FactionRegistry::~FactionRegistry() {
}

void FactionRegistry::add_faction(const Ref<Faction> &p_faction) {
	if (p_faction.is_null()) {
		return;
	}
	factions.push_back(p_faction);
}

Ref<Faction> FactionRegistry::get_faction(FactionId p_id) const {
	for (int i = 0; i < factions.size(); i++) {
		if (factions[i]->get_faction_id() == p_id) {
			return factions[i];
		}
	}
	return Ref<Faction>();
}

Ref<Faction> FactionRegistry::get_faction_by_index(int32_t p_index) const {
	if (p_index < 0 || p_index >= factions.size()) {
		return Ref<Faction>();
	}
	return factions[p_index];
}

int32_t FactionRegistry::get_faction_count() const {
	return factions.size();
}

bool FactionRegistry::are_enemies(FactionId p_a, FactionId p_b) const {
	Ref<Faction> faction_a = get_faction(p_a);
	if (faction_a.is_null()) {
		return false;
	}
	return faction_a->is_enemy(p_b);
}

bool FactionRegistry::are_allies(FactionId p_a, FactionId p_b) const {
	Ref<Faction> faction_a = get_faction(p_a);
	if (faction_a.is_null()) {
		return false;
	}
	return faction_a->is_ally(p_b);
}

void FactionRegistry::update_factions(float p_delta, const std::vector<AgentState> &p_agents) {
	for (int i = 0; i < factions.size(); i++) {
		Ref<Faction> faction = factions[i];
		FactionId fid = faction->get_faction_id();

		// Count casualties for this faction
		int32_t alive = 0;
		int32_t total = 0;
		for (int j = 0; j < p_agents.size(); j++) {
			if (p_agents[j].faction == fid) {
				total++;
				if (p_agents[j].life_state == AgentLifeState::ALIVE) {
					alive++;
				}
			}
		}

		if (total > 0) {
			float casualty_ratio = 1.0f - ((float)alive / (float)total);
			// Alertness rises as casualties increase
			faction->set_alertness(MIN(1.0f, faction->get_alertness() + casualty_ratio * p_delta * 2.0f));
			faction->set_strength(alive);
		}
	}
}
