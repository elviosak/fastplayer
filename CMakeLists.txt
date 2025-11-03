cmake_minimum_required(VERSION 3.15)

project(fastplayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets DBus)

find_package(PkgConfig)
pkg_check_modules(MPV REQUIRED mpv)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mpvwidget.cpp
        mpvwidget.h
        listview.cpp
        listview.h
        listmodel.cpp
        listmodel.h
        qthelper.hpp
        playliststyle.h
        playliststyle.cpp
)


qt_add_executable(fastplayer
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(fastplayer PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::DBus
    ${MPV_LIBRARIES}
)
set(ICON
    resources/fastplayer.svg
)
set(DESKTOP_FILES
    resources/fastplayer.desktop
    resources/fastplayer-new.desktop
)

include(GNUInstallDirs)

install(TARGETS fastplayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES ${ICON}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
install(FILES ${DESKTOP_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)


qt_finalize_executable(fastplayer)

