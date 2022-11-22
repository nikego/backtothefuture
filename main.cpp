#include "arguments.h"
#include "processor.h"

#include "trace.h"
#include "utilities.h"

namespace bttf {

config_t g_config;

} //namespace bttf

int main(int argc, char* argv[])
{
   setlocale(LC_ALL, "");

   using namespace bttf;

   arguments_t args;
   if (auto exit_code = args(argc, argv))
      return *exit_code;

   g_config.severity_level = args.severity_level;
   g_config.compression_level = args.compression_level;

   namespace fs = boost::filesystem;
   namespace chr = std::chrono;

   try
   {
      if (!fs::exists(args.input))
      {
         BTTF_ERROR() << "You specified nonexisting folder/file " << args.input;
         return EXIT_FAILURE;
      }

      auto start = chr::high_resolution_clock::now();

      if (fs::is_directory(args.input))
      {
         bttf::pack_folder(args.input, args.output);
      }
      else if (fs::is_regular_file(args.input))
      {
         bttf::unpack_file(args.input, args.output);
      }
      else
      {
         BTTF_ERROR() << "You specified unsupported kind of filesystem item: " << args.input;
         return EXIT_FAILURE;
      }

      BTTF_INFO() << "executing time: " << std::dec << chr::duration_cast<chr::milliseconds>(chr::high_resolution_clock::now() - start).count() << " ms";

      if (fs::is_directory(args.input) && args.test_unpack)
      {
         if (g_config.severity_level > boost::log::trivial::info)
            g_config.severity_level = boost::log::trivial::info;

         if (!test_unpack(args.input, args.output))
         {
            BTTF_ERROR() << "Fail, unpacked directory is not matched to the source one";
            return EXIT_FAILURE;
         }
         BTTF_INFO() << "OK, unpacked and source directories are identical";
      }
   }
   catch (std::exception const& err)
   {
      BTTF_ERROR() << "An error has occured: " << err.what() << std::endl;
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}

#if 0

      /*std::sort(files_list.begin(), files_list.end(), [](const file_metadata_ptr& a, const file_metadata_ptr& b)
         {
            return a->size < b->size;
         });*/

std::atomic<int> counter = 0;

auto start = files_list.begin();
auto iter = start;
std::advance(iter, 1);

files_list.push_back(nullptr);

for (; iter != files_list.end(); ++iter)
{
   if (*iter == nullptr || (*start)->size != (*iter)->size)
   {
      auto dist = std::distance(start, iter);
      if (dist > 1)
      {
         while (start != iter)
         {
            ++counter;
            boost::asio::post(pool, [this, &counter, file = *start]() mutable
               {
                  if (auto checksum = calc_checksum(file->name))
                  {
                     file->checksum = *checksum;
                  }
                  --counter;
               });

            (*start)->group_size = dist;
            ++start;
         }
      }
      else
      {
         ++counter;
         boost::asio::post(pool, [this, &counter, file = *start]() mutable
            {
               put_file(file);
               --counter;
            });
         ++start;
      }
   }
}

while (counter > 0)
std::this_thread::sleep_for(std::chrono::milliseconds(1));

files_list.pop_back();

files_list.sort([](const file_metadata_ptr a, const file_metadata_ptr b) -> bool
   {
      return a->size == b->size ? a->checksum < b->checksum : a->size < b->size;
   });

start = files_list.begin();
iter = start;
std::advance(iter, 1);

files_list.push_back(nullptr);

for (; iter != files_list.end(); ++iter)
{
   if (*iter == nullptr || (*start)->size != (*iter)->size && (*start)->checksum != (*iter)->checksum)
   {
      auto dist = std::distance(start, iter);
      if (dist > 1)
      {
         ++counter;
         std::list<file_metadata_ptr> list(start, iter);
         boost::asio::post(pool, [this, list, &counter]() mutable
            {
               process_file_group(list);
               --counter;
            }
         );
      }
      else
      {
         ++counter;
         boost::asio::post(pool, [this, f = *start, &counter]() mutable
            {
               put_file(f);
               --counter;
            }
         );
      }
      start = iter;
   }
}

files_list.pop_back();
#endif 