#include "font_declarations.hpp"
#include "../freetype_helpers.hpp"
#include "font_info.hpp"
#include "patch_material_io.hpp"

#include <future>

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace std::literals;

namespace sp::game_support {

namespace {

struct Glyph_metrics {
   int bearing_x;
   int bearing_y;
   unsigned int advance;
   unsigned int width;
   unsigned int height;
   unsigned int pitch;
};

struct Font_metrics {
   unsigned font_size;
   unsigned font_height;
   std::array<Glyph_metrics, glyph_count> glyphs;
};

auto load_font_metrics(FT_Face face, const FT_UInt font_size) -> Font_metrics
{
   freetype_checked_call(FT_Set_Pixel_Sizes(face, 0, font_size));

   std::array<Glyph_metrics, glyph_count> glyphs;

   for (std::size_t i = 0; i < glyph_count; ++i) {
      auto glyph_index = FT_Get_Char_Index(face, game_glyphs[i]);

      freetype_checked_call(FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP));
      freetype_checked_call(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL));

      glyphs[i].bearing_x = face->glyph->metrics.horiBearingX >> 6;
      glyphs[i].bearing_y = face->glyph->metrics.horiBearingY >> 6;
      glyphs[i].advance = face->glyph->metrics.horiAdvance >> 6;
      glyphs[i].width = face->glyph->bitmap.width;
      glyphs[i].height = face->glyph->bitmap.rows;
   }

   const unsigned int font_height = face->size->metrics.height >> 6;

   return {.font_size = font_size, .font_height = font_height, .glyphs = glyphs};
}

auto make_font_declaration(std::string name, std::string_view atlas_index,
                           std::string_view atlas_texture,
                           const Font_metrics& metrics) -> ucfb::Editor_data_chunk
{
   ucfb::Editor_data_chunk font_data;
   font_data.reserve(5864);

   {
      ucfb::Memory_writer font{ucfb::writer_headerless, font_data};

      font.emplace_child("NAME"_mn).write(name);

      {
         auto head = font.emplace_child("HEAD"_mn);

         head.write_unaligned(static_cast<std::uint16_t>(glyph_count));
         head.write_unaligned(static_cast<std::uint8_t>(1)); // atlas count
         head.write_unaligned(static_cast<std::uint8_t>(metrics.font_height));
         head.write_unaligned(static_cast<std::int8_t>(0));
         head.write_unaligned(static_cast<std::int8_t>(0));
      }

      {
         auto ftex = font.emplace_child("FTEX"_mn);

         ftex.emplace_child("NAME"_mn).write(name + "_tex0"s);

         // The way the patch material writing API was designed has really come back to haunt me...
         std::ostringstream tex_ostream;

         {
            ucfb::File_writer tex{ucfb::writer_headerless, tex_ostream};

            write_patch_material(tex,
                                 {
                                    .name = name + "_tex0"s,
                                    .type = "text"s,
                                    .overridden_rendertype = Rendertype::_interface,
                                    .resources = {{"atlas_index"s, std::string{atlas_index}},
                                                  {"atlas"s, std::string{atlas_texture}}},
                                 });
         }

         // Like, really, really come back to haunt me...
         auto tex_contents = tex_ostream.str();

         ftex.write(std::span{tex_contents}); // Seriously...
      }

      {
         auto fbod = font.emplace_child("FBOD"_mn);

         int max_bearing_y = std::numeric_limits<int>::min();

         for (auto& glyph : metrics.glyphs) {
            max_bearing_y = std::max(max_bearing_y, glyph.bearing_y);
         }

         const int y_offset = (metrics.font_height - metrics.font_size) / 2;

         for (auto i = 0; i < glyph_count; ++i) {
            auto& glyph = metrics.glyphs[i];

            const int left = std::max(glyph.bearing_x, 0);
            const int right = left + glyph.width;
            const int top = std::max(max_bearing_y - glyph.bearing_y, 0) + y_offset;
            const int bottom = top + glyph.height;

            fbod.write_unaligned(game_glyphs[i]);
            fbod.write_unaligned(static_cast<std::uint8_t>(0));
            fbod.write_unaligned(static_cast<std::uint8_t>(glyph.advance));
            fbod.write_unaligned(static_cast<std::uint8_t>(left));
            fbod.write_unaligned(static_cast<std::uint8_t>(right));
            fbod.write_unaligned(static_cast<std::uint8_t>(top));
            fbod.write_unaligned(static_cast<std::uint8_t>(bottom));
            fbod.write_unaligned(static_cast<float>((i << 2) | 0)); // left
            fbod.write_unaligned(static_cast<float>((i << 2) | 1)); // right
            fbod.write_unaligned(static_cast<float>((i << 2) | 2)); // top
            fbod.write_unaligned(static_cast<float>((i << 2) | 3)); // bottom
         }
      }
   }

   return font_data;
}

}

auto create_font_declarations(const std::filesystem::path& font_path) -> Font_declarations
{
   const static Font_declarations font_declarations = [&] {
      auto freetype_library = make_freetype_library();

      const std::vector<FT_Byte> font_data = load_font_data(font_path);

      std::array freetype_faces{make_freetype_face(freetype_library, font_data),
                                make_freetype_face(freetype_library, font_data),
                                make_freetype_face(freetype_library, font_data),
                                make_freetype_face(freetype_library, font_data)};

      const auto make_metrics_async = [&](const std::size_t i) {
         return std::async(std::launch::async, [i, &freetype_faces] {
            return load_font_metrics(freetype_faces[i], atlas_font_sizes[i]);
         });
      };

      std::array font_metrics_futures{make_metrics_async(large_index),
                                      make_metrics_async(medium_index),
                                      make_metrics_async(small_index),
                                      make_metrics_async(tiny_index)};

      std::array font_metrics{font_metrics_futures[large_index].get(),
                              font_metrics_futures[medium_index].get(),
                              font_metrics_futures[small_index].get(),
                              font_metrics_futures[tiny_index].get()};

      const auto make_declaration_from_index = [&](std::string_view name,
                                                   const std::size_t i) {
         return std::pair{name,
                          make_font_declaration(std::string{name}, atlas_index_names[i],
                                                atlas_names[i], font_metrics[i])};
      };

      Font_declarations declarations{
         make_declaration_from_index("gamefont_large"sv, large_index),
         make_declaration_from_index("gamefont_medium"sv, medium_index),
         make_declaration_from_index("gamefont_small"sv, small_index),
         make_declaration_from_index("gamefont_tiny"sv, tiny_index),
         make_declaration_from_index("gamefont_super_tiny"sv, tiny_index)};

      return declarations;
   }();

   return font_declarations;
}

}
