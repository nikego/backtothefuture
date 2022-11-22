#pragma once

#include <boost/log/trivial.hpp>
#include <boost/filesystem/path.hpp>

namespace bttf {

struct config_t
{
   boost::log::trivial::severity_level severity_level;
   int compression_level = 0;
};

extern config_t g_config;

} // namespace bttf
