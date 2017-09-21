PROJECT(flitr_image_video_quad)

SET(SOURCES
  viewer_image_quad.cpp
)

ADD_EXECUTABLE(flitr_viewer_image_quad ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_viewer_image_quad flitr)
