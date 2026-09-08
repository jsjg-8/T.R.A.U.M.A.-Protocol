#include "sector.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// --- CoverPoint ---

void CoverPoint::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_position", "pos"), &CoverPoint::set_position);
	ClassDB::bind_method(D_METHOD("get_position"), &CoverPoint::get_position);
	ClassDB::bind_method(D_METHOD("set_facing", "dir"), &CoverPoint::set_facing);
	ClassDB::bind_method(D_METHOD("get_facing"), &CoverPoint::get_facing);
	ClassDB::bind_method(D_METHOD("set_exposure", "e"), &CoverPoint::set_exposure);
	ClassDB::bind_method(D_METHOD("get_exposure"), &CoverPoint::get_exposure);
	ClassDB::bind_method(D_METHOD("set_cover_id", "id"), &CoverPoint::set_cover_id);
	ClassDB::bind_method(D_METHOD("get_cover_id"), &CoverPoint::get_cover_id);

	ClassDB::add_property("CoverPoint", PropertyInfo(Variant::VECTOR3, "position"), "set_position", "get_position");
	ClassDB::add_property("CoverPoint", PropertyInfo(Variant::VECTOR3, "facing"), "set_facing", "get_facing");
	ClassDB::add_property("CoverPoint", PropertyInfo(Variant::FLOAT, "exposure", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_exposure", "get_exposure");
}

CoverPoint::CoverPoint() {}
CoverPoint::~CoverPoint() {}

void CoverPoint::set_position(const Vector3 &pos) { position = pos; }
Vector3 CoverPoint::get_position() const { return position; }

void CoverPoint::set_facing(const Vector3 &dir) { facing = dir; }
Vector3 CoverPoint::get_facing() const { return facing; }

void CoverPoint::set_exposure(float e) { exposure = e; }
float CoverPoint::get_exposure() const { return exposure; }

void CoverPoint::set_cover_id(CoverPointId id) { cover_id = id; }
CoverPointId CoverPoint::get_cover_id() const { return cover_id; }

// --- Door ---

void Door::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_door_id", "id"), &Door::set_door_id);
	ClassDB::bind_method(D_METHOD("get_door_id"), &Door::get_door_id);
	ClassDB::bind_method(D_METHOD("set_room_a", "id"), &Door::set_room_a);
	ClassDB::bind_method(D_METHOD("get_room_a"), &Door::get_room_a);
	ClassDB::bind_method(D_METHOD("set_room_b", "id"), &Door::set_room_b);
	ClassDB::bind_method(D_METHOD("get_room_b"), &Door::get_room_b);
	ClassDB::bind_method(D_METHOD("set_locked", "locked"), &Door::set_locked);
	ClassDB::bind_method(D_METHOD("get_locked"), &Door::get_locked);
	ClassDB::bind_method(D_METHOD("set_open", "open"), &Door::set_open);
	ClassDB::bind_method(D_METHOD("get_open"), &Door::get_open);
	ClassDB::bind_method(D_METHOD("get_other_room", "from"), &Door::get_other_room);

	ClassDB::add_property("Door", PropertyInfo(Variant::BOOL, "locked"), "set_locked", "get_locked");
	ClassDB::add_property("Door", PropertyInfo(Variant::BOOL, "open"), "set_open", "get_open");
}

Door::Door() {}
Door::~Door() {}

void Door::set_door_id(DoorId id) { door_id = id; }
DoorId Door::get_door_id() const { return door_id; }

void Door::set_room_a(RoomId id) { room_a = id; }
RoomId Door::get_room_a() const { return room_a; }

void Door::set_room_b(RoomId id) { room_b = id; }
RoomId Door::get_room_b() const { return room_b; }

void Door::set_locked(bool locked) { is_locked = locked; }
bool Door::get_locked() const { return is_locked; }

void Door::set_open(bool open) { is_open = open; }
bool Door::get_open() const { return is_open; }

RoomId Door::get_other_room(RoomId from) const {
	if (from == room_a) {
		return room_b;
	} else if (from == room_b) {
		return room_a;
	}
	return INVALID_ROOM_ID;
}

// --- Room ---

void Room::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_room_id", "id"), &Room::set_room_id);
	ClassDB::bind_method(D_METHOD("get_room_id"), &Room::get_room_id);
	ClassDB::bind_method(D_METHOD("set_room_name", "name"), &Room::set_room_name);
	ClassDB::bind_method(D_METHOD("get_room_name"), &Room::get_room_name);
	ClassDB::bind_method(D_METHOD("set_district_id", "id"), &Room::set_district_id);
	ClassDB::bind_method(D_METHOD("get_district_id"), &Room::get_district_id);
	ClassDB::bind_method(D_METHOD("set_center_position", "pos"), &Room::set_center_position);
	ClassDB::bind_method(D_METHOD("get_center_position"), &Room::get_center_position);
	ClassDB::bind_method(D_METHOD("set_radius", "r"), &Room::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &Room::get_radius);
	ClassDB::bind_method(D_METHOD("add_cover_point", "id"), &Room::add_cover_point);
	ClassDB::bind_method(D_METHOD("add_door", "id"), &Room::add_door);

	ClassDB::add_property("Room", PropertyInfo(Variant::VECTOR3, "center_position"), "set_center_position", "get_center_position");
	ClassDB::add_property("Room", PropertyInfo(Variant::FLOAT, "radius", PROPERTY_HINT_RANGE, "0.1,100.0,0.1"), "set_radius", "get_radius");
}

Room::Room() {}
Room::~Room() {}

void Room::set_room_id(RoomId id) { room_id = id; }
RoomId Room::get_room_id() const { return room_id; }

void Room::set_room_name(const String &name) { room_name = name; }
String Room::get_room_name() const { return room_name; }

void Room::set_district_id(DistrictId id) { district_id = id; }
DistrictId Room::get_district_id() const { return district_id; }

void Room::set_center_position(const Vector3 &pos) { center_position = pos; }
Vector3 Room::get_center_position() const { return center_position; }

void Room::set_radius(float r) { radius = r; }
float Room::get_radius() const { return radius; }

void Room::add_cover_point(CoverPointId id) { cover_points.push_back(id); }
const std::vector<CoverPointId> &Room::get_cover_points() const { return cover_points; }

void Room::add_door(DoorId id) { doors.push_back(id); }
const std::vector<DoorId> &Room::get_doors() const { return doors; }

// --- Sector ---

void Sector::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sector_id", "id"), &Sector::set_sector_id);
	ClassDB::bind_method(D_METHOD("get_sector_id"), &Sector::get_sector_id);
	ClassDB::bind_method(D_METHOD("set_sector_name", "name"), &Sector::set_sector_name);
	ClassDB::bind_method(D_METHOD("get_sector_name"), &Sector::get_sector_name);
	ClassDB::bind_method(D_METHOD("add_room", "id"), &Sector::add_room);
}

Sector::Sector() {}
Sector::~Sector() {}

void Sector::set_sector_id(DistrictId id) { sector_id = id; }
DistrictId Sector::get_sector_id() const { return sector_id; }

void Sector::set_sector_name(const String &name) { sector_name = name; }
String Sector::get_sector_name() const { return sector_name; }

void Sector::add_room(RoomId id) { rooms.push_back(id); }
const std::vector<RoomId> &Sector::get_rooms() const { return rooms; }
