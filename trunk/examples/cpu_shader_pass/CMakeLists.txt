PROJECT(flitr_cpu_shader_pass)

SET(SOURCES
  cpu_shader_pass.cpp
)

ADD_EXECUTABLE(flitr_cpu_shader_pass ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_cpu_shader_pass flitr)
