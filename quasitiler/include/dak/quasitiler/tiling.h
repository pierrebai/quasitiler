#pragma once

#ifndef DAK_QUASITILER_TILING_H
#define DAK_QUASITILER_TILING_H

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <dak/quasitiler/tiling_point.h>
#include <dak/quasitiler/point_reporter.h>
#include <dak/quasitiler/interruptor.h>

#include <vector>


namespace dak::quasitiler
{
   ////////////////////////////////////////////////////////////////////////////
   //
   // A N-dimensional integer lattice tiling.

   struct tiling_t
   {
      ////////////////////////////////////////////////////////////////////////////
      //
      // Necessary constants.
      
      // For radian angles.
      static constexpr double M_PI = 3.14159265358979323846;

      // Maximum number of dimensions of the integer grid.
      static constexpr int MAX_DIM = vertex_t::MAX_DIM;

      // Maximum index to keep a combination of two chosen dimension from the available dimensions.
      static constexpr int MAX_TILE_COMB = MAX_DIM * (MAX_DIM - 1) / 2;

      // Maximum index to keep a combination of three chosen dimension from the available dimensions.
      static constexpr int MAX_CYLR_COMB = MAX_DIM * (MAX_DIM - 1) * (MAX_DIM - 2) / 6;

      // The two dimension that all other dimensions are projected on.
      static constexpr int TARGET_DIM = 2;


      ////////////////////////////////////////////////////////////////////////////
      //
      // Constructors.

      tiling_t(int dimension);

      // init() makes the preliminary computations; the main computation
      // is finding all the hyperplanes that bound the cylinder or region around
      // the generating plane.  Also notice that we use relative offset, so
      // the tiling must have sensible values, for example the ones provided
      // by init_default().
      //
      // init() returns false if it cannot finish the computation for
      // any reason.
      bool init(double relative_offset[]);

      // generate() computes the vertices of the tiling that fit inside
      // the tiling_bounds, plus some more to guarantee that all the tiles partialy
      // intersecting the rectagle given by tiling_bounds are computed.
      //
      // For each tiling vertex found, the function report_point() is called with
      // the corresponding lattice point. report_point() should take proper action
      // with the vertex, e.g. storing it, drawing it, etc.
      //
      // The interruptor is called periodically to provide a way of
      // stoping the computation; should return nonzero for the computation to stop.
      //
      // generate() returns false if it cannot finish the computation for
      // any reason.
      bool generate(double tiling_bounds[2][MAX_DIM], point_reporter_t& reporter, interruptor_t& an_interruptor);

      ////////////////////////////////////////////////////////////////////////////
      //
      // Tiling descriptions.

      int dimensions_count() const { return my_dimensions_count; }
      int tile_combinations_count() const { return my_tile_combinations_count; }
      const std::vector<int>& slope_orders() const { return my_slope_orders; }
      const std::vector<int>& signs() const { return my_signs; }

   private:
      ////////////////////////////////////////////////////////////////////////////
      //
      // Definition of the functions for initializing the tiling.

      bool normalize();

      // We go over all the combinations of 3 vectors out of my_dimensions_count
      // of vectors from the canonical basis. With each combination we
      // find the orthogonal vector to the tiling plane contained in the
      // space generated by the current choice.  This vector will
      // determine one face of the cylinder.
      //
      // For efficiency, we "randomize" the order of the hyperplane
      // criteria. This increases the likelihood that we can reject points
      // quickly when they lie far from the cylinder.
      //
      // We strongly assume that TARGET_DIM is 2 when we use the cross
      // product to compute the orthogonal vector.
      bool compute_cylinder();

      // Initialize the tiling my_coordinate_orders[]. Sort my_coordinate_orders[] wrt the size of the
      // projections of the lattice generators to each of the plane
      // generators.
      void sort_coordinates();

      // Compute the my_parametrization of the tiling plane with respect to
      // the major directions, and the lengths of the diagonals in each
      // direction.
      bool init_parametrization();

      // Changes a generator.
      bool set_generator(const tiling_point_t& point, int index);
      bool rotate_generators(double angle);

      // Find a bounding box for the cylinder in the ambient space.
      // bounds[] is a bounding box of the plane in generators coords.

      void compute_ambient_bounds(double tiling_bounds[2][MAX_DIM], int bounds[2][MAX_DIM]);

      // Find a point in the tiling plane.  The plane is parametrized by
      // the two main canonical directions.  Use these two main directions
      // in scan_index to determine the point in the plane.  Return the
      // result both in the ambient space coords and the plane coords
      void do_parametrization(const vertex_t& scan_index, double plane_point[], tiling_point_t& tiling_point);

      // Check if the point is inside the cylinder, with an epsilon leeway.
      bool in_cylinder(const vertex_t point);

   private:
      // Now we define elementary vector operations.
      double dot_product(double x[], double y[]);

      // Computes x = s * y
      void scalar_mult(double x[], double s, double y[]);

      // Computes x = x + s * y
      void add_to(double x[], double s, double y[]);

   public:
      // Accessed directly by the Drawing class. Oooh, evil.
      double   offset[MAX_DIM];
      double   generator[MAX_DIM][MAX_DIM];
      int      tile_index[MAX_DIM][MAX_DIM];
      int      tile_generator[MAX_TILE_COMB][2];

   private:
      double            my_parametrization[TARGET_DIM][TARGET_DIM];
      int               my_coordinate_orders[MAX_DIM] = { 0 };
      std::vector<int>  my_signs;
      std::vector<int>  my_slope_orders;
      int               my_tile_combinations_count = 0;
      int               my_dimensions_count = 5;
      int               my_cylinder_criteria_count = 0;
      double            my_cylinder_criteria[MAX_CYLR_COMB][MAX_DIM];
   };
}

#endif /* DAK_QUASITILER_TILING_H */
