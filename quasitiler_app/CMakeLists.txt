# Qt 5 stuff

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Gui REQUIRED)
find_package(Qt5 COMPONENTS WinExtras REQUIRED)

add_executable(quasitiler_app WIN32
   src/quasitiler_app.cpp
   src/main_window.cpp                  include/main_window.h
   src/dimension_editor.cpp             include/dimension_editor.h
   src/tile_group_editor.cpp            include/tile_group_editor.h

   resources/resource.rc                resources/resource.h

   "${PROJECT_SOURCE_DIR}/App.png"
   "${PROJECT_SOURCE_DIR}/LICENSE"
   "${PROJECT_SOURCE_DIR}/README.md"
)

set_target_properties(quasitiler_app PROPERTIES OUTPUT_NAME "Quasitiler")

target_link_libraries(quasitiler_app PUBLIC
   quasitiler
   utility
   geometry
   ui
   ui_qt
   QtAdditions
   Qt5::Widgets Qt5::Gui Qt5::Core Qt5::WinExtras
)

target_compile_features(quasitiler_app PUBLIC
   cxx_std_20
)

target_include_directories(quasitiler_app PUBLIC
   include
   resources
   "${PROJECT_SOURCE_DIR}/quasitiler/include"
   "${PROJECT_SOURCE_DIR}/dak_utility/utility/include"
   "${PROJECT_SOURCE_DIR}/dak_utility/geometry/include"
   "${PROJECT_SOURCE_DIR}/dak_utility/ui/include"
   "${PROJECT_SOURCE_DIR}/dak_utility/ui_qt/include"
   "${PROJECT_SOURCE_DIR}/QtAdditions/include"
)

add_custom_command(TARGET quasitiler_app POST_BUILD
   COMMAND "${Qt5_DIR}/../../../bin/windeployqt.exe" --no-webkit2 --no-angle --no-system-d3d-compiler --no-opengl-sw "$<TARGET_FILE:quasitiler_app>"
)

install(
   DIRECTORY
   "$<TARGET_FILE_DIR:quasitiler_app>/"
   DESTINATION .
   COMPONENT application
   PATTERN "*.pdb" EXCLUDE 
   PATTERN "*.ilk" EXCLUDE 
)

set(CPACK_GENERATOR NSIS)
set(CPACK_PACKAGE_NAME "Quasitiler")
set(CPACK_PACKAGE_VENDOR "Pierre Baillargeon")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Quasitiler - An application to create quasi-periodic tilings.")
set(CPACK_PACKAGE_VERSION "1.0.1")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Quasitiler")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

set(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/quasitiler_app/resources/icon.ico")
set(CPACK_NSIS_MUI_UNIICON "${PROJECT_SOURCE_DIR}/quasitiler_app/resources/icon.ico")
set(CPACK_NSIS_INSTALLED_ICON_NAME "Quasitiler.exe")
set(CPACK_NSIS_DISPLAY_NAME "Quasitiler")
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Quasitiler.lnk' '$INSTDIR\\\\Quasitiler.exe'"
)
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$START_MENU\\\\Quasitiler.lnk'"
)
INCLUDE(CPack)

