PROJECT(flitr_cpu_shader_pass)

SET(SOURCES
  simple_cpu_shader_pass.cpp
  ../glsl_shader_pass/simple_shader_pass.cpp
  cpu_shader_pass.cpp
)

ADD_EXECUTABLE(flitr_cpu_shader_pass ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_cpu_shader_pass flitr)
