#pragma once

#include <boost/filesystem/path.hpp>

namespace bttf {

void pack_folder(const boost::filesystem::path& input_folder, const boost::filesystem::path& archive);

void unpack_file(const boost::filesystem::path& file_from, const boost::filesystem::path& folder_to);

} // namespace bttf
