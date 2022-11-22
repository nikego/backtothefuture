#pragma once

#include <vector>
#include <array>
#include <string>

namespace bttf {

#pragma pack (push, 1)

struct node_hdr_t
{
   enum estatus {
      File, Link
   };

   uint32_t status     : 1;  // File or Link
   uint32_t compressed : 1;  // use external compressor
   uint32_t name_len   : 10; // up to 1K
   uint32_t file_id    : 20; // id of this file or id of other file if link
};

struct file_node_t : node_hdr_t
{
   uint32_t data_len;
   char     name[/* name_len */];
 /*char     data[ data_len ]; */
};

struct link_node_t : node_hdr_t
{
   char     name[/* name_len */];
};

#pragma pack (pop)

inline void init_hdr(node_hdr_t* hdr, node_hdr_t::estatus s, const std::string& file_name, int id, bool compressed)
{
   hdr->status = s;
   hdr->compressed = compressed;
   hdr->file_id = id;
   hdr->name_len = file_name.size();
}

inline std::vector<char> alloc_file_node_buf(const std::string& file_name, int id, uint32_t data_len, bool compressed = false)
{
   std::vector<char> buffer(sizeof(file_node_t) + file_name.size());

   auto node = reinterpret_cast<file_node_t*>(buffer.data());
   init_hdr(node, node_hdr_t::estatus::File, file_name, id, compressed);
   memcpy(node->name, file_name.c_str(), file_name.size());
   node->data_len = data_len;

   return buffer;
}

inline std::vector<char> alloc_link_node_buf(const std::string& file_name, int id)
{
   std::vector<char> buffer(sizeof(link_node_t) + file_name.size());

   auto node = reinterpret_cast<link_node_t*>(buffer.data());
   init_hdr(node, node_hdr_t::estatus::Link, file_name, id, false);
   memcpy(node->name, file_name.c_str(), file_name.size());

   return buffer;
}

const std::array<char, 4> FileHeader = { {'B', 'T', 'T', 'F'} };

} // namespace bttf
