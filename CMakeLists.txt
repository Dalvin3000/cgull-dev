cmake_minimum_required(VERSION 3.18.0)

set(CGULL_STATIC_SUFFIX "_s" CACHE STRING "Library suffix for static modules.")
set(CGULL_SHARED_SUFFIX ""   CACHE STRING "Library suffix for shared modules.")
set(CGULL_DEBUG_SUFFIX  "d"  CACHE STRING "Library suffix for debug modules.")

option(CGULL_BUILD_TESTS "Tells CMake to generate targets for unit tests." on)

project(
    cgull
    VERSION 0.1.0
    DESCRIPTION "Async primitives for C++20 inspired by BlueBird.js"
    HOMEPAGE_URL "https://github.com/Dalvin3000/cgull"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(GenerateExportHeader)


macro(gather_sources SRC HDR PHD PGH)
    file(GLOB_RECURSE SOURCES             CONFIGURE_DEPENDS  ${SRC})
    file(GLOB_RECURSE HEADERS             CONFIGURE_DEPENDS  ${HDR})
    file(GLOB         PUBLIC_HEADERS      CONFIGURE_DEPENDS  ${PHD})
    file(GLOB         PUBLIC_GUTS_HEADERS CONFIGURE_DEPENDS  ${PGH})
endmacro()


function(add_module_variation TARGET_NAME TYPE)
    add_library(${TARGET_NAME} ${TYPE})

    # suffix build-up
    if(TYPE STREQUAL "SHARED")
        set(suffix "${CGULL_SHARED_SUFFIX}")
    else()
        set(suffix "${CGULL_STATIC_SUFFIX}")
    endif()

    # sources
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source Files" FILES ${SOURCES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Header Files" FILES ${HEADERS})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME} PREFIX "Public Header Files" FILES ${PUBLIC_HEADERS})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME} PREFIX "Public Header Files" FILES ${PUBLIC_GUTS_HEADERS})

    target_sources(${TARGET_NAME}
        PRIVATE
            ${SOURCES}
            ${HEADERS}
            ${PUBLIC_HEADERS}
            ${PUBLIC_GUTS_HEADERS}
    )

    # compiler/linker options and flags
    target_compile_options(${TARGET_NAME}
        PRIVATE
            $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:
                -Wunused-result
            >
        PUBLIC
            $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:
                -fcoroutines-ts
                -fcxx-exceptions
                -fexceptions
            >

            $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/await>

            $<$<PLATFORM_ID:Windows>:/MP>
    )

    # custom props
    set_target_properties(${TARGET_NAME}
        PROPERTIES
            OUTPUT_NAME    "${PROJECT_NAME}${suffix}"
            DEBUG_POSTFIX  "${CGULL_DEBUG_SUFFIX}"

            CXX_STANDARD_REQUIRED  on
            CXX_STANDARD           20

            CXX_VISIBILITY_PRESET      hidden
            VISIBILITY_INLINES_HIDDEN  on

            VERSION  "${PROJECT_VERSION}"
    )

    # links
    target_link_libraries(${TARGET_NAME}
        PRIVATE
            Threads::Threads
    )

    # directories
    target_include_directories(${TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    # defines
    if(TYPE STREQUAL "SHARED")
        target_compile_definitions(${TARGET_NAME}
            PUBLIC
                CGULL_SHARED
        )
    endif()

    # install rules
    install(
        FILES ${PUBLIC_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    )
    install(
        FILES ${PUBLIC_GUTS_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/guts
    )
    install(
        TARGETS ${TARGET_NAME}
        EXPORT ${PROJECT_NAME}Config
    )
endfunction()


# external deps
find_package(Threads REQUIRED)

# targets
gather_sources(
    "src/*.cpp"
    "src/*.h"
    "include/cgull/*.h"
    "include/cgull/guts/*.h"
)
add_module_variation(static STATIC)
add_module_variation(shared SHARED)

# shared lib export
generate_export_header(shared
    EXPORT_MACRO_NAME         CGULL_API
    NO_EXPORT_MACRO_NAME      CGULL_LOCAL
    DEPRECATED_MACRO_NAME     CGULL_DEPRECATED
    NO_DEPRECATED_MACRO_NAME  CGULL_NO_DEPRECATED
    EXPORT_FILE_NAME          ${PROJECT_NAME}_export.h
)

install(
    FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

# install cmake rules
install(
    EXPORT ${PROJECT_NAME}Config
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake
    EXPORT_LINK_INTERFACE_LIBRARIES
)

if(ANDROID)
    install(EXPORT_ANDROID_MK ${PROJECT_NAME}Config DESTINATION share/ndk-modules)
endif()

# tests
if(CGULL_BUILD_TESTS)
    find_package(GTest)
    find_package(Boost)

    if(TARGET GTest::gtest)# AND TARGET Boost::boost)
        set(PROJECT_NAME_TESTS ${PROJECT_NAME}-tests)

        file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS  tests/*.h  tests/*.cpp)

        add_executable(${PROJECT_NAME_TESTS}
            ${TEST_SOURCES}
        )

        target_include_directories(${PROJECT_NAME_TESTS}
            PRIVATE
                ${GTEST_INCLUDE_DIRS}
        )

        target_compile_definitions(${PROJECT_NAME_TESTS}
            PRIVATE
                GTEST_LANG_CXX11=1
                GTEST_STDLIB_CXX11=1
        )

        set_target_properties(${PROJECT_NAME_TESTS}
            PROPERTIES
                VS_DEBUGGER_WORKING_DIRECTORY "\$(OutDir)"

                CXX_STANDARD_REQUIRED on
                CXX_STANDARD 20
        )

        target_link_libraries(${PROJECT_NAME_TESTS}
            PRIVATE
                shared
                # Boost::boost
                GTest::gtest
        )

        add_test(
            NAME ${PROJECT_NAME_TESTS}
            COMMAND $<TARGET_FILE:${PROJECT_NAME_TESTS}>
        )
    endif()
endif()
