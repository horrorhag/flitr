PROJECT(flitr_viewer_video_quad)

SET(SOURCES
  viewer_video_quad.cpp
)

ADD_EXECUTABLE(flitr_viewer_video_quad ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_viewer_video_quad flitr)
