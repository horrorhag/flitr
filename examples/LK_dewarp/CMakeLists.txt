PROJECT(flitr_lk_dewarp)

SET(SOURCES
  lk_dewarp_example.cpp
)

ADD_EXECUTABLE(flitr_lk_dewarp ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_lk_dewarp flitr)

