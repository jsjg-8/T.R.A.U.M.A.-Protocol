# T.R.A.U.M.A. Protocol — AGENTS.md

## Project

Godot 4.7 GDExtension (C++) third-person tactical shooter. Combines CP2077 Trauma Team + Ready or Not.

## Structure

| Path | Purpose |
|---|---|
| `src/` | C++ GDExtension source — the game engine |
| `game/` | Godot project directory (editor assets, scenes, `.gd` scripts) |
| `godot-cpp/` | Submodule (tag `10.0.0-rc2`, branch `master`), GDExtension C++ bindings |
| `doc_classes/` | XML documentation for Godot's built-in doc system |
| `bin/` | Compiled `.so`/`.dylib`/`.dll` output |
| `build/` | CMake build artifacts (Ninja) |

## Registered C++ classes

Registered in `src/register_types.cpp` at `MODULE_INITIALIZATION_LEVEL_SCENE`:

- **PlayerController** (`CharacterBody3D`) — third-person movement, camera, animation blends
- **InputManager** (`Object`, singleton) — overrides InputMap at runtime (WASD + Space, clears existing bindings)
- **HealthComponent** (`Node`) — health, damage, invulnerability timer, signals
- **TrafficLight** (`Control`) — UI texture switcher

Entry symbol: `trauma_engine_init` (from `trauma_engine_init` in `src/register_types.cpp`)
Extension config: `game/bin/trauma.gdextension`

## Build

Two parallel build systems; **SCons is the primary** (used in CI).

### SCons

```sh
scons target=template_debug platform=linux arch=x86_64 precision=single api_version=4.7
```

Generated binaries land in `bin/<platform>/` and are copied to `game/bin/<platform>/`.

### CMake (Ninja)

```sh
cmake -B build -G Ninja && ninja -C build
```

### Compilation database (IDE support)

```sh
scons compiledb=yes
# or without compiling:
scons compiledb=yes compile_commands.json
```

### CI

`.github/workflows/builds.yml` — matrix across 14 platform/arch combos × 2 target types × 2 float precisions.

## Conventions

- **C++17** standard
- **Style**: LLVM-based (`.clang-format`), tabs indent, 4-space width, access modifiers at -4 offset
- **Includes**: local `""` first (priority 1), angle-bracket `.h` second (priority 2), other angle-bracket third (priority 3)
- **EditorConfig**: tabs for most, spaces for Python/CMake/YAML
- **Physics**: Jolt Physics, runs on separate thread (`game/project.godot`)

## Gotchas

- `InputManager::initialize_input_map()` **destructively clears** `move_forward/back/left/right/jump` then rebinds them — any InputMap changes in `project.godot` for those actions are wiped at runtime.
- `custom.py` is gitignored — use it for local SCons overrides (e.g., build profile, custom flags).
- `build_profile.json` exists but is **not active** — uncomment the line in `SConstruct` to use it.

## Useful commands

```sh
# fast local dev build
scons target=template_debug platform=linux arch=x86_64 precision=single api_version=4.7 -j$(nproc)

# clang-format check (CI has this commented out)
clang-format src/* --dry-run --Werror

# ensure submodule initialized
git submodule update --init --recursive
```
