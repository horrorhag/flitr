PROJECT(flitr_keep_history_pass)

SET(SOURCES
  keep_history.cpp
)

ADD_EXECUTABLE(flitr_keep_history_pass ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_keep_history_pass flitr)
