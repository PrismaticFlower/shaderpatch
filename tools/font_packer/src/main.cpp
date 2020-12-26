﻿
#include "patch_material_io.hpp"
#include "synced_io.hpp"
#include "ucfb_writer.hpp"
#include "utility.hpp"

#include <array>
#include <filesystem>
#include <limits>
#include <string>

#include <clara.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace sp;
using namespace std::literals;

struct Glyph {
   char16_t codepoint;
   int bearing_x;
   int bearing_y;
   unsigned int advance;
   unsigned int width;
   unsigned int height;
   unsigned int pitch;
   std::vector<std::uint8_t> bitmap;
};

constexpr std::array exported_glyphs{
   u'\u001e', u'\u001f', u'\u0020', u'\u0021', u'\u0022', u'\u0023', u'\u0024',
   u'\u0025', u'\u0026', u'\u0027', u'\u0028', u'\u0029', u'\u002a', u'\u002b',
   u'\u002c', u'\u002d', u'\u002e', u'\u002f', u'\u0030', u'\u0031', u'\u0032',
   u'\u0033', u'\u0034', u'\u0035', u'\u0036', u'\u0037', u'\u0038', u'\u0039',
   u'\u003a', u'\u003b', u'\u003c', u'\u003d', u'\u003e', u'\u003f', u'\u0040',
   u'\u0041', u'\u0042', u'\u0043', u'\u0044', u'\u0045', u'\u0046', u'\u0047',
   u'\u0048', u'\u0049', u'\u004a', u'\u004b', u'\u004c', u'\u004d', u'\u004e',
   u'\u004f', u'\u0050', u'\u0051', u'\u0052', u'\u0053', u'\u0054', u'\u0055',
   u'\u0056', u'\u0057', u'\u0058', u'\u0059', u'\u005a', u'\u005b', u'\u005c',
   u'\u005d', u'\u005e', u'\u005f', u'\u0060', u'\u0061', u'\u0062', u'\u0063',
   u'\u0064', u'\u0065', u'\u0066', u'\u0067', u'\u0068', u'\u0069', u'\u006a',
   u'\u006b', u'\u006c', u'\u006d', u'\u006e', u'\u006f', u'\u0070', u'\u0071',
   u'\u0072', u'\u0073', u'\u0074', u'\u0075', u'\u0076', u'\u0077', u'\u0078',
   u'\u0079', u'\u007a', u'\u007b', u'\u007c', u'\u007d', u'\u007e', u'\u007f',
   u'\u0080', u'\u0081', u'\u0082', u'\u0083', u'\u0084', u'\u0085', u'\u0086',
   u'\u0087', u'\u0088', u'\u0089', u'\u008a', u'\u008b', u'\u008c', u'\u008d',
   u'\u008e', u'\u008f', u'\u0090', u'\u0091', u'\u0092', u'\u0093', u'\u0094',
   u'\u0095', u'\u0096', u'\u0097', u'\u0098', u'\u0099', u'\u009a', u'\u009b',
   u'\u009c', u'\u009d', u'\u009e', u'\u009f', u'\u00a0', u'\u00a1', u'\u00a2',
   u'\u00a3', u'\u00a4', u'\u00a5', u'\u00a6', u'\u00a7', u'\u00a8', u'\u00a9',
   u'\u00aa', u'\u00ab', u'\u00ac', u'\u00ad', u'\u00ae', u'\u00af', u'\u00b0',
   u'\u00b1', u'\u00b2', u'\u00b3', u'\u00b4', u'\u00b5', u'\u00b6', u'\u00b7',
   u'\u00b8', u'\u00b9', u'\u00ba', u'\u00bb', u'\u00bc', u'\u00bd', u'\u00be',
   u'\u00bf', u'\u00c0', u'\u00c1', u'\u00c2', u'\u00c3', u'\u00c4', u'\u00c5',
   u'\u00c6', u'\u00c7', u'\u00c8', u'\u00c9', u'\u00ca', u'\u00cb', u'\u00cc',
   u'\u00cd', u'\u00ce', u'\u00cf', u'\u00d0', u'\u00d1', u'\u00d2', u'\u00d3',
   u'\u00d4', u'\u00d5', u'\u00d6', u'\u00d7', u'\u00d8', u'\u00d9', u'\u00da',
   u'\u00db', u'\u00dc', u'\u00dd', u'\u00de', u'\u00df', u'\u00e0', u'\u00e1',
   u'\u00e2', u'\u00e3', u'\u00e4', u'\u00e5', u'\u00e6', u'\u00e7', u'\u00e8',
   u'\u00e9', u'\u00ea', u'\u00eb', u'\u00ec', u'\u00ed', u'\u00ee', u'\u00ef',
   u'\u00f0', u'\u00f1', u'\u00f2', u'\u00f3', u'\u00f4', u'\u00f5', u'\u00f6',
   u'\u00f7', u'\u00f8', u'\u00f9', u'\u00fa', u'\u00fb', u'\u00fc', u'\u00fd',
   u'\u00fe', u'\u00ff'};

constexpr auto glyph_count = exported_glyphs.size();

struct Packed_font {
   std::uint32_t size = 0;
   int height = 0;
   std::array<Glyph, glyph_count> glyphs;
   std::string atlas_index;
   std::string atlas_texture;
};

void save_shader_patch_font(const std::string& output_dir,
                            const std::string& name, const Packed_font& packed);

int main(int arg_count, char* args[])
{
   using namespace clara;

   bool help = false;
   auto output_dir = "./"s;
   std::string font_path = R"(C:/Windows/Fonts/ariblk.ttf)"s;
   std::string name = "gamefont_large"s;
   std::string atlas_index = "_SP_ATLAS_INDEX_large"s;
   std::string atlas_texture = "_SP_ATLAS_large"s;
   FT_UInt font_size = 14;

   // clang-format off

   auto cli = Help{help}
      | Opt{output_dir, "output directory"s}
      ["--outputdir"s]
      ("Path to place resulting .lvl files in."s)
      | Opt{font_size, "font size"s}
      ["--size"s]
      ("Font height."s)
      | Opt{font_path, "font path"s}
      ["--fontpath"s]
      ("Path to the input font file."s)
      | Opt{name, "name"s}
      ["--name"s]
      ("Name of the output font i.e gamefont_large"s)
      | Opt{atlas_index, "atlas index"s}
      ["--atlasindex"s]
      ("Name of the atlas index shader resource."s)
      | Opt{atlas_texture, "atlas texture"s}
      ["--atlastexture"s]
      ("Name of the atlas texture shader resource."s);

   // clang-format on

   const auto result = cli.parse(Args(arg_count, args));

   if (!result) {
      synced_error_print("Commandline Error: "sv, result.errorMessage());

      return 1;
   }
   else if (help) {
      synced_print(cli);

      return 0;
   }

   if (!std::filesystem::exists(output_dir)) {
      std::filesystem::create_directories(output_dir);
   }

   FT_Library library;

   auto error = FT_Init_FreeType(&library);

   if (error) throw std::runtime_error{"FreeType Error"s};

   FT_Face face;

   error = FT_New_Face(library, font_path.c_str(), 0, &face);

   if (error) throw std::runtime_error{"FreeType Error"s};

   error = FT_Set_Pixel_Sizes(face, 0, font_size);

   if (error) throw std::runtime_error{"FreeType Error"s};

   std::array<Glyph, glyph_count> rendered_glyphs;

   for (std::size_t i = 0; i < glyph_count; ++i) {
      auto glyph_index = FT_Get_Char_Index(face, exported_glyphs[i]);

      error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP);

      if (error) throw std::runtime_error{"FreeType Error"s};

      error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

      if (error) throw std::runtime_error{"FreeType Error"s};

      rendered_glyphs[i].codepoint = exported_glyphs[i];
      rendered_glyphs[i].bearing_x = face->glyph->metrics.horiBearingX >> 6;
      rendered_glyphs[i].bearing_y = face->glyph->metrics.horiBearingY >> 6;
      rendered_glyphs[i].advance = face->glyph->metrics.horiAdvance >> 6;
      rendered_glyphs[i].width = face->glyph->bitmap.width;
      rendered_glyphs[i].height = face->glyph->bitmap.rows;
      rendered_glyphs[i].pitch = face->glyph->bitmap.pitch;
   }

   save_shader_patch_font(output_dir, name,
                          {.size = font_size,
                           .height = face->size->metrics.height >> 6,
                           .glyphs = rendered_glyphs,
                           .atlas_index = atlas_index,
                           .atlas_texture = atlas_texture});
}

void save_shader_patch_font(const std::string& output_dir,
                            const std::string& name, const Packed_font& packed)
{
   auto file = ucfb::open_file_for_output(
      (std::filesystem::path{output_dir} /= name) += L".font"s);

   ucfb::File_writer out{"ucfb"_mn, file};

   {
      auto font = out.emplace_child("font"_mn);

      font.emplace_child("NAME"_mn).write(name);

      {
         auto head = font.emplace_child("HEAD"_mn);

         head.write_unaligned(static_cast<std::uint16_t>(glyph_count));
         head.write_unaligned(static_cast<std::uint8_t>(1)); // atlas count
         head.write_unaligned(static_cast<std::uint8_t>(packed.height));
         head.write_unaligned(static_cast<std::int8_t>(0));
         head.write_unaligned(static_cast<std::int8_t>(0));
      }

      {
         auto ftex = font.emplace_child("FTEX"_mn);

         ftex.emplace_child("NAME"_mn).write(name + "_tex0"s);

         write_patch_material(ftex, {.name = name + "_tex0"s,
                                     .rendertype = "text"s,
                                     .overridden_rendertype = Rendertype::_interface,
                                     .vs_resources = {packed.atlas_index},
                                     .ps_resources = {packed.atlas_texture}});
      }

      {
         auto fbod = font.emplace_child("FBOD"_mn);

         int max_bearing_y = std::numeric_limits<int>::min();

         for (auto& glyph : packed.glyphs) {
            max_bearing_y = std::max(max_bearing_y, glyph.bearing_y);
         }

         int y_offset = (packed.height - packed.size) / 2;

         for (auto i = 0; i < packed.glyphs.size(); ++i) {
            auto& glyph = packed.glyphs[i];

            int left = std::max(glyph.bearing_x, 0);
            int right = left + glyph.width;
            int top = std::max(max_bearing_y - glyph.bearing_y, 0) + y_offset;
            int bottom = top + glyph.height;

            fbod.write_unaligned(glyph.codepoint);
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
}
