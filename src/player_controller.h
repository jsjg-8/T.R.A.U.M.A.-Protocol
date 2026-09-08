
#pragma once

#include <godot_cpp/classes/animation_tree.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/spring_arm3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/engine.hpp>

namespace godot {

class PlayerController : public CharacterBody3D {
    GDCLASS(PlayerController, CharacterBody3D)

private:

    NodePath head_pivot_path;
    NodePath spring_arm_path;
    NodePath camera_path;
    NodePath animation_tree_path;


    // Camera components
    SpringArm3D* spring_arm;
    Camera3D* camera;
    Node3D* head_pivot;
    AnimationTree* animation_tree;
    
    // Blend parameters
    float blend_speed;
    float blend_direction;
    bool is_in_air;

    // Parameter names
    const String PARAM_SPEED = "blend_speed";
    const String PARAM_DIRECTION = "blend_direction";
    const String PARAM_IN_AIR = "is_in_air";
    
    // Movement variables
    float move_speed;
    float jump_force;
    float gravite;
    
    // Camera variables
    float mouse_sensitivity;
    float min_pitch;
    float max_pitch;

    float camera_distance;
    float camera_height_offset;
    bool camera_collision_mask;

protected:
    static void _bind_methods();

public:
    PlayerController();
    ~PlayerController();

    void _ready() override;
    void _input(const Ref<InputEvent>& event) override;
    void _physics_process(double delta) override;

    void set_head_pivot(Node3D* p_head_pivot);
    Node3D* get_head_pivot() const;

    void set_spring_arm(SpringArm3D* p_spring_arm);
    SpringArm3D* get_spring_arm() const;

    void set_camera(Camera3D* p_camera);
    Camera3D* get_camera() const;
    
    // Getters and setters for exported variables
    void set_move_speed(float p_move_speed);
    float get_move_speed() const;
    
    void set_jump_force(float p_jump_force);
    float get_jump_force() const;
    
    void set_mouse_sensitivity(float p_sensitivity);
    float get_mouse_sensitivity() const;

    void set_min_pitch(float p_min_pitch);
    float get_min_pitch() const;

    void set_max_pitch(float p_max_pitch);
    float get_max_pitch() const;

    void add_yaw(float amount_degrees);
    void add_pitch(float amount_degrees);
    void clamp_pitch();

    void set_camera_distance(float p_distance);
    float get_camera_distance() const;
    
    void set_camera_height_offset(float p_offset);
    float get_camera_height_offset() const;
    
    void set_camera_collision_mask(uint32_t p_mask);
    uint32_t get_camera_collision_mask() const;

    
    void set_animation_tree(AnimationTree* p_animation_tree);
    AnimationTree* get_animation_tree() const;
    void update_animation_blends();

    void set_animation_tree_path(const NodePath& p_path);
    NodePath get_animation_tree_path() const;
    
    // Blend parameter methods
    void set_blend_speed(float p_speed);
    float get_blend_speed() const;
    
    void set_blend_direction(float p_direction);
    float get_blend_direction() const;
    
    void set_is_in_air(bool p_in_air);
    bool get_is_in_air() const;

private:
    void initialize_input_actions();
};

}