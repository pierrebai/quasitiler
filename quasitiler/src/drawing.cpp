#include <dak/quasitiler/drawing.h>

#include <algorithm>


namespace dak::quasitiler
{
   // Receives points, point_reporter_t implementation.
   void drawing_t::report_point(const vertex_t& a_point)
   {
      my_vertex_storage.emplace_back(a_point);
   }

   // The locate_tiles function goes over each vertex in my_vertex_storage,
   // and finds neighboring vertices also in the list.  This determines
   // the tiles. The coordinates of the tiles are stored in my_tile_storage.
   //
   // The interruptor is called periodically to provide a way of stopping
   // the computation; should return true for the computation to stop.
   //
   // locate_tiles returns false if it cannot finish the computation for
   // any reason.

   bool drawing_t::locate_tiles(interruptor_t& an_interruptor)
   {
      std::sort(my_vertex_storage.begin(), my_vertex_storage.end());

      // Go over each vertex and find its neighbors; form the list of tiles accordingly.

      const size_t vertex_count = my_vertex_storage.size();
      for (size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
      {
         vertex_t neighbor = my_vertex_storage[vertex_index];

         // Initialize the tile search loop.
         int gen0 = -1;

         // Check all the neighbors in each direction.
         for (int ind = 0; ind < my_tiling->dimensions_count(); ++ind)
         {
            // Compute the next neighbor.
            int gen1 = my_tiling->slope_orders()[ind];
            neighbor.coords[gen1] += my_tiling->signs()[gen1];

            // Check if the neighbor in the tiling.
            const bool found = std::binary_search(my_vertex_storage.begin(), my_vertex_storage.end(), neighbor);
            if (found)
            {
               if (gen0 >= 0)
                  // We have a new tile, so store in the appropiate array; we could instead draw the tile at this point.
                  my_tile_storage[my_tiling->tile_index[gen0][gen1]].emplace_back(vertex_index);
               gen0 = gen1;
            }

            // Get ready for the next neighbour.
            neighbor.coords[gen1] = my_vertex_storage[vertex_index].coords[gen1];
         }

         // Check if the user wants to stop right now.
         if (0 == (vertex_index % 100) && an_interruptor.interrupted())
            return false;
      }

      return true;
   }

   void drawing_t::lattice_to_tiling(const vertex_t& a_lattice_point, tiling_point_t& a_tiling_point) const
   {
      a_tiling_point.x = a_tiling_point.y = 0.0f;
      for (int ind = 0; ind < my_tiling->dimensions_count(); ++ind)
      {
         a_tiling_point.x += a_lattice_point.coords[ind] * my_tiling->generator[0][ind];
         a_tiling_point.y += a_lattice_point.coords[ind] * my_tiling->generator[1][ind];
      }
   }

   void drawing_t::lattice_to_orthogonal(vertex_t a_lattice_point, tiling_point_t& an_ortho_point) const
   {
      an_ortho_point.x = an_ortho_point.y = 0.0f;
      for (int ind = 0; ind < my_tiling->dimensions_count(); ++ind)
      {
         an_ortho_point.y += (a_lattice_point.coords[ind] - my_tiling->offset[ind]) * my_tiling->generator[tiling_t::TARGET_DIM][ind];
         an_ortho_point.x += (a_lattice_point.coords[ind] - my_tiling->offset[ind]) * my_tiling->generator[tiling_t::TARGET_DIM + 1][ind];
      }
   }
}
