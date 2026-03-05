cmake_minimum_required(VERSION 3.21)

# ---- Versioning ----
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

# ---- Project ----
project(
    OHUI
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    LANGUAGES CXX
)

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# ---- C++ Standard ----
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

# ---- Strip source paths from binaries (Release only) ----
# Removes full filesystem paths from __FILE__ macros in all targets including dependencies.
# Without this, your local directory structure leaks into the shipped DLL.
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /d1trimfile:\"${CMAKE_SOURCE_DIR}/\"")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /d1trimfile:\"${CMAKE_SOURCE_DIR}/\"")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG:NONE")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG:NONE")
endif()

# ---- Dependencies ----
# spdlog, xbyak, yoga come from vcpkg
find_package(spdlog CONFIG REQUIRED)
find_package(yoga CONFIG REQUIRED)

# CommonLibSSE-NG via FetchContent (SE/AE/VR in single DLL)
include(FetchContent)
message(STATUS "Fetching CommonLibSSE-NG...")
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR ON CACHE BOOL "" FORCE)
set(ENABLE_XBYAK ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    CommonLibSSE
    GIT_REPOSITORY https://github.com/CharmedBaryon/CommonLibSSE-NG.git
    GIT_TAG v3.7.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(CommonLibSSE)

# ---- Shared compile options ----
add_library(ohui_compile_options INTERFACE)
if(MSVC)
    target_compile_options(ohui_compile_options INTERFACE
        /W4
        /WX-
        /permissive-
        /Zc:__cplusplus
        /Zc:preprocessor
        /external:anglebrackets
        /external:W0
        /MP
    )
endif()

# ---- ohui_core: testable logic, no game engine ----
set(CORE_SOURCES
    src/core/Log.cpp
    src/cosave/CosaveManager.cpp
    src/cosave/PersistenceAPI.cpp
    src/dsl/USSParser.cpp
    src/dsl/TokenStore.cpp
    src/dsl/WidgetParser.cpp
    src/layout/YogaWrapper.cpp
    src/widget/WidgetRegistry.cpp
    src/binding/DataBindingEngine.cpp
    src/widget/LayoutProfileManager.cpp
    src/widget/UndoRedoStack.cpp
    src/widget/EditCommands.cpp
    src/widget/MultiSelectState.cpp
    src/widget/EditSession.cpp
    src/binding/BindingSchema.cpp
    src/input/InputContextStack.cpp
    src/input/ActionRegistry.cpp
    src/input/RebindingStore.cpp
    src/input/ButtonPromptResolver.cpp
    src/widget/ViewportManager.cpp
    src/dsl/DSLRuntimeEngine.cpp
    src/mcm/MCMRegistrationEngine.cpp
    src/mcm/MCMCallbackEngine.cpp
    src/mcm/MCMListManager.cpp
    src/mcm/MCM2DefinitionParser.cpp
    src/mcm/MCM2ConditionEngine.cpp
    src/mcm/MCM2PersistenceManager.cpp
    src/mcm/MCM2DefinitionDiff.cpp
    src/mcm/MCM2HotReloadEngine.cpp
    src/i18n/StringTable.cpp
    src/widget/ModWidgetAPI.cpp
    # Each task appends here
)

set(CORE_HEADERS
    include/ohui/core/IFileSystem.h
    include/ohui/core/Log.h
    include/ohui/core/Result.h
    include/ohui/core/Types.h
    include/ohui/cosave/CosaveTypes.h
    include/ohui/cosave/CosaveManager.h
    include/ohui/cosave/PersistenceAPI.h
    include/ohui/dsl/Selector.h
    include/ohui/dsl/USSProperty.h
    include/ohui/dsl/USSRule.h
    include/ohui/dsl/USSParser.h
    include/ohui/dsl/TokenStore.h
    include/ohui/dsl/WidgetAST.h
    include/ohui/dsl/WidgetParser.h
    include/ohui/layout/YogaWrapper.h
    include/ohui/widget/WidgetTypes.h
    include/ohui/widget/WidgetRegistry.h
    include/ohui/binding/BindingTypes.h
    include/ohui/binding/DataBindingEngine.h
    include/ohui/widget/LayoutProfile.h
    include/ohui/widget/LayoutProfileManager.h
    include/ohui/widget/EditTypes.h
    include/ohui/widget/UndoRedoStack.h
    include/ohui/widget/EditCommands.h
    include/ohui/widget/SnapGrid.h
    include/ohui/widget/MultiSelectState.h
    include/ohui/widget/EditSession.h
    include/ohui/binding/BindingSchema.h
    include/ohui/input/InputTypes.h
    include/ohui/input/InputContext.h
    include/ohui/input/InputContextStack.h
    include/ohui/input/ActionRegistry.h
    include/ohui/input/RebindingStore.h
    include/ohui/input/ButtonPromptResolver.h
    include/ohui/widget/IViewportWidget.h
    include/ohui/widget/ViewportManager.h
    include/ohui/dsl/DrawCall.h
    include/ohui/dsl/DSLRuntimeEngine.h
    include/ohui/mcm/MCMTypes.h
    include/ohui/mcm/MCMRegistrationEngine.h
    include/ohui/mcm/MCMCallbackEngine.h
    include/ohui/mcm/MCMListManager.h
    include/ohui/mcm/MCM2Types.h
    include/ohui/mcm/MCM2DefinitionParser.h
    include/ohui/mcm/MCM2ConditionTypes.h
    include/ohui/mcm/MCM2ConditionEngine.h
    include/ohui/mcm/MCM2PersistenceManager.h
    include/ohui/mcm/MCM2DefinitionDiff.h
    include/ohui/mcm/MCM2HotReloadEngine.h
    include/ohui/i18n/StringTypes.h
    include/ohui/i18n/StringTable.h
    include/ohui/core/RegistrationTypes.h
    include/ohui/widget/ModWidgetAPI.h
    # Each task appends here
)

add_library(ohui_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

target_include_directories(ohui_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_precompile_headers(ohui_core PRIVATE include/CorePCH.h)

target_link_libraries(ohui_core
    PUBLIC spdlog::spdlog
    PRIVATE ohui_compile_options yoga::yogacore
)

target_compile_features(ohui_core PRIVATE cxx_std_23)

# ---- OHUI DLL: ships to users ----
set(DLL_SOURCES
    src/main.cpp
    src/core/GameBridge.cpp
)

set(DLL_HEADERS
    include/PCH.h
    include/Version.h
)

add_library(${PROJECT_NAME} SHARED
    ${DLL_SOURCES}
    ${DLL_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/version.rc
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_precompile_headers(${PROJECT_NAME} PRIVATE include/PCH.h)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ohui_core
    CommonLibSSE::CommonLibSSE
    ohui_compile_options
)

# ---- Tests ----
enable_testing()
add_subdirectory(tests)

# ---- Post build ----
# Set SKYRIM_MODS_PATH env var to auto-copy DLL to your mod manager's staging folder
if(DEFINED ENV{SKYRIM_MODS_PATH})
    set(SKYRIM_MODS_PATH $ENV{SKYRIM_MODS_PATH})
endif()

if(DEFINED SKYRIM_MODS_PATH)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SKYRIM_MODS_PATH}/${PROJECT_NAME}/SKSE/Plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${SKYRIM_MODS_PATH}/${PROJECT_NAME}/SKSE/Plugins/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PDB_FILE:${PROJECT_NAME}> "${SKYRIM_MODS_PATH}/${PROJECT_NAME}/SKSE/Plugins/"
        VERBATIM
    )
endif()
