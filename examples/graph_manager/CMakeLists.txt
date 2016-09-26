PROJECT(flitr_graph_manager)

SET(SOURCES
  graph_manager.cpp
)

ADD_EXECUTABLE(flitr_graph_manager ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_graph_manager flitr)
