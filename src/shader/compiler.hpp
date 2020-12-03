#pragma once

#include "bytecode_blob.hpp"
#include "entrypoint_description.hpp"
#include "source_file_store.hpp"

namespace sp::shader {

auto compile(const Source_file_store& file_store,
             const Entrypoint_description& entrypoint, const std::uint64_t static_flags,
             const Vertex_shader_flags vertex_shader_flags = Vertex_shader_flags::none) noexcept
   -> Bytecode_blob;

}
