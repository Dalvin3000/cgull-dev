# Copyright 2018 Vladislav Yaremenko
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.9.0)

PROJECT(cgull)

SET(ROOT ${PROJECT_NAME})

SET(PROJECT_NAME_STATIC         ${PROJECT_NAME}-static)
SET(PROJECT_NAME_QT             ${PROJECT_NAME}-qt)
SET(PROJECT_NAME_QT_STATIC      ${PROJECT_NAME}-qt-static)
SET(PROJECT_NAME_UV             ${PROJECT_NAME}-uv)
SET(PROJECT_NAME_UV_STATIC      ${PROJECT_NAME}-uv-static)

SET(CMAKE_DEBUG_POSTFIX -d)


#################################################
##                   sources

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


FIND_PACKAGE(Threads REQUIRED)

MACRO(MAKE_LIB PNAME OPT)
    ADD_LIBRARY(${PNAME} ${OPT} ${ARGN})

    TARGET_COMPILE_FEATURES(${PNAME} PUBLIC cxx_std_17)
    IF(MSVC)
        # SET_TARGET_PROPERTIES(${PNAME} PROPERTIES COMPILE_FLAGS "/std:c++latest")
        TARGET_COMPILE_OPTIONS(${PNAME} PUBLIC "/std:c++latest")
    ENDIF()

    TARGET_LINK_LIBRARIES(${PNAME} PUBLIC Threads::Threads)
    TARGET_INCLUDE_DIRECTORIES(${PNAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include> )

    IF("${OPT}" STREQUAL "SHARED")
        TARGET_COMPILE_DEFINITIONS(${PNAME}
            PUBLIC CGULL_SHARED
            PRIVATE CGULL_EXPORTS)
    ELSE()
        TARGET_COMPILE_DEFINITIONS(${PNAME}
            PUBLIC CGULL_STATIC)
    ENDIF()
ENDMACRO()


#################################################
##             main library shared

MAKE_LIB(${PROJECT_NAME} SHARED ${MAIN} ${GUTS})

INSTALL(FILES ${MAIN}
    DESTINATION include )
INSTALL(FILES ${GUTS_H}
    DESTINATION include/cgull )
INSTALL(TARGETS ${PROJECT_NAME}
    DESTINATION lib
    EXPORT ${PROJECT_NAME}-config )


#################################################
##             main library static

MAKE_LIB(${PROJECT_NAME_STATIC} STATIC ${MAIN} ${GUTS})

INSTALL(TARGETS ${PROJECT_NAME_STATIC}
    DESTINATION lib
    EXPORT ${PROJECT_NAME}-config )


#################################################
##             main library common

INSTALL(EXPORT ${PROJECT_NAME}-config
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake )


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
