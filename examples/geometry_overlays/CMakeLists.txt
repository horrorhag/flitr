PROJECT(flitr_geometry_overlays)

SET(SOURCES
  geometry_overlays.cpp
)

ADD_EXECUTABLE(flitr_geometry_overlays ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_geometry_overlays flitr)
