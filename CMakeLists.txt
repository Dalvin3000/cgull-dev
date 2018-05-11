# Copyright 2018 Vladislav Yaremenko
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.9.0)

PROJECT(cgull)

# some base and naming vars
OPTION(CGULL_DEBUG_GUTS "Set to true to obtain expensive and full debug information.")
OPTION(CGULL_USE_NAMESPACE "Helps escape 'Promise' symbol as 'CGull::Promise'.")

SET(ROOT ${PROJECT_NAME})

SET(PROJECT_NAME_STATIC         ${PROJECT_NAME}-s)
SET(PROJECT_NAME_QT             ${PROJECT_NAME}-qt)
SET(PROJECT_NAME_QT_STATIC      ${PROJECT_NAME}-qt-s)
SET(PROJECT_NAME_UV             ${PROJECT_NAME}-uv)
SET(PROJECT_NAME_UV_STATIC      ${PROJECT_NAME}-uv-s)

SET(_CMAKE_DEBUG_POSTFIX d)


# collecting sources
FILE(GLOB MAIN      FOLLOW_SYMLINKS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Promise PromiseQt PromiseUV)
FILE(GLOB TESTS     FOLLOW_SYMLINKS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} tests/*.h              tests/*.cpp             )
FILE(GLOB GUTS_S    FOLLOW_SYMLINKS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}                        ${ROOT}/*.cpp           )
FILE(GLOB GUTS_H    FOLLOW_SYMLINKS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${ROOT}/*.h            ${ROOT}/*.hpp           )
FILE(GLOB GUTS_UV_S FOLLOW_SYMLINKS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}                        ${ROOT}-uv/*.cpp        )
FILE(GLOB GUTS_UV_H FOLLOW_SYMLINKS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${ROOT}-uv/*.h                                 )
FILE(GLOB GUTS_QT_S FOLLOW_SYMLINKS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}                        ${ROOT}-qt/*.cpp        )
FILE(GLOB GUTS_QT_H FOLLOW_SYMLINKS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${ROOT}-qt/*.h                                 )

SET(GUTS    ${GUTS_S}    ${GUTS_H})
SET(GUTS_UV ${GUTS_UV_S} ${GUTS_UV_H})
SET(GUTS_QT ${GUTS_QT_S} ${GUTS_QT_H})

# grouping hack for some IDEs
FILE(GLOB GUTS_S_SG FOLLOW_SYMLINKS ${CMAKE_CURRENT_SOURCE_DIR}/${ROOT}/*.hpp)
SOURCE_GROUP("Source Files" FILES ${GUTS_S_SG})


# external deps search
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(Boost)


# common library creation macro
FUNCTION(MAKE_LIB PNAME)
    SET(o SHARED)
    SET(s VERSION)
    SET(m SOURCES PRIVATE_HEADERS PUBLIC_HEADERS)
    CMAKE_PARSE_ARGUMENTS(_ "${o}" "${s}" "${m}" ${ARGN})

    # default - static linking
    IF(__SHARED)
        SET(LINKAGE SHARED)
    ELSE()
        SET(LINKAGE STATIC)
    ENDIF()

    # make library
    ADD_LIBRARY(${PNAME} ${LINKAGE}  ${__SOURCES} ${__PRIVATE_HEADERS} ${__PUBLIC_HEADERS})

    # compiler feature activation
    TARGET_COMPILE_FEATURES(${PNAME} PUBLIC cxx_std_17)
    IF(MSVC)
        TARGET_COMPILE_OPTIONS(${PNAME} PUBLIC "/std:c++latest")
    ELSE()
        TARGET_COMPILE_OPTIONS(${PNAME} PUBLIC "-std=c++1z")
    ENDIF()

    # common link/comp options
    TARGET_LINK_LIBRARIES(${PNAME} PUBLIC Threads::Threads)
    TARGET_INCLUDE_DIRECTORIES(${PNAME}
        INTERFACE  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include>
    )

    # library version
    IF(__VERSION)
        SET_TARGET_PROPERTIES(${PNAME}
            PROPERTIES  VERSION ${__VERSION}
        )
    ENDIF()

    # headers
    SET_TARGET_PROPERTIES(${PNAME}
        PROPERTIES
            PRIVATE_HEADER  "${__PRIVATE_HEADERS}"
            PUBLIC_HEADER  "${__PUBLIC_HEADERS}"
    )

    # shared/static command defines
    STRING(REGEX REPLACE "[-:]" "_" PNAME_SAFE "${PNAME}")

    IF(__SHARED)
        TARGET_COMPILE_DEFINITIONS(${PNAME} PUBLIC  ${PNAME_SAFE}_SHARED)
        SET_TARGET_PROPERTIES(${PNAME} PROPERTIES  DEBUG_POSTFIX -${_CMAKE_DEBUG_POSTFIX})
        SET(${PNAME_SAFE}_SHARED true PARENT_SCOPE) # for config.no_cmake.h
    ELSE()
        TARGET_COMPILE_DEFINITIONS(${PNAME} PUBLIC  ${PNAME_SAFE}_STATIC)
        SET_TARGET_PROPERTIES(${PNAME} PROPERTIES  DEBUG_POSTFIX ${_CMAKE_DEBUG_POSTFIX})
    ENDIF()
ENDFUNCTION()


# main lib in shared and static form
MAKE_LIB(${PROJECT_NAME} SHARED
    VERSION  0.1.0
    SOURCES  ${GUTS_S}
    PUBLIC_HEADERS  ${MAIN}
    PRIVATE_HEADERS  ${GUTS_H}
)
MAKE_LIB(${PROJECT_NAME_STATIC} STATIC
    VERSION  0.1.0
    SOURCES  ${GUTS_S}
    PUBLIC_HEADERS  ${MAIN}
    PRIVATE_HEADERS  ${GUTS_H}
)

# cmake build marker
TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC CGULL_CMAKE_BUILD)
TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME_STATIC} PUBLIC CGULL_CMAKE_BUILD)

# turn on debug capabilities
IF(CGULL_DEBUG_GUTS)
    IF(TARGET Boost::boost)
        SET(CGULL_HAS_BOOST true)

        TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Boost::boost)
        TARGET_LINK_LIBRARIES(${PROJECT_NAME_STATIC} PUBLIC Boost::boost)
        TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC CGULL_HAS_BOOST=1)
        TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME_STATIC} PUBLIC CGULL_HAS_BOOST=1)
    ENDIF()

    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC CGULL_DEBUG_GUTS)
    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME_STATIC} PUBLIC CGULL_DEBUG_GUTS)
ENDIF()

# turn on symbol escape option
IF(CGULL_USE_NAMESPACE)
    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC CGULL_USE_NAMESPACE)
    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME_STATIC} PUBLIC CGULL_USE_NAMESPACE)
ENDIF()


# export compatibility header
CONFIGURE_FILE(
    config.no_cmake.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.no_cmake.h
)

# installing main libraries
INSTALL(
    TARGETS
        ${PROJECT_NAME}
        ${PROJECT_NAME_STATIC}
    EXPORT  ${PROJECT_NAME}-config

    RUNTIME DESTINATION  bin
    LIBRARY DESTINATION  lib
    ARCHIVE DESTINATION  lib
    PUBLIC_HEADER DESTINATION   include
    PRIVATE_HEADER DESTINATION  include/cgull
)
INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/config.no_cmake.h
    DESTINATION include/cgull
)
INSTALL(
    EXPORT ${PROJECT_NAME}-config
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake
)


#################################################
##                   qt spec

SET(CMAKE_AUTOMOC ON)
FIND_PACKAGE(Qt5Core QUIET)

IF(Qt5Core_FOUND)

    #################################################
    ##               qt spec shared

    MAKE_LIB(${PROJECT_NAME_QT} SHARED ${GUTS_QT})

    TARGET_LINK_LIBRARIES(${PROJECT_NAME_QT}
        PRIVATE Qt5::Core )

    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME_QT}
        PUBLIC CGULL_QT)

    INSTALL(FILES ${GUTS_QT_H}
        DESTINATION include/cgull/qt )
    INSTALL(TARGETS ${PROJECT_NAME_QT}
        DESTINATION lib
        EXPORT ${PROJECT_NAME_QT}-config)


    #################################################
    ##               qt spec static

    MAKE_LIB(${PROJECT_NAME_QT_STATIC} STATIC ${GUTS_QT})

    TARGET_LINK_LIBRARIES(${PROJECT_NAME_QT_STATIC}
        PRIVATE Qt5::Core )

    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME_QT_STATIC}
        PUBLIC CGULL_QT)

    INSTALL(TARGETS ${PROJECT_NAME_QT_STATIC}
        DESTINATION lib
        EXPORT ${PROJECT_NAME_QT}-config)


    #################################################
    ##               qt spec common

    INSTALL(EXPORT ${PROJECT_NAME_QT}-config
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION lib/cmake)

ELSE()
    MESSAGE(STATUS "Qt not found")
ENDIF()


#################################################
##                   uv spec

FIND_PACKAGE(UV QUIET)

IF(UV_FOUND)

    #################################################
    ##               uv spec shared

    MAKE_LIB(${PROJECT_NAME_UV} SHARED ${GUTS_UV})

    TARGET_LINK_LIBRARIES(${PROJECT_NAME_UV}
        PRIVATE uv::uv )

    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME_UV}
        PUBLIC CGULL_UV)

    INSTALL(FILES ${GUTS_UV_H}
        DESTINATION include/cgull/uv )
    INSTALL(TARGETS ${PROJECT_NAME_UV}
        DESTINATION lib
        EXPORT ${PROJECT_NAME_UV}-config)


    #################################################
    ##               uv spec static

    MAKE_LIB(${PROJECT_NAME_UV_STATIC} STATIC ${GUTS_UV})

    TARGET_LINK_LIBRARIES(${PROJECT_NAME_UV_STATIC}
        PRIVATE Qt5::Core )

    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME_UV_STATIC}
        PUBLIC CGULL_UV)

    INSTALL(TARGETS ${PROJECT_NAME_UV_STATIC}
        DESTINATION lib
        EXPORT ${PROJECT_NAME_UV}-config)


    #################################################
    ##               uv spec common

    INSTALL(EXPORT ${PROJECT_NAME_UV}-config
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION lib/cmake)

ELSE()
    MESSAGE(STATUS "LibUV not found")
ENDIF()


#################################################
##                    tests

FIND_PACKAGE(GTest)
FIND_PACKAGE(Boost)

IF(GTEST_LIBRARIES AND GTEST_INCLUDE_DIRS AND Boost_FOUND)
    SET(PROJECT_NAME_TESTS ${PROJECT_NAME}-tests)

    ADD_EXECUTABLE(${PROJECT_NAME_TESTS} ${TESTS})

    TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME_TESTS} PRIVATE ${GTEST_INCLUDE_DIRS})
    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME_TESTS} PRIVATE GTEST_LANG_CXX11=1 GTEST_STDLIB_CXX11=1)
    TARGET_LINK_LIBRARIES(
            ${PROJECT_NAME}-tests
        PUBLIC
            ${PROJECT_NAME}
            ${GTEST_LIBRARIES}
            Threads::Threads
            Boost::boost
        )

    IF(Qt5Core_FOUND)
        TARGET_LINK_LIBRARIES(${PROJECT_NAME_TESTS} PRIVATE Qt5::Core)
    ENDIF()
ENDIF()
