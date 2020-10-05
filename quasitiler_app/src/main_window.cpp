#include <main_window.h>
#include <dimension_editor.h>
#include <tile_group_editor.h>

#include <dak/ui/qt/convert.h>

#include <resource.h>

#include <dak/QtAdditions/QtUtilities.h>
#include <dak/QtAdditions/QWidgetListWidget.h>
#include <dak/QtAdditions/QWidgetListItem.h>

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qcolordialog.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qerrormessage.h>
#include <QtWidgets/qtoolbar.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qdockwidget.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qtoolbutton.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qgraphicsscene.h>
#include <QtWidgets/qgraphicsitem.h>
#include <QtWidgets/qinputdialog.h>
#include <QtGui/qpolygon.h>


#include <QtGui/qpainter.h>
#include <QtGui/qevent.h>

#include <QtWinExtras/qwinfunctions.h>

#include <QtCore/qstandardpaths.h>
#include <QtCore/qtimer.h>

#include <fstream>
#include <sstream>
#include <iomanip>

namespace dak::quasitiler_app
{
   using namespace QtAdditions;
   using namespace std;

   /////////////////////////////////////////////////////////////////////////
   //
   // Create the main window.

   main_window_t::main_window_t()
   {
      build_ui();
      connect_ui();
      fill_ui();
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Create the UI elements.

   void main_window_t::build_ui()
   {
      setCorner(Qt::Corner::TopLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
      setCorner(Qt::Corner::BottomLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
      setCorner(Qt::Corner::TopRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);
      setCorner(Qt::Corner::BottomRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);

      my_error_message = new QErrorMessage(this);

      build_toolbar_ui();
      build_tiling_ui();
      build_tiling_canvas();

      setWindowIcon(QIcon(QtWin::fromHICON((HICON)::LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, 256, 256, 0))));
   }

   void main_window_t::build_toolbar_ui()
   {
      auto toolbar = new QToolBar();
      toolbar->setObjectName("Main Toolbar");
      toolbar->setIconSize(QSize(32, 32));

      my_generate_tiling_action = CreateAction(tr("Generate Tiling"), IDB_GENERATE_TILING);
      my_generate_tiling_button = CreateToolButton(my_generate_tiling_action);
      toolbar->addWidget(my_generate_tiling_button);

      my_stop_tiling_action = CreateAction(tr("Stop Tiling"), IDB_STOP_TILING);
      my_stop_tiling_button = CreateToolButton(my_stop_tiling_action);
      toolbar->addWidget(my_stop_tiling_button);

      addToolBar(toolbar);
   }

   void main_window_t::build_tiling_ui()
   {
      auto tiling_dock = new QDockWidget(tr("Tiling"));
      tiling_dock->setObjectName("Tiling");
      tiling_dock->setFeatures(QDockWidget::DockWidgetFeature::DockWidgetFloatable | QDockWidget::DockWidgetFeature::DockWidgetMovable);
      auto tiling_container = new QWidget();
      auto tiling_layout = new QVBoxLayout(tiling_container);

      auto info_container = new QWidget();
      auto info_layout = new QHBoxLayout(info_container);
      info_layout->setMargin(0);

      tiling_layout->addWidget(info_container);

      my_dimension_count_label = new QLabel;
      my_dimension_count_label->setText(tr("Dimensions"));
      tiling_layout->addWidget(my_dimension_count_label);

      my_dimension_count_combo = new QComboBox;
      for (int i = 3; i <= 8; ++i)
         my_dimension_count_combo->addItem(QString().asprintf("%d", i), QVariant(i));
      tiling_layout->addWidget(my_dimension_count_combo);

      my_edge_color_editor = new ui::qt::color_editor_t(this, L"Edge Color", my_edge_color);
      tiling_layout->addWidget(my_edge_color_editor);

      my_tiling_list = new QWidgetListWidget();
      my_tiling_list->setMinimumWidth(200);
      tiling_layout->addWidget(my_tiling_list);

      tiling_dock->setWidget(tiling_container);

      addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, tiling_dock);
   }

   void main_window_t::build_tiling_canvas()
   {
      create_color_table();

      auto main_container = new QWidget;
      auto main_layout = new QVBoxLayout(main_container);

      my_tiling_canvas = new canvas_t(this, [self = this](ui::drawing_t& drw)
      {
         self->draw_tiling(drw);
      });
      my_tiling_canvas->transformer.mouse_interaction_modifier = ui::modifiers_t::none;

      main_layout->addWidget(my_tiling_canvas);

      setCentralWidget(main_container);
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Connect the signals of the UI elements.

   void main_window_t::connect_ui()
   {
      // Asynchronous generating.

      connect(this, &main_window_t::generate_tiling_done, this, &main_window_t::handle_generated_tiling, Qt::ConnectionType::QueuedConnection);

      // UI.
      my_dimension_count_combo->connect(my_dimension_count_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [self = this](int an_index)
      {
         self->my_dimensions_count = self->my_dimension_count_combo->currentData().toInt();

         self->stop_tiling();
         self->update_tiling();
         self->generate_tiling();
         self->update_toolbar();
      });

      my_edge_color_editor->on_color_changed = [self = this](ui::color_t a_color)
      {
         self->my_edge_color = a_color;
         self->draw_tiling();
      };

      // Toolbar.

      my_generate_tiling_action->connect(my_generate_tiling_action, &QAction::triggered, [self = this]()
      {
         self->stop_tiling();
         self->generate_tiling();
         self->update_toolbar();
      });

      my_stop_tiling_action->connect(my_stop_tiling_action, &QAction::triggered, [self = this]()
      {
         self->stop_tiling();
         self->update_toolbar();
      });

   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Fill the UI with the intial data.

   void main_window_t::fill_ui()
   {
      update_toolbar();
      my_dimension_count_combo->setCurrentIndex(2);
      stop_tiling();
      update_tiling();
      generate_tiling();
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Tiling.

   void main_window_t::showException(const char* message, const std::exception& ex)
   {
      if (!my_error_message)
         return;

      std::ostringstream stream;
      stream << message << "\n" << ex.what();
      my_error_message->showMessage(stream.str().c_str());
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Draw tiling.

   void main_window_t::create_color_table()
   {
      // Init color table.
      for (int row = 0; row < sizeof(my_color_table) / sizeof(my_color_table[0]); ++row)
      {
         int v = 255 - (row / 6) * 15;
         switch (row % 6)
         {
         case 0:
            my_color_table[row][0] = ui::color_t(v, 80, 80);
            my_color_table[row][1] = ui::color_t(80, 80, v);
            break;
         case 1:
            my_color_table[row][0] = ui::color_t(80, v, 80);
            my_color_table[row][1] = ui::color_t(80, v, v);
            break;
         case 2:
            my_color_table[row][0] = ui::color_t(v, 80, v);
            my_color_table[row][1] = ui::color_t(v, v, 80);
            break;
         case 3:
            my_color_table[row][0] = ui::color_t(v, v / 2, 0);
            my_color_table[row][1] = ui::color_t(0, v / 2, v);
            break;
         case 4:
            my_color_table[row][0] = ui::color_t(v / 2, v, 0);
            my_color_table[row][1] = ui::color_t(0, v, v / 2);
            break;
         case 5:
            my_color_table[row][0] = ui::color_t(v, 0, v / 2);
            my_color_table[row][1] = ui::color_t(v, v / 2, 0);
            break;
         }
      }
   }

   ui::color_t main_window_t::get_color(int a_tile_group_index, int a_parity_index) const
   {
      if (a_tile_group_index < 0 || a_tile_group_index >= sizeof(my_color_table) / sizeof(my_color_table[0]))
         return ui::color_t(0, 0, 0);

      if (a_parity_index < 0 || a_parity_index >= sizeof(my_color_table[0]) / sizeof(my_color_table[0][0]))
         return ui::color_t(0, 0, 0);

      return my_color_table[a_tile_group_index][a_parity_index];
   }

   void main_window_t::set_color(int a_tile_group_index, int a_parity_index, ui::color_t a_color)
   {
      if (a_tile_group_index < 0 || a_tile_group_index >= sizeof(my_color_table) / sizeof(my_color_table[0]))
         return;

      if (a_parity_index < 0 || a_parity_index >= sizeof(my_color_table[0]) / sizeof(my_color_table[0][0]))
         return;

      my_color_table[a_tile_group_index][a_parity_index] = a_color;
      draw_tiling();
   }

   ui::color_t main_window_t::get_tile_color(int tile_index) const
   {
      if (!my_tiling)
         return ui::color_t(0, 0, 0);

      const int row = tile_index / my_dimensions_count;
      const int col = tile_index % my_dimensions_count;

      int row_count;
      if (row >= my_dimensions_count / 2 - 1
              && my_dimensions_count % 2 == 0)
      {
         row_count = my_dimensions_count / 2 - 1;
      }
      else
      {
         row_count = my_dimensions_count - 1;
      }

      // Interpolate color.

      const double cof = (double)col / (double)row_count;
      const double cof_complement = 1. - cof;

      const ui::color_t leftColor  = get_color(row, 0);
      const ui::color_t rightColor = get_color(row, 1);
      return ui::color_t(
         (int)(cof_complement * leftColor.r + cof * rightColor.r),
         (int)(cof_complement * leftColor.g + cof * rightColor.g),
         (int)(cof_complement * leftColor.b + cof * rightColor.b));

   }

   void main_window_t::draw_tiling()
   {
      if (!my_tiling_canvas)
         return;

      my_tiling_canvas->update();
   }

   void main_window_t::draw_tiling(ui::drawing_t& a_drw)
   {
      if (!my_tiling)
         return;

      if (!my_tiling->is_generated())
         return;

      if (!my_drawing)
         return;

      // Get local for speed.

      const tiling_t tiling = *my_tiling;
      const drawing_t drawing = *my_drawing;

      const int dim = tiling.dimensions_count();
      const size_t vertex_count = drawing.my_vertex_storage.size();

      // Compute the projection of the lattice vertices.
      using quasitiler::tiling_point_t;

      tiling_point_t* vertices = new tiling_point_t[vertex_count];

      for (size_t ind = 0; ind < vertex_count; ind++)
      {
         drawing.lattice_to_tiling(drawing.my_vertex_storage[ind], vertices[ind]);
      }

      // Premultiply the edge generators by their correct sign.

      double generator[tiling_t::MAX_DIM][2];
      for (int ind = 0; ind < dim; ++ind)
      {
         generator[ind][0] = tiling.signs()[ind] * tiling.generator[0][ind];
         generator[ind][1] = tiling.signs()[ind] * tiling.generator[1][ind];
      }

      // Display the tiles.

      {
         const double zoom = 30.;

         const ui::stroke_t edgeStroke(1);

         int quad_x[4];
         int quad_y[4];
         const drawing_t::tile_list_t* my_tile_storage = drawing.my_tile_storage;
         for (int comb = tiling.tile_combinations_count(); --comb >= 0; )
         {
            const int gen0 = tiling.tile_generator[comb][0];
            const int gen1 = tiling.tile_generator[comb][1];

            const ui::color_t tileColor = get_tile_color(comb);

            for (size_t ind = 0; ind < my_tile_storage[comb].size(); ++ind)
            {
               const size_t vertex_index = my_tile_storage[comb][ind];
               quad_x[0] = (int)(zoom * (vertices[vertex_index].x));
               quad_y[0] = (int)(zoom * (vertices[vertex_index].y));
               quad_x[1] = (int)(zoom * (vertices[vertex_index].x + generator[gen0][0]));
               quad_y[1] = (int)(zoom * (vertices[vertex_index].y + generator[gen0][1]));
               quad_x[2] = (int)(zoom * (vertices[vertex_index].x + generator[gen0][0] + generator[gen1][0]));
               quad_y[2] = (int)(zoom * (vertices[vertex_index].y + generator[gen0][1] + generator[gen1][1]));
               quad_x[3] = (int)(zoom * (vertices[vertex_index].x + generator[gen1][0]));
               quad_y[3] = (int)(zoom * (vertices[vertex_index].y + generator[gen1][1]));

               {
                  ui::polygon_t polygon;
                  polygon.points.push_back(ui::point_t(quad_x[0], quad_y[0]));
                  polygon.points.push_back(ui::point_t(quad_x[1], quad_y[1]));
                  polygon.points.push_back(ui::point_t(quad_x[2], quad_y[2]));
                  polygon.points.push_back(ui::point_t(quad_x[3], quad_y[3]));
                  polygon.points.push_back(ui::point_t(quad_x[0], quad_y[0]));

                  a_drw.set_color(tileColor);
                  a_drw.fill_polygon(polygon);

                  a_drw.set_color(my_edge_color);
                  a_drw.set_stroke(edgeStroke);
                  a_drw.draw_polygon(polygon);
               }
            }
         }
      }
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // UI updates from data.

   void main_window_t::update_tiling()
   {
      my_tiling_list->clear();

      for (int i = 0; i < my_dimensions_count / 2; ++i)
      {
         auto item = new tile_group_editor_t(i, get_color(i, 0), get_color(i, 1));
         item->on_color_changed = [self = this](ui::color_t a_color, int a_tile_group_index, int a_parity_index)
         {
            self->set_color(a_tile_group_index, a_parity_index, a_color);
         };
         my_tiling_list->addItem(item);
      }

      for (int i = 0; i < my_dimensions_count; ++i)
      {
         auto item = new dimension_editor_t(i, my_tiling_bounds[0][i], my_tiling_bounds[1][i], my_tiling_offsets[i]);
         item->on_limits_changed = [self = this](int a_dim_index, double a_low_limit, double a_high_limit)
         {
            self->my_tiling_bounds[0][a_dim_index] = a_low_limit;
            self->my_tiling_bounds[1][a_dim_index] = a_high_limit;
            self->stop_tiling();
            self->generate_tiling();
         };
         item->on_offset_changed = [self = this](int a_dim_index, double an_offset)
         {
            self->my_tiling_offsets[a_dim_index] = an_offset;
            self->stop_tiling();
            self->generate_tiling();
         };
         my_tiling_list->addItem(item);
      }

      my_stop_generating = true;

      draw_tiling();
      update_toolbar();
   }

   void main_window_t::update_toolbar()
   {
      //my_load_tiling_action->setEnabled(true);
      //my_save_tiling_action->setEnabled(my_tiling != nullptr);
      //my_edit_tiling_action->setEnabled(my_tiling != nullptr);

      my_generate_tiling_action->setEnabled(my_tiling != nullptr);
      my_stop_tiling_action->setEnabled(my_tiling && !my_tiling->is_generated());
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Asynchornous tiling generating update.

   bool main_window_t::interrupted()
   {
      return my_stop_generating;
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Asynchornous tiling generating.

   void main_window_t::generate_tiling()
   {
      my_stop_generating = false;
      my_async_generating = std::async(std::launch::async, [self = this, dim_count = my_dimensions_count]()
      {
         try
         {
            auto tiling = std::make_shared<tiling_t>(dim_count);
            auto drawing = std::make_unique<drawing_t>(tiling);

            tiling->init(self->my_tiling_offsets);
            tiling->generate(self->my_tiling_bounds, *drawing, *self);
            drawing->locate_tiles(*self);

            self->generate_tiling_done(drawing.release());

            return 1;
         }
         catch (const std::exception&)
         {
            return 0;
         }
      });
   }

   void main_window_t::handle_generated_tiling(drawing_t* a_drawing)
   {
      my_tiling = a_drawing->get_tiling();
      my_drawing.reset(a_drawing);

      draw_tiling();
      update_toolbar();
   }

   void main_window_t::stop_tiling()
   {
      my_stop_generating = true;
   }
}
