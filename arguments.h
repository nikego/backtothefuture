#pragma once

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <boost/log/trivial.hpp>

#include <iostream>

namespace bttf {

namespace lt = boost::log::trivial;

struct arguments_t
{
   boost::optional<int> operator()(int argc, char* argv[])
   {
      namespace po = boost::program_options;

      po::options_description description("BackToTheFuture application. Allowed options");

      description.add_options()
         ("help",                                                                      "produce help message")
         ("input,i",          po::value(&input)->required(),                           "input folder to pack or archive file to unpack")
         ("output,o",         po::value(&output)->required(),                          "output archive file or folder to unpack")
         ("compression-level,l", po::value(&compression_level)->default_value(0),      "compression level 0..9 (0 - no compression), used only with zstd")
         ("severity-level,s", po::value(&severity_level)->default_value(lt::warning),  "severity level for output : one of 'trace','debug','info','warning','error','fatal'")
         ("test-unpack,t",    po::value(&test_unpack)->implicit_value(true),           "unpack archive after packing and compare result with source")
         ;

      po::variables_map vm;

      try
      {
         po::store(po::parse_command_line(argc, argv, description), vm);
         po::notify(vm);

         if (vm.count("help"))
         {
            std::cout << description;
            return EXIT_SUCCESS;
         }

         if (compression_level > 9 || compression_level < 0)
         {
            std::cout << "compression level must be 0..9" << std::endl;
            std::cout << description;
            return EXIT_FAILURE;
         }
      }
      catch (const std::exception& e)
      {
         std::cerr << description;
         return EXIT_FAILURE;
      }
      return boost::none;
   }

   std::string input;
   std::string output;
   int compression_level = 0;
   lt::severity_level severity_level;
   bool test_unpack = false;
};

}
