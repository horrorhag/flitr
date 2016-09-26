PROJECT(flitr_adaptive_threshold)

SET(SOURCES
  adaptive_threshold_example.cpp
)

ADD_EXECUTABLE(flitr_adaptive_threshold ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_adaptive_threshold flitr)
