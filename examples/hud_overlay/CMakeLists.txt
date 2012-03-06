PROJECT(flitr_hud_overlay)

SET(SOURCES
  hud_overlay.cpp
)

ADD_EXECUTABLE(flitr_hud_overlay ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_hud_overlay flitr)
