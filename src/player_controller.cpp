#include "player_controller.h"
#include "./input_manager.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

using namespace godot;

void PlayerController::_bind_methods() {
	// Node exports
	ClassDB::bind_method(D_METHOD("set_head_pivot", "head_pivot"), &PlayerController::set_head_pivot);
	ClassDB::bind_method(D_METHOD("get_head_pivot"), &PlayerController::get_head_pivot);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::OBJECT, "head_pivot", PROPERTY_HINT_NODE_TYPE, "Node3D"),
						  "set_head_pivot", "get_head_pivot");

	ClassDB::bind_method(D_METHOD("set_spring_arm", "spring_arm"), &PlayerController::set_spring_arm);
	ClassDB::bind_method(D_METHOD("get_spring_arm"), &PlayerController::get_spring_arm);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::OBJECT, "spring_arm", PROPERTY_HINT_NODE_TYPE, "SpringArm3D"),
						  "set_spring_arm", "get_spring_arm");

	ClassDB::bind_method(D_METHOD("set_camera", "camera"), &PlayerController::set_camera);
	ClassDB::bind_method(D_METHOD("get_camera"), &PlayerController::get_camera);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::OBJECT, "camera", PROPERTY_HINT_NODE_TYPE, "Camera3D"),
						  "set_camera", "get_camera");

	// Movement properties
	ClassDB::bind_method(D_METHOD("set_move_speed", "move_speed"), &PlayerController::set_move_speed);
	ClassDB::bind_method(D_METHOD("get_move_speed"), &PlayerController::get_move_speed);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::FLOAT, "move_speed", PROPERTY_HINT_RANGE, "0.1,20.0,0.1"),
						  "set_move_speed", "get_move_speed");

	ClassDB::bind_method(D_METHOD("set_jump_force", "jump_force"), &PlayerController::set_jump_force);
	ClassDB::bind_method(D_METHOD("get_jump_force"), &PlayerController::get_jump_force);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::FLOAT, "jump_force", PROPERTY_HINT_RANGE, "0.1,30.0,0.1"),
						  "set_jump_force", "get_jump_force");

	// Camera properties
	ClassDB::bind_method(D_METHOD("set_mouse_sensitivity", "sensitivity"), &PlayerController::set_mouse_sensitivity);
	ClassDB::bind_method(D_METHOD("get_mouse_sensitivity"), &PlayerController::get_mouse_sensitivity);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::FLOAT, "mouse_sensitivity", PROPERTY_HINT_RANGE, "0.001,0.01,0.001"),
						  "set_mouse_sensitivity", "get_mouse_sensitivity");

	ClassDB::bind_method(D_METHOD("set_min_pitch", "min_pitch"), &PlayerController::set_min_pitch);
	ClassDB::bind_method(D_METHOD("get_min_pitch"), &PlayerController::get_min_pitch);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::FLOAT, "min_pitch", PROPERTY_HINT_RANGE, "-90,0,0.1"),
						  "set_min_pitch", "get_min_pitch");

	ClassDB::bind_method(D_METHOD("set_max_pitch", "max_pitch"), &PlayerController::set_max_pitch);
	ClassDB::bind_method(D_METHOD("get_max_pitch"), &PlayerController::get_max_pitch);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::FLOAT, "max_pitch", PROPERTY_HINT_RANGE, "0,90,0.1"),
						  "set_max_pitch", "get_max_pitch");

	// Rotation methods
	ClassDB::bind_method(D_METHOD("add_yaw", "amount_degrees"), &PlayerController::add_yaw);
	ClassDB::bind_method(D_METHOD("add_pitch", "amount_degrees"), &PlayerController::add_pitch);
	ClassDB::bind_method(D_METHOD("clamp_pitch"), &PlayerController::clamp_pitch);

	// Input handling
	ClassDB::bind_method(D_METHOD("_unhandled_input", "event"), &PlayerController::_unhandled_input);

	ClassDB::bind_method(D_METHOD("set_camera_collision_mask", "mask"), &PlayerController::set_camera_collision_mask);
	ClassDB::bind_method(D_METHOD("get_camera_collision_mask"), &PlayerController::get_camera_collision_mask);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::INT, "camera_collision_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS),
						  "set_camera_collision_mask", "get_camera_collision_mask");

	ClassDB::bind_method(D_METHOD("set_animation_tree_path", "path"), &PlayerController::set_animation_tree_path);
	ClassDB::bind_method(D_METHOD("get_animation_tree_path"), &PlayerController::get_animation_tree_path);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::NODE_PATH, "animation_tree_path"),
						  "set_animation_tree_path", "get_animation_tree_path");

	ClassDB::bind_method(D_METHOD("set_blend_speed", "speed"), &PlayerController::set_blend_speed);
	ClassDB::bind_method(D_METHOD("get_blend_speed"), &PlayerController::get_blend_speed);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::FLOAT, "blend_speed", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
						  "set_blend_speed", "get_blend_speed");

	ClassDB::bind_method(D_METHOD("set_blend_direction", "direction"), &PlayerController::set_blend_direction);
	ClassDB::bind_method(D_METHOD("get_blend_direction"), &PlayerController::get_blend_direction);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::FLOAT, "blend_direction", PROPERTY_HINT_RANGE, "-1.0,1.0,0.01"),
						  "set_blend_direction", "get_blend_direction");

	ClassDB::bind_method(D_METHOD("set_is_in_air", "in_air"), &PlayerController::set_is_in_air);
	ClassDB::bind_method(D_METHOD("get_is_in_air"), &PlayerController::get_is_in_air);
	ClassDB::add_property("PlayerController",
						  PropertyInfo(Variant::BOOL, "is_in_air"),
						  "set_is_in_air", "get_is_in_air");
}

PlayerController::PlayerController() {
	head_pivot = nullptr;
	spring_arm = nullptr;
	camera = nullptr;

	move_speed = 5.0f;
	jump_force = 10.0f;
	gravite = 30.0f;

	mouse_sensitivity = 0.0003f;
	min_pitch = -89.0f;
	max_pitch = 89.0f;

	camera_distance = 1.0f;
	camera_height_offset = 1.5f;
	camera_collision_mask = 1 | 2;

	animation_tree_path = NodePath("Pivot/character/AnimationTree");
	animation_tree = nullptr;
	blend_speed = 0.0f;
	blend_direction = 0.0f;
	is_in_air = false;
}

PlayerController::~PlayerController() {}

void PlayerController::_ready() {
	Input::get_singleton()->set_use_accumulated_input(false);

	if (InputManager::get_singleton()) {
		InputManager::get_singleton()->initialize_input_map();
	} else {
		UtilityFunctions::printerr("PlayerController: InputManager singleton not found");
	}

	if (has_node(animation_tree_path)) {
		animation_tree = get_node<AnimationTree>(animation_tree_path);
		if (animation_tree) {
			UtilityFunctions::print("Found AnimationTree with BlendTree");

			// Initialize blend parameters
			animation_tree->set(PARAM_SPEED, 0.0f);
			animation_tree->set(PARAM_DIRECTION, 0.0f);
			animation_tree->set(PARAM_IN_AIR, false);
		}
	} else {
		UtilityFunctions::printerr("AnimationTree not found at path: " + animation_tree_path);
	}

	UtilityFunctions::print("PlayerController: Ready with exported node references");
}

void PlayerController::update_animation_blends() {
	if (!animation_tree) {
		return;
	}
	Vector3 velocity = get_velocity();
	bool is_on_floor_now = is_on_floor();
	Vector3 horizontal_velocity = Vector3(velocity.x, 0, velocity.z);

	// Convert world velocity to local space for directional blending
	Vector3 local_velocity = get_global_transform().basis.inverse().xform(horizontal_velocity);

	// Set blend position for 2D blend space
	if (animation_tree->get("parameters/blend_position")) {
		animation_tree->set("parameters/blend_position", Vector2(local_velocity.x, local_velocity.z));
	}

	// Set speed for 1D blending (fallback)
	float speed = Math::clamp(horizontal_velocity.length() / move_speed, 0.0f, 1.0f);
	set_blend_speed(speed);

	// Air state
	set_is_in_air(!is_on_floor_now);

	// Debug output
	// UtilityFunctions::print("Blend - Speed: " + String::num(speed) + " Direction: " + String::num(direction) + " InAir: " + String::num(!is_on_floor_now));
}

void PlayerController::_input(const Ref<InputEvent> &event) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (Input::get_singleton()->get_mouse_mode() != Input::MOUSE_MODE_CAPTURED) {
		// Mouse is not captured - check for quit or capture
		if (event->is_action_pressed("ui_cancel")) {
			// Quit the game if escape is pressed and mouse is not captured
			get_tree()->quit();
		}

		if (event->is_action_pressed("shoot")) { // Using shoot action instead of mouse button for flexibility
			Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
		}

		return; // Don't process other input when mouse is not captured
	}

	// Mouse is captured - check for release
	if (event->is_action_pressed("ui_cancel")) {
		Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
		return;
	}

	// Handle mouse look only when mouse is captured
	Ref<InputEventMouseMotion> mouse_event = event;
	if (mouse_event.is_valid()) {
		float yaw_amount = -mouse_event->get_relative().x * mouse_sensitivity;
		float pitch_amount = -mouse_event->get_relative().y * mouse_sensitivity;

		add_yaw(Math::rad_to_deg(yaw_amount));
		add_pitch(Math::rad_to_deg(pitch_amount));
	}
}
void PlayerController::_physics_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	Vector3 velocity = get_velocity();

	// Apply gravity
	if (!is_on_floor()) {
		velocity.y -= gravite * delta;
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
		direction = (camera_forward * -direction.z + camera_right * direction.x).normalized();
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
	update_animation_blends();
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

void PlayerController::set_camera_distance(float p_distance) {
	camera_distance = p_distance;
	if (spring_arm) {
		spring_arm->set_length(camera_distance);
	}
}

float PlayerController::get_camera_distance() const {
	return camera_distance;
}

void PlayerController::set_head_pivot(Node3D *p_head_pivot) {
	head_pivot = p_head_pivot;
}

Node3D *PlayerController::get_head_pivot() const {
	return head_pivot;
}

void PlayerController::set_spring_arm(SpringArm3D *p_spring_arm) {
	spring_arm = p_spring_arm;
	if (spring_arm) {
		// Use the camera_distance property instead of hardcoding
		spring_arm->set_length(camera_distance);
		spring_arm->set_collision_mask(camera_collision_mask);
	}
}

SpringArm3D *PlayerController::get_spring_arm() const {
	return spring_arm;
}

void PlayerController::set_camera(Camera3D *p_camera) {
	camera = p_camera;
	if (camera) {
		camera->set_current(true);
	}
}

Camera3D *PlayerController::get_camera() const {
	return camera;
}

// Rotation methods (adapted from GDScript)
void PlayerController::add_yaw(float amount_degrees) {
	if (Math::is_zero_approx(amount_degrees)) {
		return;
	}

	rotate_object_local(Vector3(0, 1, 0), Math::deg_to_rad(amount_degrees));
	orthonormalize();
}

void PlayerController::add_pitch(float amount_degrees) {
	if (Math::is_zero_approx(amount_degrees) || !head_pivot) {
		return;
	}

	head_pivot->rotate_object_local(Vector3(1, 0, 0), Math::deg_to_rad(amount_degrees));
	head_pivot->orthonormalize();
	clamp_pitch();
}

void PlayerController::clamp_pitch() {
	if (!head_pivot) {
		return;
	}

	Vector3 rotation = head_pivot->get_rotation();
	float pitch_rad = rotation.x;
	float min_rad = Math::deg_to_rad(min_pitch);
	float max_rad = Math::deg_to_rad(max_pitch);

	if (pitch_rad > min_rad && pitch_rad < max_rad) {
		return;
	}

	rotation.x = Math::clamp(pitch_rad, min_rad, max_rad);
	head_pivot->set_rotation(rotation);
	head_pivot->orthonormalize();
}

// Camera collision getter/setter

void PlayerController::set_camera_collision_mask(uint32_t p_mask) {
	camera_collision_mask = p_mask;
	if (spring_arm) {
		spring_arm->set_collision_mask(camera_collision_mask);
	}
}

uint32_t PlayerController::get_camera_collision_mask() const {
	return camera_collision_mask;
}

void PlayerController::set_min_pitch(float p_min_pitch) {
	min_pitch = p_min_pitch;
}

float PlayerController::get_min_pitch() const {
	return min_pitch;
}

void PlayerController::set_max_pitch(float p_max_pitch) {
	max_pitch = p_max_pitch;
}

float PlayerController::get_max_pitch() const {
	return max_pitch;
}

void PlayerController::set_blend_speed(float p_speed) {
	blend_speed = p_speed;
	if (animation_tree && animation_tree->get(PARAM_SPEED)) {
		animation_tree->set(PARAM_SPEED, blend_speed);
	}
}

float PlayerController::get_blend_speed() const {
	return blend_speed;
}

void PlayerController::set_blend_direction(float p_direction) {
	blend_direction = p_direction;
	if (animation_tree && animation_tree->get(PARAM_DIRECTION)) {
		animation_tree->set(PARAM_DIRECTION, blend_direction);
	}
}

float PlayerController::get_blend_direction() const {
	return blend_direction;
}

void PlayerController::set_is_in_air(bool p_in_air) {
	is_in_air = p_in_air;
	if (animation_tree && animation_tree->get(PARAM_IN_AIR)) {
		animation_tree->set(PARAM_IN_AIR, is_in_air);
	}
}

bool PlayerController::get_is_in_air() const {
	return is_in_air;
}

// AnimationTree setter/getter
void PlayerController::set_animation_tree(AnimationTree *p_animation_tree) {
	animation_tree = p_animation_tree;
}

AnimationTree *PlayerController::get_animation_tree() const {
	return animation_tree;
}

void PlayerController::set_animation_tree_path(const NodePath &p_path) {
	animation_tree_path = p_path;
	if (is_inside_tree() && has_node(animation_tree_path)) {
		animation_tree = get_node<AnimationTree>(animation_tree_path);
	}
}

NodePath PlayerController::get_animation_tree_path() const {
	return animation_tree_path;
}