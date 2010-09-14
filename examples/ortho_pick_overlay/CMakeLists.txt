PROJECT(flitr_ortho_pick_overlay)

SET(SOURCES
  ortho_pick_overlay.cpp
)

ADD_EXECUTABLE(flitr_ortho_pick_overlay ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_ortho_pick_overlay flitr)
