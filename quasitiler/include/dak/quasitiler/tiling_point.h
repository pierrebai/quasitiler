#pragma once

#ifndef DAK_QUASITILER_TILING_POINT_H
#define DAK_QUASITILER_TILING_POINT_H

#include <compare>


namespace dak::quasitiler
{
   ////////////////////////////////////////////////////////////////////////////
   //
   // A 2-dimension tiling point.
   //
   // Generated by the projection of a n-diemnsional grid.

   struct tiling_point_t
   {
      double x = 0;
      double y = 0;

      auto operator<=>(const tiling_point_t& an_other) const = default;
      bool operator==(const tiling_point_t& an_other) const = default;
   };
}

#endif /* DAK_QUASITILER_TILING_POINT_H */