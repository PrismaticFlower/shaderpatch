#include "draw_resources.hpp"

#include "draw_view_constants.hpp"

#include "../../shader/database.hpp"

namespace sp::shadows {

Draw_resources::Draw_resources(ID3D11Device& device, shader::Database& shaders)
{
   const D3D11_BUFFER_DESC view_constants_desc = {
      .ByteWidth = sizeof(Draw_view_gpu_constants),
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
   };

   if (FAILED(device.CreateBuffer(&view_constants_desc, nullptr,
                                  view_constants.clear_and_assign()))) {
      log_and_terminate("Failed to create shadow world view constants buffer.");
   }

   const std::array<D3D11_INPUT_ELEMENT_DESC, 6> ia_layout = {
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "POSITION_X",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 0,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "POSITION_Y",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 2,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "POSITION_Z",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 4,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "TRANSFORM",
         .SemanticIndex = 0,
         .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
         .InputSlot = 1,
         .AlignedByteOffset = 0,
         .InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA,
         .InstanceDataStepRate = 1,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "TRANSFORM",
         .SemanticIndex = 1,
         .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
         .InputSlot = 1,
         .AlignedByteOffset = 16,
         .InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA,
         .InstanceDataStepRate = 1,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "TRANSFORM",
         .SemanticIndex = 2,
         .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
         .InputSlot = 1,
         .AlignedByteOffset = 32,
         .InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA,
         .InstanceDataStepRate = 1,
      },
   };

   const std::array<D3D11_INPUT_ELEMENT_DESC, 8> ia_textured_layout = {
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "POSITION_X",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 0,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "POSITION_Y",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 2,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "POSITION_Z",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 4,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "TEXCOORD_X",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 6,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "TEXCOORD_Y",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 8,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "TRANSFORM",
         .SemanticIndex = 0,
         .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
         .InputSlot = 1,
         .AlignedByteOffset = 0,
         .InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA,
         .InstanceDataStepRate = 1,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "TRANSFORM",
         .SemanticIndex = 1,
         .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
         .InputSlot = 1,
         .AlignedByteOffset = 16,
         .InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA,
         .InstanceDataStepRate = 1,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "TRANSFORM",
         .SemanticIndex = 2,
         .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
         .InputSlot = 1,
         .AlignedByteOffset = 32,
         .InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA,
         .InstanceDataStepRate = 1,
      },
   };

   auto [opaque_shader, opaque_bytecode, opaque_ignore] =
      shaders.vertex("shadowmesh").entrypoint("opaque_vs");

   auto [hardedged_shader, hardedged_bytecode, hardedged_ignore] =
      shaders.vertex("shadowmesh").entrypoint("hardedged_vs");

   if (FAILED(device.CreateInputLayout(ia_layout.data(), ia_layout.size(),
                                       opaque_bytecode.data(), opaque_bytecode.size(),
                                       input_layout.clear_and_assign()))) {
      log_and_terminate("Failed to create shadow world input layout.");
   }

   if (FAILED(device.CreateInputLayout(ia_textured_layout.data(),
                                       ia_textured_layout.size(),
                                       hardedged_bytecode.data(),
                                       hardedged_bytecode.size(),
                                       input_layout_textured.clear_and_assign()))) {
      log_and_terminate("Failed to create shadow world input layout.");
   }

   vertex_shader = opaque_shader;
   vertex_shader_textured = hardedged_shader;

   const D3D11_RASTERIZER_DESC rasterizer_desc{.FillMode = D3D11_FILL_SOLID,
                                               .CullMode = D3D11_CULL_BACK,
                                               .FrontCounterClockwise = true};

   if (FAILED(device.CreateRasterizerState(&rasterizer_desc,
                                           rasterizer_state.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow world rasterizer state!");
   }

   const D3D11_RASTERIZER_DESC rasterizer_doublesided_desc{.FillMode = D3D11_FILL_SOLID,
                                                           .CullMode = D3D11_CULL_NONE,
                                                           .FrontCounterClockwise = true};

   if (FAILED(device.CreateRasterizerState(&rasterizer_doublesided_desc,
                                           rasterizer_state_doublesided.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow world rasterizer state!");
   }

   pixel_shader_hardedged = shaders.pixel("shadowmesh").entrypoint("hardedged_ps");

   const D3D11_SAMPLER_DESC sampler_desc{.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
                                         .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
                                         .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
                                         .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
                                         .MipLODBias = 0.0f,
                                         .MaxAnisotropy = 1,
                                         .ComparisonFunc = D3D11_COMPARISON_ALWAYS,
                                         .BorderColor = {0.0f, 0.0f, 0.0f, 0.0f},
                                         .MinLOD = 0.0f,
                                         .MaxLOD = D3D11_FLOAT32_MAX};

   if (FAILED(device.CreateSamplerState(&sampler_desc,
                                        sampler_state.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow world sampler state!");
   }
}

void Draw_resources::update(ID3D11Device& device, INT depth_bias,
                            float depth_bias_clamp, float slope_scaled_depth_bias) noexcept
{
   if (depth_bias != _depth_bias || depth_bias_clamp != _depth_bias_clamp ||
       slope_scaled_depth_bias != _slope_scaled_depth_bias) {
      _depth_bias_clamp = depth_bias_clamp;
      _slope_scaled_depth_bias = slope_scaled_depth_bias;

      D3D11_RASTERIZER_DESC rasterizer_desc{
         .FillMode = D3D11_FILL_SOLID,
         .CullMode = D3D11_CULL_BACK,
         .FrontCounterClockwise = true,
         .DepthBias = _depth_bias,
         .DepthBiasClamp = _depth_bias_clamp,
         .SlopeScaledDepthBias = _slope_scaled_depth_bias,
      };

      if (FAILED(device.CreateRasterizerState(&rasterizer_desc,
                                              rasterizer_state.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow world rasterizer state!");
      }

      rasterizer_desc.CullMode = D3D11_CULL_NONE;

      if (FAILED(device.CreateRasterizerState(&rasterizer_desc,
                                              rasterizer_state_doublesided.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow world rasterizer state!");
      }
   }
}

}