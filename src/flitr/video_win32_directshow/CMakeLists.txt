# add_subdirectory(dsvl)

set(LIB_SOURCES
	${LIB_SOURCES}
    src/flitr/video_win32_directshow/video_producer.cpp
	PARENT_SCOPE)

set(LIB_INCL_DIRS
	${LIB_INCL_DIRS}
	# src/flitr/video_win32_directshow/dsvl
    ${CMAKE_CURRENT_SOURCE_DIR}/../tinyxml
	PARENT_SCOPE
)

set(LIB_LINK_LIBRARIES
	${LIB_LINK_LIBRARIES}
	dsvl
    tinyxml
	PARENT_SCOPE
)