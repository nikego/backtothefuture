#include "processor.h"
#include "packer.h"
#include "unpacker.h"
#include "trace.h"

namespace bttf {

void pack_folder(const boost::filesystem::path& folder, const boost::filesystem::path& output_name)
{
   packer_t packer(folder, output_name);

   auto osize = boost::filesystem::file_size(output_name);

   auto& s = packer.stats();

   BTTF_INFO() << "input files " << s.files << ", input size:" << s.total_size << ", output size:" << osize << ", ratio:" << 
	(osize * 100 / s.total_size) << "%, saved files:" << s.saved_files << ", saved links:" << s.saved_links;
}

void unpack_file(const boost::filesystem::path& input_name, const boost::filesystem::path& output_folder)
{
   unpacker_t unpacker(input_name, output_folder);
}

} // namespace bttf
