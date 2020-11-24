#pragma once

#include "bytecode_blob.hpp"
#include "entrypoint_description.hpp"
#include "source_file_store.hpp"

namespace sp::shader {

class Compiler {
public:
   auto compile(const Entrypoint_description& entrypoint,
                const std::uint64_t static_flags,
                const Vertex_shader_flags vertex_shader_flags = Vertex_shader_flags::none) noexcept
      -> Bytecode_blob;

private:
   const std::wstring TEMP_file_store_source =
      LR"(C:\GitHub\swbfii-shaderpatch\assets\core\src)";

   Source_file_store _file_store{TEMP_file_store_source};
};

}
