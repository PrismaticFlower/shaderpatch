
#include "factory.hpp"
#include "../logger.hpp"

namespace sp::material {

auto Factory::create_material(const Material_config& config) noexcept -> Material
{
   return material::Material{config, _shader_factory, _shader_resource_database,
                             *_device};
}

}
