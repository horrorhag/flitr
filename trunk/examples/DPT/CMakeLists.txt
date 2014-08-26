PROJECT(flitr_dpt)

SET(SOURCES
  dpt.cpp
)

ADD_EXECUTABLE(flitr_dpt ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_dpt flitr)

