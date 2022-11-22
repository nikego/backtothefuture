#pragma once

#include "config.h"

#define BTTF_TRACE() if (bttf::g_config.severity_level == boost::log::trivial::trace)   BOOST_LOG_TRIVIAL(trace)
#define BTTF_DEBUG() if (bttf::g_config.severity_level <= boost::log::trivial::debug)   BOOST_LOG_TRIVIAL(debug)
#define BTTF_INFO()  if (bttf::g_config.severity_level <= boost::log::trivial::info)    BOOST_LOG_TRIVIAL(info)
#define BTTF_WARN()  if (bttf::g_config.severity_level <= boost::log::trivial::warning) BOOST_LOG_TRIVIAL(warning)
#define BTTF_ERROR() if (bttf::g_config.severity_level <= boost::log::trivial::error)   BOOST_LOG_TRIVIAL(error)
#define BTTF_FATAL()                                                                    BOOST_LOG_TRIVIAL(fatal)
