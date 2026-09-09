#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node.hpp>
#include <map>
#include <vector>

using namespace godot;

struct AgentRecord {
	AgentId agent_id = INVALID_AGENT_ID;
	FactionId faction = INVALID_FACTION_ID;
	bool is_alive = true;
	bool was_extracted = false;
	int32_t missions_survived = 0;
};

struct DistrictControl {
	DistrictId district_id = 0;
	FactionId controlling_faction = INVALID_FACTION_ID;
	float stability = 1.0f; // 0-1
};

struct MissionOutcome {
	MissionId mission_id = 0;
	bool success = false;
	AgentId vip_agent_id = INVALID_AGENT_ID;
	bool vip_extracted = false;
	int32_t friendly_casualties = 0;
	int32_t enemy_casualties = 0;
	FactionId primary_faction = INVALID_FACTION_ID;
};

class CampaignState : public Node {
	GDCLASS(CampaignState, Node)

private:
	std::map<AgentId, AgentRecord> agent_records;
	std::map<DistrictId, DistrictControl> district_controls;
	std::vector<MissionOutcome> mission_history;
	std::map<FactionId, float> faction_reputation;

protected:
	static void _bind_methods();

public:
	CampaignState();
	~CampaignState();

	void _ready() override;

	void record_mission_outcome(MissionId p_mission_id, bool p_success, int32_t p_friendly_casualties, int32_t p_enemy_casualties);

	void record_agent_death(AgentId p_id);
	void record_agent_extraction(AgentId p_id);
	bool is_agent_alive(AgentId p_id) const;
	int32_t get_alive_agent_count() const;

	void set_district_control(DistrictId p_id, FactionId p_faction, float p_stability);
	FactionId get_controlling_faction(DistrictId p_id) const;
	float get_district_stability(DistrictId p_id) const;

	void set_faction_reputation(FactionId p_id, float p_rep);
	float get_faction_reputation(FactionId p_id) const;

	int32_t get_total_missions() const;
	int32_t get_successful_missions() const;
	float get_success_rate() const;
};
