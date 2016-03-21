PROJECT(flitr_viewer_colour_night)

SET(SOURCES
  viewer_colour_night.cpp
)

ADD_EXECUTABLE(flitr_viewer_colour_night ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_viewer_colour_night flitr)
