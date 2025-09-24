#include "health_component.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void HealthComponent::_bind_methods() {
    // Bind properties
    ClassDB::bind_method(D_METHOD("set_max_health", "max_health"), &HealthComponent::set_max_health);
    ClassDB::bind_method(D_METHOD("get_max_health"), &HealthComponent::get_max_health);
    ClassDB::bind_method(D_METHOD("set_current_health", "current_health"), &HealthComponent::set_current_health);
    ClassDB::bind_method(D_METHOD("get_current_health"), &HealthComponent::get_current_health);
    
    ClassDB::add_property("HealthComponent", PropertyInfo(Variant::FLOAT, "max_health"), "set_max_health", "get_max_health");
    ClassDB::add_property("HealthComponent", PropertyInfo(Variant::FLOAT, "current_health"), "set_current_health", "get_current_health");
    
    // Bind methods
    ClassDB::bind_method(D_METHOD("take_damage", "damage_amount"), &HealthComponent::take_damage);
    ClassDB::bind_method(D_METHOD("heal", "heal_amount"), &HealthComponent::heal);
    ClassDB::bind_method(D_METHOD("get_health_percentage"), &HealthComponent::get_health_percentage);
    ClassDB::bind_method(D_METHOD("get_is_alive"), &HealthComponent::get_is_alive);
    ClassDB::bind_method(D_METHOD("get_is_invulnerable"), &HealthComponent::get_is_invulnerable);
    ClassDB::bind_method(D_METHOD("set_invulnerable", "invulnerable", "duration"), &HealthComponent::set_invulnerable, DEFVAL(0.0f));
    
    // Bind signals
    ADD_SIGNAL(MethodInfo("health_changed", PropertyInfo(Variant::FLOAT, "new_health"), PropertyInfo(Variant::FLOAT, "old_health")));
    ADD_SIGNAL(MethodInfo("damage_taken", PropertyInfo(Variant::FLOAT, "damage_amount")));
    ADD_SIGNAL(MethodInfo("healed", PropertyInfo(Variant::FLOAT, "heal_amount")));
    ADD_SIGNAL(MethodInfo("death"));
}

HealthComponent::HealthComponent() {
    // Constructor - initialize default values
    max_health = 100.0f;
    current_health = max_health;
    is_invulnerable = false;
    invulnerability_duration = 0.0f;
    invulnerability_timer = 0.0f;
}

HealthComponent::~HealthComponent() {
    // Destructor
}

void HealthComponent::_ready() {
    // Called when the node enters the scene tree
    UtilityFunctions::print("HealthComponent ready - Health: ", current_health, "/", max_health);
}

void HealthComponent::_process(double delta) {
    // Handle invulnerability timer
    if (is_invulnerable && invulnerability_duration > 0.0f) {
        invulnerability_timer -= delta;
        if (invulnerability_timer <= 0.0f) {
            is_invulnerable = false;
            invulnerability_duration = 0.0f;
            UtilityFunctions::print("Invulnerability ended");
        }
    }
}

void HealthComponent::take_damage(float damage_amount) {
    if (damage_amount <= 0.0f) return;
    if (is_invulnerable) {
        UtilityFunctions::print("Damage blocked - invulnerable");
        return;
    }
    if (current_health <= 0.0f) return; // Already dead
    
    float old_health = current_health;
    current_health = Math::max(0.0f, current_health - damage_amount);
    
    UtilityFunctions::print("Taking damage: ", damage_amount, " | Health: ", current_health, "/", max_health);
    
    // Emit signals
    emit_signal("damage_taken", damage_amount);
    emit_signal("health_changed", current_health, old_health);
    
    // Check for death
    if (current_health <= 0.0f && old_health > 0.0f) {
        UtilityFunctions::print("Entity died!");
        emit_signal("death");
    }
}

void HealthComponent::heal(float heal_amount) {
    if (heal_amount <= 0.0f) return;
    if (current_health <= 0.0f) return; // Can't heal the dead
    
    float old_health = current_health;
    current_health = Math::min(max_health, current_health + heal_amount);
    
    if (current_health != old_health) {
        UtilityFunctions::print("Healing: ", heal_amount, " | Health: ", current_health, "/", max_health);
        emit_signal("healed", heal_amount);
        emit_signal("health_changed", current_health, old_health);
    }
}

void HealthComponent::set_max_health(float new_max_health) {
    if (new_max_health <= 0.0f) {
        UtilityFunctions::print("Warning: Attempted to set max_health to ", new_max_health);
        return;
    }
    
    float old_current = current_health;
    max_health = new_max_health;
    
    // If current health exceeds new max, clamp it
    if (current_health > max_health) {
        current_health = max_health;
        emit_signal("health_changed", current_health, old_current);
    }
    
    UtilityFunctions::print("Max health set to: ", max_health);
}

void HealthComponent::set_current_health(float new_current_health) {
    float old_health = current_health;
    current_health = Math::clamp(new_current_health, 0.0f, max_health);
    
    if (current_health != old_health) {
        emit_signal("health_changed", current_health, old_health);
        
        // Check for death
        if (current_health <= 0.0f && old_health > 0.0f) {
            emit_signal("death");
        }
    }
}

float HealthComponent::get_health_percentage() const {
    if (max_health <= 0.0f) return 0.0f;
    return (current_health / max_health) * 100.0f;
}

void HealthComponent::set_invulnerable(bool invulnerable, float duration) {
    is_invulnerable = invulnerable;
    
    if (invulnerable && duration > 0.0f) {
        invulnerability_duration = duration;
        invulnerability_timer = duration;
        UtilityFunctions::print("Set invulnerable for ", duration, " seconds");
    } else {
        invulnerability_duration = 0.0f;
        invulnerability_timer = 0.0f;
    }
}