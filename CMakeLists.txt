# execute a script which sets the proper environment variables like PATH and VK_SDK_PATH
# cmake --preset=[win64|win32]
# ninja
# ninja install
# cpack
#
# ninja clean

cmake_minimum_required(VERSION 3.15)
project(dxvk VERSION 1.9.0 LANGUAGES CXX)

find_program(glsl_compiler
  NAMES glslangValidator
  PATHS "$ENV{VK_SDK_PATH}/Bin"
  REQUIRED
)

include(cmake/glsl.cmake)

find_program(CCACHE ccache)
if(CCACHE)
  set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(dxvk_library_path "${PROJECT_SOURCE_DIR}/lib")
else()
  set(dxvk_library_path "${PROJECT_SOURCE_DIR}/lib32")
endif()

if(NOT MSVC)
  include(cmake/CPM.cmake)
  CPMAddPackage(
    NAME mingw-std-threads
    GITHUB_REPOSITORY oltolm/mingw-std-threads
    GIT_TAG a1e82eea75c65a278bf7f220d75e536f1e5d821a
    OPTIONS "MINGW_STDTHREADS_GENERATE_STDHEADERS ON"
  )
endif()

option(LTO "enable LTO" OFF)

if(LTO)
  include(CheckIPOSupported)
  check_ipo_supported(LANGUAGES CXX)
endif()

add_library(dxvk_deps INTERFACE)

if(NOT MSVC)
  target_link_libraries(dxvk_deps
    INTERFACE
    -Wl,--enable-stdcall-fixup
    -static-libgcc
    -static-libstdc++
    -static
    mingw_stdthreads
  )
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_compile_options(dxvk_deps
      INTERFACE
      -msse2
      -msse
      -mfpmath=sse
    )
  endif()
  target_compile_options(dxvk_deps
    INTERFACE
    -fno-omit-frame-pointer
    -msse3
    -Wimplicit-fallthrough
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-cast-function-type
  )
endif()

target_compile_features(dxvk_deps INTERFACE cxx_std_17)
target_include_directories(dxvk_deps INTERFACE "${PROJECT_SOURCE_DIR}/include")
target_compile_definitions(dxvk_deps INTERFACE -DNOMINMAX)

if(MSVC)
  target_compile_options(dxvk_deps INTERFACE
    /wd26812
    /wd26495
    /wd26439
    /wd28251
    /wd26451
    /MP # parallel compilation
  )
endif()

option(ASAN "AddressSanitizer for MSVC" OFF)

if(MSVC AND ASAN)
  target_compile_options(dxvk_deps
    INTERFACE
    /fsanitize=address
    /Oy- # -fno-omit-frame-pointer
  )
  target_link_options(dxvk_deps INTERFACE /INCREMENTAL:NO)
endif()

if(NOT MSVC)
  find_program(cv2pdb cv2pdb)
  include(cmake/cv2pdb.cmake)
  if(cv2pdb)
    add_custom_target(pdb)
    message(STATUS "found \"cv2pdb\": target \"pdb\" will be available")
  endif()
endif()

option(enable_dxgi "Build DXGI" ON)
option(enable_d3d9 "Build D3D9" ON)
option(enable_d3d10 "Build D3D10" ON)
option(enable_d3d11 "Build D3D11" ON)

add_library(util
  STATIC
  src/util/com/com_guid.cpp
  src/util/com/com_private_data.cpp
  src/util/config/config.cpp
  src/util/log/log_debug.cpp
  src/util/log/log.cpp
  src/util/sha1/sha1_util.cpp
  src/util/sha1/sha1.c
  src/util/sync/sync_recursive.cpp
  src/util/util_env.cpp
  src/util/util_fps_limiter.cpp
  src/util/util_gdi.cpp
  src/util/util_luid.cpp
  src/util/util_matrix.cpp
  src/util/util_monitor.cpp
  src/util/util_string.cpp
)

target_link_libraries(util PRIVATE dxvk_deps)
set_property(SOURCE src/util/sha1/sha1.c PROPERTY LANGUAGE CXX)

add_library(spirv
  STATIC
  src/spirv/spirv_code_buffer.cpp
  src/spirv/spirv_compression.cpp
  src/spirv/spirv_module.cpp
)

target_link_libraries(spirv PRIVATE dxvk_deps)

add_library(vkcommon
  STATIC
  src/vulkan/vulkan_loader.cpp
  src/vulkan/vulkan_names.cpp
  src/vulkan/vulkan_presenter.cpp
)

target_link_libraries(vkcommon
  PRIVATE
  dxvk_deps
  "${dxvk_library_path}/vulkan-1.lib"
)

set_compiled_shaders(dxvk_headers
  src/dxvk/hud/shaders/hud_line_frag.frag
  src/dxvk/hud/shaders/hud_line_vert.vert
  src/dxvk/hud/shaders/hud_text_frag.frag
  src/dxvk/hud/shaders/hud_text_vert.vert
  src/dxvk/shaders/dxvk_blit_frag_1d.frag
  src/dxvk/shaders/dxvk_blit_frag_2d.frag
  src/dxvk/shaders/dxvk_blit_frag_3d.frag
  src/dxvk/shaders/dxvk_clear_buffer_f.comp
  src/dxvk/shaders/dxvk_clear_buffer_u.comp
  src/dxvk/shaders/dxvk_clear_image1d_f.comp
  src/dxvk/shaders/dxvk_clear_image1d_u.comp
  src/dxvk/shaders/dxvk_clear_image1darr_f.comp
  src/dxvk/shaders/dxvk_clear_image1darr_u.comp
  src/dxvk/shaders/dxvk_clear_image2d_f.comp
  src/dxvk/shaders/dxvk_clear_image2d_u.comp
  src/dxvk/shaders/dxvk_clear_image2darr_f.comp
  src/dxvk/shaders/dxvk_clear_image2darr_u.comp
  src/dxvk/shaders/dxvk_clear_image3d_f.comp
  src/dxvk/shaders/dxvk_clear_image3d_u.comp
  src/dxvk/shaders/dxvk_copy_buffer_image.comp
  src/dxvk/shaders/dxvk_copy_color_1d.frag
  src/dxvk/shaders/dxvk_copy_color_2d.frag
  src/dxvk/shaders/dxvk_copy_color_ms.frag
  src/dxvk/shaders/dxvk_copy_depth_1d.frag
  src/dxvk/shaders/dxvk_copy_depth_2d.frag
  src/dxvk/shaders/dxvk_copy_depth_ms.frag
  src/dxvk/shaders/dxvk_copy_depth_stencil_1d.frag
  src/dxvk/shaders/dxvk_copy_depth_stencil_2d.frag
  src/dxvk/shaders/dxvk_copy_depth_stencil_ms.frag
  src/dxvk/shaders/dxvk_fullscreen_geom.geom
  src/dxvk/shaders/dxvk_fullscreen_layer_vert.vert
  src/dxvk/shaders/dxvk_fullscreen_vert.vert
  src/dxvk/shaders/dxvk_pack_d24s8.comp
  src/dxvk/shaders/dxvk_pack_d32s8.comp
  src/dxvk/shaders/dxvk_present_frag_blit.frag
  src/dxvk/shaders/dxvk_present_frag_ms_amd.frag
  src/dxvk/shaders/dxvk_present_frag_ms.frag
  src/dxvk/shaders/dxvk_present_frag.frag
  src/dxvk/shaders/dxvk_present_vert.vert
  src/dxvk/shaders/dxvk_resolve_frag_d.frag
  src/dxvk/shaders/dxvk_resolve_frag_ds.frag
  src/dxvk/shaders/dxvk_resolve_frag_f_amd.frag
  src/dxvk/shaders/dxvk_resolve_frag_f.frag
  src/dxvk/shaders/dxvk_resolve_frag_i.frag
  src/dxvk/shaders/dxvk_resolve_frag_u.frag
  src/dxvk/shaders/dxvk_unpack_d24s8_as_d32s8.comp
  src/dxvk/shaders/dxvk_unpack_d24s8.comp
  src/dxvk/shaders/dxvk_unpack_d32s8.comp
)

add_library(dxvk
  STATIC
  ${dxvk_headers}
  src/dxvk/dxvk_adapter.cpp
  src/dxvk/dxvk_barrier.cpp
  src/dxvk/dxvk_buffer.cpp
  src/dxvk/dxvk_cmdlist.cpp
  src/dxvk/dxvk_compute.cpp
  src/dxvk/dxvk_context.cpp
  src/dxvk/dxvk_cs.cpp
  src/dxvk/dxvk_data.cpp
  src/dxvk/dxvk_descriptor.cpp
  src/dxvk/dxvk_device_filter.cpp
  src/dxvk/dxvk_device.cpp
  src/dxvk/dxvk_extensions.cpp
  src/dxvk/dxvk_format.cpp
  src/dxvk/dxvk_framebuffer.cpp
  src/dxvk/dxvk_gpu_event.cpp
  src/dxvk/dxvk_gpu_query.cpp
  src/dxvk/dxvk_graphics.cpp
  src/dxvk/dxvk_image.cpp
  src/dxvk/dxvk_instance.cpp
  src/dxvk/dxvk_lifetime.cpp
  src/dxvk/dxvk_main.cpp
  src/dxvk/dxvk_memory.cpp
  src/dxvk/dxvk_meta_blit.cpp
  src/dxvk/dxvk_meta_clear.cpp
  src/dxvk/dxvk_meta_copy.cpp
  src/dxvk/dxvk_meta_mipgen.cpp
  src/dxvk/dxvk_meta_pack.cpp
  src/dxvk/dxvk_meta_resolve.cpp
  src/dxvk/dxvk_openvr.cpp
  src/dxvk/dxvk_openxr.cpp
  src/dxvk/dxvk_options.cpp
  src/dxvk/dxvk_pipecache.cpp
  src/dxvk/dxvk_pipelayout.cpp
  src/dxvk/dxvk_pipemanager.cpp
  src/dxvk/dxvk_queue.cpp
  src/dxvk/dxvk_renderpass.cpp
  src/dxvk/dxvk_resource.cpp
  src/dxvk/dxvk_sampler.cpp
  src/dxvk/dxvk_shader_key.cpp
  src/dxvk/dxvk_shader.cpp
  src/dxvk/dxvk_signal.cpp
  src/dxvk/dxvk_spec_const.cpp
  src/dxvk/dxvk_staging.cpp
  src/dxvk/dxvk_state_cache.cpp
  src/dxvk/dxvk_stats.cpp
  src/dxvk/dxvk_swapchain_blitter.cpp
  src/dxvk/dxvk_unbound.cpp
  src/dxvk/dxvk_util.cpp
  src/dxvk/hud/dxvk_hud_font.cpp
  src/dxvk/hud/dxvk_hud_item.cpp
  src/dxvk/hud/dxvk_hud_renderer.cpp
  src/dxvk/hud/dxvk_hud.cpp
  src/dxvk/platform/dxvk_win32_exts.cpp
)

if(EXISTS "${PROJECT_SOURCE_DIR}/src/dxvk/dxvk_thread_pool.cpp")
  target_sources(dxvk PRIVATE src/dxvk/dxvk_thread_pool.cpp)
endif()

target_link_libraries(dxvk
  PRIVATE
  util
  spirv
  vkcommon
  dxvk_deps
)
target_include_directories(dxvk
  PRIVATE
  "${PROJECT_BINARY_DIR}/include/"
)

execute_process(
  COMMAND git describe --always
  OUTPUT_VARIABLE VCS_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)
configure_file("version.h.in" "${PROJECT_BINARY_DIR}/include/version.h")

if(enable_dxgi)
  add_library(dxgi
    SHARED
    src/dxgi/dxgi_adapter.cpp
    src/dxgi/dxgi_enums.cpp
    src/dxgi/dxgi_factory.cpp
    src/dxgi/dxgi_format.cpp
    src/dxgi/dxgi_main.cpp
    src/dxgi/dxgi_monitor.cpp
    src/dxgi/dxgi_options.cpp
    src/dxgi/dxgi_output.cpp
    src/dxgi/dxgi_swapchain.cpp
    src/dxgi/dxgi.def
    src/dxgi/version.rc
  )
  target_link_libraries(dxgi
    PRIVATE
    dxvk
    dxvk_deps
  )

  if(LTO)
    set_property(TARGET dxgi PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()

  set_property(TARGET dxgi PROPERTY PREFIX "")

  install(TARGETS dxgi RUNTIME DESTINATION ".")

  if(cv2pdb)
    add_cv2pdb_target(dxgi dxgi.dll dxgi.pdb)
    add_dependencies(pdb dxgi_pdb)
  endif()
endif(enable_dxgi)

add_library(dxbc
  STATIC
  src/dxbc/dxbc_analysis.cpp
  src/dxbc/dxbc_chunk_isgn.cpp
  src/dxbc/dxbc_chunk_shex.cpp
  src/dxbc/dxbc_common.cpp
  src/dxbc/dxbc_compiler.cpp
  src/dxbc/dxbc_decoder.cpp
  src/dxbc/dxbc_defs.cpp
  src/dxbc/dxbc_header.cpp
  src/dxbc/dxbc_module.cpp
  src/dxbc/dxbc_names.cpp
  src/dxbc/dxbc_options.cpp
  src/dxbc/dxbc_reader.cpp
  src/dxbc/dxbc_util.cpp
)

target_link_libraries(dxbc PRIVATE dxvk_deps)

set(dxgi_common_src
  src/dxgi/dxgi_format.cpp
  src/dxgi/dxgi_monitor.cpp
  src/dxgi/dxgi_swapchain.cpp
)

set_compiled_shaders(d3d11_shaders
  src/d3d11/shaders/d3d11_video_blit_frag.frag
  src/d3d11/shaders/d3d11_video_blit_vert.vert
)

set(d3d10_src
  src/d3d10/d3d10_blend.cpp
  src/d3d10/d3d10_buffer.cpp
  src/d3d10/d3d10_depth_stencil.cpp
  src/d3d10/d3d10_device.cpp
  src/d3d10/d3d10_input_layout.cpp
  src/d3d10/d3d10_multithread.cpp
  src/d3d10/d3d10_query.cpp
  src/d3d10/d3d10_rasterizer.cpp
  src/d3d10/d3d10_sampler.cpp
  src/d3d10/d3d10_texture.cpp
  src/d3d10/d3d10_util.cpp
  src/d3d10/d3d10_view_dsv.cpp
  src/d3d10/d3d10_view_rtv.cpp
  src/d3d10/d3d10_view_srv.cpp
)

set(d3d11_src
  src/d3d11/d3d11_annotation.cpp
  src/d3d11/d3d11_blend.cpp
  src/d3d11/d3d11_buffer.cpp
  src/d3d11/d3d11_class_linkage.cpp
  src/d3d11/d3d11_cmdlist.cpp
  src/d3d11/d3d11_context_def.cpp
  src/d3d11/d3d11_context_ext.cpp
  src/d3d11/d3d11_context_imm.cpp
  src/d3d11/d3d11_context.cpp
  src/d3d11/d3d11_depth_stencil.cpp
  src/d3d11/d3d11_device.cpp
  src/d3d11/d3d11_enums.cpp
  src/d3d11/d3d11_gdi.cpp
  src/d3d11/d3d11_initializer.cpp
  src/d3d11/d3d11_input_layout.cpp
  src/d3d11/d3d11_interop.cpp
  src/d3d11/d3d11_main.cpp
  src/d3d11/d3d11_options.cpp
  src/d3d11/d3d11_query.cpp
  src/d3d11/d3d11_rasterizer.cpp
  src/d3d11/d3d11_resource.cpp
  src/d3d11/d3d11_sampler.cpp
  src/d3d11/d3d11_shader.cpp
  src/d3d11/d3d11_state_object.cpp
  src/d3d11/d3d11_state.cpp
  src/d3d11/d3d11_swapchain.cpp
  src/d3d11/d3d11_texture.cpp
  src/d3d11/d3d11_util.cpp
  src/d3d11/d3d11_video.cpp
  src/d3d11/d3d11_view_dsv.cpp
  src/d3d11/d3d11_view_rtv.cpp
  src/d3d11/d3d11_view_srv.cpp
  src/d3d11/d3d11_view_uav.cpp
)

if(enable_d3d11)
  add_library(d3d11
    SHARED
    ${d3d11_shaders}
    ${dxgi_common_src}
    ${d3d10_src}
    ${d3d11_src}
    src/d3d11/d3d11.def
    src/d3d11/version.rc
  )

  target_link_libraries(d3d11
    PRIVATE
    dxgi
    dxvk
    dxbc
    dxvk_deps
  )

  target_include_directories(d3d11
    PRIVATE
    "${PROJECT_BINARY_DIR}/include/"
  )

  if(LTO)
    set_property(TARGET d3d11 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()

  set_property(TARGET d3d11 PROPERTY PREFIX "")

  install(TARGETS d3d11 RUNTIME DESTINATION ".")

  if(cv2pdb)
    add_cv2pdb_target(d3d11 d3d11.dll d3d11.pdb)
    add_dependencies(pdb d3d11_pdb)
  endif()
endif(enable_d3d11)

set(d3d10_main_src
  src/d3d10/d3d10_main.cpp
  src/d3d10/d3d10_reflection.cpp
  src/d3d10/d3d10_state_block.cpp
)

find_library(lib_d3dcompiler_43 d3dcompiler_43.lib PATHS "${dxvk_library_path}")

if(enable_d3d10)
  add_library(d3d10core
    SHARED
    src/d3d10/d3d10_core.cpp
    src/d3d10/d3d10core.def
    src/d3d10/version10_core.rc
  )

  target_link_libraries(d3d10core
    PRIVATE
    "${lib_d3dcompiler_43}"
    d3d11
    dxvk_deps
  )

  add_library(d3d10
    SHARED
    ${d3d10_main_src}
    src/d3d10/d3d10.def
    src/d3d10/version10.rc
  )

  target_link_libraries(d3d10
    PRIVATE
    "${lib_d3dcompiler_43}"
    dxgi
    dxbc
    dxvk
    d3d10core
    dxvk_deps
  )

  add_library(d3d10_1
    SHARED
    ${d3d10_main_src}
    src/d3d10/d3d10_1.def
    src/d3d10/version10_1.rc
  )

  target_link_libraries(d3d10_1
    PRIVATE
    "${lib_d3dcompiler_43}"
    dxgi
    dxbc
    dxvk
    d3d10core
    dxvk_deps
  )

  foreach(t IN ITEMS d3d10core d3d10 d3d10_1)
    set_property(TARGET ${t} PROPERTY PREFIX "")
  endforeach()

  install(TARGETS d3d10core d3d10 d3d10_1 RUNTIME DESTINATION ".")

  if(LTO)
    foreach(t IN ITEMS d3d10core d3d10 d3d10_1)
      set_property(TARGET ${t} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endforeach()
  endif()

  if(cv2pdb)
    add_cv2pdb_target(d3d10core d3d10core.dll d3d10core.pdb)
    add_cv2pdb_target(d3d10 d3d10.dll d3d10.pdb)
    add_cv2pdb_target(d3d10_1 d3d10_1.dll d3d10_1.pdb)
    add_dependencies(pdb d3d10core_pdb d3d10_pdb d3d10_1_pdb)
  endif()
endif(enable_d3d10)

add_library(dxso
  STATIC
  src/dxso/dxso_analysis.cpp
  src/dxso/dxso_code.cpp
  src/dxso/dxso_common.cpp
  src/dxso/dxso_compiler.cpp
  src/dxso/dxso_ctab.cpp
  src/dxso/dxso_decoder.cpp
  src/dxso/dxso_enums.cpp
  src/dxso/dxso_header.cpp
  src/dxso/dxso_module.cpp
  src/dxso/dxso_options.cpp
  src/dxso/dxso_reader.cpp
  src/dxso/dxso_tables.cpp
  src/dxso/dxso_util.cpp
)

target_link_libraries(dxso PRIVATE dxvk_deps)

set_compiled_shaders(d3d9_headers
  src/d3d9/shaders/d3d9_convert_a2w10v10u10.comp
  src/d3d9/shaders/d3d9_convert_l6v5u5.comp
  src/d3d9/shaders/d3d9_convert_nv12.comp
  src/d3d9/shaders/d3d9_convert_x8l8v8u8.comp
  src/d3d9/shaders/d3d9_convert_yuy2_uyvy.comp
  src/d3d9/shaders/d3d9_convert_yv12.comp
)

if(enable_d3d9)
  add_library(d3d9
    SHARED
    ${d3d9_headers}
    src/d3d9/d3d9_adapter.cpp
    src/d3d9/d3d9_buffer.cpp
    src/d3d9/d3d9_common_buffer.cpp
    src/d3d9/d3d9_common_texture.cpp
    src/d3d9/d3d9_cursor.cpp
    src/d3d9/d3d9_device.cpp
    src/d3d9/d3d9_fixed_function.cpp
    src/d3d9/d3d9_format_helpers.cpp
    src/d3d9/d3d9_format.cpp
    src/d3d9/d3d9_hud.cpp
    src/d3d9/d3d9_initializer.cpp
    src/d3d9/d3d9_interface.cpp
    src/d3d9/d3d9_main.cpp
    src/d3d9/d3d9_monitor.cpp
    src/d3d9/d3d9_multithread.cpp
    src/d3d9/d3d9_names.cpp
    src/d3d9/d3d9_options.cpp
    src/d3d9/d3d9_query.cpp
    src/d3d9/d3d9_sampler.cpp
    src/d3d9/d3d9_shader.cpp
    src/d3d9/d3d9_state.cpp
    src/d3d9/d3d9_stateblock.cpp
    src/d3d9/d3d9_surface.cpp
    src/d3d9/d3d9_swapchain.cpp
    src/d3d9/d3d9_swvp_emu.cpp
    src/d3d9/d3d9_texture.cpp
    src/d3d9/d3d9_util.cpp
    src/d3d9/d3d9_vertex_declaration.cpp
    src/d3d9/d3d9_volume.cpp
    src/d3d9/d3d9.def
    src/d3d9/version.rc
  )

  target_link_libraries(d3d9
    PRIVATE
    dxso
    dxvk
    dxvk_deps
  )

  target_include_directories(d3d9
    PRIVATE
    "${PROJECT_BINARY_DIR}/include/"
  )

  if(LTO)
    set_property(TARGET d3d9 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()

  set_property(TARGET d3d9 PROPERTY PREFIX "")

  install(TARGETS d3d9 RUNTIME DESTINATION ".")

  if(cv2pdb)
    add_cv2pdb_target(d3d9 d3d9.dll d3d9.pdb)
    add_dependencies(pdb d3d9_pdb)
  endif()
endif(enable_d3d9)

install(PROGRAMS setup_dxvk.sh DESTINATION "..")

set(CPACK_GENERATOR ZIP)
include(CPack)

option(enable_tests "enable testing" OFF)

if(enable_tests)
  include(CTest)
  add_subdirectory(tests)
endif()
