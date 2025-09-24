#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

class HealthComponent : public Node {
    GDCLASS(HealthComponent, Node)

private:
    float max_health = 100.0f;
    float current_health = 100.0f;
    bool is_invulnerable = false;
    float invulnerability_duration = 0.0f;
    float invulnerability_timer = 0.0f;

protected:
    static void _bind_methods();

public:
    HealthComponent();
    ~HealthComponent();

    // Core health methods
    void take_damage(float damage_amount);
    void heal(float heal_amount);
    void set_max_health(float new_max_health);
    void set_current_health(float new_current_health);
    void set_invulnerable(bool invulnerable, float duration = 0.0f);
    
    // Getters
    float get_max_health() const { return max_health; }
    float get_current_health() const { return current_health; }
    float get_health_percentage() const;
    bool get_is_alive() const { return current_health > 0.0f; }
    bool get_is_invulnerable() const { return is_invulnerable; }
    
    // Godot overrides
    void _ready() override;
    void _process(double delta) override;
    
    // Signals (will be bound in _bind_methods)
    void _on_health_changed(float new_health, float old_health);
    void _on_damage_taken(float damage_amount);
    void _on_healed(float heal_amount);
    void _on_death();
};
