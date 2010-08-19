PROJECT(flitr_viewer_video_history_quad)

SET(SOURCES
  viewer_video_history_quad.cpp
)

ADD_EXECUTABLE(flitr_viewer_video_history_quad ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_viewer_video_history_quad flitr)
