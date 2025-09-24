#include "player_controller.h"
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void PlayerController::_bind_methods() {
    // Bind properties so they appear in the editor
    ClassDB::bind_method(D_METHOD("set_move_speed", "move_speed"), &PlayerController::set_move_speed);
    ClassDB::bind_method(D_METHOD("get_move_speed"), &PlayerController::get_move_speed);
    ClassDB::add_property("PlayerController", 
        PropertyInfo(Variant::FLOAT, "move_speed"), 
        "set_move_speed", "get_move_speed");
    
    ClassDB::bind_method(D_METHOD("set_jump_force", "jump_force"), &PlayerController::set_jump_force);
    ClassDB::bind_method(D_METHOD("get_jump_force"), &PlayerController::get_jump_force);
    ClassDB::add_property("PlayerController", 
        PropertyInfo(Variant::FLOAT, "jump_force"), 
        "set_jump_force", "get_jump_force");
    
    ClassDB::bind_method(D_METHOD("set_mouse_sensitivity", "mouse_sensitivity"), &PlayerController::set_mouse_sensitivity);
    ClassDB::bind_method(D_METHOD("get_mouse_sensitivity"), &PlayerController::get_mouse_sensitivity);
    ClassDB::add_property("PlayerController", 
        PropertyInfo(Variant::FLOAT, "mouse_sensitivity"), 
        "set_mouse_sensitivity", "get_mouse_sensitivity");
}

PlayerController::PlayerController() {
    // Initialize default values
    move_speed = 5.0f;
    jump_force = 10.0f;
    gravity = 30.0f;
    mouse_sensitivity = 0.003f;
    camera_rotation_x = 0.0f;
    camera_rotation_y = 0.0f;
    
    spring_arm = nullptr;
    camera = nullptr;
}

PlayerController::~PlayerController() {
    // Cleanup if needed
}

void PlayerController::_ready() {
    // Get references to camera components
    spring_arm = get_node<SpringArm3D>("CameraPivot/SpringArm3D");
    camera = get_node<Camera3D>("CameraPivot/SpringArm3D/Camera3D");
    
    // Make sure we have the components
    if (spring_arm && camera) {
        // Set the camera as current
        camera->set_current(true);
        
        // Capture mouse for first-person control
        Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
    } else {
        UtilityFunctions::printerr("PlayerController: Missing camera components!");
    }
}

void PlayerController::_input(const Ref<InputEvent>& event) {
    // Handle mouse look
    Ref<InputEventMouseMotion> mouse_event = event;
    if (mouse_event.is_valid() && Input::get_singleton()->get_mouse_mode() == Input::MOUSE_MODE_CAPTURED) {
        // Rotate camera based on mouse movement
        camera_rotation_y -= mouse_event->get_relative().x * mouse_sensitivity;
        camera_rotation_x -= mouse_event->get_relative().y * mouse_sensitivity;
        
        // Clamp vertical rotation to prevent flipping
        camera_rotation_x = Math::clamp(camera_rotation_x,(float)-Math_PI / 2.0f, (float)Math_PI / 2.0f);
        
        // Apply rotations
        if (spring_arm) {
            spring_arm->set_rotation(Vector3(camera_rotation_x, camera_rotation_y, 0));
        }
    }
    
    // Handle escape key to release mouse
    if (event->is_action_pressed("ui_cancel")) {
        if (Input::get_singleton()->get_mouse_mode() == Input::MOUSE_MODE_CAPTURED) {
            Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
        } else {
            Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
        }
    }
}

void PlayerController::_physics_process(double delta) {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    
    Vector3 velocity = get_velocity();
    
    // Apply gravity
    if (!is_on_floor()) {
        velocity.y -= gravity * delta;
    }
    
    // Handle jumping
    if (is_on_floor() && Input::get_singleton()->is_action_pressed("jump")) {
        velocity.y = jump_force;
    }
    
    // Get movement input
    Vector2 input_dir = Input::get_singleton()->get_vector("move_left", "move_right", "move_forward", "move_back");
    Vector3 direction = Vector3(input_dir.x, 0, input_dir.y).normalized();
    
    // Transform direction based on camera orientation
    if (spring_arm && direction.length() > 0) {
        // Get camera's forward direction (ignoring pitch)
        Vector3 camera_forward = -spring_arm->get_global_transform().basis.get_column(2);
        camera_forward.y = 0;
        camera_forward = camera_forward.normalized();
        
        // Get camera's right direction
        Vector3 camera_right = spring_arm->get_global_transform().basis.get_column(0);
        camera_right.y = 0;
        camera_right = camera_right.normalized();
        
        // Combine directions
        direction = (camera_forward * direction.z + camera_right * direction.x).normalized();
    }
    
    // Apply movement
    if (direction != Vector3()) {
        velocity.x = direction.x * move_speed;
        velocity.z = direction.z * move_speed;
    } else {
        // Apply friction when not moving
        velocity.x = Math::lerp(velocity.x, 0.0f, 0.2f);
        velocity.z = Math::lerp(velocity.z, 0.0f, 0.2f);
    }
    
    set_velocity(velocity);
    move_and_slide();
}

// Getters and setters
void PlayerController::set_move_speed(float p_move_speed) {
    move_speed = p_move_speed;
}

float PlayerController::get_move_speed() const {
    return move_speed;
}

void PlayerController::set_jump_force(float p_jump_force) {
    jump_force = p_jump_force;
}

float PlayerController::get_jump_force() const {
    return jump_force;
}

void PlayerController::set_mouse_sensitivity(float p_sensitivity) {
    mouse_sensitivity = p_sensitivity;
}

float PlayerController::get_mouse_sensitivity() const {
    return mouse_sensitivity;
}