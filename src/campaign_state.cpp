#include "campaign_state.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void CampaignState::_bind_methods() {
	ClassDB::bind_method(D_METHOD("record_mission_outcome", "mission_id", "success", "friendly_casualties", "enemy_casualties"), &CampaignState::record_mission_outcome);
	ClassDB::bind_method(D_METHOD("record_agent_death", "id"), &CampaignState::record_agent_death);
	ClassDB::bind_method(D_METHOD("record_agent_extraction", "id"), &CampaignState::record_agent_extraction);
	ClassDB::bind_method(D_METHOD("is_agent_alive", "id"), &CampaignState::is_agent_alive);
	ClassDB::bind_method(D_METHOD("get_alive_agent_count"), &CampaignState::get_alive_agent_count);

	ClassDB::bind_method(D_METHOD("set_district_control", "id", "faction", "stability"), &CampaignState::set_district_control);
	ClassDB::bind_method(D_METHOD("get_controlling_faction", "id"), &CampaignState::get_controlling_faction);
	ClassDB::bind_method(D_METHOD("get_district_stability", "id"), &CampaignState::get_district_stability);

	ClassDB::bind_method(D_METHOD("set_faction_reputation", "id", "rep"), &CampaignState::set_faction_reputation);
	ClassDB::bind_method(D_METHOD("get_faction_reputation", "id"), &CampaignState::get_faction_reputation);

	ClassDB::bind_method(D_METHOD("get_total_missions"), &CampaignState::get_total_missions);
	ClassDB::bind_method(D_METHOD("get_successful_missions"), &CampaignState::get_successful_missions);
	ClassDB::bind_method(D_METHOD("get_success_rate"), &CampaignState::get_success_rate);
}

CampaignState::CampaignState() {
}

CampaignState::~CampaignState() {
}

void CampaignState::_ready() {
}

void CampaignState::record_mission_outcome(MissionId p_mission_id, bool p_success, int32_t p_friendly_casualties, int32_t p_enemy_casualties) {
	MissionOutcome outcome;
	outcome.mission_id = p_mission_id;
	outcome.success = p_success;
	outcome.friendly_casualties = p_friendly_casualties;
	outcome.enemy_casualties = p_enemy_casualties;
	mission_history.push_back(outcome);
}

void CampaignState::record_agent_death(AgentId p_id) {
	auto it = agent_records.find(p_id);
	if (it != agent_records.end()) {
		it->second.is_alive = false;
	} else {
		AgentRecord record;
		record.agent_id = p_id;
		record.is_alive = false;
		agent_records[p_id] = record;
	}
}

void CampaignState::record_agent_extraction(AgentId p_id) {
	auto it = agent_records.find(p_id);
	if (it != agent_records.end()) {
		it->second.was_extracted = true;
		it->second.missions_survived++;
	} else {
		AgentRecord record;
		record.agent_id = p_id;
		record.was_extracted = true;
		record.missions_survived = 1;
		agent_records[p_id] = record;
	}
}

bool CampaignState::is_agent_alive(AgentId p_id) const {
	auto it = agent_records.find(p_id);
	if (it == agent_records.end()) {
		return true; // Unknown agents assumed alive
	}
	return it->second.is_alive;
}

int32_t CampaignState::get_alive_agent_count() const {
	int32_t count = 0;
	for (auto it = agent_records.begin(); it != agent_records.end(); ++it) {
		if (it->second.is_alive) {
			count++;
		}
	}
	return count;
}

void CampaignState::set_district_control(DistrictId p_id, FactionId p_faction, float p_stability) {
	DistrictControl control;
	control.district_id = p_id;
	control.controlling_faction = p_faction;
	control.stability = p_stability;
	district_controls[p_id] = control;
}

FactionId CampaignState::get_controlling_faction(DistrictId p_id) const {
	auto it = district_controls.find(p_id);
	if (it == district_controls.end()) {
		return INVALID_FACTION_ID;
	}
	return it->second.controlling_faction;
}

float CampaignState::get_district_stability(DistrictId p_id) const {
	auto it = district_controls.find(p_id);
	if (it == district_controls.end()) {
		return 1.0f;
	}
	return it->second.stability;
}

void CampaignState::set_faction_reputation(FactionId p_id, float p_rep) {
	faction_reputation[p_id] = CLAMP(p_rep, -1.0f, 1.0f);
}

float CampaignState::get_faction_reputation(FactionId p_id) const {
	auto it = faction_reputation.find(p_id);
	if (it == faction_reputation.end()) {
		return 0.0f;
	}
	return it->second;
}

int32_t CampaignState::get_total_missions() const {
	return mission_history.size();
}

int32_t CampaignState::get_successful_missions() const {
	int32_t count = 0;
	for (int i = 0; i < mission_history.size(); i++) {
		if (mission_history[i].success) {
			count++;
		}
	}
	return count;
}

float CampaignState::get_success_rate() const {
	if (mission_history.empty()) {
		return 0.0f;
	}
	return (float)get_successful_missions() / (float)mission_history.size();
}
