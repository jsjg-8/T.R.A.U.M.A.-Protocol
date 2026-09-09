#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node.hpp>
#include <vector>

using namespace godot;

class Agent;
class Director;
class FactionRegistry;
class Squad;

class WorldSimulation : public Node {
	GDCLASS(WorldSimulation, Node)

private:
	WorldState current_state;
	WorldState next_state;

	std::vector<Agent *> agents;
	std::vector<Squad *> squads;

	// Optional observers — attached by scene, never owned here.
	// Director only reads the snapshot and enqueues events for next tick;
	// it never calls Agent/Squad methods directly (god-object boundary).
	Director *director = nullptr;
	FactionRegistry *faction_registry = nullptr;

	bool debug_verbose = false;

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

	void add_squad(Squad *squad);
	void remove_squad(SquadId id);
	Squad *get_squad(SquadId id) const;
	int32_t get_squad_count() const;

	void set_director(Director *p_director);
	Director *get_director() const;
	void set_faction_registry(FactionRegistry *p_registry);
	FactionRegistry *get_faction_registry() const;

	const WorldState &get_current_state() const { return current_state; }
	WorldState &get_mutable_state() { return next_state; }

	// GDScript-visible snapshot queries (WorldState itself is pure C++,
	// not Variant-compatible, so expose scalars instead).
	uint64_t get_tick_count() const;
	float get_elapsed_time() const;

	void set_debug_verbose(bool verbose);
	bool get_debug_verbose() const;
};
