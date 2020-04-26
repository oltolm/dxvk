# ---------------------- d3d9 -------------------------------

add_executable(d3d9-buffer WIN32 d3d9/test_d3d9_buffer.cpp)
add_executable(d3d9-clear WIN32 d3d9/test_d3d9_clear.cpp)
add_executable(d3d9-triangle WIN32 d3d9/test_d3d9_triangle.cpp)
add_executable(d3d9-l6v5u5 WIN32 d3d9/test_d3d9_l6v5u5.cpp)
add_executable(d3d9-nv12 WIN32 d3d9/test_d3d9_nv12.cpp)

add_library(test_d3d9_deps INTERFACE)
target_link_libraries(test_d3d9_deps INTERFACE util d3d9 -ld3dcompiler_47)
target_compile_features(test_d3d9_deps INTERFACE cxx_std_17)

foreach(target IN ITEMS d3d9-buffer d3d9-clear d3d9-triangle d3d9-l6v5u5 d3d9-nv12)
    target_link_libraries(${target} PRIVATE test_d3d9_deps)
endforeach()

# ---------------------- d3d11 -------------------------------

add_executable(d3d11-compute WIN32 d3d11/test_d3d11_compute.cpp)
add_executable(d3d11-formats WIN32 d3d11/test_d3d11_formats.cpp)
add_executable(d3d11-map-read WIN32 d3d11/test_d3d11_map_read.cpp)
add_executable(d3d11-streamout WIN32 d3d11/test_d3d11_streamout.cpp)
add_executable(d3d11-triangle WIN32 d3d11/test_d3d11_triangle.cpp)

add_library(test_d3d11_deps INTERFACE)
target_link_libraries(test_d3d11_deps INTERFACE util dxgi d3d11 -ld3dcompiler_47)
target_compile_features(test_d3d11_deps INTERFACE cxx_std_17)

foreach(target IN ITEMS d3d11-compute d3d11-formats d3d11-map-read d3d11-streamout d3d11-triangle)
    target_link_libraries(${target} PRIVATE test_d3d11_deps)
endforeach()

# ---------------------- dxbc -------------------------------

add_executable(dxbc-compiler WIN32 dxbc/test_dxbc_compiler.cpp)
add_executable(dxbc-disasm WIN32 dxbc/test_dxbc_disasm.cpp)
add_executable(hlsl-compiler WIN32 dxbc/test_hlsl_compiler.cpp)

add_library(test_dxbc_deps INTERFACE)
target_link_libraries(test_dxbc_deps INTERFACE dxbc dxvk)
target_compile_features(test_dxbc_deps INTERFACE cxx_std_17)
target_include_directories(test_dxbc_deps INTERFACE "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(dxbc-disasm PRIVATE -ld3dcompiler_47)
target_link_libraries(hlsl-compiler PRIVATE -ld3dcompiler_47)

foreach(target IN ITEMS dxbc-compiler dxbc-disasm hlsl-compiler)
    target_link_libraries(${target} PRIVATE test_dxbc_deps)
endforeach()

# ---------------------- dxgi -------------------------------

add_executable(dxgi-factory WIN32 dxgi/test_dxgi_factory.cpp)

add_library(test_dxgi_deps INTERFACE)
target_link_libraries(test_dxgi_deps INTERFACE util dxgi)
target_compile_features(test_dxgi_deps INTERFACE cxx_std_17)

target_link_libraries(dxgi-factory PRIVATE test_dxgi_deps)
