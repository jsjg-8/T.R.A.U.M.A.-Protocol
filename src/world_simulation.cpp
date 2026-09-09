#include "world_simulation.h"
#include "agent.h"
#include "squad.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void WorldSimulation::_bind_methods() {
	ClassDB::bind_method(D_METHOD("tick", "delta"), &WorldSimulation::tick);
	ClassDB::bind_method(D_METHOD("get_agent_count"), &WorldSimulation::get_agent_count);
	ClassDB::bind_method(D_METHOD("add_agent", "agent"), &WorldSimulation::add_agent);
	ClassDB::bind_method(D_METHOD("remove_agent", "id"), &WorldSimulation::remove_agent);
	ClassDB::bind_method(D_METHOD("get_agent", "id"), &WorldSimulation::get_agent);
	ClassDB::bind_method(D_METHOD("add_squad", "squad"), &WorldSimulation::add_squad);
	ClassDB::bind_method(D_METHOD("remove_squad", "id"), &WorldSimulation::remove_squad);
	ClassDB::bind_method(D_METHOD("get_squad", "id"), &WorldSimulation::get_squad);
	ClassDB::bind_method(D_METHOD("get_squad_count"), &WorldSimulation::get_squad_count);
	ClassDB::bind_method(D_METHOD("set_debug_verbose", "verbose"), &WorldSimulation::set_debug_verbose);
	ClassDB::bind_method(D_METHOD("get_debug_verbose"), &WorldSimulation::get_debug_verbose);
	ClassDB::bind_method(D_METHOD("get_tick_count"), &WorldSimulation::get_tick_count);
	ClassDB::bind_method(D_METHOD("get_elapsed_time"), &WorldSimulation::get_elapsed_time);

	ClassDB::add_property("WorldSimulation", PropertyInfo(Variant::BOOL, "debug_verbose"), "set_debug_verbose", "get_debug_verbose");

	ADD_SIGNAL(MethodInfo("tick_completed", PropertyInfo(Variant::FLOAT, "elapsed_time")));
}

WorldSimulation::WorldSimulation() {
	debug_verbose = false;
}

WorldSimulation::~WorldSimulation() {
	agents.clear();
}

void WorldSimulation::_ready() {
	if (debug_verbose) {
		UtilityFunctions::print("WorldSimulation: ready");
	}
}

void WorldSimulation::_physics_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	tick(delta);
}

void WorldSimulation::tick(double delta) {
	next_state = current_state;
	next_state.elapsed_time += delta;
	next_state.tick_count++;

	tick_perception(delta);
	tick_squads(delta);
	tick_agents(delta);
	tick_factions(delta);
	tick_world_mutation(delta);
	tick_director_observation(delta);

	current_state = next_state;

	emit_signal("tick_completed", current_state.elapsed_time);
}

void WorldSimulation::tick_perception(double delta) {
	for (int i = 0; i < agents.size(); i++) {
		if (agents[i]) {
			agents[i]->update_perception(delta, current_state);
		}
	}
}

void WorldSimulation::tick_squads(double delta) {
	for (int i = 0; i < squads.size(); i++) {
		if (squads[i]) {
			squads[i]->update(delta, next_state);
		}
	}
}

void WorldSimulation::tick_agents(double delta) {
	for (int i = 0; i < agents.size(); i++) {
		if (agents[i] && agents[i]->get_is_alive()) {
			agents[i]->update(delta, current_state);
		}
	}
}

void WorldSimulation::tick_factions(double delta) {
	// Phase 3: Faction updates
}

void WorldSimulation::tick_world_mutation(double delta) {
	for (int i = 0; i < agents.size(); i++) {
		if (agents[i]) {
			AgentId aid = agents[i]->get_agent_id();
			for (int j = 0; j < next_state.agents.size(); j++) {
				if (next_state.agents[j].id == aid) {
					next_state.agents[j].position = agents[i]->get_position();
					break;
				}
			}
		}
	}
}

void WorldSimulation::tick_director_observation(double delta) {
	// Phase 3: Director reads WorldState
}

void WorldSimulation::add_agent(Agent *agent) {
	if (!agent) {
		return;
	}
	agents.push_back(agent);

	AgentState state;
	state.id = agent->get_agent_id();
	state.faction = agent->get_faction_id();
	state.room = agent->get_room_id();
	state.position = agent->get_position();
	state.health = agent->get_health();
	state.max_health = agent->get_max_health();
	next_state.agents.push_back(state);

	if (debug_verbose) {
		UtilityFunctions::print("WorldSimulation: added agent ", state.id);
	}
}

void WorldSimulation::remove_agent(AgentId id) {
	for (int i = agents.size() - 1; i >= 0; i--) {
		if (agents[i] && agents[i]->get_agent_id() == id) {
			agents.erase(agents.begin() + i);
			break;
		}
	}
	for (int i = next_state.agents.size() - 1; i >= 0; i--) {
		if (next_state.agents[i].id == id) {
			next_state.agents.erase(next_state.agents.begin() + i);
			break;
		}
	}
}

Agent *WorldSimulation::get_agent(AgentId id) const {
	for (int i = 0; i < agents.size(); i++) {
		if (agents[i] && agents[i]->get_agent_id() == id) {
			return agents[i];
		}
	}
	return nullptr;
}

int32_t WorldSimulation::get_agent_count() const {
	return agents.size();
}

uint64_t WorldSimulation::get_tick_count() const {
	return current_state.tick_count;
}

float WorldSimulation::get_elapsed_time() const {
	return current_state.elapsed_time;
}

void WorldSimulation::set_debug_verbose(bool verbose) {
	debug_verbose = verbose;
}

bool WorldSimulation::get_debug_verbose() const {
	return debug_verbose;
}

void WorldSimulation::add_squad(Squad *squad) {
	if (!squad) {
		return;
	}
	squads.push_back(squad);
	if (debug_verbose) {
		UtilityFunctions::print("WorldSimulation: added squad ", squad->get_squad_id());
	}
}

void WorldSimulation::remove_squad(SquadId id) {
	for (int i = squads.size() - 1; i >= 0; i--) {
		if (squads[i] && squads[i]->get_squad_id() == id) {
			squads.erase(squads.begin() + i);
			return;
		}
	}
}

Squad *WorldSimulation::get_squad(SquadId id) const {
	for (int i = 0; i < squads.size(); i++) {
		if (squads[i] && squads[i]->get_squad_id() == id) {
			return squads[i];
		}
	}
	return nullptr;
}

int32_t WorldSimulation::get_squad_count() const {
	return squads.size();
}
