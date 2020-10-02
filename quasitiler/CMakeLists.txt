
add_library(quasitiler
   include/dak/quasitiler/drawing.h             src/drawing.cpp
   include/dak/quasitiler/interruptor.h         src/interruptor.cpp
   include/dak/quasitiler/drawing.h             src/drawing.cpp
   include/dak/quasitiler/point_reporter.h      src/point_reporter.cpp
   include/dak/quasitiler/tiling.h              src/tiling.cpp
   include/dak/quasitiler/tiling_point.h        src/tiling_point.cpp
)

target_include_directories(quasitiler PUBLIC
   include
   "${PROJECT_SOURCE_DIR}/utility/include"
)

target_compile_features(quasitiler PUBLIC cxx_std_20)

