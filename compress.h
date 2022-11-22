#pragma once

#include <boost/filesystem/path.hpp>

namespace bttf {

std::vector<char> compress_to_buffer(const void* data, size_t size, int compression_level);

bool uncompress_to_file(const void* data, size_t data_size, const boost::filesystem::path& path);

} // namespace bttf
