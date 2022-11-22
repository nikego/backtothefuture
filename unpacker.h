#pragma once

#include "structure.h"

#include <boost/filesystem.hpp>

namespace bttf {

struct unpacker_t
{
   unpacker_t(boost::filesystem::path archive, boost::filesystem::path output_folder);

private:
   void unpack();

   void write_file(const file_node_t* item, const boost::filesystem::path& path);

private:
   const boost::filesystem::path archive_;
   const boost::filesystem::path output_folder_;
};

} // namespace bttf
