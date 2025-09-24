
#pragma once

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
    // Camera components
    SpringArm3D* spring_arm;
    Camera3D* camera;
    
    // Movement variables
    float move_speed;
    float jump_force;
    float gravity;
    
    // Camera variables
    float mouse_sensitivity;
    float camera_rotation_x;
    float camera_rotation_y;

protected:
    static void _bind_methods();

public:
    PlayerController();
    ~PlayerController();

    void _ready() override;
    void _input(const Ref<InputEvent>& event) override;
    void _physics_process(double delta) override;
    
    // Getters and setters for exported variables
    void set_move_speed(float p_move_speed);
    float get_move_speed() const;
    
    void set_jump_force(float p_jump_force);
    float get_jump_force() const;
    
    void set_mouse_sensitivity(float p_sensitivity);
    float get_mouse_sensitivity() const;
};

}