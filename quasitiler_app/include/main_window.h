#pragma once

#ifndef DAK_QUASITILER_APP_MAIN_WINDOW_H
#define DAK_QUASITILER_APP_MAIN_WINDOW_H

#include <dak/quasitiler/tiling.h>
#include <dak/quasitiler/drawing.h>
#include <dak/quasitiler/interruptor.h>

#include <dak/ui/qt/function_drawing_canvas.h>

#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qlistwidget.h>

#include <dak/utility/progress.h>
#include <dak/utility/stopwatch.h>

#include <chrono>
#include <future>
#include <memory>
#include <filesystem>
#include <optional>

class QToolButton;
class QAction;
class QTreeView;
class QComboBox;
class QDockWidget;
class QTimer;
class QLineEdit;
class QLabel;
class QPushButton;
class QGraphicsView;
class QGraphicsScene;
class QErrorMessage;

namespace dak::QtAdditions
{
   struct QWidgetListWidget;
}

namespace dak::quasitiler_app
{
   using tiling_t  = dak::quasitiler::tiling_t;
   using drawing_t = dak::quasitiler::drawing_t;
   using canvas_t = dak::ui::qt::function_drawing_canvas_t;
   using QWidgetListWidget = QtAdditions::QWidgetListWidget;

   /////////////////////////////////////////////////////////////////////////
   //
   // Main window of the quasitiler app.

   class main_window_t : public QMainWindow, private utility::progress_t, public quasitiler::interruptor_t
   {
   public:
      // Create the main window.
      main_window_t();

   protected:
      // Create the UI elements.
      void build_ui();
      void build_toolbar_ui();
      void build_tiling_ui();
      void build_tiling_canvas();
      void create_color_table();
      ui::color_t get_color(int a_tile_group_index, int a_parity_index) const;
      void set_color(int a_tile_group_index, int a_parity_index, ui::color_t a_color);
      ui::color_t get_tile_color(int tile_index) const;

      // Connect the signals of the UI elements.
      void connect_ui();

      // Fill the UI with the intial data.
      void fill_ui();

      // Tiling.
      void create_tiling(int a_dim_count);
      void load_tiling();
      void save_tiling();
      void edit_tiling();

      // Asynchornous tiling generating.
      void generate_tiling();
      void verify_async_tiling_generating();
      bool is_async_filtering_ready();
      void stop_tiling();

      // Asynchornous tiling generating update.
      void update_progress(size_t a_total_count_so_far) override;

      // Interruptor.
      bool interrupted() override;

      void draw_tiling();
      void draw_tiling(ui::drawing_t& a_drw);
      void update_tiling();
      void update_toolbar();
      void update_generating_attempts();
      void update_generating_time();

      void showException(const char* message, const std::exception& ex);

      // Toolbar buttons.
      QAction*       my_load_tiling_action = nullptr;
      QToolButton*   my_load_tiling_button = nullptr;

      QAction*       my_save_tiling_action = nullptr;
      QToolButton*   my_save_tiling_button = nullptr;

      QAction*       my_edit_tiling_action = nullptr;
      QToolButton*   my_edit_tiling_button = nullptr;

      QAction*       my_generate_tiling_action = nullptr;
      QToolButton*   my_generate_tiling_button = nullptr;

      QAction*       my_stop_tiling_action = nullptr;
      QToolButton*   my_stop_tiling_button = nullptr;

      // UI elements.
      canvas_t*      my_tiling_canvas = nullptr;

      QWidgetListWidget*   my_tiling_list = nullptr;
      QLabel*        my_tiling_label = nullptr;

      QLabel*        my_generating_attempts_label = nullptr;
      QLabel*        my_generating_time_label = nullptr;

      QLabel*        my_dimension_count_label = nullptr;
      QComboBox*     my_dimension_count_combo = nullptr;

      QTimer*        my_generate_tiling_timer = nullptr;
      QErrorMessage* my_error_message = nullptr;


      // Data.
      ui::color_t    my_color_table[tiling_t::MAX_DIM / 2][2];
      double         my_tiling_bounds[2][tiling_t::MAX_DIM] =
      {
         { -20., -20., -20., -20., -20., -20., -20., -20., },
         { 20.,  20.,  20.,  20.,  20.,  20.,  20.,  20., },
      };
      double         my_tiling_offsets[tiling_t::MAX_DIM] =
      {
         0., 0., 0., 0., 0., 0., 0., 0.,
      };

      std::shared_ptr<tiling_t>     my_tiling;
      std::shared_ptr<drawing_t>    my_drawing;
      std::filesystem::path         my_tiling_filename;

      std::future<int>              my_async_generating;
      std::atomic<bool>             my_stop_generating = false;
      std::atomic<size_t>           my_generating_attempts = 0;
      
      dak::utility::stopwatch_t     my_generating_stopwatch;
      std::string                   my_generating_time_buffer;

      Q_OBJECT;
   };
}

#endif /* DAK_QUASITILER_APP_MAIN_WINDOW_H */

