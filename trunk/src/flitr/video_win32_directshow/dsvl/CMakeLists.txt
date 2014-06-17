#
# CMake build system for DSVL 0.0.8h 
# (c) Copyrights 2007-2008 Hartmut Seichter, HIT Lab NZ
#

set(lib_name dsvl)

find_package(DirectShow)

set(SRCS
	DSVL.cpp
	DSVL_GraphManager.cpp
	DSVL_Helpers.cpp
	DSVL_PixelFormat.cpp
	IEEE1394_id.cpp
	DSVL.h
	DSVL_GraphManager.h
	DSVL_Helpers.h
	DSVL_PixelFormat.h
	DSVL_PixelFormatTypes.h
	DSVL_Defines.h
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${DIRECTSHOW_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../tinyxml
	)

add_definitions(-DDSVL_EXPORTS -DTIXML_USE_STL)

add_library(dsvl SHARED ${SRCS})

if(NOT CMAKE_PROJECT_NAME MATCHES "dsvl")
	set_target_properties(dsvl
		PROPERTIES
		PROJECT_LABEL "dependency_${lib_name}"
	)
endif(NOT CMAKE_PROJECT_NAME MATCHES "dsvl")


target_link_libraries(dsvl tinyxml ${DIRECTSHOW_LIBRARIES})

if   (${CMAKE_BUILD_TYPE} MATCHES Debug)
	target_link_libraries(dsvl comsuppd comsuppwd)
else (${CMAKE_BUILD_TYPE} MATCHES Debug)
	target_link_libraries(dsvl comsupp comsuppw)
endif(${CMAKE_BUILD_TYPE} MATCHES Debug)

install(TARGETS dsvl
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
)


