#pragma once

#include "../starter/GL.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <list>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <map>

#include <hb.h>
#include <hb-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

constexpr int FONT_SIZE = 72;
constexpr int MARGIN = static_cast<int>(FONT_SIZE * 0.5f);

// The Text Renderer class; has a method that can render text.
struct TextRenderer {
    TextRenderer() = delete;
    TextRenderer(std::string const text_to_render, std::string const font_path_name, int font_size_ = FONT_SIZE);
    TextRenderer & operator=(const TextRenderer &other) = delete;
    TextRenderer(const TextRenderer &other) = delete;
    virtual ~TextRenderer();

    void Reset(std::string const text_to_render, std::string const font_path_name, int font_size_);
    void Rasterize(std::vector<glm::u8vec4> &image, size_t length, size_t &width, size_t &height);
    

    // Custom Data
    std::string text = "";
    std::string font_path = "";

    uint32_t text_length = 0;
    size_t image_width = 0;
    size_t image_height = 0;
    int font_size = 72;
    size_t cursor = 0;
    bool active = false;

private:
    /* FreeType stuff ---------- */
    FT_Library ft_library;
    FT_Face ft_face;
    FT_Error ft_error;

    /* Harfbuzz ---------------- */

    // The HB font
    hb_font_t *hb_font;

    // The HB stubs
    hb_buffer_t *hb_buffer;
    hb_glyph_info_t *info;
    hb_glyph_position_t *pos;

    // Formatting related
    float current_x = 0.0f;
    float current_y = 0.0f;
    int text_left = 0;
    int text_top = 0;
};

struct TextBuffer {
    TextRenderer renderer;
    std::vector<glm::u8vec4> buffer;
    std::string font_name;
    std::string text;
    size_t width = 0;
    size_t height = 0;
    int font_size = FONT_SIZE;

    struct CachedText {
        std::vector<glm::u8vec4> buffer;
        size_t width = 0;
        size_t height = 0;
    };
    std::map<std::pair<std::string, int>, CachedText> cache;

    TextBuffer() = delete;
    TextBuffer(std::string font_pathname);

    // methods
    std::vector<glm::u8vec4>& draw(std::string text_to_render, int fs = FONT_SIZE, bool do_cache = true);
};
