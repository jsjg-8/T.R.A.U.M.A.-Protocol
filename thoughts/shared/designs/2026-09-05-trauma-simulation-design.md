---
date: 2026-09-05
topic: "T.R.A.U.M.A. Simulation Core — World, Tactical AI, Director, and Trauma Team Loop"
status: validated
---

## Problem Statement

We need a **POC that proves the game is fun before it looks good**.

The core risk isn't rendering or guns — it's whether a **living world** (STALKER-like faction simulation) + **tactical execution** (F.E.A.R.-like room clearing) + **Trauma Team pressure** (locate → stabilize → extract under disruption) actually composes.

Current codebase is 4 GDExtension classes (`PlayerController`, `HealthComponent`, `InputManager`, `TrafficLight`) with no simulation layer. We must add a C++ simulation core that ticks independently of presentation, is headlessly testable, and scales from a handful of high-fidelity actors near the player to dozens of low-fidelity agents in the background.

Success for POC is:
- Spawn squads and watch them patrol/fight **without a player** (A-Life proof)
- Fight a squad that uses cover, flanking, and search **around the player** (tactical proof)
- Run `INSERT → LOCATE VIP → STABILIZE → ESCORT → EXTRACT` while a Director disrupts conditions (Trauma Team proof)

## Constraints

**Hard constraints:**
- **Godot 4.5 + godot-cpp 4.3 + SCons primary** — `SConstruct` uses `Glob("src/*.cpp")` (non-recursive). Must fix before introducing subdirectories.
- **Jolt on separate physics thread** (`project.godot:45` `run_on_separate_thread=true`) — world reads that touch physics must happen in `_physics_process`, never `_process`.
- **C++17, LLVM style, tabs** — `.clang-format` and `EditorConfig` are non-negotiable.
- **`InputManager` singleton is broken** — `PlayerController::_ready` null-derefs if no instance exists. Fix in Phase 0 before any spawn logic is trusted.
- **`HealthComponent` invariants** — `max_health > 0`, `current_health` clamped `[0,max]`, dead cannot `heal`. Agent health must respect these or pooling breaks.

**Design constraints:**
- **Godot owns presentation, C++ owns simulation** — strict boundary. Godot: rendering, animation, physics, navigation, input, audio, editor. C++: world tick, perception, decisions, factions, missions, Director, campaign.
- **Director never puppeteers NPCs** — it emits `DirectorEvent` that changes conditions; squads/agents react via normal perception/utility.
- **Agents act on perception, not ground truth** — no wall-hacks.

## Approach

**Chosen approach: Incremental simulation fidelity, lightest Agent first.**

We adopt the user's refined progression and reject building "world simulation" as a monolith.

```text
PHASE 0 — Foundation
  Godot↔godot-cpp → WorldSimulation → headless tick

PHASE 1 — World Exists
  Semantic map → Navigation → Agents moving

PHASE 2 — Agents Think
  Perception → Utility actions → Squads → TACTICAL POC

PHASE 3 — World Lives
  Factions → Mission → Patient → Director → Campaign → TRAUMA TEAM POC

PHASE 4 — Presentation
  Weapons, anim, VFX, audio, AV, cyberware
```

**File creation order is progressive, not big-bang:**

Immediate (Phase 0):
- `world_state.h` (pure data, no Node)
- `world_simulation.h/.cpp` (Node, tick root)
- `agent.h/.cpp` (lightweight simulation agent)

Next (Phase 1):
- `sector.h/.cpp`, `room.h`, `door.h`, `cover_point.h` (semantic graph + Resources)
- Navigation wrapper around `NavigationAgent3D`

Next (Phase 2):
- `perception_component.h/.cpp`
- `action.h`, concrete actions (`take_cover`, `flank`, etc.)
- `squad.h/.cpp`

Later (Phase 3):
- `faction.h/.cpp`, `faction_registry.h/.cpp`
- `mission.h/.cpp`, `patient.h/.cpp`
- `director.h/.cpp`, `campaign_state.h/.cpp`

**Why this ordering:** Each increment is **independently runnable headlessly**. We prove `tick() → Agent.update() → state changes` before adding spatial reasoning, prove spatial reasoning before perception, and prove perception before squad coordination. No mocks for the world — tick it.

**Alternatives considered:**
- **Build all headers upfront then implement** — rejected. Creates illusion of progress, hides coupling, and SCons glob issue would block compilation until fixed anyway.
- **Agent as `CharacterBody3D` immediately** — rejected for POC. Ties simulation to physics, makes headless tests require a scene tree and Jolt, prevents background low-fidelity agents. We choose the lighter split (below).
- **Behavior Trees / GOAP for actions** — rejected for POC. Utility scoring (`Can I? / How desirable? / Cost?`) is faster to author, trivially debuggable, and sufficient for ~10 actions. BT/GOAP can layer later.

## Architecture

**Two-level Agent separation (key refinement):**

```text
Simulation Agent  (C++ pure, ticks every frame)
      │
      │  "Move to Room B" / "TakeCover at Point 7"
      ▼
Godot Actor  (optional Node3D + NavigationAgent3D)
      │
      ▼
 actual movement / animation / collision
```

- **Simulation Agent** holds `AgentState` (faction, district, room, health, goal, perception), scores utility, and emits intents. It **does not** extend `CharacterBody3D` in POC. It can be a `Node` or `Object` owned by `WorldSimulation`.
- **Godot Actor** is an optional visual/physical representation spawned only for agents near the player (or for all agents once fidelity is proven). It owns `NavigationAgent3D` and drives `move_and_slide` when present.
- This gives us **headless tests that never instantiate a scene tree** and a path to **hundreds of background agents** at low cost.

**Module layout:**

```text
src/
├── world_state.h              # pure data structs (AgentState, RoomId, FactionId, etc.)
├── world_simulation.h/.cpp    # Node, owns tick, FactionRegistry, SectorMap, agent list
├── sector.h/.cpp              # Room graph, Door, CoverPoint Resources
├── agent.h/.cpp               # Simulation Agent (lightweight)
├── actor.h/.cpp               # Optional Godot Actor (Node3D wrapper) — Phase 1+
├── perception_component.h/.cpp# attachable perception (like HealthComponent)
├── action.h + actions/        # utility actions
├── squad.h/.cpp               # command layer
├── faction.h/.cpp
├── faction_registry.h/.cpp
├── mission.h/.cpp
├── patient.h/.cpp
├── director.h/.cpp
└── campaign_state.h/.cpp
```

**Build fix comes first:** Change `SConstruct` `Glob("src/*.cpp")` → `Glob("src/**/*.cpp")` (or explicit `Glob("src/*.cpp") + Glob("src/**/*.cpp")`) **before** creating any subdirectory. CMakeLists.txt must also list new files if CMake is used. No build-file edit per new class after that.

**Godot↔C++ boundary:**

- All simulation Nodes register at `MODULE_INITIALIZATION_LEVEL_SCENE` via `GDREGISTER_CLASS` in `register_types.cpp`.
- Scene wiring uses `NodePath` exports resolved in `_ready` with `has_node` + `printerr` guards (pattern from `PlayerController`).
- Signals for cross-system events (`perception_updated`, `tactical_state_changed`, `death`, `director_event`) — C++ emits, GDScript/HUD connects in editor.

## Components

**WorldState (pure data, no Node)**
- **Responsibility:** Snapshot of everything Director/debug/tests read. `struct AgentState { FactionId faction; DistrictId district; RoomId room; Vec3 position; HealthState health; AgentGoal goal; }` plus squad/faction/mission state.
- **Invariant:** No logic, no tick. Double-buffered if needed for thread safety — tick writes next, readers see current.

**WorldSimulation (Node, tick root)**
- **Responsibility:** Owns tick order, owns `FactionRegistry`, `SectorMap`, agent/squad lists, and the `WorldState` snapshot.
- **Tick:** Called from `_physics_process(delta)` only. Advances `tick(delta)` → perception → squad → agent → faction → world mutation → director observation. Enqueues `DirectorEvent` for next tick.
- **Lifecycle:** Fixes `InputManager` instantiation (`memnew` in `initialize_gdextension_types` or null guard in `PlayerController::_ready`) as first commit.

**Sector / Room / Door / CoverPoint (semantic map)**
- **Responsibility:** Provide **semantic** world knowledge so AI doesn't reason on raw geometry. `Room A —Door D— Room B`, `Room has 4 CoverPoints`, `Room contains Objective`, `Door is locked`.
- **Form:** `CoverPoint` is a `Resource` (position + facing + exposure). `Sector`/`Room`/`Door` are lightweight structs or Resources hand-authored for one test level in POC. Navigation mesh is Godot `NavigationMesh` — semantic graph annotates it.

**Navigation (wrapper)**
- **Responsibility:** Thin wrapper over `NavigationAgent3D`. Answers `find_path(A→B)` as polyline, handles dynamic door toggles as navmesh link enable/disable.
- **Milestone:** `street → lobby → corridor → apartment → rooftop` without stuck.

**PerceptionComponent (attachable Node, like HealthComponent)**
- **Responsibility:** Per-agent knowledge: visual cone, hearing radius, occlusion raycast, `last_known_position`, `confidence`, `observed_faction`.
- **Key rule:** Agents query `PerceptionState`, never `WorldState` directly. `heard gunshot → investigate → partial sighting → last_known → search` is the F.E.A.R. loop.

**Tactical Actions (utility-scored)**
- **Responsibility:** Discrete actions `TakeCover`, `Advance`, `Retreat`, `Suppress`, `Flank`, `Search`, `Revive`, `Protect`, `Capture`, `Attack`. Each implements `can_execute()`, `score(context)`, `cost()`, `execute()`.
- **Selection:** Agent picks highest `score - cost` each tick. Fallback to `TakeCover`/`Hold` if nothing scores.

**Squad (Node, command layer)**
- **Responsibility:** Holds `Leader`, `Members`, `Objective`, `Formation`, `KnownEnemies`, `TacticalState` (`ATTACK/DEFEND/SEARCH/RETREAT/REGROUP/PROTECT_VIP`).
- **Separation:** Squad decides **what** ("push Room B"), agent decides **how** ("which cover, which route"). Prevents every agent re-solving squad strategy.

**Faction + FactionRegistry**
- **Responsibility:** `Faction` has `territory`, `relationships`, `resources`, `objectives`, `alertness`, `strength`. Registry owns relationship matrix and territory map. Enables `Gang vs Corp → fight → territory change` without player.

**Mission + Patient + CampaignState**
- **Responsibility:** `Mission` defines `objectives → participants → insertion/extraction → success/failure`. `Patient` is minimal for POC: `critical → stabilized → transportable → extracted`. `CampaignState` persists `faction relations, reputation, dead/injured, VIP outcomes, district states` across missions.

**Director (Node, observer)**
- **Responsibility:** Reads `WorldState + MissionState + PlayerState + SquadState`, emits `DirectorEvent` (`PowerFailure`, `Reinforcement`, `Ambush`, `Lockdown`, `AVDelay`, `Fire`).
- **Boundary:** Never calls `Agent`/`Squad` methods. Changes conditions (spawn, lock door, cut power) — simulation reacts via normal perception/utility.

## Data Flow

**Single tick, every physics frame, Director deferred:**

```text
WorldSimulation::_physics_process(delta)
  │
  ├─ 1. Perception update    (each Agent: see/hear → PerceptionState)
  ├─ 2. Squad update         (each Squad: given perceptions → TacticalState)
  ├─ 3. Agent update         (each Agent: given squad order + perception → highest utility Action → intent)
  ├─ 4. Faction update       (casualties, territory, alertness)
  ├─ 5. World mutation       (doors, objectives, VIP state; Actor intents → NavigationAgent3D if actor present)
  └─ 6. Director observation (snapshot WorldState → maybe emit DirectorEvent → enqueue for NEXT tick)
```

**Why this order:** Perception before decisions prevents stale knowledge. Squad before agent prevents agents contradicting squad. Director last with next-tick application prevents re-entrancy and god-object coupling. Actor movement resolves after decisions so navigation queries see current world.

**Headless vs. in-scene:**
- **Headless:** `WorldSimulation` ticks with no Actor nodes — pure `AgentState` position updates, no physics, no navmesh. Tests run without `main.tscn`.
- **In-scene:** Same tick, but agents with actors also drive `NavigationAgent3D` → `move_and_slide` via the actor. Visual fidelity is additive, not required.

## Error Handling

- **Null guards at scene boundary:** Every `get_node<T>(path)` checks `has_node` first, logs `printerr`, and no-ops. `spring_arm`/`head_pivot`/`animation_tree` patterns from `PlayerController` are the template.
- **Physics thread discipline:** All `global_transform`, `velocity`, navmesh, and raycast reads/writes happen in `_physics_process` or via `call_deferred`. Director handlers enqueue — never mutate live state mid-tick.
- **Health invariants:** `max_health > 0` guard, `current_health` clamped, dead cannot `heal` (use `set_current_health` to revive). Prevents ghost agents in pools.
- **Navigation fallback:** If `find_path` fails, retry via semantic graph (alternate door), then fallback to `Hold` + squad `REGROUP`. Never stall tick.
- **Action fallback:** If no action scores above threshold, default to `TakeCover`/`Hold`. Tick always produces an intent.
- **InputManager fix:** `memnew(InputManager)` in `initialize_gdextension_types` or guard `if (auto *im = InputManager::get_singleton()) im->initialize_input_map();` — no null-deref.
- **Build safety:** `godot-cpp` missing is fatal. New headers use `TRAUMA_*_H` guards. `doc_classes/*.xml` optional — omit until simulation API stabilizes.
- **Logging:** `UtilityFunctions::print` for info, `printerr` for misconfig, per-frame logs behind `debug_verbose` flag.

## Testing Strategy

**Principle: No mocks for the world — tick it.**

- **Phase 0 — Headless tick:** Instantiate `WorldSimulation` + 1 `Agent` with no scene. Call `tick(1.0/60.0)` 1000 times, assert `AgentState` position/state changed, no crash. Snapshot `WorldState` each tick, diff it. Proves `WorldSimulation::_physics_process → tick → Agent.update` works without Godot scene.
- **Phase 1 — Semantic + nav:** Hand-author one level (2 rooms, 1 door, 4 covers). Spawn 4 squads (Gang A/B, Corp, VIP) with no player, tick headlessly, assert: agents moved between rooms/districts, no agent stuck (position delta > epsilon), door lock blocks path and unlock restores it.
- **Phase 2 — Tactical POC:** Player + one squad in two-room map. Emit hearing event (gunshot), assert squad transitions `IDLE → SEARCH → ATTACK`, agents occupy `CoverPoint`s, at least one `Flank` action scored. Inspects `PerceptionState`, not ground truth. Verifies `TakeCover` fallback when no flank available.
- **Phase 3 — Trauma Team POC:** Run `INSERT → LOCATE → STABILIZE → EXTRACT` with Director disabled → assert success path. Enable Director with one `Lockdown` mid-mission → assert alternate route via semantic graph. Assert `CampaignState` mutates (`VIP outcome`, `faction relation`) after mission.
- **Signal-verifiable tests:** Follow `HealthComponent` style — connect to `health_changed`/`death`/`tactical_state_changed`/`director_event` and assert emissions, not private state. C++ unit tests via `doctest` or Godot headless ` --headless --script` runner — planner picks, but must stay SCons-friendly.

## Open Questions

- **SCons fix style:** `Glob("src/**/*.cpp")` vs. explicit `Glob("src/*.cpp") + Glob("src/**/*.cpp")` — former is cleaner, latter is more explicit about root files. Either works; pick one and stick.
- **Cover authoring for POC:** Hand-placed `CoverPoint` Resources (recommended) vs. auto-generated from navmesh raycasts — hand-placed for one test level, auto-gen later.
- **Director authoring:** Code-driven conditions for POC (`if (mission_time > X && casualties > Y) emit Ambush`) vs. data-driven event table — code-driven now, table when event count > ~10.
- **Actor spawn distance:** When does a `Simulation Agent` get a `Godot Actor`? Distance-to-player threshold vs. always for POC? Recommend always for Phase 1-2, distance-cull in Phase 3 when background agents scale.
