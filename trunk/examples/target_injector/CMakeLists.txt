PROJECT(flitr_target_injector)

SET(SOURCES
  target_injector_example.cpp
)

ADD_EXECUTABLE(flitr_target_injector ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_target_injector flitr)
