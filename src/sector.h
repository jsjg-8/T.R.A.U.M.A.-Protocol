#pragma once

#include "world_state.h"

#include <vector>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// CoverPoint — position + facing direction + exposure level
class CoverPoint : public Resource {
	GDCLASS(CoverPoint, Resource)

private:
	Vector3 position;
	Vector3 facing;       // direction the cover faces (for leaning out)
	float exposure = 0.0f; // 0 = full cover, 1 = no cover
	CoverPointId cover_id = 0;

protected:
	static void _bind_methods();

public:
	CoverPoint();
	~CoverPoint();

	void set_position(const Vector3 &pos);
	Vector3 get_position() const;

	void set_facing(const Vector3 &dir);
	Vector3 get_facing() const;

	void set_exposure(float e);
	float get_exposure() const;

	void set_cover_id(CoverPointId id);
	CoverPointId get_cover_id() const;
};

// Door — connects two Rooms
class Door : public Resource {
	GDCLASS(Door, Resource)

private:
	DoorId door_id = 0;
	RoomId room_a = INVALID_ROOM_ID;
	RoomId room_b = INVALID_ROOM_ID;
	bool is_locked = false;
	bool is_open = true;

protected:
	static void _bind_methods();

public:
	Door();
	~Door();

	void set_door_id(DoorId id);
	DoorId get_door_id() const;

	void set_room_a(RoomId id);
	RoomId get_room_a() const;

	void set_room_b(RoomId id);
	RoomId get_room_b() const;

	void set_locked(bool locked);
	bool get_locked() const;

	void set_open(bool open);
	bool get_open() const;

	// Returns the other room given one side
	RoomId get_other_room(RoomId from) const;
};

// Room — a semantic region in the world
class Room : public Resource {
	GDCLASS(Room, Resource)

private:
	RoomId room_id = INVALID_ROOM_ID;
	String room_name;
	DistrictId district_id = 0;
	Vector3 center_position;
	float radius = 5.0f;  // approximate bounds for proximity checks

	std::vector<CoverPointId> cover_points;
	std::vector<DoorId> doors;

protected:
	static void _bind_methods();

public:
	Room();
	~Room();

	void set_room_id(RoomId id);
	RoomId get_room_id() const;

	void set_room_name(const String &name);
	String get_room_name() const;

	void set_district_id(DistrictId id);
	DistrictId get_district_id() const;

	void set_center_position(const Vector3 &pos);
	Vector3 get_center_position() const;

	void set_radius(float r);
	float get_radius() const;

	void add_cover_point(CoverPointId id);
	const std::vector<CoverPointId> &get_cover_points() const;

	void add_door(DoorId id);
	const std::vector<DoorId> &get_doors() const;
};

// Sector — a district containing multiple rooms
class Sector : public Resource {
	GDCLASS(Sector, Resource)

private:
	DistrictId sector_id = 0;
	String sector_name;
	std::vector<RoomId> rooms;

protected:
	static void _bind_methods();

public:
	Sector();
	~Sector();

	void set_sector_id(DistrictId id);
	DistrictId get_sector_id() const;

	void set_sector_name(const String &name);
	String get_sector_name() const;

	void add_room(RoomId id);
	const std::vector<RoomId> &get_rooms() const;
};
