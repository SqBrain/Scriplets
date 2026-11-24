# Builds the After Effects plugin. Included from CMakeLists.txt when AE_SDK_DIR is set.
#   Windows: Ribbon3DGPU.aex + Ribbon3DGPU_MultiMask.aex       (CUDA and OpenCL, loaded at run time)
#   macOS:   Ribbon3DGPU.plugin + Ribbon3DGPU_MultiMask.plugin (Metal; OpenCL)

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

# 2. The two effects from the same sources: Ribbon3DGPU ("Ribbon 3D GPU") and, with RB_MULTI_MASK,
#    Ribbon3DGPU_MultiMask ("Ribbon 3D GPU Multi-Mask", a ribbon along every mask of the layer).
function(ribbon_ae_plugin target multi)
    set(defs "")
    set(rdefs "")
    if(multi)
        set(defs RB_MULTI_MASK)
        set(rdefs /DRB_MULTI_MASK)
    endif()
    add_library(${target} MODULE src/ae/Ribbon3D.cpp src/ae/Ribbon3D_GPU.cpp src/ae/Ribbon3D_Nulls.cpp src/ae/Ribbon3D_Presets.cpp)
    target_include_directories(${target} PRIVATE ${AE_INCLUDES} src src/ae ${GEN_DIR})
    target_link_libraries(${target} PRIVATE ribbon_core)
    add_dependencies(${target} ribbon_kernel_source)
    target_compile_definitions(${target} PRIVATE ${defs})

    # OpenCL and CUDA are loaded at run time (Ribbon3D_GPULibs): no SDK or toolkit needed to build.
    target_sources(${target} PRIVATE src/ae/Ribbon3D_GPULibs.cpp)
    if(WIN32)
        target_compile_definitions(${target} PRIVATE RB_HAS_OPENCL RB_HAS_CUDA)
    elseif(APPLE)
        target_compile_definitions(${target} PRIVATE RB_HAS_OPENCL)
    endif()

    set(pipl ${GEN_DIR}/${target}_PiPL)
    if(WIN32)
        target_compile_definitions(${target} PRIVATE MSWindows WIN32 _WINDOWS NOMINMAX _CRT_SECURE_NO_WARNINGS)
        set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".aex")
        # PiPL: preprocess .r -> PiPLtool -> preprocess -> .rc (same steps as the SDK samples).
        add_custom_command(
            OUTPUT ${pipl}.rc
            COMMAND ${CMAKE_CXX_COMPILER} /nologo ${rdefs} /I${AE_HEADERS} /I${GEN_DIR} /EP /P /Fi${pipl}.rr /Tc${PIPL_R}
            COMMAND ${AE_RESOURCES}/PiPLtool.exe ${pipl}.rr ${pipl}.rrc
            COMMAND ${CMAKE_CXX_COMPILER} /nologo /DMSWindows /EP /P /Fi${pipl}.rc /Tc${pipl}.rrc
            DEPENDS ${PIPL_R} ${GEN_DIR}/Ribbon3D_PiPLValues.h
            VERBATIM)
        target_sources(${target} PRIVATE ${pipl}.rc)
    elseif(APPLE)
        enable_language(OBJCXX)
        target_sources(${target} PRIVATE src/ae/Ribbon3D_Metal.mm)
        set_source_files_properties(src/ae/Ribbon3D_Metal.mm PROPERTIES COMPILE_OPTIONS "-fno-objc-arc")
        target_compile_definitions(${target} PRIVATE RB_HAS_METAL)
        target_link_libraries(${target} PRIVATE "-framework Metal" "-framework Foundation")
        set_target_properties(${target} PROPERTIES
            BUNDLE TRUE
            BUNDLE_EXTENSION "plugin"
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in)
        set(rez_defs "")
        if(multi)
            set(rez_defs -d RB_MULTI_MASK)
        endif()
        add_custom_command(
            OUTPUT ${pipl}.rsrc
            COMMAND xcrun Rez -o ${pipl}.rsrc -useDF -script Roman -d __MACH__ -d AE_OS_MAC ${rez_defs}
                    -i ${AE_HEADERS} -i ${AE_RESOURCES} -i ${GEN_DIR} ${PIPL_R}
            DEPENDS ${PIPL_R} ${GEN_DIR}/Ribbon3D_PiPLValues.h
            VERBATIM)
        target_sources(${target} PRIVATE ${pipl}.rsrc)
        set_source_files_properties(${pipl}.rsrc PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    else()
        message(WARNING "The After Effects plugin can only be built on Windows or macOS")
    endif()
endfunction()

ribbon_ae_plugin(Ribbon3DGPU OFF)
ribbon_ae_plugin(Ribbon3DGPU_MultiMask ON)
