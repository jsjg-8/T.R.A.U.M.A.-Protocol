#pragma once


#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/global_constants.hpp>

namespace godot {

class InputManager : public Object {
    GDCLASS(InputManager, Object)

private:
    static InputManager* singleton;
    void create_action_with_key(const String& action_name, Key keycode);

protected:
    static void _bind_methods();

public:
    static InputManager* get_singleton();

    InputManager();
    ~InputManager();

    void initialize_input_map();
};

}