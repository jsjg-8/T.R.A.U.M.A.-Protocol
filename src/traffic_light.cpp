
#include "traffic_light.h"
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/variant.hpp>


void TrafficLight::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_go_texture", "texture"), &TrafficLight::set_go_texture);
    ClassDB::bind_method(D_METHOD("get_go_texture"), &TrafficLight::get_go_texture);
    ClassDB::bind_method(D_METHOD("set_stop_texture", "texture"), &TrafficLight::set_stop_texture);
    ClassDB::bind_method(D_METHOD("get_stop_texture"), &TrafficLight::get_stop_texture);
    ClassDB::bind_method(D_METHOD("set_caution_texture", "texture"), &TrafficLight::set_caution_texture);
    ClassDB::bind_method(D_METHOD("get_caution_texture"), &TrafficLight::get_caution_texture);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "go_texture", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_go_texture", "get_go_texture");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stop_texture", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_stop_texture", "get_stop_texture");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "caution_texture", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_caution_texture", "get_caution_texture");
    
}

void TrafficLight::set_go_texture(const Ref<Texture2D> &p_texture) {
    go_texture = p_texture;
};

Ref<Texture2D> TrafficLight::get_go_texture() const {
    return go_texture;
};


void TrafficLight::set_stop_texture(const Ref<Texture2D> &p_texture) {
    stop_texture = p_texture;
};

Ref<Texture2D> TrafficLight::get_stop_texture() const {
    return stop_texture;
};


void TrafficLight::set_caution_texture(const Ref<Texture2D> &p_texture) {
    caution_texture = p_texture;
};

Ref<Texture2D> TrafficLight::get_caution_texture() const {
    return caution_texture;
};



TrafficLight::TrafficLight() {
    texture_rect = memnew(TextureRect);
    add_child(texture_rect);
    texture_rect->set_anchors_preset(Control::LayoutPreset::PRESET_FULL_RECT);

}