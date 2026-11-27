/*

    Help render a bunch of text into a texture.
    Output a 2D image (texture) that will be used by later stuff

    Much of the code is directly taken from the following online examples:
    - https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
    - https://freetype.org/freetype2/docs/tutorial/example1.c

*/

#include "TextRenderer.hpp"

#include <algorithm>
#include <math.h>
#include <hb.h>
#include <hb-ft.h>

TextRenderer::TextRenderer(std::string const text_to_render, std::string const font_path_name, int fs)
: text(text_to_render), font_path(font_path_name), font_size(fs) {
    Reset(text_to_render, font_path_name, fs);
}

void TextRenderer::Reset(std::string const text_to_render, std::string const font_path_name, int fs) {
    if (active) {
        hb_buffer_destroy (hb_buffer);
        hb_font_destroy (hb_font);
        FT_Done_Face (ft_face);
        FT_Done_FreeType (ft_library);
    }

    text = text_to_render;
    font_path = font_path_name;
    font_size = fs;

    // fontfile: path to font. It's all C again
    const char *fontfile = font_path.c_str();

    if ((ft_error = FT_Init_FreeType (&ft_library)))
        abort();
    if ((ft_error = FT_New_Face (ft_library, fontfile, 0, &ft_face)))
        abort();
    if ((ft_error = FT_Set_Char_Size (ft_face, font_size*64, font_size*64, 0, 0)))
        abort();
    
    /* Create hb-ft font. */
    hb_font = hb_ft_font_create (ft_face, NULL);

    // Everything below might belong to rasterization realm instead.
    // width and height are return values.
    // The size of the rendered image is dynamic!

    // length is going to be ignored now. due to ligature and stuff

    /* Create hb-buffer and populate. */
    hb_buffer = hb_buffer_create ();
    hb_buffer_add_utf8 (hb_buffer, text.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer);

    /* Shape it! */
    hb_shape (hb_font, hb_buffer, NULL, 0);

    /* Get glyph information and positions out of the buffer. */
    text_length = hb_buffer_get_length(hb_buffer);
    info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
    pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);

    /* Rasterize at absolute positions. */
    
    // positions
    current_x = 0.0f;
    current_y = 0.0f;
    cursor = 0;

    image_width = 0;
    image_height = 0;

    text_left = 0;
    text_top = 0;
    int text_right = 0;
    int text_bottom = 0;
    bool started = false;
    for (size_t idx = 0; idx < text_length; idx++) {
        if (FT_Load_Glyph(ft_face, info[idx].codepoint, FT_LOAD_RENDER) == 0) {
            FT_Bitmap const &bitmap = ft_face->glyph->bitmap;
            if (bitmap.width != 0 && bitmap.rows != 0) {
                float x_position_origin = current_x + static_cast<float>(pos[idx].x_offset) / 64.0f;
                float y_position_origin = current_y + static_cast<float>(pos[idx].y_offset) / 64.0f;

                int x_position = static_cast<int>(x_position_origin) + ft_face->glyph->bitmap_left;
                int y_position = font_size - (static_cast<int>(y_position_origin) + ft_face->glyph->bitmap_top);
                FT_Int x_max = x_position + static_cast<int>(bitmap.width);
                FT_Int y_max = y_position + static_cast<int>(bitmap.rows);

                if (!started) {
                    text_left = x_position;
                    text_top = y_position;
                    text_right = x_max;
                    text_bottom = y_max;
                    started = true;
                } else {
                    text_left = std::min(text_left, x_position);
                    text_top = std::min(text_top, y_position);
                    text_right = std::max(text_right, x_max);
                    text_bottom = std::max(text_bottom, y_max);
                }
            }
        }

        current_x += static_cast<float>(pos[idx].x_advance) / 64.0f;
        current_y += static_cast<float>(pos[idx].y_advance) / 64.0f;
    }
    if (started) {
        image_width = static_cast<size_t>(text_right - text_left);
        image_height = static_cast<size_t>(text_bottom - text_top);
    }

    current_x = 0.0f;
    current_y = 0.0f;
    active = true;
}


void TextRenderer::Rasterize(std::vector<glm::u8vec4> &image, size_t until, size_t &width, size_t &height) {
    

    // freetype stubs
    FT_Bitmap &bitmap = ft_face->glyph->bitmap;
    FT_Error error;
    
    // ensure length is not too large
    until = until > static_cast<size_t>(text_length) ? static_cast<size_t>(text_length) : until;

    if (cursor == 0) {
        image = std::vector<glm::u8vec4>(image_width * image_height);
        std::fill(image.begin(), image.end(), glm::u8vec4{0});
    }

    // make image
    // Finally, Rasterize
    for (; cursor < until; cursor++) {
        // load a glyph (including ligature)
        error = FT_Load_Glyph(ft_face, info[cursor].codepoint, FT_LOAD_RENDER);
        if (error) {
            printf("Error loading Glyph\n");
            current_x += pos[cursor].x_advance / 64.0f;
            current_y += pos[cursor].y_advance / 64.0f;
            continue;
        }
        
        float x_position_origin = current_x + static_cast<float>(pos[cursor].x_offset) / 64.0f;
        float y_position_origin = current_y + static_cast<float>(pos[cursor].y_offset) / 64.0f;

        int x_position = static_cast<int>(x_position_origin) + ft_face->glyph->bitmap_left;
        // FONT SIZE is the baseline of the font (line below g,q,p...)
        int y_position = font_size - (static_cast<int>(y_position_origin) + ft_face->glyph->bitmap_top);

        // Move the measured bitmap bounds to the texture's top-left corner.
        x_position -= text_left;
        y_position -= text_top;

        FT_Int i, j, p, q;
        FT_Int x_max = x_position + static_cast<int>(bitmap.width);
        FT_Int y_max = y_position + static_cast<int>(bitmap.rows);

        /* for simplicity, we assume that `bitmap->pixel_mode' */
        /* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

        for (i = x_position, p = 0; i < x_max; i++, p++)
        {
            for (j = y_position, q = 0; j < y_max; j++, q++)
            {
                if (i < 0 || j < 0 ||
                    i >= static_cast<int>(image_width) || j >= static_cast<int>(image_height))
                    continue;
                // blit
                uint8_t alpha = bitmap.buffer[q * bitmap.pitch + p];
                image[(image_height - j - 1) * image_width + i] = glm::vec4(0xff, 0xff, 0xff, alpha);
            }
        }
        // Advance!
        current_x += pos[cursor].x_advance / 64.0f;
        current_y += pos[cursor].y_advance / 64.0f;
    }

    // Set return values.
    width = image_width;
    height = image_height;

}

TextRenderer::~TextRenderer() {
    hb_buffer_destroy (hb_buffer);
    hb_font_destroy (hb_font);

    FT_Done_Face (ft_face);
    FT_Done_FreeType (ft_library);
}

TextBuffer::TextBuffer(std::string font_pathname): renderer("placeholder", font_pathname), font_name(font_pathname) {
    // Call Rasterize to initialize
    renderer.Rasterize(buffer, 0, width, height);
}

std::vector<glm::u8vec4>& TextBuffer::draw(std::string text_to_render, int fs, bool do_cache) {
    text = text_to_render;
    font_size = fs;
    if (auto iter = cache.find(std::pair<std::string, int>(text_to_render, fs)); iter != cache.end()) {
        width = iter->second.width;
        height = iter->second.height;
        return iter->second.buffer;
    }

    renderer.Reset(text, font_name, font_size);
    renderer.Rasterize(buffer, renderer.text_length, width, height);
    if (do_cache) {
        cache[std::pair<std::string, int>(text_to_render, fs)] = CachedText{buffer, width, height};
    }
    return buffer;
}
