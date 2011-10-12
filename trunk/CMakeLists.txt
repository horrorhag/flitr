PROJECT(FLITr)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6.3)

# Set a default build type for single-configuration
# CMake generators if no build type is set.
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
     "Choose the type of build, options are: Debug Release RelWithDebInfo."
     FORCE)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

SET(FLITR_3RDPARTY_ROOT "" CACHE PATH "Root directory where the 3rd party dependencies can be found")

# compiler flags
IF(WIN32)
  # VS automatically defines NDEBUG in Release builds 
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _CRT_SECURE_NO_DEPRECATE /D _WIN32_WINNT=0x0501")
ELSEIF(APPLE)
  SET(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures…" FORCE)
ELSE()
  #native is only supported by recent g++ compilers
  #SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=native -mtune=native -DNDEBUG")
  SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -Wall")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG -march=native -mtune=native -Wall")
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
ENDIF()

SET(CMAKE_PREFIX_PATH ${FLITR_3RDPARTY_ROOT})
MARK_AS_ADVANCED(FORCE ${CMAKE_PREFIX_PATH})

SET(OpenSceneGraph_MARK_AS_ADVANCED ON)
FIND_PACKAGE(OpenSceneGraph 2.9.7 REQUIRED osgViewer osgUtil osgTerrain osgGA osgFX osgDB osgText osgSim osgParticle)

FIND_PATH(AVFORMAT_PATH avformat.h PATHS /usr/local/include/libavformat /usr/include/ffmpeg /usr/include/ffmpeg/libavformat /usr/include/libavformat /opt/local/include/libavformat ${FLITR_3RDPARTY_ROOT}/include/libavformat)
#MESSAGE("avformat.h found here: ${AVFORMAT_PATH}")

FIND_PATH(SWSCALE_PATH swscale.h PATHS /usr/local/include/libswscale /usr/include/ffmpeg /usr/include/ffmpeg/libswscale /usr/include/libswscale /opt/local/include/libswscale ${FLITR_3RDPARTY_ROOT}/include/libswscale)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR};${CMAKE_SOURCE_DIR}/include;/usr/local/include;/opt/local/include;${AVFORMAT_PATH};${SWSCALE_PATH};${FLITR_3RDPARTY_ROOT}/include/)

LINK_DIRECTORIES(${CMAKE_SOURCE_DIR} /usr/local/lib /opt/local/lib ${FLITR_3RDPARTY_ROOT}/lib ${FLITR_3RDPARTY_ROOT}/bin)

OPTION(FLITR_PROFILE "Enable profiling code" ON)
IF(FLITR_PROFILE)
  ADD_DEFINITIONS(-DFLITR_PROFILE)
ENDIF(FLITR_PROFILE)

OPTION(FLITR_WITH_OSGCUDA "Use osgCuda textures in the OSG consumer. Assume osgCuda and CUDA is installed." OFF)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/include/flitr/flitr_config.h.in ${CMAKE_SOURCE_DIR}/include/flitr/flitr_config.h)

IF(FLITR_WITH_OSGCUDA)
FIND_PACKAGE(CUDA 4.0 REQUIRED)
ENDIF()

# to get e.g. UINT64_C or UINT64_MAX from stdint.h
ADD_DEFINITIONS(-D__STDC_CONSTANT_MACROS)

SET(LIB_SOURCES
  src/flitr/crosshair_overlay.cpp
  src/flitr/ffmpeg_producer.cpp
  src/flitr/ffmpeg_reader.cpp
  src/flitr/ffmpeg_utils.cpp
  src/flitr/ffmpeg_writer.cpp
  src/flitr/geometry_overlay.cpp
  src/flitr/high_resolution_time.cpp
  src/flitr/image_consumer.cpp
  src/flitr/log_message.cpp
  src/flitr/manipulator_utils.cpp
  src/flitr/metadata_writer.cpp
  src/flitr/multi_ffmpeg_consumer.cpp
  src/flitr/multi_ffmpeg_producer.cpp
  src/flitr/multi_osg_consumer.cpp
  src/flitr/ortho_texture_manipulator.cpp
  src/flitr/points_overlay.cpp
  src/flitr/quad_overlay.cpp
  src/flitr/shared_image_buffer.cpp
  src/flitr/textured_quad.cpp
  
  #find include/ | grep .h | grep -v .svn | sort
  include/flitr/crosshair_overlay.h
  include/flitr/ffmpeg_producer.h
  include/flitr/ffmpeg_reader.h
  include/flitr/ffmpeg_utils.h
  include/flitr/ffmpeg_writer.h
  include/flitr/flitr_export.h
  include/flitr/flitr_stdint.h
  include/flitr/geometry_overlay.h
  include/flitr/high_resolution_time.h
  include/flitr/image_consumer.h
  include/flitr/image_format.h
  include/flitr/image.h
  include/flitr/image_metadata.h
  include/flitr/image_producer.h
  include/flitr/log_message.h
  include/flitr/manipulator_utils.h
  include/flitr/metadata_writer.h
  include/flitr/multi_ffmpeg_consumer.h
  include/flitr/multi_ffmpeg_producer.h
  include/flitr/multi_osg_consumer.h
  include/flitr/ortho_texture_manipulator.h
  include/flitr/points_overlay.h
  include/flitr/quad_overlay.h
  include/flitr/shared_image_buffer.h
  include/flitr/stats_collector.h
  include/flitr/textured_quad.h
)

#Need to replace the _gl.. calls with the relevant osg calls before the below files will compile under OSX.
IF(NOT APPLE)
  SET(LIB_SOURCES
    ${LIB_SOURCES}
    src/flitr/screen_capture_producer.cpp
    src/flitr/texture_capture_producer.cpp
    include/flitr/screen_capture_producer.h
    include/flitr/texture_capture_producer.h
  )
ENDIF()

ADD_LIBRARY(flitr SHARED
  ${LIB_SOURCES}
)

IF(WIN32) 
  TARGET_LINK_LIBRARIES(flitr opengl32 OpenThreads osg osgViewer osgDB osgGA avformat avutil avcodec swscale)
ELSEIF(APPLE)
  TARGET_LINK_LIBRARIES(flitr OpenThreads osg osgViewer osgDB osgGA avformat avutil avcodec swscale)
ELSE()
TARGET_LINK_LIBRARIES(flitr m rt OpenThreads osg osgViewer osgDB osgGA avformat avutil avcodec swscale)
ENDIF()

IF(FLITR_WITH_OSGCUDA)
  TARGET_LINK_LIBRARIES(flitr ${TARGET_LINK_LIBRARIES} osgCuda)
ENDIF()

SET_TARGET_PROPERTIES(flitr PROPERTIES COMPILE_FLAGS "-DFLITR_SHARED_LIBRARY")

ADD_SUBDIRECTORY(tests/shared_image_buffer)
ADD_SUBDIRECTORY(tests/ffmpeg_producer)

ADD_SUBDIRECTORY(examples/viewer_video_quad)
ADD_SUBDIRECTORY(examples/viewer_video_history_quad)
ADD_SUBDIRECTORY(examples/viewer_multi_video_quad)
ADD_SUBDIRECTORY(examples/glsl_shader_pass)
ADD_SUBDIRECTORY(examples/cpu_shader_pass)
ADD_SUBDIRECTORY(examples/ortho_pick_overlay)
ADD_SUBDIRECTORY(examples/keep_history_pass)
ADD_SUBDIRECTORY(examples/points_overlay)
#Need to replace the _gl.. calls with the relevant osg calls before the below examples will compile under OSX.
IF(NOT APPLE)
  ADD_SUBDIRECTORY(examples/screen_capture)
ENDIF()
IF(FLITR_WITH_OSGCUDA)
  ADD_SUBDIRECTORY(examples/cuda_auto_contrast)
ENDIF()

# Add postfix if on 64-bit
IF(UNIX AND NOT WIN32 AND NOT APPLE)
  IF(EXISTS "${CMAKE_INSTALL_PREFIX}/lib64/" AND CMAKE_SIZEOF_VOID_P EQUAL 8 )
    SET(LIB_INSTALL_POSTFIX "64" CACHE STRING "Install suffix for library installation dir")
    MARK_AS_ADVANCED(LIB_INSTALL_POSTFIX)
  ENDIF()
ENDIF()

#default
IF(NOT DEFINED LIB_INSTALL_POSTFIX)
    SET(LIB_INSTALL_POSTFIX "")
ENDIF()

#install
INSTALL(
  TARGETS flitr
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib${LIB_INSTALL_POSTFIX}
  ARCHIVE DESTINATION lib${LIB_INSTALL_POSTFIX}/static
)
# for include files install
ADD_SUBDIRECTORY(include)
