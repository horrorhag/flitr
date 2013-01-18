PROJECT(FLITr)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.3)


# Set a default build type for single-configuration
# CMake generators if no build type is set.
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
     "Choose the type of build, options are: Debug Release RelWithDebInfo."
     FORCE)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# We have some custom .cmake scripts not in the official distribution.
SET(CMAKE_MODULE_PATH "${FLITr_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")


# append a "d" for debug builds
# This gets inherited by all the libraries, but not by the executables
SET(CMAKE_DEBUG_POSTFIX "d")


# compiler flags
IF(MSVC)
  # VS automatically defines NDEBUG in Release builds 
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _CRT_SECURE_NO_DEPRECATE /D _WIN32_WINNT=0x0501")
ELSEIF(APPLE)
  SET(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures" FORCE)
ELSE()
  #native is only supported by recent g++ compilers
  #SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=native -mtune=native -DNDEBUG")
  SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -Wall")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG -march=native -mtune=native -Wall")
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
ENDIF()


# set up FLITR_3RDPARTY_ROOT. Should be used when all dependencies are in one folder. Some Windows users prefer this.
SET(FLITR_3RDPARTY_ROOT "" CACHE PATH "Additional directory where the 3rd party dependencies can be found.")
SET(CMAKE_PREFIX_PATH ${FLITR_3RDPARTY_ROOT})
MARK_AS_ADVANCED(FORCE ${CMAKE_PREFIX_PATH})


# === find Boost ===
    SET(Boost_USE_STATIC_LIBS OFF CACHE BOOL "Link boost in statically")
    SET(Boost_NO_SYSTEM_PATHS OFF CACHE BOOL "Search only BOOST_ROOT.")
    SET(BOOST_ROOT "" CACHE PATH "Path to search for custom boost library. Empty to use default paths.")
    
    MARK_AS_ADVANCED(FORCE ${CMAKE_PREFIX_PATH})
    #SET(Boost_USE_STATIC_LIBS OFF)
    SET(Boost_USE_MULTITHREADED ON)
    SET(Boost_ADDITIONAL_VERSIONS "1.49")

    FIND_PACKAGE(Boost 1.49 REQUIRED COMPONENTS filesystem system)

    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

    IF (NOT Boost_FOUND)
      MESSAGE(FATAL_ERROR "Cannot find boost, set BOOST_ROOT.")
    ENDIF(NOT Boost_FOUND)
# ==================


SET(OpenSceneGraph_MARK_AS_ADVANCED ON)
  FIND_PACKAGE(OpenSceneGraph 3.0.1 REQUIRED osgViewer osgUtil osgTerrain osgGA osgFX osgDB osgText osgSim osgParticle)
INCLUDE_DIRECTORIES(${OPENSCENEGRAPH_INCLUDE_DIR})
LINK_DIRECTORIES(${OPENSCENEGRAPH_INCLUDE_DIRS}/../bin ${OPENSCENEGRAPH_LIBRARY_DIRS})




# === find FFmpeg ===
    SET(FFmpeg_ROOT "" CACHE PATH "Path to search for custom FFmpeg library. Empty to use default paths.")
    SET(FFmpeg_NO_SYSTEM_PATHS OFF CACHE BOOL "Search only FFmpeg_ROOT.")

    FIND_PACKAGE(FFmpeg)

    IF(FFmpeg_FOUND)
      INCLUDE_DIRECTORIES(${FFmpeg_INCLUDE_DIRS})
    ENDIF()

    #MESSAGE("  FFmpeg_FOUND ${FFmpeg_FOUND}")

    IF(NOT FFmpeg_FOUND)
      MESSAGE(FATAL_ERROR "Cannot find FFmpeg, set FFmpeg_ROOT.")
    ENDIF(NOT FFmpeg_FOUND)
# ===================



OPTION(FLITR_USE_OPENMP "Enable the compiler's OpenMP directives if available." ON)
IF(FLITR_USE_OPENMP)
# === find OpenMP ===
INCLUDE(FindOpenMP)
IF(OPENMP_FOUND)
 SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
 SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
ENDIF()
# ===================
ENDIF(FLITR_USE_OPENMP)



#default include and lib directories.
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR};${CMAKE_SOURCE_DIR}/include;/usr/local/include;/opt/local/include;${AVFORMAT_PATH};${SWSCALE_PATH};${FLITR_3RDPARTY_ROOT}/include/)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR} /usr/local/lib /opt/local/lib ${FLITR_3RDPARTY_ROOT}/lib ${FLITR_3RDPARTY_ROOT}/bin)


OPTION(FLITR_PROFILE "Enable profiling code." ON)
OPTION(FLITR_WITH_OSGCUDA "Use osgCuda textures in the OSG consumer. Assume osgCuda and CUDA are installed." OFF)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/include/flitr/flitr_config.h.in ${CMAKE_SOURCE_DIR}/include/flitr/flitr_config.h)


IF(FLITR_WITH_OSGCUDA)
FIND_PACKAGE(CUDA 4.0 REQUIRED)
ENDIF()


# to get e.g. UINT64_C or UINT64_MAX from stdint.h
ADD_DEFINITIONS(-D__STDC_CONSTANT_MACROS)


SET(LIB_SOURCES
  src/flitr/hud.cpp
  src/flitr/crosshair_overlay.cpp
  src/flitr/ffmpeg_producer.cpp
  src/flitr/test_pattern_producer.cpp
  src/flitr/smulti_ffmpeg_producer.cpp
  src/flitr/ffmpeg_reader.cpp
  src/flitr/ffmpeg_utils.cpp
  src/flitr/ffmpeg_writer.cpp
  src/flitr/geometry_overlay.cpp
  src/flitr/high_resolution_time.cpp
  src/flitr/image_consumer.cpp
  src/flitr/image_processor.cpp
  src/flitr/log_message.cpp
  src/flitr/manipulator_utils.cpp
  src/flitr/metadata_writer.cpp
  src/flitr/metadata_reader.cpp
  src/flitr/multi_cpuhistogram_consumer.cpp
  src/flitr/multi_ffmpeg_consumer.cpp
  src/flitr/multi_ffmpeg_producer.cpp
  src/flitr/multi_osg_consumer.cpp
  src/flitr/ortho_texture_manipulator.cpp
  src/flitr/points_overlay.cpp
  src/flitr/quad_overlay.cpp
  src/flitr/plot2D_overlay.cpp
  src/flitr/shared_image_buffer.cpp
  src/flitr/textured_quad.cpp

  src/flitr/modules/cpu_shader_passes/cpu_shader_pass.cpp

  src/flitr/modules/lucas_kanade/ImageMultiPyramid.cpp
  src/flitr/modules/lucas_kanade/ImageStabiliserMultiLK.cpp
  #src/flitr/modules/lucas_kanade/ImageBiPyramid.cpp
  #src/flitr/modules/lucas_kanade/ImageStabiliserBiLK.cpp

  src/flitr/modules/target_injector/target_injector.cpp
  src/flitr/modules/de_motion_blur/de_motion_blur.cpp

  src/flitr/modules/glsl_shader_passes/glsl_shader_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_crop_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_translate_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_downsample_pass.cpp
  src/flitr/modules/glsl_shader_passes/homography_shader_pass.cpp

  #find include/ | grep .h | grep -v .svn | sort
  include/flitr/hud.h
  include/flitr/crosshair_overlay.h
  include/flitr/ffmpeg_producer.h
  include/flitr/test_pattern_producer.h
  include/flitr/smulti_ffmpeg_producer.h
  include/flitr/ffmpeg_reader.h
  include/flitr/ffmpeg_utils.h
  include/flitr/ffmpeg_writer.h
  include/flitr/flitr_export.h
  include/flitr/flitr_stdint.h
  include/flitr/geometry_overlay.h
  include/flitr/high_resolution_time.h
  include/flitr/image_consumer.h
  include/flitr/image_processor.h
  include/flitr/image_format.h
  include/flitr/image.h
  include/flitr/image_metadata.h
  include/flitr/image_producer.h
  include/flitr/log_message.h
  include/flitr/manipulator_utils.h
  include/flitr/metadata_writer.h
  include/flitr/metadata_reader.h
  include/flitr/multi_cpuhistogram_consumer.h
  include/flitr/multi_ffmpeg_consumer.h
  include/flitr/multi_ffmpeg_producer.h
  include/flitr/multi_osg_consumer.h
  include/flitr/ortho_texture_manipulator.h
  include/flitr/points_overlay.h
  include/flitr/quad_overlay.h
  include/flitr/plot2D_overlay.h
  include/flitr/shared_image_buffer.h
  include/flitr/stats_collector.h
  include/flitr/textured_quad.h

  include/flitr/modules/cpu_shader_passes/cpu_shader_pass.h
  include/flitr/modules/cpu_shader_passes/cpu_palette_remap_shader.h
  include/flitr/modules/cpu_shader_passes/cpu_photometric_equalisation.h

  include/flitr/modules/lucas_kanade/ImageMultiPyramid.h
  include/flitr/modules/lucas_kanade/ImageStabiliserMultiLK.h
  #include/flitr/modules/lucas_kanade/ImageBiPyramid.h
  #include/flitr/modules/lucas_kanade/ImageStabiliserBiLK.h

  include/flitr/modules/target_injector/target_injector.h
  include/flitr/modules/de_motion_blur/de_motion_blur.h

  include/flitr/modules/glsl_shader_passes/glsl_shader_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_crop_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_translate_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_downsample_pass.h
  include/flitr/modules/glsl_shader_passes/homography_shader_pass.h
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


IF(MSVC) 
  TARGET_LINK_LIBRARIES(flitr ${Boost_LIBRARIES} opengl32 OpenThreads ${OPENSCENEGRAPH_LIBRARIES} ${FFmpeg_LIBRARIES})
ELSEIF(APPLE)
  TARGET_LINK_LIBRARIES(flitr ${Boost_LIBRARIES} OpenThreads ${OPENSCENEGRAPH_LIBRARIES} ${FFmpeg_LIBRARIES})
ELSE()
TARGET_LINK_LIBRARIES(flitr ${Boost_LIBRARIES} m rt)
TARGET_LINK_LIBRARIES(flitr ${FFmpeg_LIBRARIES})

TARGET_LINK_LIBRARIES(flitr ${OPENSCENEGRAPH_LIBRARIES})
#TARGET_LINK_LIBRARIES(flitr optimised OpenThreads osgViewerd osgUtild osgTerraind osgGAd osgFXd osgDBd osgTextd osgSimd osgParticled)
#TARGET_LINK_LIBRARIES(flitr debug OpenThreadsd osgViewerd osgUtild osgTerraind osgGAd osgFXd osgDBd osgTextd osgSimd osgParticled)
ENDIF()


IF(FLITR_WITH_OSGCUDA)
  TARGET_LINK_LIBRARIES(flitr ${TARGET_LINK_LIBRARIES} osgCuda)
ENDIF()


SET_TARGET_PROPERTIES(flitr PROPERTIES COMPILE_FLAGS "-DFLITR_SHARED_LIBRARY")


ADD_SUBDIRECTORY(tests/shared_image_buffer)
ADD_SUBDIRECTORY(tests/ffmpeg_producer)


ADD_SUBDIRECTORY(examples/target_injector)
ADD_SUBDIRECTORY(examples/de_motion_blur)
ADD_SUBDIRECTORY(examples/LK_image_registration)
ADD_SUBDIRECTORY(examples/viewer_video_quad)
ADD_SUBDIRECTORY(examples/viewer_video_history_quad)
ADD_SUBDIRECTORY(examples/viewer_multi_video_quad)
ADD_SUBDIRECTORY(examples/glsl_shader_pass)
ADD_SUBDIRECTORY(examples/cpu_shader_pass)
ADD_SUBDIRECTORY(examples/ortho_pick_overlay)
ADD_SUBDIRECTORY(examples/keep_history_pass)
ADD_SUBDIRECTORY(examples/points_overlay)
#ADD_SUBDIRECTORY(examples/targeting_task_performance)
ADD_SUBDIRECTORY(examples/hud_overlay)


#Need to replace the _gl.. calls with the relevant osg calls before the below examples will compile under OSX.
IF(NOT APPLE)
  ADD_SUBDIRECTORY(examples/screen_capture)
ENDIF()


IF(FLITR_WITH_OSGCUDA)
  #ADD_SUBDIRECTORY(examples/cuda_auto_contrast)
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
#INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/share" DESTINATION ${CMAKE_INSTALL_PREFIX})


# for include files install
ADD_SUBDIRECTORY(include)


OPTION(FLITR_WITH_DOXYGEN "Generate the doxygen documentation." OFF)
IF(FLITR_WITH_DOXYGEN)
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
	configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(Doxygen ALL ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generate Doxygen documentation." VERBATIM)
  else(DOXYGEN_FOUND)
        MESSAGE(WARNING "Doxygen not found!")
  endif(DOXYGEN_FOUND)

  ENDIF(FLITR_WITH_DOXYGEN)


