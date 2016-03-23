# Locate ffmpeg
# This module defines
# FFmpeg_LIBRARIES
# FFmpeg_FOUND, if false, do not try to link to ffmpeg
# FFmpeg_INCLUDE_DIR, where to find the headers
#
# $FFMPEG_DIR is an environment variable that would
# correspond to the ./configure --prefix=$FFMPEG_DIR
#
# (Created by Robert Osfield for OpenSceneGraph.)
# Adapted by Bernardt Duvenhage for FLITr.

#In ffmpeg code, old version use "#include <header.h>" and newer use "#include <libname/header.h>"
#In FLItr, we use "#include <header.h>" for compatibility with old version of ffmpeg

#We have to search the path which contain the header.h (usefull for old version)
#and search the path which contain the libname/header.h (usefull for new version)

#Then we need to include ${FFmpeg_libname_INCLUDE_DIRS} (in old version case, use by ffmpeg header)
#                                                       (in new version case, use by ffmpeg header) 
#and ${FFmpeg_libname_INCLUDE_DIRS/libname}             (in new version case, potential use by FLITr code.)


IF("${FFmpeg_ROOT}" STREQUAL "")
    SET(FFmpeg_ROOT "$ENV{FFMPEG_DIR}")# CACHE PATH "Path to search for custom FFmpeg library.")
ENDIF()

# Macro to find header and lib directories
# example: FFmpeg_FIND(AVFORMAT avformat avformat.h)
MACRO(FFmpeg_FIND varname shortname headername)
    # old version of ffmpeg put header in $prefix/include/[ffmpeg]
    # so try to find header in include directory

SET(FFmpeg_${varname}_INCLUDE_DIRS FFmpeg_${varname}_INCLUDE_DIRS-NOTFOUND)
SET(FFmpeg_${varname}_LIBRARIES FFmpeg_${varname}_LIBRARIES-NOTFOUND)
SET(FFmpeg_${varname}_FOUND FALSE)

#Hide the FFmpeg components from standard view. FFmpeg_LIBAVFORMAT_LIBRARIES will be un-hidden later.
MARK_AS_ADVANCED(FORCE FFmpeg_${varname}_INCLUDE_DIRS)
MARK_AS_ADVANCED(FORCE FFmpeg_${varname}_LIBRARIES)
MARK_AS_ADVANCED(FORCE FFmpeg_${varname}_FOUND)

    IF(FFmpeg_NO_SYSTEM_PATHS)
        FIND_PATH(FFmpeg_${varname}_INCLUDE_DIRS lib${shortname}/${headername}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/include
			${CMAKE_INSTALL_PREFIX}
			${CMAKE_INSTALL_PREFIX}/include
            PATH_SUFFIXES ffmpeg
            DOC "Location of FFmpeg Headers"
            NO_DEFAULT_PATH
        )
    ELSE()
        FIND_PATH(FFmpeg_${varname}_INCLUDE_DIRS lib${shortname}/${headername}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/include
			${CMAKE_INSTALL_PREFIX}
			${CMAKE_INSTALL_PREFIX}/include
            $ENV{FFMPEG_DIR}/includecc
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local/include
            /usr/include
            /sw/include # Fink
            /opt/local/include # DarwinPorts
            /opt/csw/include # Blastwave
            /opt/include
            /usr/freeware/include
            PATH_SUFFIXES ffmpeg
            DOC "Location of FFmpeg Headers"
        )
    ENDIF()

    IF(FFmpeg_NO_SYSTEM_PATHS)
        FIND_PATH(FFmpeg_${varname}_INCLUDE_DIRS ${headername}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/include
			${CMAKE_INSTALL_PREFIX}
			${CMAKE_INSTALL_PREFIX}/include
            DOC "Location of FFmpeg Headers"
            PATH_SUFFIXES ffmpeg
            NO_DEFAULT_PATH
        )
    ELSE()
        FIND_PATH(FFmpeg_${varname}_INCLUDE_DIRS ${headername}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/include
			${CMAKE_INSTALL_PREFIX}
			${CMAKE_INSTALL_PREFIX}/include
            $ENV{FFMPEG_DIR}/include
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local/include
            /usr/include
            /sw/include # Fink
            /opt/local/include # DarwinPorts
            /opt/csw/include # Blastwave
            /opt/include
            /usr/freeware/include
            PATH_SUFFIXES ffmpeg
            DOC "Location of FFmpeg Headers"
        )
    ENDIF()


    IF(FFmpeg_NO_SYSTEM_PATHS)
        FIND_LIBRARY(FFmpeg_${varname}_LIBRARIES
            NAMES ${shortname}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/lib${shortname}
            ${FFmpeg_ROOT}/lib
            ${FFmpeg_ROOT}/lib_lin32
            ${FFmpeg_ROOT}/lib_win32
			${CMAKE_INSTALL_PREFIX}
			${CMAKE_INSTALL_PREFIX}/lib
            DOC "Location of FFmpeg Libraries"
            NO_DEFAULT_PATH
        )
    ELSE()
        FIND_LIBRARY(FFmpeg_${varname}_LIBRARIES
            NAMES ${shortname}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/lib
            ${FFmpeg_ROOT}/lib_lin32
            ${FFmpeg_ROOT}/lib_win32
			${CMAKE_INSTALL_PREFIX}
			${CMAKE_INSTALL_PREFIX}/lib
            $ENV{FFMPEG_DIR}/lib
            $ENV{FFMPEG_DIR}/lib_lin32
            $ENV{FFMPEG_DIR}/lib_win32
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local/lib
            /usr/local/lib64
            /usr/lib
            /usr/lib64
            /usr/lib/i386-linux-gnu
            /sw/lib
            /opt/local/lib
            /opt/csw/lib
            /opt/lib
            /usr/freeware/lib64
            DOC "Location of FFmpeg Libraries"
        )
    ENDIF()

    IF (FFmpeg_${varname}_LIBRARIES AND FFmpeg_${varname}_INCLUDE_DIRS)
        SET(FFmpeg_${varname}_FOUND TRUE)
    ELSE()
        SET(FFmpeg_${varname}_FOUND FALSE)
    ENDIF()

    #MESSAGE("${FFmpeg_${varname}_INCLUDE_DIRS}")
    #MESSAGE("${FFmpeg_${varname}_LIBRARIES}")
    #MESSAGE("  FOUND ${FFmpeg_${varname}_FOUND}")

ENDMACRO(FFmpeg_FIND)



# find stdint.h
IF(MSVC OR MINGW)

	SET(STDINT_OK TRUE)
	
ELSEIF(WIN32)

	FIND_PATH(FFmpeg_STDINT_INCLUDE_DIR stdint.h
        PATHS
        ${FFmpeg_ROOT}/include
		${CMAKE_INSTALL_PREFIX}/include
        $ENV{FFMPEG_DIR}/include
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/include
        /usr/include
        /sw/include # Fink
        /opt/local/include # DarwinPorts
        /opt/csw/include # Blastwave
        /opt/include
        /usr/freeware/include
        PATH_SUFFIXES ffmpeg
        DOC "Location of FFmpeg stdint.h Header"
    )
	
	IF(FFmpeg_STDINT_INCLUDE_DIR)
        SET(STDINT_OK TRUE)
    ELSE()
        MESSAGE("Could not find stdint.h")
    ENDIF()
	
ELSE()

    SET(STDINT_OK TRUE)

ENDIF()


SET(FFmpeg_INCLUDE_DIRS FFmpeg_INCLUDE_DIRS-NOTFOUND)# CACHE STRING "docstring")
SET(FFmpeg_LIBRARIES FFmpeg_LIBRARIES-NOTFOUND)# CACHE STRING "docstring")
SET(FFmpeg_FOUND FALSE)


FFmpeg_FIND(LIBAVFORMAT avformat avformat.h)
FFmpeg_FIND(LIBAVDEVICE avdevice avdevice.h)
FFmpeg_FIND(LIBAVCODEC  avcodec  avcodec.h)
FFmpeg_FIND(LIBAVUTIL   avutil   avutil.h)
FFmpeg_FIND(LIBSWSCALE  swscale  swscale.h)

#NOTE!: libresample is for later version of ffmpeg, please comment out the below line if not found on your system.
#FFmpeg_FIND(LIBSWRESAMPLE  swresample  swresample.h)


MARK_AS_ADVANCED(CLEAR FFmpeg_LIBAVFORMAT_LIBRARIES) #Mark one lib as non-advanced to show where FFmpeg was found.


IF(FFmpeg_LIBAVFORMAT_FOUND AND FFmpeg_LIBAVDEVICE_FOUND AND FFmpeg_LIBAVCODEC_FOUND AND FFmpeg_LIBAVUTIL_FOUND AND FFmpeg_LIBSWSCALE_FOUND AND STDINT_OK)
    SET(FFmpeg_FOUND TRUE)

    SET(FFmpeg_INCLUDE_DIRS
        ${FFmpeg_LIBAVFORMAT_INCLUDE_DIRS} #${FFmpeg_LIBAVFORMAT_INCLUDE_DIRS}/libavformat
        ${FFmpeg_LIBAVDEVICE_INCLUDE_DIRS} #${FFmpeg_LIBAVDEVICE_INCLUDE_DIRS}/libavdevice
        ${FFmpeg_LIBAVCODEC_INCLUDE_DIRS} #${FFmpeg_LIBAVCODEC_INCLUDE_DIRS}/libavcodec
        ${FFmpeg_LIBAVUTIL_INCLUDE_DIRS} #${FFmpeg_LIBAVUTIL_INCLUDE_DIRS}/libavutil
        ${FFmpeg_LIBSWSCALE_INCLUDE_DIRS} #${FFmpeg_LIBSWSCALE_INCLUDE_DIRS}/libswscale
#NOTE!: libresample is for later version of ffmpeg, please comment out the below line if not found on your system.
#        ${FFmpeg_LIBSWRESAMPLE_INCLUDE_DIRS} #${FFmpeg_LIBSWRESAMPLE_INCLUDE_DIRS}/libswresample
        #CACHE STRING  "docstring"
    )

    IF (FFmpeg_STDINT_INCLUDE_DIR)
        SET(FFmpeg_INCLUDE_DIRS
            ${FFmpeg_INCLUDE_DIRS}
            ${FFmpeg_STDINT_INCLUDE_DIR}
            #${FFmpeg_STDINT_INCLUDE_DIR}/libavformat
            #${FFmpeg_STDINT_INCLUDE_DIR}/libavdevice
            #${FFmpeg_STDINT_INCLUDE_DIR}/libavcodec
            #${FFmpeg_STDINT_INCLUDE_DIR}/libavutil
            #${FFmpeg_STDINT_INCLUDE_DIR}/libswscale
#NOTE!: libresample is for later version of ffmpeg, please comment out the below line if not found on your system.
            #${FFmpeg_STDINT_INCLUDE_DIR}/libswresample
            #CACHE  STRING  "docstring"
        )
    ENDIF()

    SET(FFmpeg_LIBRARIES
        ${FFmpeg_LIBAVFORMAT_LIBRARIES}
        ${FFmpeg_LIBAVDEVICE_LIBRARIES}
        ${FFmpeg_LIBAVCODEC_LIBRARIES}
        ${FFmpeg_LIBAVUTIL_LIBRARIES}
        ${FFmpeg_LIBSWSCALE_LIBRARIES}
#NOTE!: libresample is for later version of ffmpeg, please comment out the below line if not found on your system.
#        ${FFmpeg_LIBSWRESAMPLE_LIBRARIES}
        #CACHE  STRING  "docstring"
    )

    #MESSAGE("  ${FFmpeg_INCLUDE_DIRS}")
    #MESSAGE("  ${FFmpeg_LIBRARIES}")

ELSE()
    MESSAGE("Could not find FFmpeg")
    SET(FFmpeg_INCLUDE_DIRS FFmpeg_INCLUDE_DIRS-NOTFOUND)# CACHE  STRING  "docstring")
    SET(FFmpeg_LIBRARIES FFmpeg_LIBRARIES-NOTFOUND)# CACHE  STRING  "docstring")
    SET(FFmpeg_FOUND FALSE)
ENDIF()
