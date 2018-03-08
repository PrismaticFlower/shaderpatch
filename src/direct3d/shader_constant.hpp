#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <type_traits>

#include <d3d9.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sp::direct3d {

template<typename Shader_type, std::size_t offset, std::size_t count>
inline void set_bool_constants(IDirect3DDevice9& device,
                               const std::array<bool, count> values) noexcept
{
   static_assert((offset + count) <= 16,
                 "Offset and count overflow bool constant count!");

   std::array<BOOL, count> dx_values;
   std::copy_n(std::cbegin(values), count, std::begin(dx_values));

   if constexpr (std::is_same_v<Shader_type, IDirect3DVertexShader9>) {
      device.SetVertexShaderConstantB(offset, dx_values.data(), dx_values.size());
   }
   else if constexpr (std::is_same_v<Shader_type, IDirect3DPixelShader9>) {
      device.SetPixelShaderConstantB(offset, dx_values.data(), dx_values.size());
   }
   else {
      static_assert(false, "Invalid shader type.");
   }
}

template<std::size_t offset, std::size_t count>
inline void set_vs_bool_constants(IDirect3DDevice9& device,
                                  const std::array<bool, count> values) noexcept
{
   set_bool_constants<IDirect3DVertexShader9, offset, count>(device, values);
}

template<std::size_t offset, std::size_t count>
inline void set_ps_bool_constants(IDirect3DDevice9& device,
                                  const std::array<bool, count> values) noexcept
{
   set_bool_constants<IDirect3DPixelShader9, offset, count>(device, values);
}

template<typename Shader_type, std::size_t offset>
inline void set_bool_constant(IDirect3DDevice9& device, const bool value) noexcept
{
   set_bool_constants<Shader_type, offset, 1>(device, {value});
}

template<std::size_t offset>
inline void set_vs_bool_constant(IDirect3DDevice9& device, const bool value) noexcept
{
   set_bool_constant<IDirect3DVertexShader9, offset>(device, value);
}

template<std::size_t offset>
inline void set_ps_bool_constant(IDirect3DDevice9& device, const bool value) noexcept
{
   set_bool_constant<IDirect3DPixelShader9, offset>(device, value);
}

template<typename Type, typename Shader_type, DWORD offset, DWORD row_count>
class Shader_constant {
public:
   static_assert(
      std::is_same_v<Shader_type, IDirect3DVertexShader9> ||
         std::is_same_v<Shader_type, IDirect3DPixelShader9>,
      "Shader_type must be IDirect3DVertexShader9 or IDirect3DPixelShader9.");

   static_assert(sizeof(Type) <= (sizeof(float) * 4) * row_count,
                 "Row count too small for type.");

   static_assert(std::is_standard_layout_v<Type>,
                 "Type must be standard layout.");

   void reset(IDirect3DDevice9& device) noexcept
   {
      set(device, _value);
   }

   void set(IDirect3DDevice9& device, const Type& value) noexcept
   {
      _value = value;

      std::array<float, row_count * 4> values{};

      std::memcpy(values.data(), &value, sizeof(Type));

      if constexpr (std::is_same_v<Shader_type, IDirect3DVertexShader9>) {
         device.SetVertexShaderConstantF(offset, values.data(), row_count);
      }
      else if constexpr (std::is_same_v<Shader_type, IDirect3DPixelShader9>) {
         device.SetPixelShaderConstantF(offset, values.data(), row_count);
      }
      else {
         static_assert(false, "Invalid shader type.");
      }
   }

   void local_update(const Type& value) noexcept
   {
      _value = value;
   }

   const Type& get() const noexcept
   {
      return _value;
   }

private:
   Type _value{};
};

template<typename Type, DWORD offset, DWORD row_count>
using Vs_shader_constant =
   Shader_constant<Type, IDirect3DVertexShader9, offset, row_count>;

template<DWORD offset>
using Vs_1f_shader_constant = Vs_shader_constant<float, offset, 1>;

template<DWORD offset>
using Vs_2f_shader_constant = Vs_shader_constant<glm::vec2, offset, 1>;

template<DWORD offset>
using Vs_3f_shader_constant = Vs_shader_constant<glm::vec3, offset, 1>;

template<DWORD offset>
using Vs_4f_shader_constant = Vs_shader_constant<glm::vec4, offset, 1>;

template<DWORD offset>
using Vs_Mat4_shader_constant = Vs_shader_constant<glm::mat4, offset, 4>;

template<typename Type, DWORD offset, DWORD row_count>
using Ps_shader_constant =
   Shader_constant<Type, IDirect3DPixelShader9, offset, row_count>;

template<DWORD offset>
using Ps_1f_shader_constant = Ps_shader_constant<float, offset, 1>;

template<DWORD offset>
using Ps_2f_shader_constant = Ps_shader_constant<glm::vec2, offset, 1>;

template<DWORD offset>
using Ps_3f_shader_constant = Ps_shader_constant<glm::vec3, offset, 1>;

template<DWORD offset>
using Ps_4f_shader_constant = Ps_shader_constant<glm::vec4, offset, 1>;

template<DWORD offset>
using Ps_Mat4_shader_constant = Ps_shader_constant<glm::mat4, offset, 4>;
}
