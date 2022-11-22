#include "packer.h"
#include "config.h"
#include "utilities.h"
#include "trace.h"
#include "compress.h"

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

#include <vector>
#include <map>

namespace bttf {

namespace fs = boost::filesystem;

struct metadata_t
{
   metadata_t(fs::path n)
      : name(std::move(n))
   {
   }

   fs::path name;
   int      id = 0;
   size_t   checksum = 0;
   uint32_t size = 0;
   bool     saved = false;
};

packer_t::packer_t(const boost::filesystem::path& input_folder, const boost::filesystem::path& archive_name)
   : input_folder_(input_folder)
   , ostream_(archive_name, std::ios::binary)
{
   stats_.files = scan_folder();

   if (stats_.files == 0)
      throw std::runtime_error("Input folder " + input_folder.string() + " is empty, nothing to do");

   pack();

   ostream_.close();

   stats_.output_size   = fs::file_size(archive_name);

   files_list_.clear();
   header_is_written_ = false;
}

size_t packer_t::scan_folder()
{
   auto iterator = fs::recursive_directory_iterator(input_folder_);

   int file_counter = 0;

   for (const auto& iter : iterator)
   {
      const auto& file_name = iter.path();
      try
      {
         if (fs::is_regular(iter))
         {
            auto file = std::make_shared<metadata_t>(file_name);

            file->id    = ++file_counter;
            file->saved = false;
            file->size  = fs::file_size(file_name);

            auto iter = files_list_.find({ 0, file->size });

            if (iter == files_list_.end())
               files_list_.insert({ {0, file->size}, {file} });
            else
               iter->second.push_back(file);

            stats_.total_size += file->size;
         }
      }
      catch (const std::exception& e)
      {
         BTTF_WARN() << "An error has occured while scanning input folder : " << e.what();
      }
   }
   return file_counter;
}

void packer_t::pack()
{
   using namespace boost::asio;

   std::unique_ptr<thread_pool> pool(new thread_pool());

   write_header();

   for (auto iter = files_list_.begin(); iter != files_list_.end();)
   {
      std::unique_lock<std::mutex> _(files_mut_);

      if (iter->first.first == 0)
      {
         if (iter->second.size() == 1)
         {
            post(*pool, [this, file = iter->second.front()]
               {
                  write_file(file);
               });
         }
         else
         {
            for (auto file : iter->second)
            {
               post(*pool, [this, file]
                  {
                     try
                     {
                        file->checksum = calc_checksum(file->name);

                        std::unique_lock<std::mutex> _(files_mut_);
                        files_list_[{file->checksum, file->size}].push_back(file);
                     }
                     catch (const std::exception& e)
                     {
                        BTTF_WARN() << "calculating of checksum '" << file->name.string() << "' failed " << e.what();
                     }
                  });
            }
         }
         iter = files_list_.erase(iter);
      }
      else
         ++iter;
   }
   pool->join();

   pool.reset(new thread_pool());

   for (auto& iterator : files_list_)
   {
      auto& vec = iterator.second;

      post(*pool, [this, vec]() mutable
         {
            process_file_group(vec);
         });
   }
   pool->join();
}

void packer_t::write_header()
{
   if (!header_is_written_)
   {
      ostream_.write(FileHeader.data(), FileHeader.size());
      header_is_written_ = true;
   }
}

void packer_t::write_file(metadata_ptr mt)
{
   if (!mt->saved)
   {
      using namespace boost::interprocess;
      try
      {
         file_mapping mapping(mt->name.string().c_str(), read_only);
         mapped_region region(mapping, read_only);

         const char* data = static_cast<const char*>(region.get_address());
         size_t data_size = region.get_size();

         auto hdr_buf = alloc_file_node_buf(fs::relative(mt->name, input_folder_).string(), mt->id, mt->size);
         std::vector<char> outbuffer;

         if (mt->size > 0 && g_config.compression_level > 0)
         {
            outbuffer = compress_to_buffer(region.get_address(), mt->size, g_config.compression_level);
            if (outbuffer.size() > 0)
            {
               hdr_buf = alloc_file_node_buf(fs::relative(mt->name, input_folder_).string(), mt->id, outbuffer.size(), true);
               data = outbuffer.data();
               data_size = outbuffer.size();
            }
         }

         std::unique_lock<std::mutex> _(ostream_mut_);

         ostream_.write(hdr_buf.data(), hdr_buf.size());
         ostream_.write(data, data_size);

         mt->saved = true;
         ++stats_.saved_files;
      }
      catch (const std::exception& e)
      {
         BTTF_ERROR() << "Exception :" << e.what();
      }
   }
}

void packer_t::write_link(metadata_ptr mt, int other_id)
{
   if (!mt->saved)
   {
      auto buffer = alloc_link_node_buf(fs::relative(mt->name, input_folder_).string(), other_id);

      std::unique_lock<std::mutex> _(ostream_mut_);

      ostream_.write(buffer.data(), buffer.size());

      mt->saved = true;
      ++stats_.saved_links;
   }
}

void packer_t::process_file_group(std::vector<metadata_ptr>& vec)
{
   while (!vec.empty())
   {
      auto file = vec.front();
      write_file(file);
      vec.erase(vec.begin());

      for (auto iter = vec.begin(); iter != vec.end(); )
      {
         try
         {
            if (equal_files((*iter)->name, file->name))
            {
               write_link(*iter, file->id);
               iter = vec.erase(iter);
            }
            else
               ++iter;
         }
         catch (const std::exception& e)
         {
            BTTF_WARN() << "comparing of files '" << file->name << "' and '" << (*iter)->name << "' failed " << e.what();
            iter = vec.erase(iter);
         }
      }
   }
}

} // namespace bttf
