cmake_minimum_required(VERSION 3.7.0)

# The dak/quasitiler library, app and its tests.

project(quasitiler)

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)

add_subdirectory(quasitiler)
add_subdirectory(quasitiler_tests)
add_subdirectory(quasitiler_app)

add_subdirectory(dak_utility/utility)
add_subdirectory(dak_utility/geometry)
add_subdirectory(dak_utility/ui)
add_subdirectory(dak_utility/ui_qt)

add_subdirectory(QtAdditions)

