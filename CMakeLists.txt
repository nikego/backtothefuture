cmake_minimum_required (VERSION 3.20)
project(BackToTheFuture)

set(PREDEFINED_TARGETS_FOLDER "CustomTargets")

if (ZSTD_DIR)
  find_library(ZSTD_LIB "libzstd_static.lib" PATH_SUFFIXES lib static HINTS "${ZSTD_DIR}")
  find_file(ZSTD_INC "zstd.h" PATH_SUFFIXES include HINTS "${ZSTD_DIR}")

  get_filename_component(ZSTD_INC_DIR ${ZSTD_INC} PATH)

  if (ZSTD_LIB AND ZSTD_INC)
     set(USE_ZSTD 1)
  else()
     set(USE_ZSTD 0)
  endif()

  message(STATUS "zstd lib ${ZSTD_LIB} ${ZSTD_INC_DIR}")
endif()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_VERBOSE ON)

find_package(Boost 1.60.0 COMPONENTS filesystem system date_time program_options log)

set(CPP
   main.cpp
   processor.cpp
   unpacker.cpp
   packer.cpp
   utilities.cpp
   compress.cpp
)

set(HEADERS
   arguments.h
   processor.h
   unpacker.h
   packer.h
   structure.h
   utilities.h
   config.h
   trace.h
   compress.h
)

add_executable( ${PROJECT_NAME} ${CPP} ${HEADERS})

set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")

source_group("" FILES ${CPP} )

source_group("" FILES ${HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE  /ENTRY:mainCRTStartup")

add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0601 -DUSE_ZSTD=${USE_ZSTD})

target_include_directories( ${PROJECT_NAME}
  PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${ZSTD_INC_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})

if (ZSTD_LIB)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${ZSTD_LIB})
endif()
