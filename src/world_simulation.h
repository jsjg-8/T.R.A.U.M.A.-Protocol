#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node.hpp>
#include <vector>

using namespace godot;

class Agent;

class WorldSimulation : public Node {
	GDCLASS(WorldSimulation, Node)

private:
	WorldState current_state;
	WorldState next_state;

	std::vector<Agent *> agents;

	void tick_perception(double delta);
	void tick_squads(double delta);
	void tick_agents(double delta);
	void tick_factions(double delta);
	void tick_world_mutation(double delta);
	void tick_director_observation(double delta);

protected:
	static void _bind_methods();

public:
	WorldSimulation();
	~WorldSimulation();

	void _ready() override;
	void _physics_process(double delta) override;

	void tick(double delta);

	void add_agent(Agent *agent);
	void remove_agent(AgentId id);
	Agent *get_agent(AgentId id) const;
	int32_t get_agent_count() const;

	const WorldState &get_current_state() const { return current_state; }
	WorldState &get_mutable_state() { return next_state; }

	void set_debug_verbose(bool verbose);
	bool get_debug_verbose() const;

private:
	bool debug_verbose = false;
};
