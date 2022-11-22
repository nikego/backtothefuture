#include "unpacker.h"
#include "trace.h"
#include "compress.h"

#include <boost/filesystem/fstream.hpp>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

#include <boost/log/trivial.hpp>
#include <boost/noncopyable.hpp>

#include <vector>
#include <map>

namespace fs = boost::filesystem;

namespace bttf {

unpacker_t::unpacker_t(fs::path archive, fs::path output_folder)
   : archive_(std::move(archive))
   , output_folder_(std::move(output_folder))
{
   if (!fs::exists(output_folder_))
      fs::create_directories(output_folder_);

   unpack();
}

void unpacker_t::write_file(const file_node_t* item, const fs::path& path)
{
   try
   {
      if (!fs::exists(path.parent_path()))
         fs::create_directories(path.parent_path());

      auto data = reinterpret_cast<const char*>(item) + sizeof(file_node_t) + item->name_len;
      if (item->compressed)
      {
         if (!uncompress_to_file(data, item->data_len, path))
         {
            BTTF_ERROR() << "An error has occured while decompressing data";
         }
      }
      else
      {
         fs::ofstream ofs;
         ofs.exceptions(std::ofstream::badbit);
         ofs.open(path, std::ios::binary);
         ofs.write(data, item->data_len);
      }
   }
   catch (const std::exception& e)
   {
      BTTF_ERROR() << "An error has occured while writing the file " << path << ", :" << e.what();
   }
}

void unpacker_t::unpack()
{
   using namespace boost::interprocess;
   using namespace boost::asio;

   file_mapping mapping(archive_.wstring().c_str(), read_only);
   mapped_region region(mapping, read_only);

   const char* data = static_cast<char*>(region.get_address());

   auto src = data;
   auto end = data + region.get_size();

   if (!std::equal(src, src + FileHeader.size(), FileHeader.data()))
      throw std::runtime_error("Input file is not a correct archive");

   src += FileHeader.size();

   std::map<int, const file_node_t*> list_of_files;

   thread_pool pool(4);

   while (src < end)
   {
      auto item = reinterpret_cast<const node_hdr_t*>(src);

      if (item->status == node_hdr_t::estatus::File)
      {
         auto fitem = static_cast<const file_node_t*>(item);

         list_of_files[item->file_id] = fitem;

         auto oname = output_folder_ / fs::path(fitem->name, fitem->name + item->name_len);

         post(pool, [this, fitem, oname = std::move(oname)]
            {
               write_file(fitem, oname);
            });

         src += sizeof(file_node_t) + fitem->name_len + fitem->data_len;
      }
      else if (item->status == node_hdr_t::estatus::Link)
      {
         auto litem = static_cast<const link_node_t*>(item);

         auto iter = list_of_files.find(litem->file_id);

         if (iter != list_of_files.end())
         {
            auto oname = output_folder_ / fs::path(litem->name, litem->name + litem->name_len);

            post(pool, [this, fitem = iter->second, oname = std::move(oname)]
               {
                  write_file(fitem, oname);
               });
         }
         else
            throw std::runtime_error("Incorrect structure of the archive");

         src += sizeof(link_node_t) + litem->name_len;
      }
   }
   pool.join();
}

} // namespace bttf
