#pragma once

#include "structure.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <map>

namespace bttf {

struct metadata_t;

using metadata_ptr = std::shared_ptr<metadata_t>;

struct packer_stats_t
{
   std::atomic<size_t> files        = 0;
   std::atomic<size_t> output_size  = 0;
   std::atomic<size_t> total_size   = 0;
   std::atomic<size_t> saved_files  = 0;
   std::atomic<size_t> saved_links  = 0;
};

struct packer_t
{
   packer_t(const boost::filesystem::path& input_folder, const boost::filesystem::path& archive);

   const packer_stats_t& stats() const
   {
      return stats_;
   }

private:
   void pack();
   size_t scan_folder();

   void process_file_group(std::vector<metadata_ptr>& vec);

   void write_link(metadata_ptr mt, int other_id);
   void write_file(metadata_ptr mt);
   void write_header();

private:
   const boost::filesystem::path& input_folder_;

   std::map<std::pair<size_t, uint32_t>, std::vector<metadata_ptr>> files_list_;
   std::mutex files_mut_;

   bool header_is_written_ = false;

   std::mutex ostream_mut_;
   boost::filesystem::ofstream ostream_;

   packer_stats_t stats_;
};

} // namespace bttf
