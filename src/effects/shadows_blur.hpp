#pragma once

#include "../shader_database.hpp"
#include "com_ptr.hpp"
#include "com_ref.hpp"
#include "rendertarget_allocator.hpp"

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp::effects {

#include <d3d9.h>

struct Shadows_blur_params {
   bool enabled = false;

   float radius_scale = 8.0f;
};

class Shadows_blur {
public:
   Shadows_blur(Com_ref<IDirect3DDevice9> device);

   void params(Shadows_blur_params params) noexcept
   {
      _params = params;
   }

   auto params() const noexcept
   {
      return _params;
   }

   void apply(const Shader_group& shaders, Rendertarget_allocator& allocator,
              IDirect3DTexture9& depth, IDirect3DTexture9& from_to) noexcept;

   void drop_device_resources() noexcept;

private:
   void do_pass(IDirect3DSurface9& dest, IDirect3DTexture9& source) const noexcept;

   void set_ps_constants(glm::ivec2 resolution) const noexcept;

   void setup_samplers() const noexcept;

   void setup_vetrex_stream() noexcept;

   void create_resources() noexcept;

   Com_ref<IDirect3DDevice9> _device;
   Com_ptr<IDirect3DVertexDeclaration9> _vertex_decl;
   Com_ptr<IDirect3DVertexBuffer9> _vertex_buffer;

   Shadows_blur_params _params{};
};

}

namespace YAML {
template<>
struct convert<sp::effects::Shadows_blur_params> {
   static Node encode(const sp::effects::Shadows_blur_params& config)
   {
      using namespace std::literals;

      YAML::Node node;

      node["Enabled"s] = config.enabled;
      node["RadiusScale"s] = config.radius_scale;

      return node;
   }

   static bool decode(const Node& node, sp::effects::Shadows_blur_params& config)
   {
      using namespace std::literals;

      config = sp::effects::Shadows_blur_params{};

      config.enabled = node["Enabled"s].as<bool>(config.enabled);
      config.radius_scale = node["RadiusScale"s].as<float>(config.radius_scale);

      return true;
   }
};
}
