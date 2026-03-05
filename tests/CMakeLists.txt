find_package(Catch2 3 CONFIG REQUIRED)

set(TEST_SOURCES
    SanityTests.cpp
    LayoutTests.cpp
    CosaveTests.cpp
    USSParserTests.cpp
    PersistenceTests.cpp
    TokenStoreTests.cpp
    WidgetRegistryTests.cpp
    DataBindingTests.cpp
    LayoutProfileTests.cpp
    EditModeTests.cpp
)

add_executable(ohui_tests ${TEST_SOURCES})

target_include_directories(ohui_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_link_libraries(ohui_tests PRIVATE
    ohui_core
    ohui_compile_options
    Catch2::Catch2WithMain
)

target_precompile_headers(ohui_tests REUSE_FROM ohui_core)
target_compile_features(ohui_tests PRIVATE cxx_std_23)

include(CTest)
include(Catch)
catch_discover_tests(ohui_tests)
