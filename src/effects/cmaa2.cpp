
#include "cmaa2.hpp"
#include "enum_flags.hpp"

#include <array>

#include <gsl/gsl>

namespace sp::effects {

using namespace std::literals;

namespace {
constexpr auto cmaa_pack_single_sample_edge_to_half_width = true;
const glm::uvec2 cmaa2_cs_input_kernel_size{16, 16};

enum class cmaa2_flags {
   none = 0b0,
   edge_detection_luma_separate_texture = 0b1
};

constexpr bool marked_as_enum_flag(cmaa2_flags) noexcept
{
   return true;
}

auto create_texture_uav(ID3D11Device1& device, const DXGI_FORMAT format,
                        const UINT width, const UINT height) noexcept
   -> Com_ptr<ID3D11UnorderedAccessView>
{
   const CD3D11_TEXTURE2D_DESC tex_desc{format, width,
                                        height, 1,
                                        0,      D3D11_BIND_UNORDERED_ACCESS};

   Com_ptr<ID3D11Texture2D> texture;
   Com_ptr<ID3D11UnorderedAccessView> uav;

   device.CreateTexture2D(&tex_desc, nullptr, texture.clear_and_assign());
   device.CreateUnorderedAccessView(texture.get(), nullptr, uav.clear_and_assign());

   return uav;
}

auto create_structured_buffer_uav(ID3D11Device1& device, const UINT size,
                                  const UINT stride) noexcept
   -> Com_ptr<ID3D11UnorderedAccessView>
{
   const CD3D11_BUFFER_DESC buffer_desc{size,
                                        D3D11_BIND_UNORDERED_ACCESS,
                                        D3D11_USAGE_DEFAULT,
                                        0,
                                        D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
                                        stride};

   Com_ptr<ID3D11Buffer> buffer;
   device.CreateBuffer(&buffer_desc, nullptr, buffer.clear_and_assign());

   Com_ptr<ID3D11UnorderedAccessView> uav;
   const CD3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{D3D11_UAV_DIMENSION_BUFFER,
                                                    DXGI_FORMAT_UNKNOWN, 0,
                                                    size / stride};
   device.CreateUnorderedAccessView(buffer.get(), &uav_desc, uav.clear_and_assign());

   return uav;
}
}

CMAA2::CMAA2(Com_ptr<ID3D11Device1> device, shader::Group_compute& shaders) noexcept
   : _edges_color2x2{shaders.entrypoint("EdgesColor2x2CS"sv, cmaa2_flags::none)},
     _compute_dispatch_args{
        shaders.entrypoint("ComputeDispatchArgsCS"sv, cmaa2_flags::none)},
     _process_candidates{shaders.entrypoint("ProcessCandidatesCS"sv, cmaa2_flags::none)},
     _deferred_color_apply2x2{
        shaders.entrypoint("DeferredColorApply2x2CS"sv, cmaa2_flags::none)},
     _edges_color2x2_luma_separate{
        shaders.entrypoint("EdgesColor2x2CS"sv,
                           cmaa2_flags::edge_detection_luma_separate_texture)},
     _compute_dispatch_args_luma_separate{
        shaders.entrypoint("ComputeDispatchArgsCS"sv,
                           cmaa2_flags::edge_detection_luma_separate_texture)},
     _process_candidates_luma_separate{

        shaders.entrypoint("ProcessCandidatesCS"sv,
                           cmaa2_flags::edge_detection_luma_separate_texture)},
     _deferred_color_apply2x2_luma_separate{
        shaders.entrypoint("DeferredColorApply2x2CS"sv,
                           cmaa2_flags::edge_detection_luma_separate_texture)},
     _point_sampler{[&] {
        CD3D11_SAMPLER_DESC desc{CD3D11_DEFAULT{}};

        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

        Com_ptr<ID3D11SamplerState> sampler;
        device->CreateSamplerState(&desc, sampler.clear_and_assign());

        return sampler;
     }()},
     _device{device}
{
}

void CMAA2::apply(ID3D11DeviceContext1& dc, Profiler& profiler,
                  CMAA2_input_output input_output) noexcept
{
   dc.ClearState();

   if (std::exchange(_size, glm::uvec2{input_output.width, input_output.height}) !=
       glm::uvec2{input_output.width, input_output.height}) {
      update_resources();
   }

   execute(dc, profiler, input_output);
}

void CMAA2::clear_resources() noexcept
{
   _size = {0, 0};

   _working_edges_uav = nullptr;
   _working_shape_candidates_uav = nullptr;
   _working_deferred_blend_location_list_uav = nullptr;
   _working_deferred_blend_item_list_uav = nullptr;
   _working_deferred_blend_item_list_heads_uav = nullptr;

   _working_control_buffer = nullptr;
   _working_control_uav = nullptr;
}

void CMAA2::execute(ID3D11DeviceContext1& dc, Profiler& profiler,
                    CMAA2_input_output input_output) noexcept
{
   Profile full_profile{profiler, dc, "CMAA2"sv};

   const bool luma_input = input_output.luma_srv != nullptr;

   ID3D11ComputeShader& edges_color2x2 =
      luma_input ? *_edges_color2x2_luma_separate : *_edges_color2x2;
   ID3D11ComputeShader& compute_dispatch_args =
      luma_input ? *_compute_dispatch_args_luma_separate : *_compute_dispatch_args;
   ID3D11ComputeShader& process_candidates =
      luma_input ? *_process_candidates_luma_separate : *_process_candidates;
   ID3D11ComputeShader& deferred_color_apply2x2 =
      luma_input ? *_deferred_color_apply2x2_luma_separate : *_deferred_color_apply2x2;

   auto* const sampler = _point_sampler.get();
   dc.CSSetSamplers(0, 1, &sampler);

   std::array<ID3D11UnorderedAccessView*, 7>
      uavs{nullptr,
           _working_edges_uav.get(),
           _working_shape_candidates_uav.get(),
           _working_deferred_blend_location_list_uav.get(),
           _working_deferred_blend_item_list_uav.get(),
           _working_deferred_blend_item_list_heads_uav.get(),
           _working_control_uav.get()};

   std::array<ID3D11ShaderResourceView*, 4> srvs{&input_output.input, nullptr,
                                                 nullptr, input_output.luma_srv};

   dc.CSSetUnorderedAccessViews(0, uavs.size(), uavs.data(), nullptr);
   dc.CSSetShaderResources(0, srvs.size(), srvs.data());

   {
      Profile profile{profiler, dc, "CMAA2 - Edge Detection"sv};

      const auto output_kernel_size = cmaa2_cs_input_kernel_size - 2u;
      const auto thread_group_count =
         (_size + output_kernel_size * 2u - 1u) / (output_kernel_size * 2u);

      dc.CSSetShader(&edges_color2x2, nullptr, 0);
      dc.Dispatch(thread_group_count.x, thread_group_count.y, 1);
   }

   {
      Profile profile{profiler, dc, "CMAA2 - Dispatch Indirect Args #1"sv};

      dc.CSSetShader(&compute_dispatch_args, nullptr, 0);
      dc.Dispatch(2, 1, 1);
   }

   {
      Profile profile{profiler, dc, "CMAA2 - Process Shape Candidates"sv};

      dc.CSSetShader(&process_candidates, nullptr, 0);
      dc.DispatchIndirect(_working_control_buffer.get(), 0);
   }

   {
      Profile profile{profiler, dc, "CMAA2 - Dispatch Indirect Args #2"sv};

      dc.CSSetShader(&compute_dispatch_args, nullptr, 0);
      dc.Dispatch(1, 2, 1);
   }

   srvs[0] = nullptr;
   uavs[0] = &input_output.output;

   dc.CSSetShaderResources(0, srvs.size(), srvs.data());
   dc.CSSetUnorderedAccessViews(0, uavs.size(), uavs.data(), nullptr);

   {
      Profile profile{profiler, dc, "CMAA2 - Deferred Color Apply"sv};

      dc.CSSetShader(&deferred_color_apply2x2, nullptr, 0);
      dc.DispatchIndirect(_working_control_buffer.get(), 0);
   }

   constexpr std::array<ID3D11UnorderedAccessView*, 7> null_uavs{};
   constexpr std::array<ID3D11ShaderResourceView*, 4> null_srvs{};
   constexpr ID3D11SamplerState* null_samp = nullptr;

   dc.CSSetUnorderedAccessViews(0, null_uavs.size(), null_uavs.data(), nullptr);
   dc.CSSetShaderResources(0, null_srvs.size(), null_srvs.data());
   dc.CSSetSamplers(0, 1, &null_samp);
   dc.CSSetShader(nullptr, nullptr, 0);
}

void CMAA2::update_resources() noexcept
{
   // Create working buffers
   {
      // Create edges buffer
      {
         const UINT edges_width = cmaa_pack_single_sample_edge_to_half_width
                                     ? ((_size.x + 1) / 2)
                                     : _size.x;
         _working_edges_uav = create_texture_uav(*_device, DXGI_FORMAT_R8_UINT,
                                                 edges_width, _size.y);
      }

      // Create shape candidates buffer
      {
         const UINT buffer_size = _size.x * _size.y / 4 * sizeof(UINT);
         _working_shape_candidates_uav =
            create_structured_buffer_uav(*_device, buffer_size, sizeof(UINT));
      }

      // Create blend location list buffer
      {
         const UINT buffer_size = (_size.x * _size.y + 3) / 6 * sizeof(UINT);
         _working_deferred_blend_location_list_uav =
            create_structured_buffer_uav(*_device, buffer_size, sizeof(UINT));
      }

      // Create blend list buffer
      {
         const UINT buffer_size = (_size.x * _size.y / 2) * sizeof(UINT) * 2;
         _working_deferred_blend_item_list_uav =
            create_structured_buffer_uav(*_device, buffer_size, sizeof(UINT) * 2);
      }

      // Create deffered blend list head buffer
      _working_deferred_blend_item_list_heads_uav =
         create_texture_uav(*_device, DXGI_FORMAT_R32_UINT, (_size.x + 1) / 2,
                            (_size.y + 1) / 2);

      // Create control buffer
      {
         constexpr std::array<UINT, 16> init_data{};
         const CD3D11_BUFFER_DESC buffer_desc{sizeof(init_data),
                                              D3D11_BIND_UNORDERED_ACCESS,
                                              D3D11_USAGE_DEFAULT,
                                              0,
                                              D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS |
                                                 D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS,
                                              sizeof(UINT)};
         const D3D11_SUBRESOURCE_DATA sub_res_data{init_data.data(), 0, 0};

         _device->CreateBuffer(&buffer_desc, &sub_res_data,
                               _working_control_buffer.clear_and_assign());

         D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
         uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
         uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
         uav_desc.Buffer.FirstElement = 0;
         uav_desc.Buffer.NumElements = 16;
         uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

         _device->CreateUnorderedAccessView(_working_control_buffer.get(), &uav_desc,
                                            _working_control_uav.clear_and_assign());
      }
   }
}

}
