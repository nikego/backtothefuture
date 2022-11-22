#pragma once

#include <boost/optional.hpp>
#include <boost/filesystem/path.hpp>

namespace bttf {

size_t calc_checksum(const boost::filesystem::path& file);

bool equal_files(const boost::filesystem::path& a, const boost::filesystem::path& b);

boost::optional<size_t> calc_dir_checksum(const boost::filesystem::path& dir);

std::string make_uuid();

bool test_unpack(const boost::filesystem::path& input, const boost::filesystem::path& output);

} // namespace bttf
