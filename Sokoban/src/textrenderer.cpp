#include "stdafx.h"
#include "textrenderer.h"
#include "common_constants.h"
#include "color_constants.h"

TextRenderer::TextRenderer() :
	text_shader_{ Shader("shaders/text_shader.vs", "shaders/text_shader.fs") } {}

TextRenderer::~TextRenderer() {}

void TextRenderer::init() {
	// Load the font library
	if (FT_Init_FreeType(&ft_)) {
		std::cout << "Failed to initialize FreeType" << std::endl;
	}
	// Initialize our fonts
	text_shader_.use();
	kalam_ = make_font("resources/kalam/Kalam-Bold.ttf", 72);
}

void TextRenderer::render_text() {
	text_shader_.use();
	for (auto& drawer : drawers_) {
		drawer->render();
	}
	if (room_label_) {
		room_label_->update();
		room_label_->render();
	}
}

void TextRenderer::make_room_label(std::string label) {
	room_label_ = std::make_unique<RoomLabelDrawer>(
		kalam_.get(), COLOR_VECTORS[CYAN], label);
}

std::unique_ptr<Font> TextRenderer::make_font(std::string path, unsigned int font_size) {
	return std::make_unique<Font>(ft_, text_shader_, path, font_size);
}


StringDrawer::StringDrawer(Font* font, glm::vec4 color,
	std::string label, float x, float y, float sx, float sy) :
	color_{ color }, shader_{ font->shader_ }, tex_{ font->tex_ } {
	font->generate_string_verts(label.c_str(), x, y, sx, sy, vertices_, &width_);
	glGenVertexArrays(1, &VAO_);
	glBindVertexArray(VAO_);
	glGenBuffers(1, &VBO_);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_);
	glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(TextVertex), vertices_.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)offsetof(TextVertex, Position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)offsetof(TextVertex, TexCoords));
}

StringDrawer::~StringDrawer() {
	glDeleteVertexArrays(1, &VAO_);
	glDeleteBuffers(1, &VBO_);
}

void StringDrawer::set_color(int color) {
	color_ = COLOR_VECTORS[color];
}

void StringDrawer::set_color(glm::vec4 color) {
	color_ = color;
}


void StringDrawer::render() {
	shader_.setVec4("color", color_);
	glBindVertexArray(VAO_);
	glBindTexture(GL_TEXTURE_2D, tex_);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices_.size());
}


RoomLabelDrawer::RoomLabelDrawer(Font* font, glm::vec4 color, std::string label) :
	StringDrawer(font, color, label, 0, 0.8f, 1, 1),
	cooldown_{ AREA_NAME_DISPLAY_FRAMES } {}

RoomLabelDrawer::~RoomLabelDrawer() {}

void RoomLabelDrawer::update() {
	if (cooldown_) {
		--cooldown_;
		if (cooldown_ > AREA_NAME_DISPLAY_FRAMES - AREA_NAME_FADE_FRAMES) {
			color_.w = (float)(AREA_NAME_DISPLAY_FRAMES - cooldown_) / (float)AREA_NAME_FADE_FRAMES;
		} else if (cooldown_ < AREA_NAME_FADE_FRAMES) {
			color_.w = (float)cooldown_ / (float)AREA_NAME_FADE_FRAMES;
		} else {
			color_.w = 1;
		}
	}
}


Font::Font(FT_Library ft, Shader text_shader, std::string path, unsigned int font_size) :
	shader_{ text_shader },
	tex_width_{ 1 << 9 }, tex_height_{ 1 << 9 } {
	if (FT_New_Face(ft, path.c_str(), 0, &face_)) {
		std::cout << "Failed to load the font" << std::endl;
	}
	FT_Set_Pixel_Sizes(face_, 0, font_size);
	init_glyphs(font_size);
}

Font::~Font() {
	glDeleteTextures(1, &tex_);
}

void Font::init_glyphs(int font_size) {
	// Figure out the size the texture has to be
	auto* g = face_->glyph;

	unsigned int left = 0;
	unsigned int top = 0;
	unsigned int max_height = 0;
	// Not every character is even in the font!
	const char* good_characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:,;'\"(!?)+-*/= ";
	for (const char* c = good_characters; *c; ++c) {
		if (FT_Load_Char(face_, *c, FT_LOAD_RENDER)) {
			continue;
		}
		if (left + g->bitmap.width >= tex_width_) {
			top += max_height + 1;
			left = 0;
			max_height = 0;
		}
		glyphs_[*c] = {
			left,
			top,
			g->bitmap_left,
			g->bitmap_top,
			g->bitmap.width,
			g->bitmap.rows,
			g->advance.x >> 6,
			g->advance.y >> 6,
		};
		left += g->bitmap.width + 1;
		max_height = std::max(max_height, g->bitmap.rows);
	}
	while (tex_height_ <= top + max_height) {
		tex_height_ <<= 1;
	}

	glGenTextures(1, &tex_);
	glBindTexture(GL_TEXTURE_2D, tex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tex_width_, tex_height_, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Copy the glyphs into the texture
	for (const char* c = good_characters; *c; ++c) {
		if (FT_Load_Char(face_, *c, FT_LOAD_RENDER)) {
			continue;
		}
		GlyphPos cur = glyphs_[*c];

		glTexSubImage2D(GL_TEXTURE_2D, 0, cur.left, cur.top, cur.width, cur.height, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
	}
}

void Font::render_glyphs() {}

void Font::generate_string_verts(const char* text, float x, float y, float sx, float sy, std::vector<TextVertex>& text_verts, float* width) {
	sx *= 2.0f / SCREEN_WIDTH;
	sy *= 2.0f / SCREEN_HEIGHT;

	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	const char *p;
	float text_width = 0;
	for (p = text; *p; p++) {
		text_width += glyphs_[*p].advance_x;
	}
	*width = text_width * sx;
	x -= *width / 2;

	float su = 1.0f / tex_width_;
	float sv = 1.0f / tex_height_;

	// Every character takes 6 vertices
	text_verts.reserve(6 * strlen(text));
	for (p = text; *p; p++) {
		GlyphPos glyph = glyphs_[*p];

		float x2 = x + glyph.left_bear * sx;
		float y2 = -y - glyph.top_bear * sy;
		float w = glyph.width * sx;
		float h = glyph.height * sy;

		float u = glyph.left * su;
		float v = glyph.top * sv;
		float du = glyph.width * su;
		float dv = glyph.height * sv;

		TextVertex box[4] = {
			TextVertex{{x2, -y2}, {u, v}},
			TextVertex{{x2 + w, -y2}, {u + du, v}},
			TextVertex{{x2, -y2 - h}, {u, v + dv}},
			TextVertex{{x2 + w, -y2 - h}, {u + du, v + dv}},
		};
		for (int i : {0, 1, 2, 2, 1, 3}) {
			text_verts.push_back(box[i]);
		}
		x += glyph.advance_x * sx;
		y += glyph.advance_y * sy;
	}
}
