#pragma once

#ifndef DAK_QUASITILER_DRAWING_H
#define DAK_QUASITILER_DRAWING_H

#include <dak/quasitiler/point_reporter.h>
#include <dak/quasitiler/tiling.h>

#include <memory>
#include <vector>


namespace dak::quasitiler
{
   ////////////////////////////////////////////////////////////////////////////
   //
   // What receives the generated tiling points.

   struct drawing_t : point_reporter_t
   {
      using tile_list_t = std::vector<size_t>;
      using vertex_list_t = std::vector<vertex_t>;

      // Constructor, associate the drawing with the given tiling.
      drawing_t(std::shared_ptr<tiling_t> a_tiling) : my_tiling(a_tiling) { }

      // Access to the drawing data.
      const vertex_list_t& get_vertex_storage() const { return my_vertex_storage; }
      const tile_list_t* get_tile_storage() const { return my_tile_storage; }

      // Receives points, point_reporter_t implementation.
      void report_point(const vertex_t& a_point) override;

      // The locate_tiles function goes over each vertex in my_vertex_storage,
      // and finds neighboring vertices also in the list.  This determines
      // the tiles. The coordinates of the tiles are stored in my_tile_storage.
      //
      // The interruptor is called periodically to provide a way of stopping
      // the computation; should return true for the computation to stop.
      //
      // locate_tiles returns false if it cannot finish the computation for
      // any reason.
      bool locate_tiles(interruptor_t& an_interruptor);

      // Convert lattice point to 2D point.
      void lattice_to_tiling(const vertex_t& a_lattice_point, tiling_point_t& a_tiling_point) const;
      void lattice_to_orthogonal(vertex_t a_lattice_point, tiling_point_t& an_ortho_point) const;

   public:
      std::shared_ptr<tiling_t>  my_tiling;
      vertex_list_t              my_vertex_storage;
      tile_list_t                my_tile_storage[tiling_t::MAX_TILE_COMB];

   };
}

#endif /* DAK_QUASITILER_DRAWING_H */
