cmake_minimum_required(VERSION 3.20)
project(Ribbon3DGPU LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------ kernel source for run-time compilation
set(KERNEL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
add_custom_command(
    OUTPUT ${GEN_DIR}/RibbonKernelSource.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GEN_DIR}
    COMMAND ${CMAKE_COMMAND} -DKERNEL_DIR=${KERNEL_DIR} -DOUT=${GEN_DIR}/RibbonKernelSource.h
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedKernel.cmake
    DEPENDS ${KERNEL_DIR}/RibbonShared.h ${KERNEL_DIR}/RibbonKernel.h ${KERNEL_DIR}/RibbonKernel.cl
            ${KERNEL_DIR}/RibbonKernel.metal ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedKernel.cmake
    COMMENT "Embedding Ribbon3D kernel source")
add_custom_target(ribbon_kernel_source DEPENDS ${GEN_DIR}/RibbonKernelSource.h)

# ------------------------------------------------------------------ portable core
find_package(Threads REQUIRED)
add_library(ribbon_core STATIC
    src/core/RibbonGeometry.cpp
    src/core/RibbonPacket.cpp
    src/core/RibbonCPU.cpp)
target_include_directories(ribbon_core PUBLIC src ${GEN_DIR})
target_link_libraries(ribbon_core PUBLIC Threads::Threads)
add_dependencies(ribbon_core ribbon_kernel_source)
set_target_properties(ribbon_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(ribbon_core PRIVATE /W4 /fp:fast)
else()
    target_compile_options(ribbon_core PRIVATE -Wall -Wextra -O2)
endif()

set(AE_SDK_DIR "" CACHE PATH "Path to the After Effects SDK 'Examples' folder (contains Headers, Util, Resources)")

# ------------------------------------------------------------------ tests (any platform)
option(RIBBON_BUILD_TESTS "Build the core / kernel tests" ON)
if(RIBBON_BUILD_TESTS)
    enable_testing()
    add_executable(ribbon_tests tests/test_core.cpp)
    target_link_libraries(ribbon_tests PRIVATE ribbon_core)
    add_test(NAME ribbon_core COMMAND ribbon_tests)

    find_package(OpenCL QUIET)
    if(OpenCL_FOUND)
        add_executable(ribbon_opencl_test tests/test_opencl.cpp)
        target_link_libraries(ribbon_opencl_test PRIVATE ribbon_core OpenCL::OpenCL)
        add_test(NAME ribbon_opencl_parity COMMAND ribbon_opencl_test)

        # Plugin glue driven by a fake AE host (SDK stub headers), CPU 8/32 bpc and OpenCL GPU path.
        # Only without AE_SDK_DIR: the stub headers must never mix with the real SDK.
        if(NOT AE_SDK_DIR)
            add_executable(ribbon_host_sim tests/host_sim/host_sim.cpp src/ae/Ribbon3D.cpp src/ae/Ribbon3D_GPU.cpp
                                           src/ae/Ribbon3D_GPULibs.cpp src/ae/Ribbon3D_Nulls.cpp)
            target_include_directories(ribbon_host_sim PRIVATE tests/host_sim/sdk_stub src src/ae ${GEN_DIR})
            target_compile_definitions(ribbon_host_sim PRIVATE RB_HAS_OPENCL RB_HAS_CUDA)
            target_link_libraries(ribbon_host_sim PRIVATE ribbon_core OpenCL::OpenCL ${CMAKE_DL_LIBS})
            add_test(NAME ribbon_host_sim COMMAND ribbon_host_sim)
            # The second effect, "Ribbon 3D GPU Multi-Mask": the same sources with RB_MULTI_MASK.
            add_executable(ribbon_host_sim_multi tests/host_sim/host_sim.cpp src/ae/Ribbon3D.cpp src/ae/Ribbon3D_GPU.cpp
                                                 src/ae/Ribbon3D_GPULibs.cpp src/ae/Ribbon3D_Nulls.cpp)
            target_include_directories(ribbon_host_sim_multi PRIVATE tests/host_sim/sdk_stub src src/ae ${GEN_DIR})
            target_compile_definitions(ribbon_host_sim_multi PRIVATE RB_HAS_OPENCL RB_HAS_CUDA RB_MULTI_MASK)
            target_link_libraries(ribbon_host_sim_multi PRIVATE ribbon_core OpenCL::OpenCL ${CMAKE_DL_LIBS})
            add_test(NAME ribbon_host_sim_multi COMMAND ribbon_host_sim_multi)
        endif()
    endif()
endif()

# ------------------------------------------------------------------ After Effects plugin (Windows / macOS)
if(AE_SDK_DIR)
    include(cmake/AEPlugin.cmake)
endif()
