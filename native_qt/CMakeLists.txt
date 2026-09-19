cmake_minimum_required(VERSION 3.20)
project(PDFCilingiri LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PopplerQt6 REQUIRED poppler-qt6)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/PageCard.cpp
    src/EditorDialog.cpp
    src/ImageEffects.cpp
    src/MemoryCache.cpp
    src/RenderWorker.cpp
    src/Commands.cpp
    native_qt.rc
)

set(HEADERS
    src/MainWindow.h
    src/PageCard.h
    src/EditorDialog.h
    src/ImageEffects.h
    src/MemoryCache.h
    src/RenderWorker.h
    src/Commands.h
)

add_executable(PDFCilingiri WIN32 ${SOURCES} ${HEADERS})

target_include_directories(PDFCilingiri PRIVATE ${PopplerQt6_INCLUDE_DIRS} src)
target_link_directories(PDFCilingiri PRIVATE ${PopplerQt6_LIBRARY_DIRS})
target_link_libraries(PDFCilingiri PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${PopplerQt6_LIBRARIES}
)
