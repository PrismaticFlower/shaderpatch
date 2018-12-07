
#include "shader_patch.hpp"
#include "../logger.hpp"
#include "utility.hpp"

#include <comdef.h>

namespace sp::core {

constexpr auto fixed_width = 800;
constexpr auto fixed_height = 600;

namespace {

constexpr std::array supported_feature_levels{D3D_FEATURE_LEVEL_11_0};

auto create_device(IDXGIAdapter2&) noexcept -> Com_ptr<ID3D11Device1>
{
   Com_ptr<ID3D11Device> device;

   if (const auto result =
          D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                            D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
                            supported_feature_levels.data(),
                            supported_feature_levels.size(), D3D11_SDK_VERSION,
                            device.clear_and_assign(), nullptr, nullptr);
       FAILED(result)) {

      log_and_terminate("Failed to create Direct3D 11 device! Reason: ",
                        _com_error{result}.ErrorMessage());
   }

   Com_ptr<ID3D11Device1> device1;

   if (const auto result = device->QueryInterface(device1.clear_and_assign());
       FAILED(result)) {
      log_and_terminate("Failed to create Direct3D 11.1 device!");
   }

   Com_ptr<ID3D11Debug> debug;
   if (auto result = device->QueryInterface(debug.clear_and_assign());
       SUCCEEDED(result)) {
      Com_ptr<ID3D11InfoQueue> infoqueue;
      if (result = debug->QueryInterface(infoqueue.clear_and_assign());
          SUCCEEDED(result)) {
         infoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
         infoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);

         std::array hide{D3D11_MESSAGE_ID_DEVICE_DRAW_VIEW_DIMENSION_MISMATCH,
                         D3D11_MESSAGE_ID_DEVICE_DRAW_SAMPLER_NOT_SET};

         D3D11_INFO_QUEUE_FILTER filter{};
         filter.DenyList.NumIDs = hide.size();
         filter.DenyList.pIDList = hide.data();
         infoqueue->AddStorageFilterEntries(&filter);
      }
   }

   return device1;
}

auto create_swapchain(ID3D11Device1& device, IDXGIAdapter2& adapter,
                      const HWND window, const UINT width,
                      const UINT height) noexcept -> Com_ptr<IDXGISwapChain1>
{
   Com_ptr<IDXGIFactory2> factory;
   adapter.GetParent(__uuidof(IDXGIFactory2), factory.void_clear_and_assign());

   DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;

   swap_chain_desc.Width = width;
   swap_chain_desc.Height = height;
   swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
   swap_chain_desc.Stereo = false;
   swap_chain_desc.SampleDesc = {1, 0};
   swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
   swap_chain_desc.BufferCount = 2;
   swap_chain_desc.Scaling = DXGI_SCALING_NONE;
   swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
   swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
   swap_chain_desc.Flags = 0;

   Com_ptr<IDXGISwapChain1> swapchain;

   if (const auto result =
          factory->CreateSwapChainForHwnd(&device, window, &swap_chain_desc, nullptr,
                                          nullptr, swapchain.clear_and_assign());
       FAILED(result)) {
      log_and_terminate("Failed to create Direct3D 11 device! Reason: ",
                        _com_error{result}.ErrorMessage());
   }

   factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);

   return swapchain;
}

}

Shader_patch::Shader_patch(IDXGIAdapter2& adapter, const HWND window) noexcept
   : _device{create_device(adapter)},
     _swapchain{create_swapchain(*_device, adapter, window, fixed_width, fixed_height)}
{
   bind_static_resources();
   set_viewport(0, 0, fixed_width, fixed_height);
}

void Shader_patch::reset() noexcept
{
   _device_context->ClearState();
   _game_rendertargets.resize(1);

   bind_static_resources();
   set_viewport(0, 0, fixed_width, fixed_height);
}

void Shader_patch::present() noexcept
{
   if (const auto result = _swapchain->Present(0, 0); FAILED(result)) {
      log_and_terminate("Frame Present call failed! reason: ",
                        _com_error{result}.ErrorMessage());
   }

   if (_current_game_rendertarget == _game_backbuffer_index) {
      _om_targets_dirty = true;
   }
}

auto Shader_patch::get_back_buffer() noexcept -> Game_rendertarget_id
{
   return _game_backbuffer_index;
}

auto Shader_patch::create_rasterizer_state(const D3D11_RASTERIZER_DESC d3d11_rasterizer_desc) noexcept
   -> Com_ptr<ID3D11RasterizerState>
{
   Com_ptr<ID3D11RasterizerState> state;

   _device->CreateRasterizerState(&d3d11_rasterizer_desc, state.clear_and_assign());

   return state;
}

auto Shader_patch::create_depthstencil_state(const D3D11_DEPTH_STENCIL_DESC depthstencil_desc) noexcept
   -> Com_ptr<ID3D11DepthStencilState>
{
   Com_ptr<ID3D11DepthStencilState> state;

   _device->CreateDepthStencilState(&depthstencil_desc, state.clear_and_assign());

   return state;
}

auto Shader_patch::create_blend_state(const D3D11_BLEND_DESC blend_state_desc) noexcept
   -> Com_ptr<ID3D11BlendState>
{
   Com_ptr<ID3D11BlendState> state;

   _device->CreateBlendState(&blend_state_desc, state.clear_and_assign());

   return state;
}

auto Shader_patch::create_query(const D3D11_QUERY_DESC desc) noexcept
   -> Com_ptr<ID3D11Query>
{
   Com_ptr<ID3D11Query> query;

   _device->CreateQuery(&desc, query.clear_and_assign());

   return query;
}

auto Shader_patch::create_game_rendertarget(const UINT width, const UINT height) noexcept
   -> Game_rendertarget_id
{
   const int index = _game_rendertargets.size();
   auto& rt = _game_rendertargets.emplace_back();
   rt.width = static_cast<std::uint16_t>(width);
   rt.height = static_cast<std::uint16_t>(height);

   const auto texture_desc =
      CD3D11_TEXTURE2D_DESC{DXGI_FORMAT_B8G8R8A8_UNORM,
                            width,
                            height,
                            1,
                            1,
                            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET};

   _device->CreateTexture2D(&texture_desc, nullptr, rt.texture.clear_and_assign());
   _device->CreateRenderTargetView(rt.texture.get(), nullptr,
                                   rt.rtv.clear_and_assign());
   _device->CreateShaderResourceView(rt.texture.get(), nullptr,
                                     rt.srv.clear_and_assign());

   return Game_rendertarget_id{index};
}

void Shader_patch::destroy_game_rendertarget(const Game_rendertarget_id id) noexcept
{
   _game_rendertargets[static_cast<int>(id)] = {};

   if (id == _current_game_rendertarget) {
      set_rendertarget(_game_backbuffer_index);
   }
}

auto Shader_patch::create_game_texture2d(const DirectX::ScratchImage& image) noexcept
   -> Game_texture
{
   Expects(image.GetMetadata().mipLevels != 0 &&
           image.GetMetadata().arraySize == 1 && image.GetMetadata().depth == 1);

   const auto& metadata = image.GetMetadata();
   const auto typeless_format = DirectX::MakeTypeless(metadata.format);

   const auto desc =
      CD3D11_TEXTURE2D_DESC{typeless_format,      metadata.width,
                            metadata.height,      1,
                            metadata.mipLevels,   D3D11_BIND_SHADER_RESOURCE,
                            D3D11_USAGE_IMMUTABLE};

   const auto initial_data = [&] {
      std::vector<D3D11_SUBRESOURCE_DATA> initial_data;
      initial_data.reserve(metadata.mipLevels);

      for (int mip = 0; mip < metadata.mipLevels; ++mip) {
         auto& data = initial_data.emplace_back();

         data.pSysMem = image.GetImage(mip, 0, 0)->pixels;
         data.SysMemPitch = image.GetImage(mip, 0, 0)->rowPitch;
         data.SysMemSlicePitch = image.GetImage(mip, 0, 0)->slicePitch;
      }

      return initial_data;
   }();

   Com_ptr<ID3D11Texture2D> texture;

   if (const auto result = _device->CreateTexture2D(&desc, initial_data.data(),
                                                    texture.clear_and_assign());
       FAILED(result)) {
      log(Log_level::error, "Failed to create game texture! reason: ",
          _com_error{result}.ErrorMessage());

      return {};
   }

   Com_ptr<ID3D11ShaderResourceView> srv;
   {
      const auto srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE2D,
                                          metadata.format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srv_desc,
                                               srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   const auto srgb_format = DirectX::MakeSRGB(metadata.format);

   if (metadata.format == srgb_format) {
      return Game_texture{srv, srv};
   }

   Com_ptr<ID3D11ShaderResourceView> srgb_srv;
   {
      const auto srgb_srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE2D, srgb_format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srgb_srv_desc,
                                               srgb_srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRGB SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   return Game_texture{std::move(srv), std::move(srgb_srv)};
}

auto Shader_patch::create_game_texture3d(const DirectX::ScratchImage& image) noexcept
   -> Game_texture
{
   Expects(image.GetMetadata().mipLevels != 0 && image.GetMetadata().arraySize == 1);

   const auto& metadata = image.GetMetadata();
   const auto typeless_format = DirectX::MakeTypeless(metadata.format);

   const auto desc =
      CD3D11_TEXTURE3D_DESC{typeless_format,      metadata.width,
                            metadata.height,      metadata.depth,
                            metadata.mipLevels,   D3D11_BIND_SHADER_RESOURCE,
                            D3D11_USAGE_IMMUTABLE};

   const auto initial_data = [&] {
      std::vector<D3D11_SUBRESOURCE_DATA> initial_data;
      initial_data.reserve(metadata.mipLevels);

      for (int mip = 0; mip < metadata.mipLevels; ++mip) {
         auto& data = initial_data.emplace_back();

         data.pSysMem = image.GetImage(mip, 0, 0)->pixels;
         data.SysMemPitch = image.GetImage(mip, 0, 0)->rowPitch;
         data.SysMemSlicePitch = image.GetImage(mip, 0, 0)->slicePitch;
      }

      return initial_data;
   }();

   Com_ptr<ID3D11Texture3D> texture;

   if (const auto result = _device->CreateTexture3D(&desc, initial_data.data(),
                                                    texture.clear_and_assign());
       FAILED(result)) {
      log(Log_level::error, "Failed to create game texture! reason: ",
          _com_error{result}.ErrorMessage());

      return {};
   }

   Com_ptr<ID3D11ShaderResourceView> srv;
   {
      const auto srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE3D,
                                          metadata.format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srv_desc,
                                               srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   const auto srgb_format = DirectX::MakeSRGB(metadata.format);

   if (metadata.format == srgb_format) {
      return Game_texture{srv, srv};
   }

   Com_ptr<ID3D11ShaderResourceView> srgb_srv;
   {
      const auto srgb_srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE3D, srgb_format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srgb_srv_desc,
                                               srgb_srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRGB SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   return Game_texture{std::move(srv), std::move(srgb_srv)};
}

auto Shader_patch::create_game_texture_cube(const DirectX::ScratchImage& image) noexcept
   -> Game_texture
{
   Expects(image.GetMetadata().mipLevels != 0 &&
           image.GetMetadata().depth == 1 && image.GetMetadata().arraySize == 6);

   const auto& metadata = image.GetMetadata();
   const auto typeless_format = DirectX::MakeTypeless(metadata.format);

   const auto desc = CD3D11_TEXTURE2D_DESC{typeless_format,
                                           metadata.width,
                                           metadata.height,
                                           6,
                                           metadata.mipLevels,
                                           D3D11_BIND_SHADER_RESOURCE,
                                           D3D11_USAGE_IMMUTABLE,
                                           0,
                                           1,
                                           0,
                                           D3D11_RESOURCE_MISC_TEXTURECUBE};

   const auto initial_data = [&] {
      std::vector<D3D11_SUBRESOURCE_DATA> initial_data;
      initial_data.reserve(metadata.mipLevels * 6);

      for (int item = 0; item < 6; ++item) {
         for (int mip = 0; mip < metadata.mipLevels; ++mip) {
            auto& data = initial_data.emplace_back();

            data.pSysMem = image.GetImage(mip, item, 0)->pixels;
            data.SysMemPitch = image.GetImage(mip, item, 0)->rowPitch;
            data.SysMemSlicePitch = image.GetImage(mip, item, 0)->slicePitch;
         }
      }

      return initial_data;
   }();

   Com_ptr<ID3D11Texture2D> texture;

   if (const auto result = _device->CreateTexture2D(&desc, initial_data.data(),
                                                    texture.clear_and_assign());
       FAILED(result)) {
      log(Log_level::error, "Failed to create game texture! reason: ",
          _com_error{result}.ErrorMessage());

      return {};
   }

   Com_ptr<ID3D11ShaderResourceView> srv;
   {
      const auto srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURECUBE,
                                          metadata.format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srv_desc,
                                               srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   const auto srgb_format = DirectX::MakeSRGB(metadata.format);

   if (metadata.format == srgb_format) {
      return Game_texture{srv, srv};
   }

   Com_ptr<ID3D11ShaderResourceView> srgb_srv;
   {
      const auto srgb_srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURECUBE, srgb_format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srgb_srv_desc,
                                               srgb_srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRGB SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   return Game_texture{std::move(srv), std::move(srgb_srv)};
}

auto Shader_patch::create_game_input_layout(
   const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) noexcept -> Game_input_layout
{
   return _input_layout_factory.game_create(layout);
}

auto Shader_patch::create_game_shader(const Shader_metadata metadata) noexcept -> Game_shader
{
   Game_shader game_shader;

   auto& state =
      _shader_database.rendertypes.at(metadata.rendertype_name).at(metadata.shader_name);

   game_shader.vs = state.at_if(metadata.vertex_shader_flags);
   game_shader.vs_compressed =
      state.at_if(metadata.vertex_shader_flags | Vertex_shader_flags::compressed);
   game_shader.ps = state.at(metadata.pixel_shader_flags);
   game_shader.rendertype = metadata.rendertype;
   game_shader.shader_name = metadata.shader_name;
   game_shader.srgb_state = metadata.srgb_state;

   if (!game_shader.vs && !game_shader.vs_compressed)
      log_and_terminate("Game_shader has no vertex shader!");
   if (!game_shader.ps) log_and_terminate("Game_shader has no pixel shader!");

   return game_shader;
}

auto Shader_patch::create_ia_buffer(const UINT size, const bool vertex_buffer,
                                    const bool index_buffer, const bool dynamic) noexcept
   -> Com_ptr<ID3D11Buffer>
{
   Com_ptr<ID3D11Buffer> buffer;

   const UINT bind_flags = (vertex_buffer ? D3D11_BIND_VERTEX_BUFFER : 0) |
                           (index_buffer ? D3D11_BIND_INDEX_BUFFER : 0);
   const auto usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
   const UINT cpu_access = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

   const auto desc = CD3D11_BUFFER_DESC{size, bind_flags, usage, cpu_access};

   if (const auto result =
          _device->CreateBuffer(&desc, nullptr, buffer.clear_and_assign());
       FAILED(result)) {
      log_and_terminate("Failed to create game IA buffer! reason: ",
                        _com_error{result}.ErrorMessage());
   }

   return buffer;
}

void Shader_patch::update_ia_buffer(ID3D11Buffer& buffer, const UINT offset,
                                    const UINT size, const std::byte* data) noexcept
{
   const D3D11_BOX box{offset, 0, 0, offset + size, 1, 1};

   _device_context->UpdateSubresource(&buffer, 0, &box, data, 0, 0);
}

auto Shader_patch::map_ia_buffer(ID3D11Buffer& buffer, const D3D11_MAP map_type) noexcept
   -> std::byte*
{
   D3D11_MAPPED_SUBRESOURCE mapped;

   _device_context->Map(&buffer, 0, map_type, 0, &mapped);

   return static_cast<std::byte*>(mapped.pData);
}

void Shader_patch::unmap_ia_buffer(ID3D11Buffer& buffer) noexcept
{
   _device_context->Unmap(&buffer, 0);
}

void Shader_patch::stretch_rendertarget(const Game_rendertarget_id source,
                                        const RECT* source_rect,
                                        const Game_rendertarget_id dest,
                                        const RECT* dest_rect) noexcept
{
   const auto& src_rt = _game_rendertargets[static_cast<int>(source)];
   const auto& dest_rt = _game_rendertargets[static_cast<int>(dest)];

   if (!source_rect && !dest_rect) {
      _device_context->CopyResource(dest_rt.texture.get(), src_rt.texture.get());

      return;
   }

   const auto src_box = rect_to_box(
      source_rect ? *source_rect : RECT{0, 0, src_rt.width - 1, src_rt.height - 1});
   const auto dest_box = rect_to_box(
      dest_rect ? *dest_rect : RECT{0, 0, src_rt.width - 1, src_rt.height - 1});

   if (!boxes_same_size(dest_box, src_box)) {
      // TODO: Implement stretching!

      return;
   }

   _device_context->CopySubresourceRegion(dest_rt.texture.get(), 0, dest_box.left,
                                          dest_box.top, dest_box.front,
                                          src_rt.texture.get(), 0, &src_box);
}

void Shader_patch::color_fill_rendertarget(const Game_rendertarget_id rendertarget,
                                           const glm::vec4 color, const RECT* rect) noexcept
{
   _device_context
      ->ClearView(_game_rendertargets[static_cast<int>(rendertarget)].rtv.get(),
                  &color.x, rect, rect ? 1 : 0);
}

void Shader_patch::clear_rendertarget(const glm::vec4 color) noexcept
{

   _device_context->ClearRenderTargetView(
      _game_rendertargets[static_cast<int>(_current_game_rendertarget)].rtv.get(),
      &color.x);
}

void Shader_patch::clear_depthstencil(const float depth, const UINT8 stencil,
                                      const bool clear_depth,
                                      const bool clear_stencil) noexcept
{
   if (!_current_depthstencil) return;

   const UINT clear_flags = (clear_depth ? D3D11_CLEAR_DEPTH : 0) |
                            (clear_stencil ? D3D11_CLEAR_STENCIL : 0);

   _device_context->ClearDepthStencilView(_current_depthstencil->dsv.get(),
                                          clear_flags, depth, stencil);
}

void Shader_patch::set_index_buffer(ID3D11Buffer& buffer, const UINT offset) noexcept
{
   _device_context->IASetIndexBuffer(&buffer, DXGI_FORMAT_R16_UINT, offset);
}

void Shader_patch::set_vertex_buffer(ID3D11Buffer& buffer, const UINT offset,
                                     const UINT stride) noexcept
{
   auto* ptr_buffer = &buffer;

   _device_context->IASetVertexBuffers(0, 1, &ptr_buffer, &stride, &offset);
}

void Shader_patch::set_input_layout(const Game_input_layout& input_layout,
                                    const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout_desc) noexcept
{
   _device_context->IASetInputLayout(input_layout.layout.get());

   _input_layout_desc = layout_desc;

   if (std::exchange(_ia_input_compressed, input_layout.compressed) !=
       input_layout.compressed) {
      _device_context->VSSetShader(_ia_input_compressed
                                      ? _game_shader.vs_compressed.get()
                                      : _game_shader.vs.get(),
                                   nullptr, 0);
   }
}

void Shader_patch::set_primitive_topology(const D3D11_PRIMITIVE_TOPOLOGY topology) noexcept
{
   _device_context->IASetPrimitiveTopology(topology);
}

void Shader_patch::set_game_shader(const Game_shader& shader) noexcept
{
   _game_shader = shader;

   _device_context->VSSetShader(_ia_input_compressed
                                   ? _game_shader.vs_compressed.get()
                                   : _game_shader.vs.get(),
                                nullptr, 0);
   _device_context->PSSetShader(_game_shader.ps.get(), nullptr, 0);
}

void Shader_patch::set_rendertarget(const Game_rendertarget_id rendertarget) noexcept
{
   _om_targets_dirty = true;
   _current_game_rendertarget = rendertarget;
}

void Shader_patch::set_depthstencil(const Game_depthstencil depthstencil) noexcept
{
   _om_targets_dirty = true;

   if (depthstencil == Game_depthstencil::nearscene)
      _current_depthstencil = &_nearscene_depthstencil;
   else if (depthstencil == Game_depthstencil::farscene)
      _current_depthstencil = &_farscene_depthstencil;
   else if (depthstencil == Game_depthstencil::reflectionscene)
      _current_depthstencil = &_reflectionscene_depthstencil;
   else
      _current_depthstencil = nullptr;
}

void Shader_patch::set_viewport(float x, float y, float width, float height) noexcept
{
   const auto viewport = CD3D11_VIEWPORT{x, y, width, height};

   _device_context->RSSetViewports(1, &viewport);
}

void Shader_patch::set_rasterizer_state(ID3D11RasterizerState& rasterizer_state) noexcept
{
   _device_context->RSSetState(&rasterizer_state);
}

void Shader_patch::set_depthstencil_state(ID3D11DepthStencilState& depthstencil_state,
                                          const UINT8 stencil_ref) noexcept
{
   _device_context->OMSetDepthStencilState(&depthstencil_state, stencil_ref);
}

void Shader_patch::set_blend_state(ID3D11BlendState& blend_state) noexcept
{
   constexpr std::array blend_factor{1.f, 1.f, 1.f, 1.f};

   _device_context->OMSetBlendState(&blend_state, blend_factor.data(), 0xffffffff);
}

void Shader_patch::set_fog_state(const bool enabled, const glm::vec4 color) noexcept
{
   _cb_draw_ps_dirty = true;
   _cb_draw_ps.fog_enabled = enabled;
   _cb_draw_ps.fog_color = color;
}

void Shader_patch::set_texture(const UINT slot, const Game_texture& texture) noexcept
{
   Expects(slot < 4);

   _game_textures[slot] = texture;
   _ps_textures_dirty = true;
}

void Shader_patch::set_texture(const UINT slot,
                               const Game_rendertarget_id rendertarget) noexcept
{
   Expects(slot < 4);

   const auto& srv = _game_rendertargets[static_cast<int>(rendertarget)].srv;

   _game_textures[slot] = {srv, srv};
   _ps_textures_dirty = true;
   _om_targets_dirty = true;

   _device_context->OMSetRenderTargets(0, nullptr, nullptr);
}

void Shader_patch::set_projtex_mode(const Projtex_mode) noexcept
{
   // TODO: Make stuff happen!
}

void Shader_patch::set_constants(const cb::Scene_tag, const UINT offset,
                                 const gsl::span<const std::array<float, 4>> constants) noexcept
{
   _cb_scene_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_scene) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());
}

void Shader_patch::set_constants(const cb::Draw_tag, const UINT offset,
                                 const gsl::span<const std::array<float, 4>> constants) noexcept
{
   _cb_draw_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_draw) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());
}

void Shader_patch::set_constants(const cb::Skin_tag, const UINT offset,
                                 const gsl::span<const std::array<float, 4>> constants) noexcept
{
   _cb_skin_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_skin) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());
}

void Shader_patch::set_constants(const cb::Draw_ps_tag, const UINT offset,
                                 const gsl::span<const std::array<float, 4>> constants) noexcept
{
   _cb_draw_ps_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_draw_ps) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());
}

void Shader_patch::draw(const UINT vertex_count, const UINT start_vertex) noexcept
{
   update_dirty_state();

   _device_context->Draw(vertex_count, start_vertex);
}

void Shader_patch::draw_indexed(const UINT index_count, const UINT start_index,
                                const UINT start_vertex) noexcept
{
   update_dirty_state();

   _device_context->DrawIndexed(index_count, start_index, start_vertex);
}

void Shader_patch::begin_query(ID3D11Query& query) noexcept
{
   _device_context->Begin(&query);
}

void Shader_patch::end_query(ID3D11Query& query) noexcept
{
   _device_context->End(&query);
}

auto Shader_patch::get_query_data(ID3D11Query& query, const bool flush,
                                  gsl::span<std::byte> data) noexcept -> Query_result
{
   const auto result =
      _device_context->GetData(&query, data.data(), data.size(),
                               flush ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH);

   if (result == S_OK) return Query_result::success;
   if (result == S_FALSE) return Query_result::notready;

   return Query_result::error;
}

auto Shader_patch::get_backbuffer_views() noexcept -> Game_rendertarget
{
   DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;
   _swapchain->GetDesc1(&swap_chain_desc);

   Game_rendertarget rt;

   rt.width = static_cast<std::uint16_t>(swap_chain_desc.Width);
   rt.height = static_cast<std::uint16_t>(swap_chain_desc.Height);

   _swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                         rt.texture.void_clear_and_assign());

   _device->CreateRenderTargetView(rt.texture.get(), nullptr,
                                   rt.rtv.clear_and_assign());

   return rt;
}

void Shader_patch::bind_static_resources() noexcept
{
   _device_context->VSSetConstantBuffers(
      0, 2, std::array{_cb_scene_buffer.get(), _cb_draw_buffer.get()}.data());

   auto* const cb_skin_buffer = _cb_skin_buffer_srv.get();
   _device_context->VSSetShaderResources(0, 1, &cb_skin_buffer);

   auto* const cb_draw_ps_buffer = _cb_draw_ps_buffer.get();
   _device_context->PSSetConstantBuffers(0, 1, &cb_draw_ps_buffer);
}

void Shader_patch::update_dirty_state() noexcept
{
   if (std::exchange(_ps_textures_dirty, false)) {
      _device_context->PSSetShaderResources(0, 4,
                                            std::array{_game_textures[0].srv.get(),
                                                       _game_textures[1].srv.get(),
                                                       _game_textures[2].srv.get(),
                                                       _game_textures[3].srv.get()}
                                               .data());
   }

   if (std::exchange(_om_targets_dirty, false)) {
      const auto& rtv =
         _game_rendertargets[static_cast<int>(_current_game_rendertarget)].rtv.get();

      _device_context->OMSetRenderTargets(1, &rtv,
                                          _current_depthstencil
                                             ? _current_depthstencil->dsv.get()
                                             : nullptr);
   }

   if (std::exchange(_cb_scene_dirty, false)) {
      D3D11_MAPPED_SUBRESOURCE map;

      _device_context->Map(_cb_scene_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

      std::memcpy(map.pData, &_cb_scene, sizeof(_cb_scene));

      _device_context->Unmap(_cb_scene_buffer.get(), 0);
   }

   if (std::exchange(_cb_draw_dirty, false)) {
      D3D11_MAPPED_SUBRESOURCE map;

      _device_context->Map(_cb_draw_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

      std::memcpy(map.pData, &_cb_draw, sizeof(_cb_draw));

      _device_context->Unmap(_cb_draw_buffer.get(), 0);
   }

   if (std::exchange(_cb_skin_dirty, false)) {
      D3D11_MAPPED_SUBRESOURCE map;

      _device_context->Map(_cb_skin_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

      std::memcpy(map.pData, &_cb_skin, sizeof(_cb_skin));

      _device_context->Unmap(_cb_skin_buffer.get(), 0);
   }

   if (std::exchange(_cb_draw_ps_dirty, false)) {
      D3D11_MAPPED_SUBRESOURCE map;

      _device_context->Map(_cb_draw_ps_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD,
                           0, &map);

      std::memcpy(map.pData, &_cb_draw_ps, sizeof(_cb_draw_ps));

      _device_context->Unmap(_cb_draw_ps_buffer.get(), 0);
   }
}

}
