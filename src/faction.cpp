#include "faction.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void Faction::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_faction_id", "id"), &Faction::set_faction_id);
	ClassDB::bind_method(D_METHOD("get_faction_id"), &Faction::get_faction_id);

	ClassDB::bind_method(D_METHOD("set_faction_name", "name"), &Faction::set_faction_name);
	ClassDB::bind_method(D_METHOD("get_faction_name"), &Faction::get_faction_name);

	ClassDB::bind_method(D_METHOD("set_alertness", "alertness"), &Faction::set_alertness);
	ClassDB::bind_method(D_METHOD("get_alertness"), &Faction::get_alertness);

	ClassDB::bind_method(D_METHOD("set_strength", "strength"), &Faction::set_strength);
	ClassDB::bind_method(D_METHOD("get_strength"), &Faction::get_strength);

	ClassDB::bind_method(D_METHOD("add_ally", "id"), &Faction::add_ally);
	ClassDB::bind_method(D_METHOD("add_enemy", "id"), &Faction::add_enemy);
	ClassDB::bind_method(D_METHOD("is_enemy", "id"), &Faction::is_enemy);
	ClassDB::bind_method(D_METHOD("is_ally", "id"), &Faction::is_ally);
	ClassDB::bind_method(D_METHOD("remove_ally", "id"), &Faction::remove_ally);
	ClassDB::bind_method(D_METHOD("remove_enemy", "id"), &Faction::remove_enemy);

	ClassDB::add_property("Faction", PropertyInfo(Variant::INT, "faction_id"), "set_faction_id", "get_faction_id");
	ClassDB::add_property("Faction", PropertyInfo(Variant::STRING, "faction_name"), "set_faction_name", "get_faction_name");
	ClassDB::add_property("Faction", PropertyInfo(Variant::FLOAT, "alertness", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_alertness", "get_alertness");
	ClassDB::add_property("Faction", PropertyInfo(Variant::INT, "strength"), "set_strength", "get_strength");
}

Faction::Faction() {
}

Faction::~Faction() {
}

void Faction::set_faction_id(FactionId p_id) { faction_id = p_id; }
FactionId Faction::get_faction_id() const { return faction_id; }

void Faction::set_faction_name(const String &p_name) { faction_name = p_name; }
String Faction::get_faction_name() const { return faction_name; }

void Faction::set_alertness(float p_alertness) { alertness = CLAMP(p_alertness, 0.0f, 1.0f); }
float Faction::get_alertness() const { return alertness; }

void Faction::set_strength(int32_t p_strength) { strength = p_strength; }
int32_t Faction::get_strength() const { return strength; }

void Faction::add_ally(FactionId p_id) {
	for (int i = 0; i < allies.size(); i++) {
		if (allies[i] == p_id) {
			return;
		}
	}
	allies.push_back(p_id);
}

void Faction::add_enemy(FactionId p_id) {
	for (int i = 0; i < enemies.size(); i++) {
		if (enemies[i] == p_id) {
			return;
		}
	}
	enemies.push_back(p_id);
}

bool Faction::is_enemy(FactionId p_id) const {
	for (int i = 0; i < enemies.size(); i++) {
		if (enemies[i] == p_id) {
			return true;
		}
	}
	return false;
}

bool Faction::is_ally(FactionId p_id) const {
	for (int i = 0; i < allies.size(); i++) {
		if (allies[i] == p_id) {
			return true;
		}
	}
	return false;
}

void Faction::remove_ally(FactionId p_id) {
	for (int i = allies.size() - 1; i >= 0; i--) {
		if (allies[i] == p_id) {
			allies.erase(allies.begin() + i);
			return;
		}
	}
}

void Faction::remove_enemy(FactionId p_id) {
	for (int i = enemies.size() - 1; i >= 0; i--) {
		if (enemies[i] == p_id) {
			enemies.erase(enemies.begin() + i);
			return;
		}
	}
}
