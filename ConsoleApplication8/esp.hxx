#pragma once

#include <string>
#include "imgui/imgui.h"
#include "sdk.hxx"
#include "menu.hxx"

namespace esp {

 inline constexpr int HEAD_BONE = 110;

 inline int dbg_scanned = 0;
inline int dbg_drawn = 0;
inline int dbg_skipped_pawn = 0;
inline int dbg_skipped_team = 0;
inline int dbg_skipped_mesh = 0;
inline int dbg_skipped_bone = 0;
inline int dbg_skipped_screen = 0;
inline std::string dbg_reason = "not run yet";

inline void draw_box(ImDrawList* dl, const ImVec2& top, const ImVec2& bottom, ImU32 col, float thickness, bool outline) {
	if (!dl)
		return;
	const float height = bottom.y - top.y;
	if (height <= 2.0f)
		return;
	const float width = height * 0.5f;
	const float cx = (top.x + bottom.x) * 0.5f;
	const ImVec2 a(cx - width * 0.5f, top.y);
	const ImVec2 b(cx + width * 0.5f, bottom.y);
	if (outline)
		dl->AddRect(ImVec2(a.x - 1.0f, a.y - 1.0f), ImVec2(b.x + 1.0f, b.y + 1.0f),
			IM_COL32(0, 0, 0, 255), 0.0f, 0, thickness + 1.0f);
	dl->AddRect(a, b, col, 0.0f, 0, thickness);
}


inline void render(ImDrawList* dl) {
	dbg_scanned = 0; dbg_drawn = 0;
	dbg_skipped_pawn = 0; dbg_skipped_team = 0; dbg_skipped_mesh = 0;
	dbg_skipped_bone = 0; dbg_skipped_screen = 0;

	if (!dl) { dbg_reason = "no drawlist"; return; }
	if (!menu::visuals::enabled || !menu::visuals::box_esp) { dbg_reason = "menu gate off (Enable+Box ESP)"; return; }
	if (!cache::player_array) { dbg_reason = "player_array null (cache::update failing?)"; return; }
	if (cache::player_count <= 0 || cache::player_count > 200) { dbg_reason = "player_count insane"; return; }

	const ImU32 col = ImGui::ColorConvertFloat4ToU32(menu::visuals::box_color);
	const float thickness = menu::visuals::box_thickness;
	const bool outline = menu::visuals::box_outline;

	for (int i = 0; i < cache::player_count; ++i) {
		++dbg_scanned;
		const std::uintptr_t state = read<std::uintptr_t>(cache::player_array + (std::uintptr_t)i * sizeof(std::uintptr_t));
		if (!state) { ++dbg_skipped_pawn; continue; }

		const int team = read<int>(state + offsets::player::TeamIndex);
		if (team != 0 && team == cache::my_team_id) { ++dbg_skipped_team; continue; } 

		const std::uintptr_t pawn = read<std::uintptr_t>(state + offsets::player::PawnPrivate);
		if (!pawn || pawn == cache::local_pawn) { ++dbg_skipped_pawn; continue; }

		const std::uintptr_t mesh = read<std::uintptr_t>(pawn + offsets::player::Mesh);
		if (!mesh) { ++dbg_skipped_mesh; continue; }

		const Vector3 head3d = get_bone(mesh, HEAD_BONE); 
		const Vector3 feet3d = get_bone(mesh, 0);         
		if ((head3d.x == 0.0 && head3d.y == 0.0 && head3d.z == 0.0) ||
			(feet3d.x == 0.0 && feet3d.y == 0.0 && feet3d.z == 0.0)) { ++dbg_skipped_bone; continue; }

		const Vector2 head = world_to_screen(head3d);
		const Vector2 feet = world_to_screen(feet3d);

		if (feet.x < -500.0 || feet.x > (double)settings::width + 500.0 ||
			feet.y < -500.0 || feet.y > (double)settings::height + 500.0) { ++dbg_skipped_screen; continue; }

		const float box_height = (float)std::abs(head.y - feet.y);
		if (box_height <= 2.0f) { ++dbg_skipped_screen; continue; }
		const float box_width = box_height * 0.50f;
		const float left = (float)head.x - box_width * 0.5f;
		const float top = (float)head.y - 10.0f;    
		const float bottom = (float)feet.y + 10.0f;

		if (outline)
			dl->AddRect(ImVec2(left - 1.0f, top - 1.0f), ImVec2(left + box_width + 1.0f, bottom + 1.0f),
				IM_COL32(0, 0, 0, 255), 0.0f, 0, thickness + 1.0f);
		dl->AddRect(ImVec2(left, top), ImVec2(left + box_width, bottom),
			col, 0.0f, 0, thickness);
		++dbg_drawn;
	}

	if (dbg_drawn > 0)
		dbg_reason = "drawing";
	else if (dbg_skipped_team == dbg_scanned)
		dbg_reason = "all skipped as teammates (team id / offset?)";
	else if (dbg_skipped_mesh > 0 && dbg_skipped_mesh == dbg_scanned - dbg_skipped_pawn)
		dbg_reason = "mesh/root null for all (offsets stale?)";
	else if (dbg_skipped_bone > 0)
		dbg_reason = "bone null (mesh offset / bone id?)";
	else if (dbg_skipped_screen == dbg_scanned)
		dbg_reason = "all off-screen (camera/w2s wrong?)";
	else
		dbg_reason = "scanned, none drawable";
}

} 