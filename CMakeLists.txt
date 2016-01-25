PROJECT(FLITr)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.3)


IF(UNIX AND NOT WIN32 AND NOT APPLE)
	IF(EXISTS "${CMAKE_INSTALL_PREFIX}/lib64/" AND CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)
	ENDIF()
ENDIF(UNIX AND NOT WIN32 AND NOT APPLE)


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
  add_definitions(-DWIN32)
  add_definitions(-D__STDC_LIMIT_MACROS)
  # VS automatically defines NDEBUG in Release builds 
  # SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _CRT_SECURE_NO_DEPRECATE /D _WIN32_WINNT=0x0501")
ELSEIF(APPLE)
  SET(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures" FORCE)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_C_LANGUAGE_STANDARD "c11")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_C_LIBRARY "libc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++ -g -Wall")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -g -Wall")
ELSE()
  #native is only supported by recent g++ compilers
  #SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=native -mtune=native -DNDEBUG")
  SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -Wall -std=c++11")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG -march=native -mtune=native -Wall -std=c++11")
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++11")
ENDIF()


# set up FLITR_3RDPARTY_ROOT. Should be used when all dependencies are in one folder. Some Windows users prefer this.
SET(FLITR_3RDPARTY_ROOT "" CACHE PATH "Additional directory where the 3rd party dependencies can be found.")
SET(CMAKE_PREFIX_PATH ${FLITR_3RDPARTY_ROOT})
MARK_AS_ADVANCED(FORCE ${CMAKE_PREFIX_PATH})


# === find Boost ===
    #SET(Boost_USE_STATIC_LIBS OFF CACHE BOOL "Link boost in statically")
    #SET(Boost_NO_SYSTEM_PATHS OFF CACHE BOOL "Search only BOOST_ROOT.")
    #SET(BOOST_ROOT "" CACHE PATH "Path to search for custom boost library. Empty to use default paths.")
    
    #MARK_AS_ADVANCED(FORCE ${CMAKE_PREFIX_PATH})
    #SET(Boost_USE_STATIC_LIBS OFF)
    #SET(Boost_USE_MULTITHREADED ON)
    #SET(Boost_ADDITIONAL_VERSIONS "1.49")

    #FIND_PACKAGE(Boost 1.46 REQUIRED COMPONENTS filesystem system)

    #INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
    #LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

    #IF (NOT Boost_FOUND)
    #  MESSAGE(FATAL_ERROR "Cannot find boost, set BOOST_ROOT.")
    #ENDIF(NOT Boost_FOUND)
# ==================


# === Find OSG ===#
SET(OpenSceneGraph_MARK_AS_ADVANCED ON)
FIND_PACKAGE(OpenSceneGraph REQUIRED osgViewer osgUtil osgTerrain osgGA osgFX osgDB osgText osgSim osgParticle)
INCLUDE_DIRECTORIES(${OPENSCENEGRAPH_INCLUDE_DIR})
LINK_DIRECTORIES(${OPENSCENEGRAPH_INCLUDE_DIRS}/../bin ${OPENSCENEGRAPH_LIBRARY_DIRS})
#=================#


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
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    ADD_DEFINITIONS(-DUSE_OPENMP)
endif()
# ===================
ENDIF(FLITR_USE_OPENMP)


#default include and lib directories.
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR};${CMAKE_SOURCE_DIR}/include;/usr/local/include;/opt/local/include;${AVFORMAT_PATH};${SWSCALE_PATH};${FLITR_3RDPARTY_ROOT}/include/)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR} /usr/local/lib /opt/local/lib ${FLITR_3RDPARTY_ROOT}/lib ${FLITR_3RDPARTY_ROOT}/bin)


OPTION(FLITR_PROFILE "Enable profiling code." ON)
OPTION(FLITR_WITH_OSGCUDA "Use osgCuda textures in the OSG consumer. Assume osgCuda and CUDA are installed." OFF)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/include/flitr/flitr_config.h.in ${CMAKE_SOURCE_DIR}/include/flitr/flitr_config.h)


IF(FLITR_WITH_OSGCUDA)
        # uses FindCUDA from CMake
        FIND_PACKAGE(CUDA 4.0 REQUIRED)
        LINK_DIRECTORIES(/usr/local/cuda/lib64)
        INCLUDE_DIRECTORIES(${INCLUDE_DIRECTORIES}/C/common/inc)
ENDIF()


# to get e.g. UINT64_C or UINT64_MAX from stdint.h
ADD_DEFINITIONS(-D__STDC_CONSTANT_MACROS)


SET(LIB_SOURCES
  src/flitr/multi_raw_video_file_consumer.cpp
  src/flitr/raw_video_file_writer.cpp
  src/flitr/raw_video_file_reader.cpp
  src/flitr/raw_video_file_producer.cpp
  src/flitr/ffmpeg_producer.cpp
  src/flitr/test_pattern_producer.cpp
  src/flitr/smulti_ffmpeg_producer.cpp
  src/flitr/smulti_raw_video_file_producer.cpp
  src/flitr/ffmpeg_reader.cpp
  src/flitr/ffmpeg_utils.cpp
  src/flitr/ffmpeg_writer.cpp
  src/flitr/high_resolution_time.cpp
  src/flitr/image_consumer.cpp
  src/flitr/image_processor.cpp
  src/flitr/image_processor_utils.cpp
  src/flitr/hud.cpp
  src/flitr/log_message.cpp
  src/flitr/manipulator_utils.cpp
  src/flitr/metadata_writer.cpp
  src/flitr/metadata_reader.cpp
  src/flitr/multi_cpuhistogram_consumer.cpp
  src/flitr/multi_ffmpeg_consumer.cpp
  src/flitr/multi_ffmpeg_producer.cpp
  src/flitr/multi_osg_consumer.cpp
  src/flitr/ortho_texture_manipulator.cpp
  src/flitr/image_multiplexer.cpp

  src/flitr/modules/geometry_overlays/crosshair_overlay.cpp
  src/flitr/modules/geometry_overlays/geometry_overlay.cpp
  src/flitr/modules/geometry_overlays/points_overlay.cpp
  src/flitr/modules/geometry_overlays/quad_overlay.cpp
  src/flitr/modules/geometry_overlays/plot2D_overlay.cpp
  src/flitr/modules/geometry_overlays/text_label_overlay.cpp

  src/flitr/shared_image_buffer.cpp
  src/flitr/textured_quad.cpp

  src/flitr/modules/cpu_shader_passes/cpu_shader_pass.cpp
  src/flitr/modules/cpu_shader_passes/cpu_find_discreet_objects_pass.cpp
  
  src/flitr/modules/lucas_kanade/ImageMultiPyramid.cpp
  src/flitr/modules/lucas_kanade/ImageStabiliserMultiLK.cpp
  #src/flitr/modules/lucas_kanade/ImageBiPyramid.cpp
  #src/flitr/modules/lucas_kanade/ImageStabiliserBiLK.cpp

  src/flitr/modules/target_injector/target_injector.cpp
  src/flitr/modules/de_motion_blur/de_motion_blur.cpp

  src/flitr/modules/flitr_image_processors/transform2D/fip_transform2D.cpp
  src/flitr/modules/flitr_image_processors/flip/fip_flip.cpp
  src/flitr/modules/flitr_image_processors/rotate/fip_rotate.cpp
  src/flitr/modules/flitr_image_processors/dpt/fip_dpt.cpp
  src/flitr/modules/flitr_image_processors/crop/fip_crop.cpp
  src/flitr/modules/flitr_image_processors/msr/fip_msr.cpp
  src/flitr/modules/flitr_image_processors/tonemap/fip_tonemap.cpp
  src/flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_y_f32.cpp
  src/flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_y_8.cpp
  src/flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_rgb_f32.cpp
  src/flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_rgb_8.cpp
  src/flitr/modules/flitr_image_processors/gradient_image/fip_gradient_image.cpp
  src/flitr/modules/flitr_image_processors/average_image/fip_average_image.cpp
  src/flitr/modules/flitr_image_processors/average_image/fip_average_image_iir.cpp
  src/flitr/modules/flitr_image_processors/beat_image/fip_beat_image.cpp
  src/flitr/modules/flitr_image_processors/photometric_equalise/fip_photometric_equalise.cpp
  src/flitr/modules/flitr_image_processors/gaussian_downsample/fip_gaussian_downsample.cpp
  src/flitr/modules/flitr_image_processors/gaussian_filter/fip_gaussian_filter.cpp
  src/flitr/modules/flitr_image_processors/unsharp_mask/fip_unsharp_mask.cpp
  src/flitr/modules/flitr_image_processors/dewarp/fip_lk_dewarp.cpp
  src/flitr/modules/flitr_image_processors/dewarp/fip_lk_dewarp_ex.cpp
  src/flitr/modules/flitr_image_processors/stabilise/fip_lk_stabilise.cpp
  src/flitr/modules/flitr_image_processors/simulate/fip_camera_shake.cpp
  src/flitr/modules/flitr_image_processors/simulate/fip_test_pattern.cpp

  src/flitr/modules/glsl_shader_passes/glsl_masked_gain_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_shader_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_gamma_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_sigmoid_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_copy_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_crop_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_translate_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_downsample_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_downsample_x_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_downsample_y_pass.cpp
  src/flitr/modules/glsl_shader_passes/homography_shader_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_gaussian_filter_x_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_gaussian_filter_y_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_keep_history_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_threshold_pass.cpp
  src/flitr/modules/glsl_shader_passes/glsl_grayscale_pass.cpp

  #find include/ | grep .h | grep -v .svn | sort
  include/flitr/raw_video_file_utils.h
  include/flitr/multi_raw_video_file_consumer.h
  include/flitr/raw_video_file_writer.h
  include/flitr/raw_video_file_reader.h
  include/flitr/raw_video_file_producer.h
  include/flitr/hud.h
  include/flitr/ffmpeg_producer.h
  include/flitr/test_pattern_producer.h
  include/flitr/smulti_ffmpeg_producer.h
  include/flitr/smulti_raw_video_file_producer.h
  include/flitr/ffmpeg_reader.h
  include/flitr/ffmpeg_utils.h
  include/flitr/ffmpeg_writer.h
  include/flitr/flitr_export.h
  include/flitr/flitr_stdint.h
  include/flitr/high_resolution_time.h
  include/flitr/image_consumer.h
  include/flitr/image_processor.h
  include/flitr/image_processor_utils.h
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
  include/flitr/image_multiplexer.h

  include/flitr/modules/parameters/parameters.h

  include/flitr/modules/geometry_overlays/crosshair_overlay.h
  include/flitr/modules/geometry_overlays/geometry_overlay.h
  include/flitr/modules/geometry_overlays/points_overlay.h
  include/flitr/modules/geometry_overlays/quad_overlay.h
  include/flitr/modules/geometry_overlays/plot2D_overlay.h
  include/flitr/modules/geometry_overlays/text_label_overlay.h

  include/flitr/shared_image_buffer.h
  include/flitr/stats_collector.h
  include/flitr/textured_quad.h

  # Video
  include/flitr/video_producer.h
  # include/flitr/video_linux_v4l.h
  include/flitr/video_win32_directshow.h

  include/flitr/modules/cpu_shader_passes/cpu_shader_pass.h
  include/flitr/modules/cpu_shader_passes/cpu_palette_remap_shader.h
  include/flitr/modules/cpu_shader_passes/cpu_photometric_equalisation.h
  include/flitr/modules/cpu_shader_passes/cpu_local_photometric_equalisation.h
  include/flitr/modules/cpu_shader_passes/cpu_uniform_noise_shader.h
  include/flitr/modules/cpu_shader_passes/cpu_find_discreet_objects_pass.h

  include/flitr/modules/lucas_kanade/ImageMultiPyramid.h
  include/flitr/modules/lucas_kanade/ImageStabiliserMultiLK.h
  #include/flitr/modules/lucas_kanade/ImageBiPyramid.h
  #include/flitr/modules/lucas_kanade/ImageStabiliserBiLK.h

  include/flitr/modules/target_injector/target_injector.h
  include/flitr/modules/de_motion_blur/de_motion_blur.h

  include/flitr/modules/flitr_image_processors/transform2D/fip_transform2D.h
  include/flitr/modules/flitr_image_processors/flip/fip_flip.h
  include/flitr/modules/flitr_image_processors/rotate/fip_rotate.h
  include/flitr/modules/flitr_image_processors/dpt/fip_dpt.h
  include/flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_y_f32.h
  include/flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_y_8.h
  include/flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_rgb_f32.h
  include/flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_rgb_8.h
  include/flitr/modules/flitr_image_processors/tonemap/fip_tonemap.h
  include/flitr/modules/flitr_image_processors/msr/fip_msr.h
  include/flitr/modules/flitr_image_processors/crop/fip_crop.h
  include/flitr/modules/flitr_image_processors/gradient_image/fip_gradient_image.h
  include/flitr/modules/flitr_image_processors/average_image/fip_average_image.h
  include/flitr/modules/flitr_image_processors/average_image/fip_average_image_iir.h
  include/flitr/modules/flitr_image_processors/beat_image/fip_beat_image.h
  include/flitr/modules/flitr_image_processors/photometric_equalise/fip_photometric_equalise.h
  include/flitr/modules/flitr_image_processors/gaussian_downsample/fip_gaussian_downsample.h
  include/flitr/modules/flitr_image_processors/gaussian_filter/fip_gaussian_filter.h
  include/flitr/modules/flitr_image_processors/unsharp_mask/fip_unsharp_mask.h
  include/flitr/modules/flitr_image_processors/dewarp/fip_lk_dewarp.h
  include/flitr/modules/flitr_image_processors/dewarp/fip_lk_dewarp_ex.h
  include/flitr/modules/flitr_image_processors/stabilise/fip_lk_stabilise.h
  include/flitr/modules/flitr_image_processors/simulate/fip_camera_shake.h
  include/flitr/modules/flitr_image_processors/simulate/fip_test_pattern.h

  include/flitr/modules/glsl_shader_passes/glsl_masked_gain_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_shader_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_gamma_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_sigmoid_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_copy_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_crop_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_translate_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_downsample_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_downsample_x_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_downsample_y_pass.h
  include/flitr/modules/glsl_shader_passes/homography_shader_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_gaussian_filter_x_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_gaussian_filter_y_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_keep_history_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_threshold_pass.h
  include/flitr/modules/glsl_shader_passes/glsl_grayscale_pass.h
  
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

# tinyxml
ADD_SUBDIRECTORY(src/flitr/tinyxml)
  
# Video
if(WIN32)
	ADD_SUBDIRECTORY(src/flitr/video_win32_directshow)
    add_definitions(-DTIXML_USE_STL)
endif(WIN32)
if(UNIX)
    IF(NOT APPLE)
        ADD_SUBDIRECTORY(src/flitr/video_linux_v4l)
    ENDIF()
endif(UNIX)

INCLUDE_DIRECTORIES(
	${LIB_INCL_DIRS}
)

source_group("src" FILES ${LIB_SOURCES})
	
ADD_LIBRARY(flitr SHARED
  ${LIB_SOURCES}
)


IF(WIN32) 
  TARGET_LINK_LIBRARIES(flitr ${Boost_LIBRARIES} opengl32 OpenThreads ${OPENSCENEGRAPH_LIBRARIES} ${FFmpeg_LIBRARIES})
ELSEIF(APPLE)
  TARGET_LINK_LIBRARIES(flitr ${Boost_LIBRARIES} ${OPENSCENEGRAPH_LIBRARIES} ${FFmpeg_LIBRARIES})
ELSE()
  TARGET_LINK_LIBRARIES(flitr ${Boost_LIBRARIES} m rt)

  TARGET_LINK_LIBRARIES(flitr ${FFmpeg_LIBRARIES})
  TARGET_LINK_LIBRARIES(flitr lzma)

  TARGET_LINK_LIBRARIES(flitr ${OPENSCENEGRAPH_LIBRARIES})
#TARGET_LINK_LIBRARIES(flitr optimised OpenThreads osgViewerd osgUtild osgTerraind osgGAd osgFXd osgDBd osgTextd osgSimd osgParticled)
#TARGET_LINK_LIBRARIES(flitr debug OpenThreadsd osgViewerd osgUtild osgTerraind osgGAd osgFXd osgDBd osgTextd osgSimd osgParticled)
ENDIF()

TARGET_LINK_LIBRARIES(flitr ${TARGET_LINK_LIBRARIES} ${LIB_LINK_LIBRARIES})

IF(FLITR_WITH_OSGCUDA)
  TARGET_LINK_LIBRARIES(flitr ${TARGET_LINK_LIBRARIES} osgCuda)
ENDIF()


SET_TARGET_PROPERTIES(flitr PROPERTIES COMPILE_FLAGS "-DFLITR_SHARED_LIBRARY")


ADD_SUBDIRECTORY(tests/shared_image_buffer)
ADD_SUBDIRECTORY(tests/ffmpeg_producer)


ADD_SUBDIRECTORY(examples/msr)
ADD_SUBDIRECTORY(examples/average_image)
ADD_SUBDIRECTORY(examples/beat_image)
ADD_SUBDIRECTORY(examples/gaussian_filter)
ADD_SUBDIRECTORY(examples/target_injector)
ADD_SUBDIRECTORY(examples/de_motion_blur)

ADD_SUBDIRECTORY(examples/LK_image_registration_cpu)
ADD_SUBDIRECTORY(examples/LK_dewarp_cpu)
ADD_SUBDIRECTORY(examples/DPT)

ADD_SUBDIRECTORY(examples/LK_image_registration)
ADD_SUBDIRECTORY(examples/viewer_video_quad)
ADD_SUBDIRECTORY(examples/viewer_video_history_quad)
ADD_SUBDIRECTORY(examples/viewer_multi_video_quad)
ADD_SUBDIRECTORY(examples/glsl_shader_pass)
ADD_SUBDIRECTORY(examples/cpu_shader_pass)
ADD_SUBDIRECTORY(examples/ortho_pick_overlay)
ADD_SUBDIRECTORY(examples/keep_history_pass)
#ADD_SUBDIRECTORY(examples/targeting_task_performance)
ADD_SUBDIRECTORY(examples/producer_multiplexer)
ADD_SUBDIRECTORY(examples/video_producer)

#Need to replace the _gl.. calls with the relevant osg calls before the below examples will compile under OSX.
IF(NOT APPLE)
  ADD_SUBDIRECTORY(examples/save_texture_to_video_file)
  ADD_SUBDIRECTORY(examples/hud_overlay)
  ADD_SUBDIRECTORY(examples/points_overlay)
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
  ARCHIVE DESTINATION lib${LIB_INSTALL_POSTFIX}
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


