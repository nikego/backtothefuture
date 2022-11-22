#include "trace.h"

#include <boost/noncopyable.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

namespace fs = boost::filesystem;

#if USE_ZSTD

#include <zstd.h>

namespace bttf {

std::vector<char> compress_to_buffer(const void* data, size_t size, int compression_level)
{
   size_t bound = ZSTD_compressBound(size);

   std::vector<char> buffer(bound);

   size_t res = ZSTD_compress(buffer.data(), bound, data, size, compression_level);

   if (ZSTD_isError(res))
   {
      BTTF_ERROR() << "zstd compress failed :" << ZSTD_getErrorName(res);
   }
   else if (res < size)
   {
      buffer.resize(res);
      return buffer;
   }
   return {};
}

struct zstd_dstream : boost::noncopyable
{
   zstd_dstream()
      : c(ZSTD_createDStream())
   {
   }

   size_t init()
   {
      return ZSTD_initDStream(c);
   }

   ~zstd_dstream()
   {
      ZSTD_freeDStream(c);
   }

   ZSTD_DStream* c;
};

bool uncompress_to_file(const void* data, size_t data_size, const fs::path& path)
{
   zstd_dstream ctx;
   ctx.init();

   auto is = ZSTD_DStreamInSize();
   auto os = ZSTD_DStreamOutSize();

   std::vector<char> buffer(os);

   ZSTD_outBuffer out{ buffer.data(),  buffer.size(), 0 };
   ZSTD_inBuffer  in { data,           data_size,     0 };

   fs::ofstream ofs;
   ofs.exceptions(std::ofstream::badbit);
   ofs.open(path, std::ios::binary);

   for (;;)
   {
      auto res = ZSTD_decompressStream(ctx.c, &out, &in);

      if (ZSTD_isError(res))
      {
         BTTF_ERROR() << "uncompress stream failed : " << ZSTD_getErrorName(res);
         return false;
      }

      if (out.pos > 0)
      {
         ofs.write(static_cast<const char*>(out.dst), out.pos);
         if (out.pos == out.size)
         {
            out.pos = 0;
            continue;
         }
      }

      if (res == 0)
         return true;

      if (in.pos < in.size)
      {
         BTTF_DEBUG() << "oops, incorrect input data to uncompress";
         return false;
      }
      // hmm, res > 0, it is strange...
      return true;
   }
}

} // namespace bttf

#else // !USE_ZSTD

namespace bttf {

std::vector<char> compress_to_buffer(const void* data, size_t size, int compression_level)
{
   static bool once = []
   {
      BTTF_ERROR() << "compressing is not supported; rebuild with ZSTD";
      return true;
   }();

   return {};
}

bool uncompress_to_file(const void* data, size_t data_size, const fs::path& path)
{
   static bool once = []
   {
      BTTF_ERROR() << "decompressing is not supported; rebuild with ZSTD";
      return true;
   }();

   return false;
}

} // namespace bttf

#endif 




