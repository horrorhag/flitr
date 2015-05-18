PROJECT(flitr_msr)

SET(SOURCES
  main.cpp
)

ADD_EXECUTABLE(flitr_msr ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_msr flitr)
