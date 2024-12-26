cmake_minimum_required(VERSION 3.5)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

file(READ "release/version.txt" APP_VER_FULL)

project(beam-inspector VERSION ${APP_VER_FULL} LANGUAGES C CXX)

string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP APP_VER_YEAR "%Y")

string(REPLACE "." ";" APP_VER_LIST ${APP_VER_FULL})
list(GET APP_VER_LIST 0 APP_VER_MAJOR)
list(GET APP_VER_LIST 1 APP_VER_MINOR)
list(GET APP_VER_LIST 2 APP_VER_PATCH)

configure_file(
    release/version.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    @ONLY
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


option(WITH_IDS "Add support for cameras from IDS (SDK must be installed)" OFF)
set(WITH_IDS OFF)

option(WITH_OPENCV "Add support for OPENCV (OPENCV must be installed)" ON)
set(WITH_OPENCV ON)


#add_compile_options(
#    -O3
#    -ffast-math
#    -funsafe-math-optimizations
#    -msse4.2
#)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets PrintSupport)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets PrintSupport)



file(GLOB_RECURSE DIR_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE DIR_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE DIR_HEADLESS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)
file(GLOB_RECURSE DIR_QRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.qrc)

add_subdirectory(libs/qcustomplot)
add_subdirectory(libs/beam_render)
add_subdirectory(libs/beam_calc)
add_subdirectory(libs/orion)

set(LIB_RESOURCES
    libs/orion/resources.qrc
)

SET(TS_FILES
src/zh_CN.ts
)
find_package(Qt5LinguistTools)
qt5_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${TS_FILES} OPTIONS -source-language en_US -no-obsolete)



if(WITH_OPENCV)
    find_package(OpenCV COMPONENTS core highgui imgproc REQUIRED)
    if(OpenCV_VERSION_MAJOR GREATER 2)
        # for opencv >= 3 we need the imgcodecs module which is not present in opencv < 3 so run again
        find_package(OpenCV COMPONENTS core highgui imgproc imgcodecs videoio REQUIRED)
    endif(OpenCV_VERSION_MAJOR GREATER 2)
    message(STATUS "--------------- OpenCV_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS} ${OpenCV_VERSION}-----------------")


endif()


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${LIB_RESOURCES}
        ${QM_FILES} 
    )
else()
    add_executable(${PROJECT_NAME}
         ${DIR_SRCS}
        ${DIR_HEADERS}
        ${DIR_HEADLESS}
        ${DIR_QRC}
    )
endif()


target_link_libraries(${PROJECT_NAME} PRIVATE
    qcustomplot
    cgn_beam_render
    cgn_beam_calc
    orion
    Qt::Widgets
    ${OpenCV_LIBS}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER org.orion-project.beam-inspector
    MACOSX_BUNDLE_BUNDLE_VERSION ${APP_VER_FULL}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    BUILD_DATE="${BUILD_DATE}"
    APP_VER_MAJOR=${APP_VER_MAJOR}
    APP_VER_MINOR=${APP_VER_MINOR}
    APP_VER_PATCH=${APP_VER_PATCH}
    APP_VER_YEAR=${APP_VER_YEAR}
    QT_USE_QSTRINGBUILDER
)

if(WITH_IDS)
    if (NOT TARGET ids_peak_comfort_c)
        find_package(ids_peak_comfort_c REQUIRED)
    endif()

    add_definitions(-DWITH_IDS)

    get_target_property(IDS_PEAK_INCLUDE_DIR
        ids_peak_comfort_c::ids_peak_comfort_c
        INTERFACE_INCLUDE_DIRECTORIES
    )
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${IDS_PEAK_INCLUDE_DIR}
    )
endif()


if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
