#include "stdafx.h"
#include "editortab.h"

#include "point.h"
#include "color_constants.h"

EditorTab::EditorTab(EditorState* editor, GraphicsManager* gfx): editor_ {editor}, gfx_ {gfx} {}

EditorTab::~EditorTab() {}

void EditorTab::init() {}

void EditorTab::handle_left_click(EditorRoom* eroom, Point3 pos) {}

void EditorTab::handle_right_click(EditorRoom* eroom, Point3 pos) {}

void clamp(int* n, int a, int b) {
    *n = std::max(a, std::min(b, *n));
}

void color_button(int color_id) {
	glm::vec4 color = COLOR_VECTORS[color_id];
	ImGui::ColorButton("##COLOR_BUTTON", ImVec4(color.x, color.y, color.z, color.w), 0, ImVec2(40, 40));
}