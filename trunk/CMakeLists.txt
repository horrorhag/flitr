PROJECT(FLITr)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6.3)

# Set a default build type for single-configuration
# CMake generators if no build type is set.
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
     "Choose the type of build, options are: Debug Release RelWithDebInfo."
     FORCE)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# compiler flags
IF(WIN32)
 # VS automatically defines NDEBUG in Release builds 
 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _CRT_SECURE_NO_DEPRECATE /D _WIN32_WINNT=0x0501")
ELSE(WIN32)
 #native is only supported by recent g++ compilers
 #SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=native -mtune=native -DNDEBUG")
 SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
 SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG -march=native -mtune=native")
 SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
ENDIF(WIN32)

FIND_PATH(AVFORMAT_PATH avformat.h PATHS /usr/local/include/libavformat /usr/include/ffmpeg /usr/include/ffmpeg/libavformat /usr/include/libavformat /opt/local/include/libavformat)
#MESSAGE("avformat.h found here: ${AVFORMAT_PATH}")

FIND_PATH(SWSCALE_PATH swscale.h PATHS /usr/local/include/libswscale /usr/include/ffmpeg /usr/include/ffmpeg/libswscale /usr/include/libswscale /opt/local/include/libswscale)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include;/usr/local/include;${AVFORMAT_PATH};${SWSCALE_PATH})

OPTION(FLITR_PROFILE "Enable profiling code" ON)
IF(FLITR_PROFILE)
  ADD_DEFINITIONS(-DFLITR_PROFILE)
ENDIF(FLITR_PROFILE)

# to get e.g. UINT64_C or UINT64_MAX from stdint.h
ADD_DEFINITIONS(-D__STDC_CONSTANT_MACROS)

SET(LIB_SOURCES
  src/flitr/ffmpeg_utils.cpp
  src/flitr/log_message.cpp
  src/flitr/ffmpeg_reader.cpp
  src/flitr/ffmpeg_writer.cpp
  src/flitr/metadata_writer.cpp
  src/flitr/ffmpeg_producer.cpp
  src/flitr/shared_image_buffer.cpp
  src/flitr/image_consumer.cpp
  src/flitr/multi_osg_consumer.cpp
  src/flitr/textured_quad.cpp
  src/flitr/multi_ffmpeg_producer.cpp
  src/flitr/multi_ffmpeg_consumer.cpp
  src/flitr/high_resolution_time.cpp
  src/flitr/manipulator_utils.cpp
  src/flitr/ortho_texture_manipulator.cpp
  src/flitr/geometry_overlay.cpp
  src/flitr/quad_overlay.cpp
  src/flitr/crosshair_overlay.cpp
  
  include/flitr/log_message.h
  include/flitr/vs_stdint.h
  include/flitr/image.h
  include/flitr/image_consumer.h
  include/flitr/ffmpeg_reader.h
  include/flitr/ffmpeg_writer.h
  include/flitr/metadata_writer.h
  include/flitr/ffmpeg_producer.h
  include/flitr/shared_image_buffer.h
  include/flitr/flitr_stdint.h
  include/flitr/ffmpeg_utils.h
  include/flitr/image_producer.h
  include/flitr/multi_osg_consumer.h
  include/flitr/textured_quad.h
  include/flitr/multi_ffmpeg_producer.h
  include/flitr/multi_ffmpeg_consumer.h
  include/flitr/high_resolution_time.h
  include/flitr/stats_collector.h
  include/flitr/manipulator_utils.h
  include/flitr/ortho_texture_manipulator.h
  include/flitr/geometry_overlay.h
  include/flitr/quad_overlay.h
  include/flitr/crosshair_overlay.h

)

ADD_LIBRARY(flitr SHARED
  ${LIB_SOURCES}
)
TARGET_LINK_LIBRARIES(flitr m rt OpenThreads osg osgViewer osgDB osgGA avformat avutil avcodec swscale)

ADD_SUBDIRECTORY(tests/shared_image_buffer)
ADD_SUBDIRECTORY(tests/ffmpeg_producer)

ADD_SUBDIRECTORY(examples/viewer_video_quad)
ADD_SUBDIRECTORY(examples/viewer_video_history_quad)
ADD_SUBDIRECTORY(examples/viewer_multi_video_quad)
ADD_SUBDIRECTORY(examples/glsl_shader_pass)
ADD_SUBDIRECTORY(examples/ortho_pick_overlay)

#install
INSTALL(
  TARGETS flitr
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib/static
)
# for include files install
ADD_SUBDIRECTORY(include)
