#include "input_manager.h"
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/input.hpp>

using namespace godot;

InputManager* InputManager::singleton = nullptr;


void InputManager::_bind_methods() { }

InputManager* InputManager::get_singleton() {
    return singleton;
}

InputManager::InputManager() {
    singleton = this;
}

InputManager::~InputManager() {
    singleton = nullptr;
}

void InputManager::initialize_input_map() {
    InputMap* input_map = InputMap::get_singleton();
    
    PackedStringArray actions_to_clear = { "move_forward", "move_back", "move_left", "move_right", "jump" };
    for (int i = 0; i < actions_to_clear.size(); i++) {
        if (input_map->has_action(actions_to_clear[i])) {
            input_map->erase_action(actions_to_clear[i]);
        }
    }
    
    // Create actions with their key bindings
    create_action_with_key("move_forward", KEY_W);
    create_action_with_key("move_back", KEY_S);
    create_action_with_key("move_left", KEY_A);
    create_action_with_key("move_right", KEY_D);
    create_action_with_key("jump", KEY_SPACE);
    
    input_map->add_action("shoot");
    Ref<InputEventMouseButton> mouse_event;
    mouse_event.instantiate();
    mouse_event->set_button_index(MOUSE_BUTTON_LEFT);
    input_map->action_add_event("shoot", mouse_event);
    UtilityFunctions::print("InputManager: Input map initialized successfully");
}

void InputManager::create_action_with_key(const String& action_name, Key keycode) {
    InputMap* input_map = InputMap::get_singleton();
    
    input_map->add_action(action_name);
    
    Ref<InputEventKey> event;
    event.instantiate();
    event->set_keycode(keycode);
    input_map->action_add_event(action_name, event);
}