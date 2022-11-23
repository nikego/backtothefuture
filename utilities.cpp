#include "utilities.h"
#include "processor.h"
#include "trace.h"

#include <boost/filesystem.hpp>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <boost/container_hash/hash.hpp>

#include <boost/lexical_cast.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

#include <map>

namespace fs = boost::filesystem;

namespace bttf {

using namespace boost::interprocess;

#if 1

size_t calc_checksum(const fs::path& file)
{
   file_mapping mapping(file.string().c_str(), read_only);
   mapped_region region(mapping, read_only);

   const char* data = static_cast<char*>(region.get_address());
   std::size_t size = region.get_size();

   return boost::hash_range(data, data + size);
}

#else

// alternative implementation
size_t calc_checksum(const fs::path& file)
{
   auto size = fs::file_size(file);

   fs::ifstream ifs;
   ifs.exceptions(std::ifstream::badbit);
   ifs.open(file, std::ios::binary);

   const size_t BUF_SIZE = 256 * 1024ULL;

   std::vector<char> buffer(BUF_SIZE);
   auto remain = size;

   size_t hash = 0;
   while (remain > 0)
   {
      ifs.read(buffer.data(), std::min(remain, BUF_SIZE));
      size_t read_bytes = ifs.gcount();
      boost::hash_range(hash, buffer.data(), buffer.data() + read_bytes);
      remain -= read_bytes;
   }
   return hash;
}

#endif

bool equal_files(const fs::path& a, const fs::path& b)
{
   file_mapping mapping_a(a.string().c_str(), read_only);
   mapped_region region_a(mapping_a, read_only);

   const char* addr_a = static_cast<char*>(region_a.get_address());
   std::size_t size = region_a.get_size();  // files must have the same size

   file_mapping mapping_b(a.string().c_str(), read_only);
   mapped_region region_b(mapping_b, read_only);

   const char* addr_b = static_cast<char*>(region_b.get_address());

   return std::equal(addr_a, addr_a + size, addr_b);
}

boost::optional<size_t> calc_dir_checksum(const fs::path& dir)
{
   try
   {
      auto iterator = fs::recursive_directory_iterator(dir);

      std::map<std::string, bool> files_list;

      for (const auto& iter : iterator)
      {
         if (fs::is_regular(iter) || fs::is_directory(iter))
         {
            const auto& file_name = iter.path();
            files_list.insert({ fs::relative(file_name, dir).string(),fs::is_regular(iter) });
         }
      }

      size_t hash_value = 0;

      for (const auto& iter : files_list)
      {
         boost::hash_range(hash_value, iter.first.begin(), iter.first.end());

         if (iter.second)
         {
            file_mapping mapping((dir / iter.first).string().c_str(), read_only);
            mapped_region region(mapping, read_only);

            const char* data = static_cast<char*>(region.get_address());

            boost::hash_range(hash_value, data, data + region.get_size());
         }
      }

      return hash_value;
   }
   catch (const std::exception& e)
   {
      BTTF_ERROR() << "Exception in calc_dir_checksum(): " << e.what();
   }

   return boost::none;
}

std::string make_uuid()
{
   return boost::lexical_cast<std::string>((boost::uuids::random_generator())());
}

struct time_period_t
{
   time_period_t()
      : time_(std::chrono::high_resolution_clock::now())
   {
   }

   int in_ms()
   {
      auto tmp = time_;
      time_ = std::chrono::high_resolution_clock::now();
      return std::chrono::duration_cast<std::chrono::milliseconds>(time_ - tmp).count();
   }

   std::chrono::high_resolution_clock::time_point time_;
};

bool test_unpack(const fs::path& input, const fs::path& output)
{
   try
   {
      BTTF_DEBUG() << "unpacking output file...";

      auto temp_dir = fs::temp_directory_path() / make_uuid();

      time_period_t period;

      bttf::unpack_file(output, temp_dir);

      BTTF_DEBUG() << "unpacking takes " << period.in_ms() << " ms";

      BTTF_DEBUG() << "comparing directories...";

      boost::asio::thread_pool pool(2);
      boost::optional<size_t> hash_i = 0;
      boost::optional<size_t> hash_o = 0;

      boost::asio::post(pool, [&hash_i, input]    { hash_i = calc_dir_checksum(input);    });
      boost::asio::post(pool, [&hash_o, temp_dir] { hash_o = calc_dir_checksum(temp_dir); });

      pool.join();

      BTTF_DEBUG() << "comparing takes " << period.in_ms() << " ms";

      fs::remove_all(temp_dir);

      if (!hash_i || !hash_o)
         return false;

      return (hash_i == hash_o);
   }
   catch (const std::exception& e)
   {
      BTTF_ERROR() << "Exception : " << e.what();
   }
   return false;
}

} // namespace bttf
