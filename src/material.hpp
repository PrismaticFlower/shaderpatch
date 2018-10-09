#pragma once

#include "com_ref.hpp"
#include "patch_material.hpp"
#include "shader_database.hpp"
#include "texture.hpp"
#include "texture_database.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <gsl/gsl>

#include <d3d9.h>

namespace sp {

class Material {
public:
   Material(const Material_info& info, Com_ref<IDirect3DDevice9> device,
            const Shader_rendertype_collection& rendertypes,
            const Texture_database& textures) noexcept;

   Material(const Material&) = default;
   Material& operator=(const Material&) = default;

   Material(Material&&) = default;
   Material& operator=(Material&&) = default;

   auto overridden_rendertype() const noexcept -> Rendertype;

   auto info() const noexcept -> Material_info;

   void bind() const noexcept;

   void update(const std::string& shader_name, const Vertex_shader_flags vertex_flags,
               const Pixel_shader_flags pixel_flags) const noexcept;

   constexpr static auto material_textures_offset = 4;
   constexpr static auto max_material_textures = 8;
   constexpr static auto max_material_constants = 8;

private:
   const Rendertype _overridden_rendertype;

   Com_ref<IDirect3DDevice9> _device;

   const std::array<Texture, max_material_textures> _textures;
   const std::array<float, max_material_constants * 4> _constants{};

   const Shader_rendertype& _rendertype;
   const Material_info _info;
};
}
