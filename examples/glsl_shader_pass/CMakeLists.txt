PROJECT(flitr_glsl_shader_pass)

SET(SOURCES
  glsl_shader_pass.cpp
)

ADD_EXECUTABLE(flitr_glsl_shader_pass ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_glsl_shader_pass flitr)
