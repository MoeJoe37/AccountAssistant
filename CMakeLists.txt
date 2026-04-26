cmake_minimum_required(VERSION 3.20)
project(AccountAssistant VERSION 6.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts PrintSupport Svg)
find_package(ZLIB REQUIRED)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/Datatablewidget.cpp
    src/ClassicDataTableWidget.cpp
    src/Chartselectiondialog.cpp
    src/Draggablechartcard.cpp
    src/Resultswidget.cpp
    src/Accountswidget.cpp
    src/Supplierswidget.cpp
    src/chartswidget.cpp
    src/pdfexporter.cpp
    src/settingsdialog.cpp
)

set(HEADERS
    src/appdata.h
    src/translations.h
    src/themebox.h
    src/mainwindow.h
    src/Datatablewidget.h
    src/ClassicDataTableWidget.h
    src/Chartselectiondialog.h
    src/Draggablechartcard.h
    src/Resultswidget.h
    src/Accountswidget.h
    src/Supplierswidget.h
    src/chartswidget.h
    src/pdfexporter.h
    src/settingsdialog.h
)

if(WIN32)
    add_executable(AccountAssistant WIN32 ${SOURCES} ${HEADERS})
elseif(APPLE)
    add_executable(AccountAssistant MACOSX_BUNDLE ${SOURCES} ${HEADERS})
else()
    add_executable(AccountAssistant ${SOURCES} ${HEADERS})
endif()

if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/icon.ico")
    set(APP_ICON_RC "${CMAKE_BINARY_DIR}/appicon.rc")
    file(WRITE "${APP_ICON_RC}" "IDI_ICON1 ICON \"${CMAKE_SOURCE_DIR}/icon.ico\"\n")
    target_sources(AccountAssistant PRIVATE "${APP_ICON_RC}")
endif()

target_include_directories(AccountAssistant PRIVATE src)

target_link_libraries(AccountAssistant PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets
    Qt6::Charts Qt6::PrintSupport Qt6::Svg
    ZLIB::ZLIB)

# Windows: auto-deploy Qt DLLs after build
if(WIN32)
    find_program(WINDEPLOYQT windeployqt
        HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET AccountAssistant POST_BUILD
            COMMAND ${WINDEPLOYQT}
                --no-translations --no-system-d3d-compiler --no-opengl-sw
                $<TARGET_FILE:AccountAssistant>
            COMMENT "windeployqt...")
    endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/icon.ico")
    add_custom_command(TARGET AccountAssistant POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/icon.ico"
            "$<TARGET_FILE_DIR:AccountAssistant>/icon.ico"
        COMMENT "Copy optional app icon")
endif()

install(TARGETS AccountAssistant
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(EXISTS "${CMAKE_SOURCE_DIR}/icon.ico")
    install(FILES "${CMAKE_SOURCE_DIR}/icon.ico" DESTINATION . OPTIONAL)
endif()
