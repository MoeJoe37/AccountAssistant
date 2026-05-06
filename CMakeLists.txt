cmake_minimum_required(VERSION 3.20)
project(AccountAssistant VERSION 7.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts PrintSupport Svg)
find_package(ZLIB REQUIRED)
include(GNUInstallDirs)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/Datatablewidget.cpp
    src/ClassicDataTableWidget.cpp
    src/Chartselectiondialog.cpp
    src/Draggablechartcard.cpp
    src/Resultswidget.cpp
    src/Accountswidget.cpp
    src/OtherRevenuesWidget.cpp
    src/SummaryWidget.cpp
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
    src/OtherRevenuesWidget.h
    src/SummaryWidget.h
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

# Windows: auto-deploy Qt DLLs, plugins, compiler runtime, and software OpenGL.
# Do not exclude opengl32sw.dll or D3D helper DLLs; on some Windows 11 hybrid-GPU PCs
# the app can start as a background process without showing a window if these fallbacks
# are missing or if the GPU driver cannot create the requested context.
if(WIN32)
    find_program(WINDEPLOYQT windeployqt
        HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET AccountAssistant POST_BUILD
            COMMAND ${WINDEPLOYQT}
                --compiler-runtime
                $<TARGET_FILE:AccountAssistant>
            COMMENT "windeployqt with Windows 11 compatibility fallbacks...")

        get_filename_component(QT_BIN_DIR "${WINDEPLOYQT}" DIRECTORY)
        foreach(_qt_runtime_dll opengl32sw.dll d3dcompiler_47.dll D3DCompiler_47.dll)
            if(EXISTS "${QT_BIN_DIR}/${_qt_runtime_dll}")
                add_custom_command(TARGET AccountAssistant POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${QT_BIN_DIR}/${_qt_runtime_dll}"
                        "$<TARGET_FILE_DIR:AccountAssistant>/${_qt_runtime_dll}"
                    COMMENT "Copy Windows graphics fallback ${_qt_runtime_dll}")
            endif()
        endforeach()
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
