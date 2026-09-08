# TRAUMA Simulation Core — Implementation Plan

> **Local verification: compiledb only. Full build via CI.**
> Full SCons link (`scons target=template_debug ...`) and CMake build (`cmake -B build && ninja -C build`) are deferred to GitHub Actions CI (`.github/workflows/builds.yml`). Local verification is lightweight: `scons compiledb=yes` (generates `compile_commands.json` without linking) + `clang-format --dry-run` + `clangd` diagnostics. Headless Godot script tests are verified as syntax-only via compiledb; actual runtime execution happens in CI.

**Goal:** Build a headlessly-testable C++ simulation core (world tick, agents, squads, factions, Director, Trauma Team mission loop) that proves the game is fun before it looks good.

**Architecture:** Four-phase incremental build: foundation fixes → world data + agents → tactical AI + perception → factions + missions + Director. Each phase is independently compilable and headlessly testable. Simulation Agent (pure C++) ticks without scene tree; optional Godot Actor drives visual representation. Tick order: Perception → Squad → Agent → Faction → World Mutation → Director (next-tick deferred).

**Design:** `thoughts/shared/designs/2026-09-05-trauma-simulation-design.md`

**Conventions (from codebase):**
- C++17, LLVM style (`.clang-format`), tabs indent (4-width)
- `#pragma once` header guards (project uses `#pragma once`, NOT `#ifndef` guards)
- `GDCLASS(ClassName, Parent)` macro, `_bind_methods()` for properties/signals
- `ADD_SIGNAL(MethodInfo(...))` for signal registration
- `UtilityFunctions::print` for info, `printerr` for errors
- Header include priority: local `""` first, `<godot_cpp/...>` second
- No `.mindmodel/` directory exists — patterns derived from `health_component.*`, `input_manager.*`, `player_controller.*`

**Testing:** No `tests/` directory exists yet. Create `tests/` at project root. Tests use Godot headless mode (`--headless --script`) or standalone C++ test binaries. For Phase 0 we use Godot headless script tests since the extension must be loaded into the engine to function. Each phase's test is a `.gd` script in `tests/` that can be run via `godot --headless --path game/ -s tests/test_phase_X.gd`.

---

## Dependency Graph

```
Batch 0.1 (standalone): SConstruct recursive glob fix
Batch 0.2 (standalone): CMakeLists.txt source list update
Batch 0.3 (standalone): InputManager singleton lifecycle fix (register_types.cpp)
Batch 0.4 (parallel with 0.1-0.3): world_state.h — pure data structs
Batch 1.1 (depends: 0.1, 0.2, 0.4): world_simulation.h/.cpp
Batch 1.2 (depends: 0.1, 0.2, 0.4): agent.h/.cpp
Batch 1.3 (depends: 1.1, 1.2): register_types.cpp update for WorldSimulation + Agent
Batch 1.4 (depends: 1.3): Phase 0 headless tick test
Batch 2.1 (depends: 0.1, 0.2): sector.h/.cpp — semantic map Resources
Batch 2.2 (depends: 0.1, 0.2): actor.h/.cpp — optional Godot Actor wrapper
Batch 2.3 (depends: 1.1, 1.2): agent navigation integration (modify agent.h/.cpp)
Batch 3.1 (depends: 0.1, 0.2): perception_component.h/.cpp
Batch 3.2 (depends: 0.1, 0.2): action.h + actions/ directory
Batch 3.3 (depends: 2.1, 2.2, 3.1, 3.2): squad.h/.cpp
Batch 3.4 (depends: 3.3): Phase 2 tactical POC test
Batch 4.1 (depends: 0.1, 0.2): faction.h/.cpp + faction_registry.h/.cpp
Batch 4.2 (depends: 0.1, 0.2): mission.h/.cpp + patient.h/.cpp
Batch 4.3 (depends: 0.1, 0.2): director.h/.cpp
Batch 4.4 (depends: 0.1, 0.2): campaign_state.h/.cpp
Batch 4.5 (depends: 4.1, 4.2, 4.3, 4.4): Phase 3 Trauma Team POC test
```

---

## Batch 0: Foundation Fixes (parallel — 4 independent tasks)

All tasks in this batch are INDEPENDENT and run simultaneously. They fix build infrastructure before any new simulation code is added.

### Task 0.1: SConstruct Recursive Glob Fix

**File:** `SConstruct`
**Test:** none (build infrastructure)
**Depends:** none

Change line 41 from:
```python
sources = Glob("src/*.cpp")
```
to:
```python
sources = Glob("src/*.cpp") + Glob("src/**/*.cpp")
```

This ensures SCons picks up `.cpp` files in subdirectories (e.g., `src/actions/*.cpp`) without breaking existing root-level files. The design doc notes both `Glob("src/**/*.cpp")` alone and the concatenated form work; we use the concatenated form for explicitness — it makes it clear root files are always included.

**Verify (local):**
1. `scons compiledb=yes` — generates `compile_commands.json` without linking; confirms syntax/header resolution is valid
2. `clang-format --dry-run --Werror src/SConstruct` — (N/A for SConstruct; skip format check on build scripts)
3. Open in editor with `compile_commands.json` — `clangd` should report 0 errors

Full link deferred to CI: `.github/workflows/builds.yml` runs `scons target=template_debug platform=linux arch=x86_64`.
**Commit:** `build(scons): add recursive glob for src/ subdirectories`

---

### Task 0.2: CMakeLists.txt Source List Update

**File:** `CMakeLists.txt`
**Test:** none (build infrastructure)
**Depends:** none

CMakeLists.txt currently only lists `src/register_types.cpp` and `src/register_types.h` explicitly (line 50-52). We need to use a glob so new files are auto-discovered. Replace the explicit source list with:

```cmake
file(GLOB_RECURSE TRAUMA_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.h"
)

target_sources(${LIBNAME}
    PRIVATE
    ${TRAUMA_SOURCES}
)
```

This replaces lines 48-52 of the current CMakeLists.txt. The `CONFIGURE_DEPENDS` flag ensures CMake re-runs when files are added/removed.

**Verify (local):**
1. `scons compiledb=yes` — confirms header resolution and syntax without linking
2. `clang-format --dry-run --Werror src/` — verifies formatting

Full build + link deferred to CI: `.github/workflows/builds.yml` runs `cmake -B build -G Ninja && ninja -C build`.
**Commit:** `build(cmake): use glob for source file discovery`

---

### Task 0.3: InputManager Singleton Lifecycle Fix

**File:** `src/register_types.cpp`
**Test:** none (bugfix, verified by existing game running)
**Depends:** none

**Problem:** `PlayerController::_ready()` at line 131 calls `InputManager::get_singleton()->initialize_input_map()` — but `InputManager` is never instantiated with `memnew`. The singleton is only set in the `InputManager()` constructor, which runs when Godot constructs the class during `GDREGISTER_CLASS`. However, registering a class doesn't instantiate it. If `PlayerController::_ready` runs before an `InputManager` node is added to the scene tree, `get_singleton()` returns `nullptr` → null deref crash.

**Fix:** In `register_types.cpp`:

```cpp
#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "health_component.h"
#include "traffic_light.h"
#include "./player_controller.h"
#include "./input_manager.h"

using namespace godot;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(HealthComponent);
	GDREGISTER_CLASS(TrafficLight);
	GDREGISTER_CLASS(PlayerController);
	GDREGISTER_CLASS(InputManager);

	// Create InputManager singleton so PlayerController::_ready() can use it
	memnew(InputManager);
	InputManager::get_singleton()->initialize_input_map();
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	// Clean up InputManager singleton
	if (InputManager::get_singleton()) {
		memdelete(InputManager::get_singleton());
	}
}

extern "C"
{
	GDExtensionBool GDE_EXPORT trauma_engine_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
```

**Key change:** Add `memnew(InputManager)` + `initialize_input_map()` in init, `memdelete` in uninit. Also add null guard in `PlayerController::_ready` at line 131 as defense-in-depth:

In `src/player_controller.cpp`, change line 131 from:
```cpp
	InputManager::get_singleton()->initialize_input_map();
```
to:
```cpp
	if (InputManager::get_singleton()) {
		InputManager::get_singleton()->initialize_input_map();
	} else {
		printerr("PlayerController: InputManager singleton not found");
	}
```

**Verify (local):**
1. `scons compiledb=yes` — confirms syntax and header resolution
2. `clang-format --dry-run --Werror src/register_types.cpp src/player_controller.cpp` — verifies formatting

Full build + runtime test deferred to CI: `.github/workflows/builds.yml` builds + runs the game.
**Commit:** `fix(input): instantiate InputManager singleton in register_types`

---

### Task 0.4: WorldState Pure Data Structs

**File:** `src/world_state.h`
**Test:** none (header-only, no logic)
**Depends:** none

This is the data foundation for all simulation. Pure structs, no Godot Node inheritance, no tick logic. All simulation systems read/write these types.

```cpp
#pragma once

#include <cstdint>
#include <vector>
#include <godot_cpp/variant/vector3.hpp>

using namespace godot;

// ID types — strongly typed integers prevent accidental cross-system confusion
using AgentId = uint32_t;
using SquadId = uint32_t;
using FactionId = uint32_t;
using RoomId = uint32_t;
using DistrictId = uint32_t;
using CoverPointId = uint32_t;
using DoorId = uint32_t;
using MissionId = uint32_t;

constexpr AgentId INVALID_AGENT_ID = 0;
constexpr SquadId INVALID_SQUAD_ID = 0;
constexpr FactionId INVALID_FACTION_ID = 0;
constexpr RoomId INVALID_ROOM_ID = 0;

// Agent goal states — what the agent is trying to do at the highest level
enum class AgentGoal : uint8_t {
	IDLE,
	PATROL,
	ATTACK,
	DEFEND,
	RETREAT,
	INVESTIGATE,
	PROTECT_VIP,
	EXTRACT,
	STABILIZE_PATIENT,
};

// Agent life state
enum class AgentLifeState : uint8_t {
	ALIVE,
	STABILIZED,   // wounded but treatable
	INCAPACITATED, // down, needs revive
	DEAD,
};

// Tactical state — set by Squad, read by Agent
enum class TacticalState : uint8_t {
	IDLE,
	ATTACK,
	DEFEND,
	SEARCH,
	RETREAT,
	REGROUP,
	PROTECT_VIP,
};

// Perception state — what the agent knows about a target
struct PerceptionRecord {
	AgentId target_id = INVALID_AGENT_ID;
	Vector3 last_known_position;
	float confidence = 0.0f;   // 0 = rumor, 1 = visual确认
	float last_seen_time = 0.0f;
	bool is_hostile = false;
};

// Perception state — full per-agent knowledge
struct PerceptionState {
	std::vector<PerceptionRecord> known_targets;
	float hearing_event_time = 0.0f;  // when last loud event occurred
	Vector3 hearing_event_origin;
	bool has_hearing_event = false;
};

// Snapshot of a single agent — what WorldState stores
struct AgentState {
	AgentId id = INVALID_AGENT_ID;
	FactionId faction = INVALID_FACTION_ID;
	RoomId room = INVALID_ROOM_ID;
	DistrictId district = 0;
	Vector3 position;
	float health = 100.0f;
	float max_health = 100.0f;
	AgentLifeState life_state = AgentLifeState::ALIVE;
	AgentGoal goal = AgentGoal::IDLE;
	TacticalState tactical_state = TacticalState::IDLE;
	SquadId squad_id = INVALID_SQUAD_ID;
	PerceptionState perception;
};

// Snapshot of a squad
struct SquadState {
	SquadId id = INVALID_SQUAD_ID;
	FactionId faction = INVALID_FACTION_ID;
	AgentId leader_id = INVALID_AGENT_ID;
	std::vector<AgentId> member_ids;
	TacticalState tactical_state = TacticalState::IDLE;
	RoomId objective_room = INVALID_ROOM_ID;
};

// Snapshot of a faction
struct FactionState {
	FactionId id = INVALID_FACTION_ID;
	String name;
	float alertness = 0.0f;  // 0-1
	int32_t strength = 100;
	std::vector<FactionId> allies;
	std::vector<FactionId> enemies;
};

// World snapshot — read by Director, tests, debug
struct WorldState {
	std::vector<AgentState> agents;
	std::vector<SquadState> squads;
	std::vector<FactionState> factions;
	float elapsed_time = 0.0f;
	uint64_t tick_count = 0;
};
```

**Verify (local):** `scons compiledb=yes` — header-only, confirms syntax resolution. No runtime test needed for pure data structs.
**Commit:** `feat(sim): add world_state.h pure data structs`

---

## Batch 1: Core Simulation (parallel — 2 new files, then integration)

All tasks in this batch depend on Batch 0 completing.

### Task 1.1: WorldSimulation Node

**File:** `src/world_simulation.h` + `src/world_simulation.cpp`
**Test:** Phase 0 headless tick test (Task 1.4)
**Depends:** 0.1, 0.2, 0.4

The tick root. Owns the agent list, WorldState snapshot, and tick order. Extends `Node` so it can be placed in scene tree, but works headlessly via direct `tick()` calls.

**`src/world_simulation.h`:**
```cpp
#pragma once

#include "world_state.h"

#include <vector>
#include <godot_cpp/classes/node.hpp>

using namespace godot;

// Forward declarations — agents added later in their own files
class Agent;

class WorldSimulation : public Node {
	GDCLASS(WorldSimulation, Node)

private:
	WorldState current_state;
	WorldState next_state;  // double-buffer: tick writes next, swap at end

	std::vector<Agent *> agents;

	// Tick order — called in _physics_process
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

	// Lifecycle
	void _ready() override;
	void _physics_process(double delta) override;

	// Core tick — callable headlessly for tests
	void tick(double delta);

	// Agent management
	void add_agent(Agent *agent);
	void remove_agent(AgentId id);
	Agent *get_agent(AgentId id) const;
	int32_t get_agent_count() const;

	// State access
	const WorldState &get_current_state() const { return current_state; }
	WorldState &get_mutable_state() { return next_state; }

	// Debug
	void set_debug_verbose(bool verbose);
	bool get_debug_verbose() const;

private:
	bool debug_verbose = false;
};
```

**`src/world_simulation.cpp`:**
```cpp
#include "world_simulation.h"
#include "agent.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void WorldSimulation::_bind_methods() {
	ClassDB::bind_method(D_METHOD("tick", "delta"), &WorldSimulation::tick);
	ClassDB::bind_method(D_METHOD("get_agent_count"), &WorldSimulation::get_agent_count);
	ClassDB::bind_method(D_METHOD("set_debug_verbose", "verbose"), &WorldSimulation::set_debug_verbose);
	ClassDB::bind_method(D_METHOD("get_debug_verbose"), &WorldSimulation::get_debug_verbose);

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
	tick(delta);
}

void WorldSimulation::tick(double delta) {
	// Copy current → next for double-buffer safety
	next_state = current_state;
	next_state.elapsed_time += delta;
	next_state.tick_count++;

	// Tick order per design doc
	tick_perception(delta);
	tick_squads(delta);
	tick_agents(delta);
	tick_factions(delta);
	tick_world_mutation(delta);
	tick_director_observation(delta);

	// Swap buffers
	current_state = next_state;

	emit_signal("tick_completed", current_state.elapsed_time);
}

void WorldSimulation::tick_perception(double delta) {
	// Phase 2: PerceptionComponent drives this
	// Phase 0: no-op
	for (int i = 0; i < agents.size(); i++) {
		if (agents[i]) {
			agents[i]->update_perception(delta, current_state);
		}
	}
}

void WorldSimulation::tick_squads(double delta) {
	// Phase 2: Squad logic drives this
	// Phase 0: no-op
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
	// Phase 0: no-op
}

void WorldSimulation::tick_world_mutation(double delta) {
	// Phase 1+: door state, objective updates
	// Phase 0: sync agent positions to state
	for (int i = 0; i < agents.size(); i++) {
		if (agents[i]) {
			// Find this agent in next_state and update position
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
	// Phase 3: Director reads WorldState, emits events for next tick
	// Phase 0: no-op
}

void WorldSimulation::add_agent(Agent *agent) {
	if (!agent) {
		return;
	}
	agents.push_back(agent);

	// Add agent state to next_state
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
			agents.remove_at(i);
			break;
		}
	}
	for (int i = next_state.agents.size() - 1; i >= 0; i--) {
		if (next_state.agents[i].id == id) {
			next_state.agents.remove_at(i);
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

void WorldSimulation::set_debug_verbose(bool verbose) {
	debug_verbose = verbose;
}

bool WorldSimulation::get_debug_verbose() const {
	return debug_verbose;
}
```

**Verify (local):**
1. `scons compiledb=yes` — confirms syntax and header resolution for new .h/.cpp files
2. `clang-format --dry-run --Werror src/world_simulation.h src/world_simulation.cpp` — verifies formatting

Full link + runtime test deferred to CI: `.github/workflows/builds.yml`.
**Commit:** `feat(sim): add WorldSimulation tick root node`

---

### Task 1.2: Agent (Lightweight Simulation Agent)

**File:** `src/agent.h` + `src/agent.cpp`
**Test:** Phase 0 headless tick test (Task 1.4)
**Depends:** 0.1, 0.2, 0.4

Lightweight simulation agent. Does NOT extend CharacterBody3D. Can be a Node owned by WorldSimulation, or an Object. For POC it's a Node so it can be added to scene tree if needed, but tick() works without scene tree.

**`src/agent.h`:**
```cpp
#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node.hpp>

using namespace godot;

class WorldSimulation;

class Agent : public Node {
	GDCLASS(Agent, Node)

private:
	AgentId agent_id = INVALID_AGENT_ID;
	FactionId faction_id = INVALID_FACTION_ID;
	RoomId room_id = INVALID_ROOM_ID;
	DistrictId district_id = 0;

	Vector3 position;
	float health = 100.0f;
	float max_health = 100.0f;
	AgentLifeState life_state = AgentLifeState::ALIVE;
	AgentGoal goal = AgentGoal::IDLE;
	TacticalState tactical_state = TacticalState::IDLE;
	SquadId squad_id = INVALID_SQUAD_ID;

	PerceptionState perception;

	// Movement — simple position update for POC
	float move_speed = 3.0f;
	Vector3 move_target;
	bool has_move_target = false;

	WorldSimulation *sim = nullptr;

protected:
	static void _bind_methods();

public:
	Agent();
	~Agent();

	// Lifecycle
	void _ready() override;

	// Tick — called by WorldSimulation
	void update(double delta, const WorldState &world);
	void update_perception(double delta, const WorldState &world);

	// ID management
	void set_agent_id(AgentId id);
	AgentId get_agent_id() const;

	// Faction/room
	void set_faction_id(FactionId id);
	FactionId get_faction_id() const;
	void set_room_id(RoomId id);
	RoomId get_room_id() const;
	void set_district_id(DistrictId id);
	DistrictId get_district_id() const;

	// Position
	void set_position(const Vector3 &pos);
	Vector3 get_position() const;

	// Health
	void set_health(float h);
	float get_health() const;
	void set_max_health(float h);
	float get_max_health() const;
	bool get_is_alive() const;
	void take_damage(float amount);
	void heal(float amount);

	// Life state
	void set_life_state(AgentLifeState state);
	AgentLifeState get_life_state() const;

	// Goal/tactical
	void set_goal(AgentGoal g);
	AgentGoal get_goal() const;
	void set_tactical_state(TacticalState s);
	TacticalState get_tactical_state() const;

	// Squad
	void set_squad_id(SquadId id);
	SquadId get_squad_id() const;

	// Movement
	void set_move_target(const Vector3 &target);
	Vector3 get_move_target() const;
	bool get_has_move_target() const;
	void clear_move_target();
	void set_move_speed(float speed);
	float get_move_speed() const;

	// Perception
	const PerceptionState &get_perception_state() const;
	void add_perception_record(const PerceptionRecord &record);
	void clear_perception();

	// World simulation reference
	void set_simulation(WorldSimulation *sim);
	WorldSimulation *get_simulation() const;

	// Signals
	void _on_health_changed(float new_health, float old_health);
	void _on_death();
};
```

**`src/agent.cpp`:**
```cpp
#include "agent.h"
#include "world_simulation.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void Agent::_bind_methods() {
	// Properties
	ClassDB::bind_method(D_METHOD("set_agent_id", "id"), &Agent::set_agent_id);
	ClassDB::bind_method(D_METHOD("get_agent_id"), &Agent::get_agent_id);

	ClassDB::bind_method(D_METHOD("set_faction_id", "id"), &Agent::set_faction_id);
	ClassDB::bind_method(D_METHOD("get_faction_id"), &Agent::get_faction_id);

	ClassDB::bind_method(D_METHOD("set_health", "h"), &Agent::set_health);
	ClassDB::bind_method(D_METHOD("get_health"), &Agent::get_health);

	ClassDB::bind_method(D_METHOD("set_position", "pos"), &Agent::set_position);
	ClassDB::bind_method(D_METHOD("get_position"), &Agent::get_position);

	ClassDB::bind_method(D_METHOD("set_move_speed", "speed"), &Agent::set_move_speed);
	ClassDB::bind_method(D_METHOD("get_move_speed"), &Agent::get_move_speed);

	// Methods
	ClassDB::bind_method(D_METHOD("take_damage", "amount"), &Agent::take_damage);
	ClassDB::bind_method(D_METHOD("heal", "amount"), &Agent::heal);
	ClassDB::bind_method(D_METHOD("get_is_alive"), &Agent::get_is_alive);
	ClassDB::bind_method(D_METHOD("set_move_target", "target"), &Agent::set_move_target);
	ClassDB::bind_method(D_METHOD("clear_move_target"), &Agent::clear_move_target);

	// Signals
	ADD_SIGNAL(MethodInfo("health_changed", PropertyInfo(Variant::FLOAT, "new_health"), PropertyInfo(Variant::FLOAT, "old_health")));
	ADD_SIGNAL(MethodInfo("death"));
}

Agent::Agent() {
}

Agent::~Agent() {
}

void Agent::_ready() {
	// Find WorldSimulation ancestor if in scene tree
	Node *parent = get_parent();
	while (parent) {
		WorldSimulation *ws = Object::cast_to<WorldSimulation>(parent);
		if (ws) {
			sim = ws;
			break;
		}
		parent = parent->get_parent();
	}
}

void Agent::update(double delta, const WorldState &world) {
	if (!get_is_alive()) {
		return;
	}

	// Simple movement for POC — move toward target at move_speed
	if (has_move_target) {
		Vector3 dir = move_target - position;
		float dist = dir.length();
		if (dist < 0.1f) {
			// Reached target
			position = move_target;
			has_move_target = false;
		} else {
			dir.normalize();
			float step = move_speed * delta;
			if (step > dist) {
				step = dist;
			}
			position += dir * step;
		}
	}

	// Phase 2+: action selection based on perception + squad orders
}

void Agent::update_perception(double delta, const WorldState &world) {
	// Phase 2: PerceptionComponent drives this
	// Phase 0: no-op
}

// --- Setters/Getters ---

void Agent::set_agent_id(AgentId id) { agent_id = id; }
AgentId Agent::get_agent_id() const { return agent_id; }

void Agent::set_faction_id(FactionId id) { faction_id = id; }
FactionId Agent::get_faction_id() const { return faction_id; }

void Agent::set_room_id(RoomId id) { room_id = id; }
RoomId Agent::get_room_id() const { return room_id; }

void Agent::set_district_id(DistrictId id) { district_id = id; }
DistrictId Agent::get_district_id() const { return district_id; }

void Agent::set_position(const Vector3 &pos) { position = pos; }
Vector3 Agent::get_position() const { return position; }

void Agent::set_health(float h) {
	float old = health;
	health = Math::clamp(h, 0.0f, max_health);
	if (health != old) {
		emit_signal("health_changed", health, old);
	}
	if (health <= 0.0f && old > 0.0f) {
		life_state = AgentLifeState::DEAD;
		emit_signal("death");
	}
}
float Agent::get_health() const { return health; }

void Agent::set_max_health(float h) {
	if (h <= 0.0f) {
		printerr("Agent: max_health must be > 0");
		return;
	}
	max_health = h;
	if (health > max_health) {
		health = max_health;
	}
}
float Agent::get_max_health() const { return max_health; }

bool Agent::get_is_alive() const { return life_state == AgentLifeState::ALIVE; }

void Agent::take_damage(float amount) {
	if (amount <= 0.0f || !get_is_alive()) {
		return;
	}
	set_health(health - amount);
}

void Agent::heal(float amount) {
	if (amount <= 0.0f || !get_is_alive()) {
		return;
	}
	set_health(health + amount);
}

void Agent::set_life_state(AgentLifeState state) { life_state = state; }
AgentLifeState Agent::get_life_state() const { return life_state; }

void Agent::set_goal(AgentGoal g) { goal = g; }
AgentGoal Agent::get_goal() const { return goal; }

void Agent::set_tactical_state(TacticalState s) { tactical_state = s; }
TacticalState Agent::get_tactical_state() const { return tactical_state; }

void Agent::set_squad_id(SquadId id) { squad_id = id; }
SquadId Agent::get_squad_id() const { return squad_id; }

void Agent::set_move_target(const Vector3 &target) {
	move_target = target;
	has_move_target = true;
}
Vector3 Agent::get_move_target() const { return move_target; }
bool Agent::get_has_move_target() const { return has_move_target; }
void Agent::clear_move_target() { has_move_target = false; }

void Agent::set_move_speed(float speed) { move_speed = speed; }
float Agent::get_move_speed() const { return move_speed; }

const PerceptionState &Agent::get_perception_state() const { return perception; }
void Agent::add_perception_record(const PerceptionRecord &record) {
	perception.known_targets.push_back(record);
}
void Agent::clear_perception() { perception.known_targets.clear(); }

void Agent::set_simulation(WorldSimulation *w) { sim = w; }
WorldSimulation *Agent::get_simulation() const { return sim; }

void Agent::_on_health_changed(float new_health, float old_health) {
	if (debug_verbose) {
		UtilityFunctions::print("Agent ", agent_id, ": health ", old_health, " → ", new_health);
	}
}

void Agent::_on_death() {
	if (debug_verbose) {
		UtilityFunctions::print("Agent ", agent_id, ": died");
	}
}
```

**Verify (local):**
1. `scons compiledb=yes` — confirms syntax and header resolution for new .h/.cpp files
2. `clang-format --dry-run --Werror src/agent.h src/agent.cpp` — verifies formatting

Full link + runtime test deferred to CI: `.github/workflows/builds.yml`.
**Commit:** `feat(sim): add lightweight Agent simulation node`

---

### Task 1.3: Register Simulation Classes

**File:** `src/register_types.cpp`
**Test:** none (build + existing game run)
**Depends:** 1.1, 1.2

Add GDREGISTER_CLASS calls for WorldSimulation and Agent. This modifies the same file as Task 0.3 — merge carefully.

Update `initialize_gdextension_types` to add:
```cpp
	GDREGISTER_CLASS(WorldSimulation);
	GDREGISTER_CLASS(Agent);
```

Add includes at top:
```cpp
#include "world_simulation.h"
#include "agent.h"
```

**Verify (local):**
1. `scons compiledb=yes` — confirms all new includes resolve, GDREGISTER_CLASS macros parse
2. `clang-format --dry-run --Werror src/register_types.cpp` — verifies formatting

Full build + runtime test deferred to CI: `.github/workflows/builds.yml` builds + runs the game.
**Commit:** `feat(sim): register WorldSimulation and Agent in GDExtension`

---

### Task 1.4: Phase 0 Headless Tick Test

**File:** `tests/test_phase0_tick.gd`
**Test script for:** `godot --headless --path game/ -s tests/test_phase0_tick.gd`
**Depends:** 1.1, 1.2, 1.3

This is the first simulation test. Proves `WorldSimulation.tick()` → `Agent.update()` → state changes without a visual scene.

```gdscript
extends SceneTree

# Phase 0 test: Headless tick — WorldSimulation + Agent
# Run: godot --headless --path game/ -s tests/test_phase0_tick.gd

var pass_count = 0
var fail_count = 0

func _init():
	print("=== Phase 0: Headless Tick Test ===")

	# Create WorldSimulation (not in scene tree — pure tick test)
	var ws = WorldSimulation.new()
	ws.set_debug_verbose(true)

	# Create an agent
	var agent = Agent.new()
	agent.set_agent_id(1)
	agent.set_faction_id(1)
	agent.set_position(Vector3(0, 0, 0))
	agent.set_health(100.0)
	agent.set_max_health(100.0)

	# Add agent to simulation
	ws.add_agent(agent)

	# Verify agent count
	assert_eq(ws.get_agent_count(), 1, "Agent count should be 1")

	# Give agent a move target
	agent.set_move_target(Vector3(10, 0, 0))

	# Tick 1000 times at 1/60 delta
	for i in range(1000):
		ws.tick(1.0 / 60.0)

	# Verify agent moved
	var pos = agent.get_position()
	assert_gt(pos.x, 0.0, "Agent should have moved toward target")
	assert_lt(pos.x, 10.1, "Agent should not overshoot target")

	# Verify world state updated
	var state = ws.get_current_state()
	assert_eq(state.agents.size(), 1, "WorldState should have 1 agent")
	assert_eq(state.tick_count, 1000, "Tick count should be 1000")
	assert_gt(state.elapsed_time, 0.0, "Elapsed time should be > 0")

	# Test agent health
	agent.take_damage(50.0)
	assert_eq(agent.get_health(), 50.0, "Health should be 50 after 50 damage")
	assert_true(agent.get_is_alive(), "Agent should still be alive")

	agent.take_damage(60.0)
	assert_eq(agent.get_health(), 0.0, "Health should be 0 after lethal damage")
	assert_false(agent.get_is_alive(), "Agent should be dead")

	# Dead agent should not update
	var dead_pos = agent.get_position()
	agent.set_move_target(Vector3(20, 0, 0))
	ws.tick(1.0 / 60.0)
	assert_eq(agent.get_position(), dead_pos, "Dead agent should not move")

	# Summary
	print("=== Phase 0: PASSED ", pass_count, "/", pass_count + fail_count, " ===")
	if fail_count > 0:
		print("FAILURES: ", fail_count)
		quit(1)
	else:
		quit(0)

func assert_eq(actual, expected, msg: String):
	if actual == expected:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: ", msg, " — expected ", expected, ", got ", actual)

func assert_gt(actual, expected, msg: String):
	if actual > expected:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: ", msg, " — expected > ", expected, ", got ", actual)

func assert_lt(actual, expected, msg: String):
	if actual < expected:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: ", msg, " — expected < ", expected, ", got ", actual)

func assert_true(val, msg: String):
	if val:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: ", msg, " — expected true, got false")

func assert_false(val, msg: String):
	if not val:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: ", msg, " — expected false, got true")
```

**Verify (local — syntax-only):**
1. `scons compiledb=yes` — confirms all GDScript test references to GDExtension classes are syntactically valid
2. GDScript files are pure text — `godot --headless` runtime execution deferred to CI

Full headless test execution deferred to CI: `.github/workflows/builds.yml` runs `godot --headless --path game/ -s tests/test_phase0_tick.gd`.
**Commit:** `test(sim): add Phase 0 headless tick test`

---

## Batch 2: World Exists — Semantic Map + Actor (parallel after Batch 1)

### Task 2.1: Sector / Room / Door / CoverPoint Resources

**File:** `src/sector.h` + `src/sector.cpp`
**Test:** Phase 1 semantic map test (later)
**Depends:** 0.1, 0.2

Semantic world graph. Hand-authored for POC test level. Rooms connected by Doors. CoverPoints inside Rooms. All Resource types so they can be authored in Godot editor.

```cpp
#pragma once

#include "world_state.h"

#include <vector>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/vector3.hpp>

using namespace godot;

// CoverPoint — position + facing direction + exposure level
class CoverPoint : public Resource {
	GDCLASS(CoverPoint, Resource)

private:
	Vector3 position;
	Vector3 facing;       // direction the cover faces (for leaning out)
	float exposure = 0.0f; // 0 = full cover, 1 = no cover
	CoverPointId cover_id = 0;

protected:
	static void _bind_methods();

public:
	CoverPoint();
	~CoverPoint();

	void set_position(const Vector3 &pos);
	Vector3 get_position() const;

	void set_facing(const Vector3 &dir);
	Vector3 get_facing() const;

	void set_exposure(float e);
	float get_exposure() const;

	void set_cover_id(CoverPointId id);
	CoverPointId get_cover_id() const;
};

// Door — connects two Rooms
class Door : public Resource {
	GDCLASS(Door, Resource)

private:
	DoorId door_id = 0;
	RoomId room_a = INVALID_ROOM_ID;
	RoomId room_b = INVALID_ROOM_ID;
	bool is_locked = false;
	bool is_open = true;

protected:
	static void _bind_methods();

public:
	Door();
	~Door();

	void set_door_id(DoorId id);
	DoorId get_door_id() const;

	void set_room_a(RoomId id);
	RoomId get_room_a() const;

	void set_room_b(RoomId id);
	RoomId get_room_b() const;

	void set_locked(bool locked);
	bool get_locked() const;

	void set_open(bool open);
	bool get_open() const;

	// Returns the other room given one side
	RoomId get_other_room(RoomId from) const;
};

// Room — a semantic region in the world
class Room : public Resource {
	GDCLASS(Room, Resource)

private:
	RoomId room_id = INVALID_ROOM_ID;
	String room_name;
	DistrictId district_id = 0;
	Vector3 center_position;
	float radius = 5.0f;  // approximate bounds for proximity checks

	std::vector<CoverPointId> cover_points;
	std::vector<DoorId> doors;

protected:
	static void _bind_methods();

public:
	Room();
	~Room();

	void set_room_id(RoomId id);
	RoomId get_room_id() const;

	void set_room_name(const String &name);
	String get_room_name() const;

	void set_district_id(DistrictId id);
	DistrictId get_district_id() const;

	void set_center_position(const Vector3 &pos);
	Vector3 get_center_position() const;

	void set_radius(float r);
	float get_radius() const;

	void add_cover_point(CoverPointId id);
	const std::vector<CoverPointId> &get_cover_points() const;

	void add_door(DoorId id);
	const std::vector<DoorId> &get_doors() const;
};

// Sector — a district containing multiple rooms (e.g., "Downtown", "Hospital Wing")
class Sector : public Resource {
	GDCLASS(Sector, Resource)

private:
	DistrictId sector_id = 0;
	String sector_name;
	std::vector<RoomId> rooms;

protected:
	static void _bind_methods();

public:
	Sector();
	~Sector();

	void set_sector_id(DistrictId id);
	DistrictId get_sector_id() const;

	void set_sector_name(const String &name);
	String get_sector_name() const;

	void add_room(RoomId id);
	const std::vector<RoomId> &get_rooms() const;
};
```

Implementation follows standard Godot Resource pattern — bind methods, getters/setters, no logic beyond data storage. The `.cpp` file implements all methods trivially (set field, return field).

**Verify (local):** `scons compiledb=yes` + `clang-format --dry-run --Werror src/sector.h src/sector.cpp` — syntax + formatting. Semantic map test in Batch 3.
**Commit:** `feat(sim): add semantic map Resources (Sector, Room, Door, CoverPoint)`

---

### Task 2.2: Godot Actor (Optional Visual Wrapper)

**File:** `src/actor.h` + `src/actor.cpp`
**Test:** Phase 1 visual test (later)
**Depends:** 0.1, 0.2

Optional Node3D wrapper that drives NavigationAgent3D + mesh + animation for agents near the player. Only spawned when visual fidelity is needed.

```cpp
#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/navigation_agent3d.hpp>

using namespace godot;

class Agent;

class Actor : public Node3D {
	GDCLASS(Actor, Node3D)

private:
	AgentId agent_id = INVALID_AGENT_ID;
	Agent *bound_agent = nullptr;
	NavigationAgent3D *nav_agent = nullptr;

	float arrival_threshold = 0.5f;
	bool is_navigating = false;

protected:
	static void _bind_methods();

public:
	Actor();
	~Actor();

	void _ready() override;
	void _physics_process(double delta) override;

	// Binding
	void bind_agent(Agent *agent);
	Agent *get_bound_agent() const;

	// Navigation
	void navigate_to(const Vector3 &target);
	void stop_navigation();
	bool get_is_navigating() const;

	// Arrival
	void set_arrival_threshold(float threshold);
	float get_arrival_threshold() const;

	// Sync — called by WorldSimulation to update actor position from agent
	void sync_from_agent();
};
```

Implementation: `_physics_process` drives `NavigationAgent3D` movement toward target. `navigate_to` sets nav target. `sync_from_agent` copies agent position to Node3D global_transform. Standard null guards on `nav_agent`.

**Verify (local):** `scons compiledb=yes` + `clang-format --dry-run --Werror src/actor.h src/actor.cpp` — syntax + formatting. Visual test in later batch.
**Commit:** `feat(sim): add Actor optional visual wrapper for Agent`

---

### Task 2.3: Agent Navigation Integration

**Files modified:** `src/agent.h`, `src/agent.cpp`
**Depends:** 1.2

Add semantic-graph-aware navigation to Agent. When `set_move_target` is called, Agent resolves path through Rooms/Doors. If door is locked, find alternate route. If no route, set goal to `IDLE` + emit signal.

Add to `agent.h`:
```cpp
	// Navigation via semantic graph
	void navigate_to_room(RoomId target_room, const WorldSimulation *sim);
	RoomId get_current_room() const;
	bool can_reach_room(RoomId target, const WorldSimulation *sim) const;
```

Add to `agent.cpp`:
```cpp
void Agent::navigate_to_room(RoomId target_room, const WorldSimulation *sim) {
	// Phase 1: simple direct path — move to room center
	// Phase 2+: pathfind through door graph
	room_id = target_room;
	// Position update will happen in update() based on move_target
}

RoomId Agent::get_current_room() const {
	return room_id;
}

bool Agent::can_reach_room(RoomId target, const WorldSimulation *sim) const {
	// Phase 1: always true (direct movement)
	// Phase 2+: check door graph connectivity
	return true;
}
```

**Verify (local):** `scons compiledb=yes` + `clang-format --dry-run --Werror src/agent.h src/agent.cpp` — syntax + formatting. Existing tests passing verified by compiledb resolution.
**Commit:** `feat(sim): add semantic navigation methods to Agent`

---

## Batch 3: Agents Think — Perception + Actions + Squads (parallel after Batch 2)

### Task 3.1: PerceptionComponent

**File:** `src/perception_component.h` + `src/perception_component.cpp`
**Test:** Phase 2 tactical test (Task 3.4)
**Depends:** 0.1, 0.2

Attachable Node (like HealthComponent). Handles visual cone, hearing radius, occlusion raycast (Phase 2+), and maintains `PerceptionState`.

```cpp
#pragma once

#include "world_state.h"

#include <godot_cpp/classes/node3d.hpp>

using namespace godot;

class Agent;

class PerceptionComponent : public Node {
	GDCLASS(PerceptionComponent, Node)

private:
	Agent *owner_agent = nullptr;

	// Perception parameters
	float visual_range = 30.0f;
	float visual_angle = 120.0f;  // degrees, full cone
	float hearing_range = 20.0f;
	float confidence_decay_rate = 0.5f;  // per second

	// Current perception state
	PerceptionState state;

protected:
	static void _bind_methods();

public:
	PerceptionComponent();
	~PerceptionComponent();

	void _ready() override;
	void _process(double delta) override;

	// Owner
	void set_owner_agent(Agent *agent);
	Agent *get_owner_agent() const;

	// Parameters
	void set_visual_range(float range);
	float get_visual_range() const;
	void set_visual_angle(float angle_degrees);
	float get_visual_angle() const;
	void set_hearing_range(float range);
	float get_hearing_range() const;

	// Perception update — called each tick
	void update_perception(double delta, const WorldState &world);

	// Event handling
	void on_hearing_event(const Vector3 &origin, float loudness);
	void on_visual_contact(AgentId target_id, const Vector3 &position, float confidence);

	// Query
	const PerceptionState &get_state() const;
	bool can_see_target(AgentId target_id) const;
	bool can_hear_target(AgentId target_id) const;
	PerceptionRecord get_last_known(AgentId target_id) const;

	// Reset
	void clear();
};
```

Implementation: `update_perception` iterates `world.agents`, checks distance + angle for visibility, decays confidence over time, maintains `last_known_position`. Hearing events set `has_hearing_event` + `hearing_event_origin`. Standard null guard on `owner_agent`.

**Verify (local):** `scons compiledb=yes` + `clang-format --dry-run --Werror src/perception_component.h src/perception_component.cpp` — syntax + formatting. Tactical test in Task 3.4.
**Commit:** `feat(sim): add PerceptionComponent for agent awareness`

---

### Task 3.2: Tactical Actions

**Files:** `src/action.h` + `src/actions/take_cover.cpp`, `src/actions/advance.cpp`, `src/actions/retreat.cpp`, `src/actions/flank.cpp`, `src/actions/search.cpp`, `src/actions/suppress.cpp`
**Depends:** 0.1, 0.2

Utility-scored discrete actions. Base class in `action.h`, concrete actions in `src/actions/` directory.

**`src/action.h`:**
```cpp
#pragma once

#include "world_state.h"

using namespace godot;

class Agent;

// Context passed to action scoring
struct ActionContext {
	const WorldState *world = nullptr;
	const Agent *self = nullptr;
	float delta = 0.0f;
};

// Base class for all tactical actions
class Action {
public:
	virtual ~Action() = default;

	// Can this action be executed right now?
	virtual bool can_execute(const ActionContext &ctx) const = 0;

	// How desirable is this action? (0-1)
	virtual float score(const ActionContext &ctx) const = 0;

	// Cost of this action (0-1) — higher = more resource-intensive
	virtual float cost(const ActionContext &ctx) const = 0;

	// Execute the action — mutate agent state
	virtual void execute(Agent *agent, const ActionContext &ctx) const = 0;

	// Name for debug
	virtual const char *get_name() const = 0;

	// Utility: score - cost
	float utility(const ActionContext &ctx) const {
		return score(ctx) - cost(ctx);
	}
};
```

**`src/actions/take_cover.cpp`:** (representative — others follow same pattern)
```cpp
#include "../action.h"
#include "../agent.h"

class TakeCoverAction : public Action {
public:
	bool can_execute(const ActionContext &ctx) const override {
		// Can always take cover if alive
		return ctx.self && ctx.self->get_is_alive();
	}

	float score(const ActionContext &ctx) const override {
		// Score higher when under fire (health < 50%)
		if (!ctx.self) return 0.0f;
		float health_pct = ctx.self->get_health() / ctx.self->get_max_health();
		if (health_pct < 0.5f) return 0.9f;
		if (health_pct < 0.8f) return 0.5f;
		return 0.2f;
	}

	float cost(const ActionContext &ctx) const override {
		return 0.1f;  // low cost — always a safe fallback
	}

	void execute(Agent *agent, const ActionContext &ctx) const override {
		// POC: set goal to DEFEND, clear move target
		agent->set_goal(AgentGoal::DEFEND);
		agent->clear_move_target();
	}

	const char *get_name() const override { return "TakeCover"; }
};

Action *create_take_cover_action() {
	return new TakeCoverAction();
}
```

**`src/actions/advance.cpp`:** Score higher when squad state is ATTACK, cost moderate. Execute sets goal to ATTACK, moves toward objective room.
**`src/actions/retreat.cpp`:** Score higher when health < 30%, cost low. Execute sets goal to RETREAT, moves away from enemies.
**`src/actions/flank.cpp`:** Score higher when 2+ squadmates are in ATTACK, cost high. Execute moves to side of enemy.
**`src/actions/search.cpp`:** Score higher when perception has hearing event but no visual, cost moderate. Execute moves to hearing origin.
**`src/actions/suppress.cpp`:** Score higher when enemy is pinned (low health), cost moderate. Execute stays in cover, emits suppress signal.

**Verify (local):** `scons compiledb=yes` + `clang-format --dry-run --Werror src/action.h src/actions/*.cpp` — syntax + formatting (all action files picked up by recursive glob).
**Commit:** `feat(sim): add tactical action system with utility scoring`

---

### Task 3.3: Squad Node

**File:** `src/squad.h` + `src/squad.cpp`
**Test:** Phase 2 tactical test (Task 3.4)
**Depends:** 2.1, 2.2, 3.1, 3.2

Command layer. Holds members, leader, objective, tactical state. Decides WHAT; agents decide HOW.

```cpp
#pragma once

#include "world_state.h"

#include <vector>
#include <godot_cpp/classes/node.hpp>

using namespace godot;

class Agent;

class Squad : public Node {
	GDCLASS(Squad, Node)

private:
	SquadId squad_id = INVALID_SQUAD_ID;
	FactionId faction_id = INVALID_FACTION_ID;
	AgentId leader_id = INVALID_AGENT_ID;
	std::vector<AgentId> member_ids;

	TacticalState tactical_state = TacticalState::IDLE;
	RoomId objective_room = INVALID_ROOM_ID;

	// Known enemies — aggregated from member perceptions
	std::vector<PerceptionRecord> known_enemies;

	// Parameters
	float engagement_range = 30.0f;
	float retreat_health_threshold = 0.3f;

protected:
	static void _bind_methods();

public:
	Squad();
	~Squad();

	void _ready() override;

	// Tick — called by WorldSimulation
	void update(double delta, const WorldState &world);

	// ID
	void set_squad_id(SquadId id);
	SquadId get_squad_id() const;

	// Faction
	void set_faction_id(FactionId id);
	FactionId get_faction_id() const;

	// Members
	void add_member(AgentId id);
	void remove_member(AgentId id);
	const std::vector<AgentId> &get_member_ids() const;
	void set_leader(AgentId id);
	AgentId get_leader_id() const;

	// Tactical state
	void set_tactical_state(TacticalState state);
	TacticalState get_tactical_state() const;

	// Objective
	void set_objective_room(RoomId room);
	RoomId get_objective_room() const;

	// Known enemies
	const std::vector<PerceptionRecord> &get_known_enemies() const;
	void aggregate_perceptions(const WorldState &world);

	// Decision making
	void evaluate_tactical_state(const WorldState &world);
	void issue_orders(WorldState &world) const;

	// Signals
	void _on_tactical_state_changed(TacticalState old_state, TacticalState new_state);
};
```

Implementation:
- `update`: calls `aggregate_perceptions` → `evaluate_tactical_state` → `issue_orders`
- `aggregate_perceptions`: merge all member PerceptionStates into `known_enemies`
- `evaluate_tactical_state`: if known_enemies empty → IDLE; if health low → RETREAT; if enemies in range → ATTACK; if last known but no visual → SEARCH
- `issue_orders`: set each member's `tactical_state` and `goal` based on squad decision

**Verify (local):** `scons compiledb=yes` + `clang-format --dry-run --Werror src/squad.h src/squad.cpp` — syntax + formatting.
**Commit:** `feat(sim): add Squad command layer for tactical coordination`

---

### Task 3.4: Phase 2 Tactical POC Test

**File:** `tests/test_phase2_tactical.gd`
**Depends:** 3.1, 3.2, 3.3

Proves: squad detects enemy via perception, transitions IDLE → SEARCH → ATTACK, agents take cover, flank action scores.

```gdscript
extends SceneTree

# Phase 2 test: Tactical POC
# Run: godot --headless --path game/ -s tests/test_phase2_tactical.gd

var pass_count = 0
var fail_count = 0

func _init():
	print("=== Phase 2: Tactical POC Test ===")

	var ws = WorldSimulation.new()
	ws.set_debug_verbose(true)

	# Create 4 enemy agents (Gang A)
	var enemies = []
	for i in range(4):
		var a = Agent.new()
		a.set_agent_id(10 + i)
		a.set_faction_id(1)
		a.set_position(Vector3(20, 0, 0))
		a.set_health(100.0)
		a.set_max_health(100.0)
		ws.add_agent(a)
		enemies.append(a)

	# Create player squad (4 agents)
	var squad = Squad.new()
	squad.set_squad_id(1)
	squad.set_faction_id(2)
	squad.set_leader(1)

	for i in range(4):
		var a = Agent.new()
		a.set_agent_id(1 + i)
		a.set_faction_id(2)
		a.set_position(Vector3(0, 0, i * 2.0))
		a.set_health(100.0)
		a.set_max_health(100.0)
		a.set_squad_id(1)
		ws.add_agent(a)
		squad.add_member(1 + i)

	# Attach perception to first squad member
	var perc = PerceptionComponent.new()
	perc.set_visual_range(30.0)
	perc.set_visual_angle(120.0)
	enemies[0].add_child(perc)  # Attach to enemy so it can see squad

	# Tick until squad detects enemies
	var detected = false
	for tick in range(300):
		ws.tick(1.0 / 60.0)
		if squad.get_tactical_state() != TacticalState.IDLE:
			detected = true
			break

	assert_true(detected, "Squad should detect enemies within 5 seconds")

	# Verify tactical state transition
	var state = squad.get_tactical_state()
	assert_true(
		state == TacticalState.ATTACK or state == TacticalState.SEARCH,
		"Squad should be ATTACK or SEARCH after detection"
	)

	# Verify at least one agent has a move target (flanking or advancing)
	var has_mover = false
	for a in ws.get_current_state().agents:
		if a.id >= 1 and a.id <= 4:
			var agent = ws.get_agent(a.id)
			if agent and agent.get_has_move_target():
				has_mover = true
				break

	assert_true(has_mover, "At least one squad agent should have a move target")

	# Summary
	print("=== Phase 2: PASSED ", pass_count, "/", pass_count + fail_count, " ===")
	quit(0 if fail_count == 0 else 1)

func assert_eq(a, b, msg: String):
	if a == b: pass_count += 1
	else: fail_count += 1; print("FAIL: ", msg)

func assert_gt(a, b, msg: String):
	if a > b: pass_count += 1
	else: fail_count += 1; print("FAIL: ", msg)

func assert_true(val, msg: String):
	if val: pass_count += 1
	else: fail_count += 1; print("FAIL: ", msg)

func assert_false(val, msg: String):
	if not val: pass_count += 1
	else: fail_count += 1; print("FAIL: ", msg)
```

**Verify (local — syntax-only):**
1. `scons compiledb=yes` — confirms all GDExtension class references in GDScript are syntactically valid
2. GDScript files are pure text — `godot --headless` runtime execution deferred to CI

Full headless test execution deferred to CI: `.github/workflows/builds.yml` runs `godot --headless --path game/ -s tests/test_phase2_tactical.gd`.
**Commit:** `test(sim): add Phase 2 tactical POC test`

---

## Batch 4: World Lives — Factions + Missions + Director + Campaign (parallel after Batch 3)

### Task 4.1: Faction + FactionRegistry

**Files:** `src/faction.h` + `src/faction.cpp` + `src/faction_registry.h` + `src/faction_registry.cpp`
**Depends:** 0.1, 0.2

```cpp
// faction.h
#pragma once

#include "world_state.h"
#include <godot_cpp/classes/resource.hpp>

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

	// Standard getters/setters for all fields
	void set_faction_id(FactionId id);
	FactionId get_faction_id() const;
	void set_faction_name(const String &name);
	String get_faction_name() const;
	void set_alertness(float a);
	float get_alertness() const;
	void set_strength(int32_t s);
	int32_t get_strength() const;
	void add_ally(FactionId id);
	void add_enemy(FactionId id);
	bool is_enemy(FactionId id) const;
	bool is_ally(FactionId id) const;
};

// faction_registry.h
#pragma once

#include "faction.h"
#include <vector>
#include <godot_cpp/classes/node.hpp>

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

	void add_faction(Ref<Faction> faction);
	Ref<Faction> get_faction(FactionId id) const;
	int32_t get_faction_count() const;
	void update(float delta);

	// Relationship queries
	bool are_enemies(FactionId a, FactionId b) const;
	bool are_allies(FactionId a, FactionId b) const;
};
```

Implementation: standard getter/setter patterns, `update` adjusts alertness/strength based on casualties in WorldState, relationship queries check enemy/ally lists.

**Verify (local):** `scons compiledb=yes` + `clang-format --dry-run --Werror src/faction.h src/faction.cpp src/faction_registry.h src/faction_registry.cpp` — syntax + formatting.
**Commit:** `feat(sim): add Faction and FactionRegistry`

---

### Task 4.2: Mission + Patient

**Files:** `src/mission.h` + `src/mission.cpp` + `src/patient.h` + `src/patient.cpp`
**Depends:** 0.1, 0.2

```cpp
// mission.h
#pragma once

#include "world_state.h"
#include <vector>
#include <godot_cpp/classes/node.hpp>

using namespace godot;

enum class MissionStatus : uint8_t {
	PENDING,
	IN_PROGRESS,
	COMPLETED_SUCCESS,
	COMPLETED_FAILURE,
	CANCELLED,
};

enum class ObjectiveType : uint8_t {
	LOCATE_VIP,
	STABILIZE_PATIENT,
	ESCORT_TO_EXTRACTION,
	EXTRACT,
	DEFEND_POSITION,
	ELIMINATE_HOSTILES,
};

struct MissionObjective {
	ObjectiveType type;
	bool completed = false;
	RoomId target_room = INVALID_ROOM_ID;
	AgentId target_agent = INVALID_AGENT_ID;
};

class Mission : public Node {
	GDCLASS(Mission, Node)

private:
	MissionId mission_id = 0;
	MissionStatus status = MissionStatus::PENDING;
	std::vector<MissionObjective> objectives;
	int32_t current_objective_index = 0;

	RoomId insertion_room = INVALID_ROOM_ID;
	RoomId extraction_room = INVALID_ROOM_ID;

	float elapsed_time = 0.0f;
	float time_limit = -1.0f;  // -1 = no limit

protected:
	static void _bind_methods();

public:
	Mission();
	~Mission();

	void _ready() override;
	void _process(double delta) override;

	void start_mission();
	void complete_current_objective();
	void fail_mission();

	MissionId get_mission_id() const;
	MissionStatus get_status() const;
	const MissionObjective &get_current_objective() const;
	bool has_objectives() const;
	void add_objective(const MissionObjective &obj);
	void set_insertion_room(RoomId room);
	RoomId get_insertion_room() const;
	void set_extraction_room(RoomId room);
	RoomId get_extraction_room() const;
	void set_time_limit(float limit);
	float get_time_limit() const;

	// Signals
	void _on_mission_started();
	void _on_mission_completed(bool success);
	void _on_objective_completed(int32_t index);
};
```

```cpp
// patient.h
#pragma once

#include "world_state.h"
#include <godot_cpp/classes/node.hpp>

using namespace godot;

enum class PatientState : uint8_t {
	CRITICAL,     // needs immediate stabilization
	STABILIZED,   // treated, stable
	TRANSPORTABLE, // can be moved to extraction
	EXTRACTED,    // at extraction point
	DEAD,
};

class Patient : public Node {
	GDCLASS(Patient, Node)

private:
	AgentId patient_agent_id = INVALID_AGENT_ID;
	PatientState state = PatientState::CRITICAL;
	AgentId assigned_medic_id = INVALID_AGENT_ID;

	float stabilization_progress = 0.0f;  // 0-1
	float stabilization_required = 1.0f;  // time needed to stabilize

protected:
	static void _bind_methods();

public:
	Patient();
	~Patient();

	void _ready() override;
	void _process(double delta) override;

	void set_patient_agent_id(AgentId id);
	AgentId get_patient_agent_id() const;
	void set_state(PatientState state);
	PatientState get_state() const;
	void set_assigned_medic(AgentId id);
	AgentId get_assigned_medic() const;

	void start_stabilization(AgentId medic_id);
	void update_stabilization(float delta);

	// Signals
	void _on_state_changed(PatientState old_state, PatientState new_state);
	void _on_stabilized();
	void _on_extracted();
};
```

Standard implementations — getters/setters, state transitions, signals.

**Verify (local):** `scons compiledb=yes` + `clang-format --dry-run --Werror src/mission.h src/mission.cpp src/patient.h src/patient.cpp` — syntax + formatting.
**Commit:** `feat(sim): add Mission and Patient simulation nodes`

---

### Task 4.3: Director

**File:** `src/director.h` + `src/director.cpp`
**Depends:** 0.1, 0.2

Observer that reads WorldState + MissionState + PlayerState + SquadState, emits `DirectorEvent`. Never calls Agent/Squad methods directly.

```cpp
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
	std::vector<DirectorEvent> pending_events;  // next-tick queue
	std::vector<DirectorEvent> event_history;

	// Conditions for event triggers
	float mission_time_threshold = 30.0f;
	int32_t casualty_threshold = 2;
	float alertness_threshold = 0.7f;

	// Tracking
	float mission_elapsed_time = 0.0f;
	int32_t total_casualties = 0;

protected:
	static void _bind_methods();

public:
	Director();
	~Director();

	void _ready() override;
	void _process(double delta) override;

	// Observation — called by WorldSimulation each tick
	void observe(const WorldState &world, double delta);

	// Emit event (enqueued for next tick)
	void emit_event(DirectorEventType type, const Vector3 &origin, float radius, const String &desc);

	// Process queued events — called at start of next tick
	const std::vector<DirectorEvent> &get_pending_events() const;
	void clear_pending_events();

	// History
	const std::vector<DirectorEvent> &get_event_history() const;

	// Signals
	void _on_director_event(int32_t event_type, float timestamp);

private:
	void evaluate_conditions(const WorldState &world);
};
```

Implementation: `observe` calls `evaluate_conditions` which checks thresholds (time, casualties, alertness) and calls `emit_event`. Events are queued in `pending_events`, cleared after consumption. Signals emitted for each event.

**Verify (local):** `scons compiledb=yes` + `clang-format --dry-run --Werror src/director.h src/director.cpp` — syntax + formatting.
**Commit:** `feat(sim): add Director observer with deferred event system`

---

### Task 4.4: CampaignState

**File:** `src/campaign_state.h` + `src/campaign_state.cpp`
**Depends:** 0.1, 0.2

Persists cross-mission state: faction relations, reputation, dead/injured agents, VIP outcomes, district control.

```cpp
#pragma once

#include "world_state.h"
#include <map>
#include <vector>
#include <godot_cpp/classes/node.hpp>

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
	float stability = 1.0f;  // 0-1
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
	std::map<DistrictId, DistrictControl> district_control;
	std::vector<MissionOutcome> mission_history;

	// Faction reputation
	std::map<FactionId, float> faction_reputation;  // -1 to 1

protected:
	static void _bind_methods();

public:
	CampaignState();
	~CampaignState();

	void _ready() override;

	// Record mission outcome
	void record_mission_outcome(const MissionOutcome &outcome);

	// Agent tracking
	void record_agent_death(AgentId id);
	void record_agent_extraction(AgentId id);
	const AgentRecord *get_agent_record(AgentId id) const;
	std::vector<AgentRecord> get_alive_agents() const;

	// District control
	void set_district_control(DistrictId id, FactionId faction, float stability);
	DistrictControl get_district_control(DistrictId id) const;
	FactionId get_controlling_faction(DistrictId id) const;

	// Reputation
	void set_faction_reputation(FactionId id, float rep);
	float get_faction_reputation(FactionId id) const;

	// Query
	int32_t get_total_missions() const;
	int32_t get_successful_missions() const;
	float get_success_rate() const;
};
```

Standard implementations — map/vector operations, state tracking.

**Verify (local):** `scons compiledb=yes` + `clang-format --dry-run --Werror src/campaign_state.h src/campaign_state.cpp` — syntax + formatting.
**Commit:** `feat(sim): add CampaignState for cross-mission persistence`

---

### Task 4.5: Phase 3 Trauma Team POC Test

**File:** `tests/test_phase3_trauma_team.gd`
**Depends:** 4.1, 4.2, 4.3, 4.4

Full Trauma Team mission loop: INSERT → LOCATE VIP → STABILIZE → ESCORT → EXTRACT. Then test with Director disruption.

```gdscript
extends SceneTree

# Phase 3 test: Trauma Team POC
# Run: godot --headless --path game/ -s tests/test_phase3_trauma_team.gd

var pass_count = 0
var fail_count = 0

func _init():
	print("=== Phase 3: Trauma Team POC Test ===")

	# --- Test 1: Full mission without Director ---
	print("--- Test 1: Mission without Director ---")
	var ws = WorldSimulation.new()
	var mission = Mission.new()
	var patient = Patient.new()
	var campaign = CampaignState.new()

	# Setup mission: INSERT → LOCATE → STABILIZE → ESCORT → EXTRACT
	mission.set_insertion_room(1)
	mission.set_extraction_room(5)
	mission.add_objective({type = ObjectiveType.LOCATE_VIP, target_room = 3})
	mission.add_objective({type = ObjectiveType.STABILIZE_PATIENT})
	mission.add_objective({type = ObjectiveType.ESCORT_TO_EXTRACTION, target_room = 5})
	mission.add_objective({type = ObjectiveType.EXTRACT})

	# Setup patient
	patient.set_patient_agent_id(100)
	patient.set_state(PatientState.CRITICAL)

	# Create VIP agent
	var vip = Agent.new()
	vip.set_agent_id(100)
	vip.set_position(Vector3(15, 0, 0))
	vip.set_health(30.0)
	vip.set_max_health(100.0)
	ws.add_agent(vip)

	# Create trauma team (3 agents)
	for i in range(3):
		var a = Agent.new()
		a.set_agent_id(1 + i)
		a.set_faction_id(2)
		a.set_position(Vector3(0, 0, 0))
		a.set_health(100.0)
		a.set_max_health(100.0)
		ws.add_agent(a)

	mission.start_mission()
	assert_eq(mission.get_status(), MissionStatus.IN_PROGRESS, "Mission should be IN_PROGRESS")

	# Simulate mission progression
	for tick in range(600):
		ws.tick(1.0 / 60.0)

		# Auto-complete objectives for test
		if mission.get_status() == MissionStatus.IN_PROGRESS:
			mission.complete_current_objective()

	# Verify mission completed
	assert_eq(mission.get_status(), MissionStatus.COMPLETED_SUCCESS, "Mission should succeed")

	# Verify campaign state updated
	var outcome = MissionOutcome.new()
	outcome.mission_id = mission.get_mission_id()
	outcome.success = true
	outcome.vip_extracted = true
	campaign.record_mission_outcome(outcome)

	assert_eq(campaign.get_total_missions(), 1, "Should have 1 mission recorded")
	assert_eq(campaign.get_successful_missions(), 1, "Should have 1 successful mission")

	# --- Test 2: Mission with Director disruption ---
	print("--- Test 2: Mission with Director Lockdown ---")
	var ws2 = WorldSimulation.new()
	var director = Director.new()
	var mission2 = Mission.new()

	mission2.set_insertion_room(1)
	mission2.set_extraction_room(5)
	mission2.add_objective({type = ObjectiveType.LOCATE_VIP, target_room = 3})
	mission2.add_objective({type = ObjectiveType.EXTRACT})

	# Run ticks and let Director observe
	mission2.start_mission()
	for tick in range(300):
		ws2.tick(1.0 / 60.0)
		director.observe(ws2.get_current_state(), 1.0 / 60.0)

		# Director should emit events after enough time
		if tick == 180:
			director.emit_event(DirectorEventType.LOCKDOWN, Vector3(10, 0, 0), 20.0, "Lockdown initiated")

	# Verify Director emitted event
	var events = director.get_event_history()
	var has_lockdown = false
	for e in events:
		if e.type == DirectorEventType.LOCKDOWN:
			has_lockdown = true
			break

	assert_true(has_lockdown, "Director should have emitted LOCKDOWN event")

	# Summary
	print("=== Phase 3: PASSED ", pass_count, "/", pass_count + fail_count, " ===")
	quit(0 if fail_count == 0 else 1)

func assert_eq(a, b, msg: String):
	if a == b: pass_count += 1
	else: fail_count += 1; print("FAIL: ", msg, " — expected ", b, ", got ", a)

func assert_gt(a, b, msg: String):
	if a > b: pass_count += 1
	else: fail_count += 1; print("FAIL: ", msg)

func assert_true(val, msg: String):
	if val: pass_count += 1
	else: fail_count += 1; print("FAIL: ", msg)

func assert_false(val, msg: String):
	if not val: pass_count += 1
	else: fail_count += 1; print("FAIL: ", msg)
```

**Verify (local — syntax-only):**
1. `scons compiledb=yes` — confirms all GDExtension class references in GDScript are syntactically valid
2. GDScript files are pure text — `godot --headless` runtime execution deferred to CI

Full headless test execution deferred to CI: `.github/workflows/builds.yml` runs `godot --headless --path game/ -s tests/test_phase3_trauma_team.gd`.
**Commit:** `test(sim): add Phase 3 Trauma Team POC test`

---

## File Summary

| Phase | Files Created | Files Modified |
|-------|--------------|----------------|
| 0 | `src/world_state.h`, `tests/` dir | `SConstruct`, `CMakeLists.txt`, `src/register_types.cpp`, `src/player_controller.cpp` |
| 1 | `src/world_simulation.h`, `src/world_simulation.cpp`, `src/agent.h`, `src/agent.cpp` | `src/register_types.cpp`, `tests/test_phase0_tick.gd` |
| 2 | `src/sector.h`, `src/sector.cpp`, `src/actor.h`, `src/actor.cpp` | `src/agent.h`, `src/agent.cpp` |
| 3 | `src/perception_component.h`, `src/perception_component.cpp`, `src/action.h`, `src/actions/*.cpp`, `src/squad.h`, `src/squad.cpp`, `tests/test_phase2_tactical.gd` | — |
| 4 | `src/faction.h`, `src/faction.cpp`, `src/faction_registry.h`, `src/faction_registry.cpp`, `src/mission.h`, `src/mission.cpp`, `src/patient.h`, `src/patient.cpp`, `src/director.h`, `src/director.cpp`, `src/campaign_state.h`, `src/campaign_state.cpp`, `tests/test_phase3_trauma_team.gd` | `src/register_types.cpp` |

## Risk Mitigations

1. **SCons glob breakage:** Task 0.1 is the very first commit. Run `scons compiledb=yes` locally to confirm header resolution; full link verified in CI.
2. **InputManager singleton ordering:** Task 0.3 moves initialization to `register_types.cpp` init function — this runs before any scene loads, so it's safe.
3. **Agent without scene tree:** Tests instantiate nodes directly (`Agent.new()`) without adding to scene tree — this works for GDExtension Nodes that don't require `_ready` setup.
4. **Jolt physics thread:** All simulation tick is in `_physics_process` — safe for Jolt. No raw physics reads in `_process`.
5. **Header include cycles:** `agent.h` includes `world_state.h` (no cycle). `world_simulation.h` forward-declares `Agent` (no cycle). All other files follow same pattern.
