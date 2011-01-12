PROJECT(flitr_points_overlay)

SET(SOURCES
  points_overlay.cpp
)

ADD_EXECUTABLE(flitr_points_overlay ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_points_overlay flitr)
