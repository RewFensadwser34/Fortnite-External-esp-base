#pragma once

#include <cstdint>
#include <cmath>
#include <d3d9.h>

#include "Driver.hxx"
#include "offsets.hxx"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288419716939937510
#endif

class Vector2 {
public:
	Vector2() : x(0.0), y(0.0) {}
	Vector2(double _x, double _y) : x(_x), y(_y) {}
	double x, y;
};

class Vector3 {
public:
	Vector3() : x(0.0), y(0.0), z(0.0) {}
	Vector3(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
	double x, y, z;
	inline double dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
	inline double distance(const Vector3& v) const {
		const double dx = v.x - x, dy = v.y - y, dz = v.z - z;
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}
	Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
};

struct FQuat { double x, y, z, w; };

struct FTransform {
	FQuat rot;
	Vector3 translation;
	char pad[4];
	Vector3 scale;
	char pad1[4];

	D3DMATRIX to_matrix_with_scale() const {
		D3DMATRIX m{};
		m._41 = (float)translation.x;
		m._42 = (float)translation.y;
		m._43 = (float)translation.z;
		const float x2 = (float)(rot.x + rot.x);
		const float y2 = (float)(rot.y + rot.y);
		const float z2 = (float)(rot.z + rot.z);
		const float xx2 = (float)(rot.x * x2);
		const float yy2 = (float)(rot.y * y2);
		const float zz2 = (float)(rot.z * z2);
		m._11 = (1.0f - (yy2 + zz2)) * (float)scale.x;
		m._22 = (1.0f - (xx2 + zz2)) * (float)scale.y;
		m._33 = (1.0f - (xx2 + yy2)) * (float)scale.z;
		const float yz2 = (float)(rot.y * z2);
		const float wx2 = (float)(rot.w * x2);
		m._32 = (yz2 - wx2) * (float)scale.z;
		m._23 = (yz2 + wx2) * (float)scale.y;
		const float xy2 = (float)(rot.x * y2);
		const float wz2 = (float)(rot.w * z2);
		m._21 = (xy2 - wz2) * (float)scale.y;
		m._12 = (xy2 + wz2) * (float)scale.x;
		const float xz2 = (float)(rot.x * z2);
		const float wy2 = (float)(rot.w * y2);
		m._31 = (xz2 + wy2) * (float)scale.z;
		m._13 = (xz2 - wy2) * (float)scale.x;
		m._14 = 0.0f;
		m._24 = 0.0f;
		m._34 = 0.0f;
		m._44 = 1.0f;
		return m;
	}
};

inline D3DMATRIX matrix_multiplication(const D3DMATRIX& pm1, const D3DMATRIX& pm2) {
	D3DMATRIX pout{};
	pout._11 = pm1._11 * pm2._11 + pm1._12 * pm2._21 + pm1._13 * pm2._31 + pm1._14 * pm2._41;
	pout._12 = pm1._11 * pm2._12 + pm1._12 * pm2._22 + pm1._13 * pm2._32 + pm1._14 * pm2._42;
	pout._13 = pm1._11 * pm2._13 + pm1._12 * pm2._23 + pm1._13 * pm2._33 + pm1._14 * pm2._43;
	pout._14 = pm1._11 * pm2._14 + pm1._12 * pm2._24 + pm1._13 * pm2._34 + pm1._14 * pm2._44;
	pout._21 = pm1._21 * pm2._11 + pm1._22 * pm2._21 + pm1._23 * pm2._31 + pm1._24 * pm2._41;
	pout._22 = pm1._21 * pm2._12 + pm1._22 * pm2._22 + pm1._23 * pm2._32 + pm1._24 * pm2._42;
	pout._23 = pm1._21 * pm2._13 + pm1._22 * pm2._23 + pm1._23 * pm2._33 + pm1._24 * pm2._43;
	pout._24 = pm1._21 * pm2._14 + pm1._22 * pm2._24 + pm1._23 * pm2._34 + pm1._24 * pm2._44;
	pout._31 = pm1._31 * pm2._11 + pm1._32 * pm2._21 + pm1._33 * pm2._31 + pm1._34 * pm2._41;
	pout._32 = pm1._31 * pm2._12 + pm1._32 * pm2._22 + pm1._33 * pm2._32 + pm1._34 * pm2._42;
	pout._33 = pm1._31 * pm2._13 + pm1._32 * pm2._23 + pm1._33 * pm2._33 + pm1._34 * pm2._43;
	pout._34 = pm1._31 * pm2._14 + pm1._32 * pm2._24 + pm1._33 * pm2._34 + pm1._34 * pm2._44;
	pout._41 = pm1._41 * pm2._11 + pm1._42 * pm2._21 + pm1._43 * pm2._31 + pm1._44 * pm2._41;
	pout._42 = pm1._41 * pm2._12 + pm1._42 * pm2._22 + pm1._43 * pm2._32 + pm1._44 * pm2._42;
	pout._43 = pm1._41 * pm2._13 + pm1._42 * pm2._23 + pm1._43 * pm2._33 + pm1._44 * pm2._43;
	pout._44 = pm1._41 * pm2._14 + pm1._42 * pm2._24 + pm1._43 * pm2._34 + pm1._44 * pm2._44;
	return pout;
}

inline D3DMATRIX to_matrix(const Vector3& rot, const Vector3& origin = Vector3(0, 0, 0)) {
	const float radpitch = (float)(rot.x * M_PI / 180.0);
	const float radyaw = (float)(rot.y * M_PI / 180.0);
	const float radroll = (float)(rot.z * M_PI / 180.0);
	const float sp = std::sin(radpitch);
	const float cp = std::cos(radpitch);
	const float sy = std::sin(radyaw);
	const float cy = std::cos(radyaw);
	const float sr = std::sin(radroll);
	const float cr = std::cos(radroll);
	D3DMATRIX matrix{};
	matrix.m[0][0] = cp * cy;
	matrix.m[0][1] = cp * sy;
	matrix.m[0][2] = sp;
	matrix.m[0][3] = 0.f;
	matrix.m[1][0] = sr * sp * cy - cr * sy;
	matrix.m[1][1] = sr * sp * sy + cr * cy;
	matrix.m[1][2] = -sr * cp;
	matrix.m[1][3] = 0.f;
	matrix.m[2][0] = -(cr * sp * cy + sr * sy);
	matrix.m[2][1] = cy * sr - cr * sp * sy;
	matrix.m[2][2] = cr * cp;
	matrix.m[2][3] = 0.f;
	matrix.m[3][0] = (float)origin.x;
	matrix.m[3][1] = (float)origin.y;
	matrix.m[3][2] = (float)origin.z;
	matrix.m[3][3] = 1.f;
	return matrix;
}

struct Camera {
	Vector3 location;
	Vector3 rotation;
	float fov = 90.0f;
};

struct FNRot {
	double a = 0.0;
	char pad_0008[24] = {};
	double b = 0.0;
	char pad_0028[424] = {};
	double c = 0.0;
};


namespace settings {
	inline int width = 1920;
	inline int height = 1080;
	inline int screen_center_x = 960;
	inline int screen_center_y = 540;

	inline void set_resolution(int w, int h) {
		if (w <= 0 || h <= 0)
			return;
		width = w;
		height = h;
		screen_center_x = w / 2;
		screen_center_y = h / 2;
	}
} 


inline std::uintptr_t decrypt_uworld(std::uintptr_t image_base) {
	if (!image_base)
		return 0;
	const std::uintptr_t gengine = read<std::uintptr_t>(image_base + offsets::GEngine);
	if (!gengine)
		return 0;
	const std::uintptr_t game_viewport = read<std::uintptr_t>(gengine + offsets::GameViewport);
	if (!game_viewport)
		return 0;
	return read<std::uintptr_t>(game_viewport + offsets::ViewportClient);
}

namespace cache {
	inline std::uintptr_t base = 0;
	inline std::uintptr_t uworld = 0;
	inline std::uintptr_t game_instance = 0;
	inline std::uintptr_t local_players = 0;
	inline std::uintptr_t local_player = 0;
	inline std::uintptr_t player_controller = 0;
	inline std::uintptr_t local_pawn = 0;
	inline std::uintptr_t root_component = 0;
	inline std::uintptr_t player_state = 0;
	inline Vector3 relative_location;
	inline int my_team_id = 0;
	inline std::uintptr_t game_state = 0;
	inline std::uintptr_t player_array = 0;
	inline int player_count = 0;
	inline float closest_distance = 0.0f;
	inline std::uintptr_t closest_mesh = 0;
	inline Camera local_camera;

	inline bool update(std::uintptr_t image_base) {
		if (!image_base)
			return false;
		base = image_base;

		uworld = decrypt_uworld(base);
		if (!uworld)
			uworld = read<std::uintptr_t>(base + offsets::core::UWORLD);
		if (!uworld)
			return false;
		game_instance = read<std::uintptr_t>(uworld + offsets::core::GameInstance);
		if (!game_instance)
			return false;
		local_players = read<std::uintptr_t>(game_instance + offsets::player::LocalPlayers);
		if (!local_players)
			return false;
		local_player = read<std::uintptr_t>(local_players);
		if (!local_player)
			return false;
		player_controller = read<std::uintptr_t>(local_player + offsets::player::PlayerController);
		if (!player_controller)
			return false;
		local_pawn = read<std::uintptr_t>(player_controller + offsets::player::LocalPawn);
		if (!local_pawn)
			return false;
		root_component = read<std::uintptr_t>(local_pawn + offsets::core::RootComponent);
		if (root_component)
			relative_location = read<Vector3>(root_component + offsets::core::RelativeLocation);
		player_state = read<std::uintptr_t>(local_pawn + offsets::player::PlayerState);
		if (player_state)
			my_team_id = read<int>(player_state + offsets::player::TeamIndex);
		game_state = read<std::uintptr_t>(uworld + offsets::core::GameState);
		if (!game_state)
			return false;
		player_array = read<std::uintptr_t>(game_state + offsets::core::PlayerArray);
		player_count = read<int>(game_state + offsets::core::PlayerArray + sizeof(std::uintptr_t));
		if (!player_array || player_count <= 0 || player_count > 200)
			return false;
		return true;
	}
} 

inline Camera get_view_point() {
	Camera view_point{};
	if (!cache::uworld)
		return view_point;
	const std::uintptr_t location_pointer = read<std::uintptr_t>(cache::uworld + offsets::core::LocationPointer);
	const std::uintptr_t rotation_pointer = read<std::uintptr_t>(cache::uworld + offsets::core::RotationPointer);
	if (!location_pointer || !rotation_pointer)
		return view_point;
	FNRot fnrot{};
	fnrot.a = read<double>(rotation_pointer);
	fnrot.b = read<double>(rotation_pointer + 0x20);
	fnrot.c = read<double>(rotation_pointer + 0x1D0);
	view_point.location = read<Vector3>(location_pointer);
	view_point.rotation.x = std::asin(fnrot.c) * (180.0 / M_PI);
	view_point.rotation.y = ((std::atan2(fnrot.a * -1.0, fnrot.b) * (180.0 / M_PI)) * -1.0) * -1.0;
	if (cache::player_controller)
		view_point.fov = read<float>(cache::player_controller + offsets::core::FOV) * 90.0f;
	return view_point;
}

inline Camera TUNGTUNGCAMERA() { return get_view_point(); }

inline Vector2 world_to_screen(const Vector3& world_location) {
	cache::local_camera = get_view_point();
	const D3DMATRIX temp_matrix = to_matrix(cache::local_camera.rotation);
	const Vector3 vaxisx(temp_matrix.m[0][0], temp_matrix.m[0][1], temp_matrix.m[0][2]);
	const Vector3 vaxisy(temp_matrix.m[1][0], temp_matrix.m[1][1], temp_matrix.m[1][2]);
	const Vector3 vaxisz(temp_matrix.m[2][0], temp_matrix.m[2][1], temp_matrix.m[2][2]);
	const Vector3 vdelta = world_location - cache::local_camera.location;
	Vector3 vtransformed(vdelta.dot(vaxisy), vdelta.dot(vaxisz), vdelta.dot(vaxisx));
	if (vtransformed.z < 1.0)
		vtransformed.z = 1.0;
	const float tan_half = std::tan((float)(cache::local_camera.fov * M_PI / 360.0));
	const double x = (double)settings::screen_center_x + vtransformed.x * (((double)settings::screen_center_x / tan_half)) / vtransformed.z;
	const double y = (double)settings::screen_center_y - vtransformed.y * (((double)settings::screen_center_x / tan_half)) / vtransformed.z;
	return Vector2(x, y);
}

inline Vector2 TUNGTUNGWorldtoscreen(const Vector3& world_location) { return world_to_screen(world_location); }

inline Vector3 get_bone(std::uintptr_t mesh, int bone_id) {
	if (!mesh)
		return Vector3();
	std::uintptr_t bone_array = read<std::uintptr_t>(mesh + offsets::core::BoneArray_cache);
	if (!bone_array)
		bone_array = read<std::uintptr_t>(mesh + offsets::core::BoneArray); // fallback, was duplicated _cache
	if (!bone_array)
		return Vector3();
	const FTransform bone = read<FTransform>(bone_array + (std::uintptr_t)bone_id * 0x60);
	const FTransform component_to_world = read<FTransform>(mesh + offsets::core::ComponentToWorld);
	const D3DMATRIX matrix = matrix_multiplication(bone.to_matrix_with_scale(), component_to_world.to_matrix_with_scale());
	return Vector3(matrix._41, matrix._42, matrix._43);
}

inline Vector3 TUNGTUNGBONE(std::uintptr_t mesh, int bone_id) { return get_bone(mesh, bone_id); }
