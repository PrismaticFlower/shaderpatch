#pragma once

#include <boost/filesystem.hpp>

namespace sp {

void munge_spfx(const boost::filesystem::path& spfx_path,
                const boost::filesystem::path& output_dir);

}
