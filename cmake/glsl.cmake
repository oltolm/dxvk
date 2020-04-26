include_guard()

macro(set_compiled_shaders res)
  if(NOT glsl_compiler)
    message(FATAL_ERROR "variable \"glsl_compiler\" not set")
  endif()

  file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/include/")

  foreach(shader_path IN ITEMS ${ARGN})
    get_filename_component(shader_name_without_ext "${shader_path}" NAME_WE)
    add_custom_command(
      OUTPUT "include/${shader_name_without_ext}.h"
      COMMAND "${glsl_compiler}" --quiet -V --vn "${shader_name_without_ext}" "${PROJECT_SOURCE_DIR}/${shader_path}" -o "${PROJECT_BINARY_DIR}/include/${shader_name_without_ext}.h"
      DEPENDS "${shader_path}"
    )
    list(APPEND ${res} "include/${shader_name_without_ext}.h")
  endforeach()
endmacro()
