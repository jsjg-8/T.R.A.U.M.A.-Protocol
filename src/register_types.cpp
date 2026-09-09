#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "./input_manager.h"
#include "./player_controller.h"
#include "actor.h"
#include "agent.h"
#include "faction.h"
#include "faction_registry.h"
#include "health_component.h"
#include "perception_component.h"
#include "sector.h"
#include "squad.h"
#include "traffic_light.h"
#include "world_simulation.h"

using namespace godot;

void initialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(Agent);
	GDREGISTER_CLASS(HealthComponent);
	GDREGISTER_CLASS(TrafficLight);
	GDREGISTER_CLASS(WorldSimulation);
	GDREGISTER_CLASS(PlayerController);
	GDREGISTER_CLASS(InputManager);
	GDREGISTER_CLASS(CoverPoint);
	GDREGISTER_CLASS(Door);
	GDREGISTER_CLASS(Room);
	GDREGISTER_CLASS(Sector);
	GDREGISTER_CLASS(Actor);
	GDREGISTER_CLASS(PerceptionComponent);
	GDREGISTER_CLASS(Squad);
	GDREGISTER_CLASS(Faction);
	GDREGISTER_CLASS(FactionRegistry);
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

extern "C" {
// Initialization
GDExtensionBool GDE_EXPORT trauma_engine_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
	init_obj.register_initializer(initialize_gdextension_types);
	init_obj.register_terminator(uninitialize_gdextension_types);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}