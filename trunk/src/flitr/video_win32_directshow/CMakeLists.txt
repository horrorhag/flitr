add_subdirectory(dsvl)

set(LIB_SOURCES
	${LIB_SOURCES}
    src/flitr/video_win32_directshow/video_producer.cpp
	PARENT_SCOPE)

set(LIB_INCL_DIRS
	${LIB_INCL_DIRS}
	src/flitr/video_win32_directshow/dsvl
	PARENT_SCOPE
)

set(LIB_LINK_LIBRARIES
	${LIB_LINK_LIBRARIES}
	dsvl
	PARENT_SCOPE
)