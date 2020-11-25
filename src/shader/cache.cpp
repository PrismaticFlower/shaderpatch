#include "cache.hpp"
#include "../logger.hpp"
#include "binary_io_winapi.hpp"
#include "shader_patch_version.hpp"

#include <functional>
#include <ranges>
#include <span>
#include <vector>

using namespace std::literals;

namespace sp::shader {

void Cache::save_to_file(const std::filesystem::path& cache_path)
{
   try {
      const auto write_path = std::filesystem::path{cache_path} += L".TEMP"sv;

      Binary_writer file{write_path};

      file.write(current_shader_patch_version.major,
                 current_shader_patch_version.minor,
                 current_shader_patch_version.patch,
                 current_shader_patch_version.prerelease_stage,
                 current_shader_patch_version.prerelease);

      const auto write_stage_cache =
         [&file]<typename K, typename V>(const Basic_cache_map<K, V>& cache) {
            file.write(cache.size());

            for (const auto& [index, entry] : cache) {
               file.write(index.group.size(), index.group);
               file.write(index.entrypoint.size(), index.entrypoint);
               file.write(index.static_flags);

               if constexpr (std::is_same_v<K, Cache_index_vs>) {
                  file.write(index.game_flags);
               }

               file.write(entry.bytecode.size(), entry.bytecode);
            }
         };

      write_stage_cache(_cs_cache);
      write_stage_cache(_vs_cache);
      write_stage_cache(_ds_cache);
      write_stage_cache(_hs_cache);
      write_stage_cache(_gs_cache);
      write_stage_cache(_ps_cache);

      file.close();

      std::filesystem::rename(write_path, cache_path);

      log_debug("Saved shader bytecode cache."sv);
   }
   catch (std::exception&) {
      log(Log_level::warning, "Failed to save shader cache to file!"sv);
   }
}

void Cache::load_from_file(ID3D11Device5& device, const std::filesystem::path& cache_path)
{
   if (!std::filesystem::exists(cache_path)) {
      log(Log_level::info, "Shader bytecode cache no present on disk."sv);

      return;
   }

   try {
      Binary_reader file{cache_path};

      Shader_patch_version cache_sp_version{};

      file.read_to(cache_sp_version.major, cache_sp_version.minor,
                   cache_sp_version.patch, cache_sp_version.prerelease_stage,
                   cache_sp_version.prerelease);

      if (cache_sp_version != current_shader_patch_version) return;

      const auto read_stage_cache =
         [&]<typename K, typename V>(Basic_cache_map<K, V>& cache, auto create) {
            const auto entry_count = file.read<std::size_t>();

            for (std::size_t i = 0; i < entry_count; ++i) {
               const auto read_string = [&file](auto& out) {
                  out.resize(file.read<std::size_t>());

                  file.read_to(out);
               };

               K index;

               read_string(index.group);
               read_string(index.entrypoint);

               file.read_to(index.static_flags);

               if constexpr (std::is_same_v<K, Cache_index_vs>) {
                  file.read_to(index.game_flags);
               }

               Bytecode_blob bytecode{file.read<std::size_t>()};

               file.read_to(bytecode);

               Com_ptr<V> shader;

               if (const auto result =
                      std::invoke(create, device, bytecode.data(), bytecode.size(),
                                  nullptr, shader.clear_and_assign());
                   FAILED(result)) {
                  log_debug("Failed to create shader from cached bytecode {}:{}({:x})"sv,
                            index.group, index.entrypoint, index.static_flags);

                  return;
               }

               log_debug("Loaded and created cached shader {}:{}({:x})"sv,
                         index.group, index.entrypoint, index.static_flags);

               cache.emplace(std::move(index),
                             Cache_entry<V>{.shader = std::move(shader),
                                            .bytecode = std::move(bytecode)});
            }
         };

      read_stage_cache(_cs_cache, &ID3D11Device5::CreateComputeShader);
      read_stage_cache(_vs_cache, &ID3D11Device5::CreateVertexShader);
      read_stage_cache(_ds_cache, &ID3D11Device5::CreateDomainShader);
      read_stage_cache(_hs_cache, &ID3D11Device5::CreateHullShader);
      read_stage_cache(_gs_cache, &ID3D11Device5::CreateGeometryShader);
      read_stage_cache(_ps_cache, &ID3D11Device5::CreatePixelShader);
   }
   catch (std::exception&) {
      log(Log_level::warning,
          "Failed to load shader cache to file! Slow startup expected."sv);
   }
}

}
