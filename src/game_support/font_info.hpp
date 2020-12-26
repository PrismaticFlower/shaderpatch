#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace sp::game_support {

inline constexpr std::array game_glyphs{
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

inline constexpr auto glyph_count = game_glyphs.size();

inline constexpr std::array<std::string_view, 4>
   atlas_index_names{"_SP_ATLAS_INDEX_large", "_SP_ATLAS_INDEX_medium",
                     "_SP_ATLAS_INDEX_small", "_SP_ATLAS_INDEX_tiny"};

inline constexpr std::array<std::string_view, 4> atlas_names{"_SP_ATLAS_large",
                                                             "_SP_ATLAS_medium",
                                                             "_SP_ATLAS_small",
                                                             "_SP_ATLAS_tiny"};

inline constexpr std::array<std::uint32_t, 4> atlas_font_sizes{16u, 14u, 12u, 10u};

inline constexpr std::size_t large_index = 0;
inline constexpr std::size_t medium_index = 1;
inline constexpr std::size_t small_index = 2;
inline constexpr std::size_t tiny_index = 3;

}
