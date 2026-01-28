cmake_minimum_required(VERSION 3.16)
project(TemperatureMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# Find Qwt - try multiple methods
find_package(Qt6Qwt6 QUIET)

if(NOT Qt6Qwt6_FOUND)
    # Try pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(QWT QUIET Qwt6)
    endif()
endif()

# If still not found, try manual search
if(NOT Qt6Qwt6_FOUND AND NOT QWT_FOUND)
    # macOS Homebrew path
    if(APPLE)
        execute_process(
            COMMAND brew --prefix qwt
            OUTPUT_VARIABLE HOMEBREW_QWT_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(HOMEBREW_QWT_PATH)
            find_library(QWT_LIBRARY NAMES qwt PATHS "${HOMEBREW_QWT_PATH}/lib" NO_DEFAULT_PATH)
            find_path(QWT_INCLUDE_DIR NAMES qwt_plot.h PATHS "${HOMEBREW_QWT_PATH}/include" NO_DEFAULT_PATH)
        endif()
    endif()
    
    # Standard paths
    if(NOT QWT_LIBRARY)
        find_library(QWT_LIBRARY NAMES qwt)
    endif()
    if(NOT QWT_INCLUDE_DIR)
        find_path(QWT_INCLUDE_DIR NAMES qwt_plot.h)
    endif()
endif()

add_executable(temperature_gui
    src/temperature_gui.cpp
)

target_link_libraries(temperature_gui
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Link Qwt if found
if(Qt6Qwt6_FOUND)
    target_link_libraries(temperature_gui Qt6::Qwt6)
elseif(QWT_LIBRARY)
    target_include_directories(temperature_gui PRIVATE ${QWT_INCLUDE_DIR})
    target_link_libraries(temperature_gui ${QWT_LIBRARY})
else()
    message(WARNING "Qwt library not found. Build may fail.")
endif()

# For macOS, you might need to set the deployment target
if(APPLE)
    set_target_properties(temperature_gui PROPERTIES
        MACOSX_DEPLOYMENT_TARGET 10.13
    )
endif()
