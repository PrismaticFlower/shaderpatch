#pragma once

#include <boost/filesystem.hpp>

namespace sp {

void munge_texture(boost::filesystem::path config_file_path,
                   const boost::filesystem::path& output_dir) noexcept;
}
