
add_library(quasitiler
   include/dak/quasitiler/drawing.h             src/drawing.cpp
   include/dak/quasitiler/interruptor.h
   include/dak/quasitiler/point_reporter.h
   include/dak/quasitiler/tiling.h              src/tiling.cpp
   include/dak/quasitiler/tiling_point.h
)

target_include_directories(quasitiler PUBLIC
   include
   "${PROJECT_SOURCE_DIR}/dak_utility/utility/include"
   "${PROJECT_SOURCE_DIR}/dak_utility/geometry/include"
)

target_compile_features(quasitiler PUBLIC cxx_std_20)

