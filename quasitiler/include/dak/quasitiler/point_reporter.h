#pragma once

#ifndef DAK_QUASITILER_POINT_REPORTER_H
#define DAK_QUASITILER_POINT_REPORTER_H

#include <compare>


namespace dak::quasitiler
{
   ////////////////////////////////////////////////////////////////////////////
   //
   // Integer vertex coordinates. A point in a n-dimensional integer grid.

   struct vertex_t
   {
      static constexpr int MAX_DIM = 8;

      int coords[MAX_DIM] = { 0 };

      auto operator<=>(const vertex_t& an_other) const = default;
      bool operator==(const vertex_t& an_other) const = default;
   };

   ////////////////////////////////////////////////////////////////////////////
   //
   // Report generated tiling points to an interested party.

   struct point_reporter_t
   {
      virtual ~point_reporter_t() = default;

      virtual void report_point(const vertex_t& a_point) = 0;
   };
}

#endif /* DAK_QUASITILER_POINT_REPORTER_H */
