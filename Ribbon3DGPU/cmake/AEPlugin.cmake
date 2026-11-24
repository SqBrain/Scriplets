# Builds the After Effects plugin. Included from CMakeLists.txt when AE_SDK_DIR is set.
#   Windows: Ribbon3DGPU.aex   (CUDA if the CUDA toolkit is found, OpenCL if OpenCL is found)
#   macOS:   Ribbon3DGPU.plugin (Metal; OpenCL if found)

set(AE_HEADERS "${AE_SDK_DIR}/Headers")
set(AE_RESOURCES "${AE_SDK_DIR}/Resources")
if(NOT EXISTS "${AE_HEADERS}/AE_Effect.h")
    message(FATAL_ERROR "AE_SDK_DIR must point to the SDK 'Examples' folder: ${AE_HEADERS}/AE_Effect.h not found")
endif()
set(AE_INCLUDES "${AE_HEADERS}" "${AE_HEADERS}/SP" "${AE_SDK_DIR}/Util")
set(PIPL_R "${CMAKE_CURRENT_SOURCE_DIR}/src/ae/Ribbon3D_PiPL.r")

# 1. PiPL numbers evaluated from the SDK's own flag enums (PiPL and code always agree).
add_executable(ribbon_pipl_gen tools/PiPLValuesGen.cpp)
target_include_directories(ribbon_pipl_gen PRIVATE ${AE_INCLUDES} src/ae)
if(WIN32)
    target_compile_definitions(ribbon_pipl_gen PRIVATE MSWindows WIN32 _WINDOWS NOMINMAX)
endif()
add_custom_command(
    OUTPUT ${GEN_DIR}/Ribbon3D_PiPLValues.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GEN_DIR}
    COMMAND ribbon_pipl_gen ${GEN_DIR}/Ribbon3D_PiPLValues.h
    DEPENDS ribbon_pipl_gen ${CMAKE_CURRENT_SOURCE_DIR}/src/ae/Ribbon3D_Flags.h
    VERBATIM)

# 2. The plugin.
add_library(Ribbon3DGPU MODULE src/ae/Ribbon3D.cpp src/ae/Ribbon3D_GPU.cpp src/ae/Ribbon3D_Nulls.cpp)
target_include_directories(Ribbon3DGPU PRIVATE ${AE_INCLUDES} src src/ae ${GEN_DIR})
target_link_libraries(Ribbon3DGPU PRIVATE ribbon_core)
add_dependencies(Ribbon3DGPU ribbon_kernel_source)

# OpenCL and CUDA are loaded at run time (Ribbon3D_GPULibs): no SDK or toolkit needed to build.
target_sources(Ribbon3DGPU PRIVATE src/ae/Ribbon3D_GPULibs.cpp)
if(WIN32)
    target_compile_definitions(Ribbon3DGPU PRIVATE RB_HAS_OPENCL RB_HAS_CUDA)
elseif(APPLE)
    target_compile_definitions(Ribbon3DGPU PRIVATE RB_HAS_OPENCL)
endif()

if(WIN32)
    target_compile_definitions(Ribbon3DGPU PRIVATE MSWindows WIN32 _WINDOWS NOMINMAX _CRT_SECURE_NO_WARNINGS)
    set_target_properties(Ribbon3DGPU PROPERTIES PREFIX "" SUFFIX ".aex")
    # PiPL: preprocess .r -> PiPLtool -> preprocess -> .rc (same steps as the SDK samples).
    add_custom_command(
        OUTPUT ${GEN_DIR}/Ribbon3D_PiPL.rc
        COMMAND ${CMAKE_CXX_COMPILER} /nologo /I${AE_HEADERS} /I${GEN_DIR} /EP /P /Fi${GEN_DIR}/Ribbon3D_PiPL.rr /Tc${PIPL_R}
        COMMAND ${AE_RESOURCES}/PiPLtool.exe ${GEN_DIR}/Ribbon3D_PiPL.rr ${GEN_DIR}/Ribbon3D_PiPL.rrc
        COMMAND ${CMAKE_CXX_COMPILER} /nologo /DMSWindows /EP /P /Fi${GEN_DIR}/Ribbon3D_PiPL.rc /Tc${GEN_DIR}/Ribbon3D_PiPL.rrc
        DEPENDS ${PIPL_R} ${GEN_DIR}/Ribbon3D_PiPLValues.h
        VERBATIM)
    target_sources(Ribbon3DGPU PRIVATE ${GEN_DIR}/Ribbon3D_PiPL.rc)
elseif(APPLE)
    enable_language(OBJCXX)
    target_sources(Ribbon3DGPU PRIVATE src/ae/Ribbon3D_Metal.mm)
    set_source_files_properties(src/ae/Ribbon3D_Metal.mm PROPERTIES COMPILE_OPTIONS "-fno-objc-arc")
    target_compile_definitions(Ribbon3DGPU PRIVATE RB_HAS_METAL)
    target_link_libraries(Ribbon3DGPU PRIVATE "-framework Metal" "-framework Foundation")
    set_target_properties(Ribbon3DGPU PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "plugin"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in)
    add_custom_command(
        OUTPUT ${GEN_DIR}/Ribbon3DGPU.rsrc
        COMMAND xcrun Rez -o ${GEN_DIR}/Ribbon3DGPU.rsrc -useDF -script Roman -d __MACH__ -d AE_OS_MAC
                -i ${AE_HEADERS} -i ${AE_RESOURCES} -i ${GEN_DIR} ${PIPL_R}
        DEPENDS ${PIPL_R} ${GEN_DIR}/Ribbon3D_PiPLValues.h
        VERBATIM)
    target_sources(Ribbon3DGPU PRIVATE ${GEN_DIR}/Ribbon3DGPU.rsrc)
    set_source_files_properties(${GEN_DIR}/Ribbon3DGPU.rsrc PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
else()
    message(WARNING "The After Effects plugin can only be built on Windows or macOS")
endif()
