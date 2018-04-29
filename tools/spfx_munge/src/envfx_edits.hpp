#pragma once

#include <boost/filesystem.hpp>

namespace sp {

void edit_envfx(const boost::filesystem::path& fx_path,
                const boost::filesystem::path& output_dir);

}
