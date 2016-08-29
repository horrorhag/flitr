PROJECT(flitr_laser_interlock)

SET(SOURCES
  main.cpp
)

ADD_EXECUTABLE(flitr_laser_interlock ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_laser_interlock flitr)
