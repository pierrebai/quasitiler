#include <dak/quasitiler/tiling.h>

#include <cmath>
#include <algorithm>


namespace dak::quasitiler
{
   ////////////////////////////////////////////////////////////////////////////
   //
   // Utility functions.

   // Rounding error limit.
   static constexpr double EPSILON = 0.000001;

   static const int my_sign(double f)
   {
      return  f < 0 ? -1
            : f > 0 ? 1
            : 0;
   }

   static const int epsilon_compare(double x, double y)
   {
      return  (x < y - EPSILON) ? -1
            : (x > y + EPSILON) ? 1
            : 0;
   }

   static const int my_floor(double x)
   {
      return (int)floor(x);
   }

   static const int my_ceil(double x)
   {
      return (int)ceil(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   //
   // Constructor.

   tiling_t::tiling_t(int a_dim_count)
      : my_dimensions_count(a_dim_count)
   {
      // Initilize the tiling to the values corresponding
      // to the most symmetrical tiles.
      for (int dim_index = 0; dim_index < my_dimensions_count; ++dim_index)
      {
         double theta = M_PI * ((double)dim_index / (double)my_dimensions_count - 0.5f);
         generator[0][dim_index] = cos(theta);
         generator[1][dim_index] = sin(theta);
      }
   }

   // init() makes the preliminary computations; the main computation
   // is finding all the hyperplanes that bound the cylinder or region around
   // the generating plane.  Also notice that we use relative offset, so
   // the tiling must have sensible values, for example the ones provided
   // by init_default().
   //
   // init() returns false if it cannot finish the computation for
   // any reason.

   bool tiling_t::init(double relative_offset[])
   {
      my_cylinder_criteria_count = my_dimensions_count * (my_dimensions_count - 1) * (my_dimensions_count - 2) / 6;
      my_tile_combinations_count = my_dimensions_count * (my_dimensions_count - 1) / 2;

      // Check the input and complete the generators to an orthonormal
      // basis of the ambient space.

      if (!normalize())
         return false;

      // Express the relative offset ( which is expressed in the
      // generator basis ) in the canonical basis.  We ignore the first
      // two components of the relative offset, since without loss of
      // generality we assume that the offset is orthogonal to the
      // tiling plane.

      for (int ind = 0; ind < my_dimensions_count; ++ind)
         offset[ind] = 0.0f;
      for (int ind = TARGET_DIM; ind < my_dimensions_count; ++ind)
         add_to(offset, relative_offset[ind], generator[ind]);

      if (!compute_cylinder())
         return false;

      // Sort the usual coordinate basis wrt this tiling.

      sort_coordinates();

      // Compute the my_parametrization of the tiling with respect to the
      // major directions, and the lengths of the diagonals in each
      // direction.

      if (!init_parametrization())
         return false;

      return true;
   }


   ////////////////////////////////////////////////////////////////////////////
   //
   // Implementation.

   bool tiling_t::normalize()
   {
      // Complete the generator array to a basis
      // of the ambient space ( hopefully, change latter )

      // Initialize the first generator of the orthogonal space
      // to ( 1, -1, 1, -1,... )

      for (int ind2 = 0; ind2 < my_dimensions_count; ++ind2)
         generator[TARGET_DIM][ind2] = ((ind2 % 2) != 0 ? -1.0f : 1.0f);

      // Initialize the second generator of the orthogonal space
      // to ( 1, 1, 1, ... )

      if (TARGET_DIM + 1 < my_dimensions_count)
         for (int ind2 = 0; ind2 < my_dimensions_count; ++ind2)
            generator[TARGET_DIM + 1][ind2] = 1.0f;

      // Initialize the rest with the canonical basis, which may not
      // give a linearly independent basis, but it is highly improbable

      for (int ind = TARGET_DIM + 2; ind < my_dimensions_count; ++ind)
         for (int ind2 = 0; ind2 < my_dimensions_count; ++ind2)
            generator[ind][ind2] = (ind == ind2 ? 1.0f : 0.0f);

      // Grahm-Schmitt on the generators of tiling's plane

      for (int ind = 0; ind < my_dimensions_count; ++ind)
      {
         double sum[MAX_DIM];
         for (int ind2 = 0; ind2 < my_dimensions_count; ++ind2)
            sum[ind2] = 0.0f;
         for (int ind2 = 0; ind2 < ind; ++ind2)
         {
            double scalar = dot_product(generator[ind], generator[ind2]);
            add_to(sum, scalar, generator[ind2]);
         }
         add_to(generator[ind], -1.0f, sum);

         double scalar = sqrt(dot_product(generator[ind], generator[ind]));

         if (epsilon_compare(scalar, 0) == 0)
            return false;

         scalar_mult(generator[ind], 1 / scalar, generator[ind]);
      }

      // Try to make the first generator of the orthogonal space to have
      // integer coords.  Probably this should be somewhere else, so
      // change latter

      // scalar_mult(generator[TARGET_DIM], sqrt(my_dimensions_count ), generator[TARGET_DIM] );

      // And check that the tiling's plane doesn't contain nor is
      // perpendicular to any of the lattice directions

      for (int ind = 0; ind < my_dimensions_count; ++ind)
      {
         double projection[MAX_DIM];
         for (int ind2 = 0; ind2 < my_dimensions_count; ++ind2)
            projection[ind2] = 0.0f;
         /* Project the ind-th generator into theTiling */
         for (int ind2 = 0; ind2 < TARGET_DIM; ++ind2)
            add_to(projection, generator[ind2][ind], generator[ind2]);
         double scalar = sqrt(dot_product(projection, projection));
         if (epsilon_compare(scalar, 0) == 0)
            return false;
         if (epsilon_compare(scalar, 1) == 0)
            return false;
      }

      return true;
   }

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

   static constexpr int primes[] =
   {
      2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31,
      37, 41, 43, 47, 53, 59, 61, 67, 71, 0
   };

   bool tiling_t::compute_cylinder()
   {
      // Initialize the indexing of the combinations.
      int choice[TARGET_DIM + 1] = { 0 };
      for (int ind = 0; ind <= TARGET_DIM; ++ind)
         choice[ind] = ind;

      // Randomize order of criteria to increase chances of rejection.
      int randomizer = 1;
      int max_abs_res = 1;
      for (int ind = 0; primes[ind] > 1; ++ind)
      {
         if (0 == (my_cylinder_criteria_count % primes[ind]))
            continue;

         int res = primes[ind] % my_cylinder_criteria_count;
         if (res > my_cylinder_criteria_count / 2)
            res -= my_cylinder_criteria_count;

         int abs_res = (int)std::abs(res);
         if (abs_res > max_abs_res)
         {
            max_abs_res = abs_res;
            randomizer = primes[ind];
         }
      }

      // Loop through all the combinations.
      bool choosen[MAX_DIM] = { false };
      for (int local_crit_count = 0; local_crit_count < my_cylinder_criteria_count; ++local_crit_count)
      {
         // Fill criteria in "random" order.
         int crit_index = ((randomizer * local_crit_count) % my_cylinder_criteria_count);

         // Turn flags for the current combination.
         for (int ind = 0; ind < my_dimensions_count; ++ind)
            choosen[ind] = false;
         for (int ind = 0; ind <= TARGET_DIM; ++ind)
            choosen[choice[ind]] = true;

         // Find the vector orthogonal to the current choice of coordinate
         // axis and theTiling subspace.
         double x[3] = { 0 };
         double y[3] = { 0 };
         double z[3] = { 0 };
         for (int ind = 0, dim = 0; ind < my_dimensions_count; ++ind)
         {
            if (choosen[ind])
            {
               x[dim] = generator[0][ind];
               y[dim] = generator[1][ind];
               ++dim;
            }
         }

         // Compute the cross product in R^3.
         z[0] = x[1] * y[2] - y[1] * x[2];
         z[1] = x[2] * y[0] - y[2] * x[0];
         z[2] = x[0] * y[1] - y[0] * x[1];

         // Put back the orthogonal vector back in the ambient space.
         for (int ind = 0, dim = 0; ind < my_dimensions_count; ++ind)
         {
            if (choosen[ind])
            {
               my_cylinder_criteria[crit_index][ind] = z[dim];
               ++dim;
            }
            else
            {
               my_cylinder_criteria[crit_index][ind] = 0.0f;
            }
         }

         // Now find the extreme values for the criteria.

         double scalar = 0.0f;
         int sign = 0;
         for (int ind = 0; ind < my_dimensions_count; ++ind)
         {
            scalar += std::abs(my_cylinder_criteria[crit_index][ind]);
            if (0 == sign)
               sign = my_sign(my_cylinder_criteria[crit_index][ind]);
         }

         // It is against the hypothesis for the tiling to have a
         // singular projection of a face of the cell, so we report
         // this.  The cylinder would be OK, it is only one less face to
         // check.

         if (epsilon_compare(scalar, 0) == 0)
            return false;

         // Multiply by 2.0 since the cell is centered at 0 an has
         // diameter 1/2, divide by scalar so 1.0 is the criteria, and
         // make the first nonzero coordinate positive so we can
         // distinguish between opposite corners.

         scalar_mult(my_cylinder_criteria[crit_index],
            sign * 2.0f / scalar, my_cylinder_criteria[crit_index]);

         // Increment to the next combination.  Find first which choice
         // can be incremented next.

         int ind = TARGET_DIM;
         while (ind > 0 && choice[ind] >= my_dimensions_count - 3 + ind)
            --ind;
         choice[ind++]++;
         for (; ind <= TARGET_DIM; ++ind)
            choice[ind] = choice[ind - 1] + 1;
      }
      return true;
   }

   // Initialize the tiling my_coordinate_orders[]. Sort my_coordinate_orders[] wrt the size of the
   // projections of the lattice generators to each of the plane
   // generators.

   void tiling_t::sort_coordinates()
   {
      // Initialize the tiling my_coordinate_orders[].

      for (int ind0 = 0; ind0 < my_dimensions_count; ++ind0)
         my_coordinate_orders[ind0] = ind0;

      // Sort my_coordinate_orders[] wrt the size of the projections of the lattice
      // generators to each of the plane generators.

      double aux[MAX_DIM];

      for (int ind0 = 0; ind0 < TARGET_DIM; ++ind0)
      {
         // Fill the auxiliary array with the projection lengths.
         for (int ind1 = ind0; ind1 < my_dimensions_count; ++ind1)
            aux[my_coordinate_orders[ind1]] = std::abs(generator[ind0][my_coordinate_orders[ind1]]);

         // Find maximum.
         for (int ind1 = ind0; ind1 < my_dimensions_count; ++ind1)
         {
            if (aux[my_coordinate_orders[ind0]] < aux[my_coordinate_orders[ind1]])
            {
               int temp = my_coordinate_orders[ind0];
               my_coordinate_orders[ind0] = my_coordinate_orders[ind1];
               my_coordinate_orders[ind1] = temp;
            }
         }
      }

      // Sort the directions so the projections have the proper
      // orientation to find the tiles.

      for (int ind0 = 0; ind0 < my_dimensions_count; ++ind0)
      {
         my_slope_orders.emplace_back(ind0);
         my_signs.emplace_back(epsilon_compare(generator[0][ind0], 0));
         if (my_signs[ind0] == 0)
            my_signs[ind0] = 1;
         aux[ind0] = my_signs[ind0] * generator[1][ind0] / sqrt(generator[0][ind0] * generator[0][ind0] + generator[1][ind0] * generator[1][ind0]);
      }

      for (int ind0 = 0; ind0 < my_dimensions_count - 1; ++ind0)
      {
         for (int ind1 = ind0; ind1 < my_dimensions_count; ++ind1)
         {
            if (aux[my_slope_orders[ind0]] > aux[my_slope_orders[ind1]])
            {
               int temp = my_slope_orders[ind0];
               my_slope_orders[ind0] = my_slope_orders[ind1];
               my_slope_orders[ind1] = temp;
            }
         }
      }

      // We generate the tables with all the possible combinations
      // choosing two generators out of my_dimensions_count.  The combinations
      // are generated so that tiles of the same size (in the symmetrical case)
      // are consecutive in the table.

      int comb = 0;
      for (int size = 1; size <= my_dimensions_count / 2; ++size)
      {
         for (int frst = 0; frst < my_dimensions_count - 1; ++frst)
         {
            const int gen0 = my_slope_orders[frst];

            if (frst + size < my_dimensions_count)
            {
               const int gen1 = my_slope_orders[frst + size];
               tile_index[gen0][gen1] = comb;
               tile_generator[comb][0] = gen0;
               tile_generator[comb][1] = gen1;
               ++comb;
            }

            if (frst - size < 0 && size != my_dimensions_count - size)
            {
               const int gen2 = my_slope_orders[frst + my_dimensions_count - size];
               tile_index[gen0][gen2] = comb;
               tile_generator[comb][0] = gen0;
               tile_generator[comb][1] = gen2;
               ++comb;
            }
         }
      }
   }


   // Compute the my_parametrization of the tiling plane with respect to
   // the major directions, and the lengths of the diagonals in each
   // direction.

   bool tiling_t::init_parametrization()
   {
      double a[TARGET_DIM][TARGET_DIM];

      // Assuming that TARGET_DIM is 2, find the inverse matrix.

      a[0][0] = generator[0][my_coordinate_orders[0]];
      a[0][1] = generator[1][my_coordinate_orders[0]];
      a[1][0] = generator[0][my_coordinate_orders[1]];
      a[1][1] = generator[1][my_coordinate_orders[1]];

      double det = a[0][0] * a[1][1] - a[1][0] * a[0][1];
      if (epsilon_compare(det, 0) == 0) return false;

      my_parametrization[0][0] = a[1][1] / det;
      my_parametrization[0][1] = -a[0][1] / det;
      my_parametrization[1][0] = -a[1][0] / det;
      my_parametrization[1][1] = a[0][0] / det;

      return true;
   }

   // Changes a generator.
   bool tiling_t::set_generator(const tiling_point_t& point, int index)
   {
      generator[0][index] = point.x;
      generator[1][index] = point.y;
      return init(offset);
   }

   bool tiling_t::rotate_generators(double angle)
   {
      double cos = ::cos(angle);
      double sin = ::sin(angle);
      for (int ind = 0; ind < my_dimensions_count; ++ind)
      {
         const double x = generator[0][ind];
         const double y = generator[1][ind];
         double x2 = (double)(x * cos - y * sin);
         double y2 = (double)(y * cos + x * sin);
         if (x2 < 0)
         {
            x2 *= -1;
            y2 *= -1;
         }
         generator[0][ind] = x2;
         generator[1][ind] = y2;
      }
      return init(offset);
   }

   // Find a bounding box for the cylinder in the ambient space.
   // bounds[] is a bounding box of the plane in generators coords.
   void tiling_t::compute_ambient_bounds(double tiling_bounds[2][MAX_DIM], int bounds[2][MAX_DIM])
   {
      double corner[2][2][MAX_DIM];

      for (int ind0 = 0; ind0 < 2; ++ind0)
         for (int ind1 = 0; ind1 < 2; ++ind1)
         {
            // Compute the corners of the tiling plane in the ambient space.

            for (int ind = 0; ind < my_dimensions_count; ++ind)
               corner[ind0][ind1][ind] = offset[ind];
            add_to(corner[ind0][ind1], tiling_bounds[ind0][0], generator[0]);
            add_to(corner[ind0][ind1], tiling_bounds[ind1][1], generator[1]);
         }

      // Now we find the max and min.

      double ambient_bounds[2][MAX_DIM];

      for (int ind = 0; ind < my_dimensions_count; ++ind)
      {
         ambient_bounds[0][ind] = corner[0][0][ind];
         ambient_bounds[1][ind] = corner[0][0][ind];
         for (int ind0 = 0; ind0 < 2; ++ind0)
            for (int ind1 = 0; ind1 < 2; ++ind1)
            {
               if (ambient_bounds[0][ind] > corner[ind0][ind1][ind])
                  ambient_bounds[0][ind] = corner[ind0][ind1][ind];
               if (ambient_bounds[1][ind] < corner[ind0][ind1][ind])
                  ambient_bounds[1][ind] = corner[ind0][ind1][ind];
            }

         // Add/substract sqrt of dim to acomodate for the cylinder
         // thickness.

         double thick = (double)sqrt(my_dimensions_count);
         bounds[0][ind] = my_floor(ambient_bounds[0][ind] - thick);
         bounds[1][ind] = my_ceil(ambient_bounds[1][ind] + thick);
      }
   }


   // Find a point in the tiling plane.  The plane is parametrized by
   // the two main canonical directions.  Use these two main directions
   // in scan_index to determine the point in the plane.  Return the
   // result both in the ambient space coords and the plane coords
   void tiling_t::do_parametrization(const vertex_t& scan_index, double plane_point[], tiling_point_t& tiling_point)
   {
      for (int ind0 = 0; ind0 < my_dimensions_count; ++ind0)
         plane_point[ind0] = offset[ind0];

      for (int ind0 = 0; ind0 < TARGET_DIM; ++ind0)
      {
         // Change coordinates in the plane, from canonical main coords,
         // to the generators coords.

         double scalar = 0.0f;
         for (int ind1 = 0; ind1 < TARGET_DIM; ++ind1)
            scalar += my_parametrization[ind0][ind1]
            * (scan_index.coords[my_coordinate_orders[ind1]] - offset[my_coordinate_orders[ind1]]);

         // Compute the projection.

         if (0 != ind0)
            tiling_point.y = scalar;
         else
            tiling_point.x = scalar;

         add_to(plane_point, scalar, generator[ind0]);
      }
   }


   // Returns false if the point is not EPSILON inside the cylinder,
   // true otherwise.
   bool tiling_t::in_cylinder(const vertex_t point)
   {
      // Translate by the offset.

      double trans_point[MAX_DIM];
      for (int ind1 = 0; ind1 < my_dimensions_count; ++ind1)
         trans_point[ind1] = point.coords[ind1] - offset[ind1];

      // Now check if the point is inside all the faces of the cylinder.

      for (int ind = 0; ind < my_cylinder_criteria_count; ++ind)
      {
         // Compute the dot product.  For efficiency, no function call.

         double dot_p = 0.0f;
         double* current_criteria = my_cylinder_criteria[ind];
         for (int ind1 = 0; ind1 < my_dimensions_count; ++ind1)
            dot_p += current_criteria[ind1] * trans_point[ind1];
         double ans = 1.0f - std::abs(dot_p);
         if (ans < EPSILON) return false;  // outside.
      }

      return true;
   }

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

   bool tiling_t::generate(double tiling_bounds[2][MAX_DIM], point_reporter_t& reporter, interruptor_t& an_interruptor)
   {
      my_is_generated = false;

      // Find the bounds relative to the ambient space, for the bounds
      // in the tiling subspace in.

      int bounds[2][MAX_DIM];
      compute_ambient_bounds(tiling_bounds, bounds);

      // Initialize the indices for the scaning of this tiling.

      vertex_t scan_index;
      for (int ind = 0; ind < TARGET_DIM; ++ind)
         scan_index.coords[my_coordinate_orders[ind]] = bounds[0][my_coordinate_orders[ind]];

      // Scaning this tiling.

      double diag = sqrt(2.0);
      double plane_point[MAX_DIM];
      tiling_point_t tiling_point;
      while (scan_index.coords[my_coordinate_orders[0]] <= bounds[1][my_coordinate_orders[0]])
      {
         // Find the next point in the tiling my_parametrization.

         do_parametrization(scan_index, plane_point, tiling_point);

         // Do some preliminary clipping here.

         if (tiling_point.x > (tiling_bounds[0][0] - 2.0f)
            && tiling_point.x < (tiling_bounds[1][0] + 2.0f)
            && tiling_point.y >(tiling_bounds[0][1] - 2.0f)
            && tiling_point.y < (tiling_bounds[1][1] + 2.0f))
         {
            // Find the bounds for the intersection of the tiling's
            // plane with the remaining coordinates.

            int local_bounds[2][MAX_DIM];
            for (int dim = TARGET_DIM; dim < my_dimensions_count; ++dim)
            {
               local_bounds[0][my_coordinate_orders[dim]] = my_ceil(plane_point[my_coordinate_orders[dim]] - diag);
               local_bounds[1][my_coordinate_orders[dim]] = my_floor(plane_point[my_coordinate_orders[dim]] + diag);
            }

            // Scan for all the intersecting points above the current
            // plane_point.

            for (int ind = TARGET_DIM; ind < my_dimensions_count; ++ind)
               scan_index.coords[my_coordinate_orders[ind]] = local_bounds[0][my_coordinate_orders[ind]];

            // Scaning.

            while (scan_index.coords[my_coordinate_orders[TARGET_DIM]] <= local_bounds[1][my_coordinate_orders[TARGET_DIM]])
            {
               if (in_cylinder(scan_index))
                  reporter.report_point(scan_index);

               // Increment the scan_index to the next point.

               int ind = my_dimensions_count - 1;
               while ((++(scan_index.coords[my_coordinate_orders[ind]])) > local_bounds[1][my_coordinate_orders[ind]]
                  && ind > TARGET_DIM)
               {
                  scan_index.coords[my_coordinate_orders[ind]] = local_bounds[0][my_coordinate_orders[ind]];
                  ind--;
               }
            }
         }

         // Find the next point in the scaning in the my_parametrization.

         int ind = TARGET_DIM - 1;
         while ((++(scan_index.coords[my_coordinate_orders[ind]]))
            > bounds[1][my_coordinate_orders[ind]] && ind > 0)
         {
            scan_index.coords[my_coordinate_orders[ind]] = bounds[0][my_coordinate_orders[ind]];
            ind--;
         }

         // Should we abort the computation.

         if (an_interruptor.interrupted())
            return false;
      }

      my_is_generated = true;
      return true;
   }


   // Now we define elementary vector operations.

   double tiling_t::dot_product(double x[], double y[])
   {
      double prod = 0.0f;

      for (int ind = my_dimensions_count; --ind >= 0; )
         prod += x[ind] * y[ind];
      return prod;
   }

   // Computes x = s * y
   void tiling_t::scalar_mult(double x[], double s, double y[])
   {
      for (int ind = my_dimensions_count; --ind >= 0; )
         x[ind] = s * y[ind];
   }

   // Computes x = x + s * y
   void tiling_t::add_to(double x[], double s, double y[])
   {
      for (int ind = my_dimensions_count; --ind >= 0; )
         x[ind] += s * y[ind];
   }
}
