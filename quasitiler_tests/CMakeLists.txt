
add_library(quasitiler_tests SHARED
   src/drawing_tests.cpp
   src/tiling_tests.cpp

   include/dak/quasitiler_tests/helpers.h
)

target_link_libraries(quasitiler_tests PUBLIC
   quasitiler
   utility
)

target_compile_features(quasitiler_tests PUBLIC cxx_std_20)

target_include_directories(quasitiler_tests PUBLIC
   "${PROJECT_SOURCE_DIR}/quasitiler/include"
   "${PROJECT_SOURCE_DIR}/utility/include"
   "${PROJECT_SOURCE_DIR}/quasitiler_tests/include"
)

