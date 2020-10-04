#include <main_window.h>

#include <resource.h>

#include <dak/QtAdditions/QtUtilities.h>

#include <QtWidgets/qboxlayout.h>
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
      : progress_t(10000), my_generating_stopwatch(my_generating_time_buffer)
   {
      build_ui();
      fill_ui();
      connect_ui();
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

      my_generate_tiling_timer = new QTimer(this);
      my_generate_tiling_timer->setSingleShot(true);
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

      my_load_tiling_action = CreateAction(tr("Load Tiling"), IDB_OPEN_TILING, QKeySequence(QKeySequence::StandardKey::Open));
      my_load_tiling_button = CreateToolButton(my_load_tiling_action);
      toolbar->addWidget(my_load_tiling_button);

      my_save_tiling_action = CreateAction(tr("Save Tiling"), IDB_SAVE_TILING, QKeySequence(QKeySequence::StandardKey::Save));
      my_save_tiling_button = CreateToolButton(my_save_tiling_action);
      toolbar->addWidget(my_save_tiling_button);

      my_edit_tiling_action = CreateAction(tr("Edit Tiling"), IDB_EDIT_TILING);
      my_edit_tiling_button = CreateToolButton(my_edit_tiling_action);
      toolbar->addWidget(my_edit_tiling_button);

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

      my_generating_time_label = new QLabel;
      my_generating_time_label->hide();
      info_layout->addWidget(my_generating_time_label);

      my_generating_attempts_label = new QLabel;
      my_generating_attempts_label->hide();
      info_layout->addWidget(my_generating_attempts_label);

      tiling_layout->addWidget(info_container);

      my_tiling_label = new QLabel;
      my_tiling_label->hide();
      tiling_layout->addWidget(my_tiling_label);

      my_dimension_count_label = new QLabel;
      my_dimension_count_label->setText(tr("Dimensions"));
      tiling_layout->addWidget(my_dimension_count_label);

      my_dimension_count_combo = new QComboBox;
      for (int i = 3; i <= 8; ++i)
         my_dimension_count_combo->addItem(QString().asprintf("%d", i), QVariant(i));
      tiling_layout->addWidget(my_dimension_count_combo);

      my_tiling_list = new QListWidget();
      my_tiling_list->setMinimumWidth(200);
      my_tiling_list->setSelectionMode(QListWidget::SelectionMode::SingleSelection);
      my_tiling_list->setSelectionBehavior(QListWidget::SelectionBehavior::SelectRows);
      tiling_layout->addWidget(my_tiling_list);

      tiling_dock->setWidget(tiling_container);

      addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, tiling_dock);
   }

   void main_window_t::build_tiling_canvas()
   {
      create_color_table();

      auto main_container = new QWidget;
      auto main_layout = new QVBoxLayout(main_container);

      my_tiling_canvas = new canvas_t(this, [self=this](ui::drawing_t& drw)
      {
         self->draw_tiling(drw);
      });
      //my_tiling_canvas->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

      main_layout->addWidget(my_tiling_canvas);

      setCentralWidget(main_container);
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Connect the signals of the UI elements.

   void main_window_t::connect_ui()
   {
      // Asynchronous generating.

      my_generate_tiling_timer->connect(my_generate_tiling_timer, &QTimer::timeout, [self = this]()
      {
         self->verify_async_tiling_generating();
      });

      my_dimension_count_combo->connect(my_dimension_count_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [self = this](int an_index)
      {
         self->generate_tiling();
         self->update_toolbar();
      });

      // Toolbar.

      my_load_tiling_action->connect(my_load_tiling_action, &QAction::triggered, [self = this]()
      {
         self->load_tiling();
         self->update_toolbar();
      });

      my_save_tiling_action->connect(my_save_tiling_action, &QAction::triggered, [self = this]()
      {
         self->save_tiling();
         self->update_toolbar();
      });

      my_edit_tiling_action->connect(my_edit_tiling_action, &QAction::triggered, [self = this]()
      {
         self->edit_tiling();
         self->update_toolbar();
      });

      my_generate_tiling_action->connect(my_generate_tiling_action, &QAction::triggered, [self = this]()
      {
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
      // Nothing to fill initially.
      if (my_tiling)
      {
         my_dimension_count_combo->setCurrentIndex(my_tiling->dimensions_count() - 3);
      }
      update_toolbar();
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Tiling.

   void main_window_t::load_tiling()
   {
      static constexpr char tiling_file_types[] = "Tiling Text files (*.txt);;All files (*.*)";

      try
      {
         // We must stop on load because otherwise the threads are preventing the dialog from opening!
         stop_tiling();

         filesystem::path path = AskOpen(tr("Load Tiling"), tr(tiling_file_types), this);
         if (path.empty())
            return;

         std::ifstream tiling_stream(path);
         //tiling_stream >> my_tiling; // TODO
         my_tiling_filename = path;
         update_tiling();
      }
      catch (const std::exception& ex)
      {
         showException("Could not load the tiling:", ex);
      }
   }

   void main_window_t::save_tiling()
   {
      static constexpr char tiling_file_types[] = "Tiling Text files (*.txt);;All files (*.*)";

      try
      {
         // We must stop on save because otherwise the threads are preventing the dialog from opening!
         stop_tiling();

         filesystem::path path = AskSave(tr("Save Tiling"), tr(tiling_file_types), my_tiling_filename.string().c_str(), this);
         if (path.empty())
            return;

         std::ofstream tiling_stream(path);
         //tiling_stream << my_tiling; TODO
         my_tiling_filename = path;
         update_tiling();
      }
      catch (const std::exception& ex)
      {
         showException("Could not load the tiling:", ex);
      }
   }

   void main_window_t::edit_tiling()
   {
      try
      {
         // We must stop on save because otherwise the threads are preventing the dialog from opening!
         stop_tiling();

         std::string tiling_text;
         {
            std::ostringstream tiling_stream;
            //tiling_stream << my_tiling; TODO
            tiling_text = tiling_stream.str();
         }

         auto new_text = QInputDialog::getMultiLineText(this, "Edit Tiling", "Modify the tiling description", tiling_text.c_str());

         if (!new_text.isEmpty())
         {
            tiling_text = new_text.toStdString();
            std::istringstream tiling_stream(tiling_text);
            //tiling_stream >> my_tiling; TODO
         }

         update_tiling();
      }
      catch (const std::exception& ex)
      {
         showException("Could not edit the tiling:", ex);
      }
   }

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
      for (int row = 0; row < tiling_t::MAX_DIM / 2; ++row)
      {
         int v = 255 - row * 15;
         switch (row % 3)
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
         }
      }
   }

   ui::color_t main_window_t::get_color(int index1, int index2) const
   {
      if (index1 < 0 || index1 >= sizeof(my_color_table) / sizeof(my_color_table[0]))
         return ui::color_t(0, 0, 0);

      if (index2 < 0 || index2 >= sizeof(my_color_table[0]) / sizeof(my_color_table[0][0]))
         return ui::color_t(0, 0, 0);

      return my_color_table[index1][index2];
   }

   ui::color_t main_window_t::get_tile_color(int tile_index) const
   {
      if (!my_tiling)
         return ui::color_t(0, 0, 0);

      const int row = tile_index / my_tiling->dimensions_count();
      const int col = tile_index % my_tiling->dimensions_count();

      int row_count;
      if (row >= my_tiling->dimensions_count() / 2 - 1
              && my_tiling->dimensions_count() % 2 == 0)
      {
         row_count = my_tiling->dimensions_count() / 2 - 1;
      }
      else
      {
         row_count = my_tiling->dimensions_count() - 1;
      }

      // Interpolate color.

      const double cof = (double)col / (double)row_count;

      ui::color_t leftColor  = my_color_table[row][0];
      ui::color_t rightColor = my_color_table[row][1];
      return ui::color_t(
         (int)((1.0 - cof) * leftColor.r   + cof * rightColor.r),
         (int)((1.0 - cof) * leftColor.g + cof * rightColor.g),
         (int)((1.0 - cof) * leftColor.b  + cof * rightColor.b));

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

      //if (tilesDisplayed || edgesDisplayed || verticesDisplayed)
      {
         const double zoom = 30.;

         const ui::color_t edgeColor = ui::color_t(128, 128, 128);
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

                  a_drw.set_color(edgeColor);
                  a_drw.set_stroke(edgeStroke);
                  a_drw.draw_polygon(polygon);
               }
            }

            // Check if the user wants to stop right now

     //	    if ( interrupted ( ) )
     //	       return;
         }
      }
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // UI updates from data.

   void main_window_t::update_tiling()
   {
      if (my_tiling_filename.filename().string().size() > 0)
      {
         my_tiling_label->setText(my_tiling_filename.filename().string().c_str());
         my_tiling_label->show();
      }
      else
      {
         my_tiling_label->hide();
      }

      if (!my_tiling)
         return;

      my_tiling_list->clear();

      // TODO: fill tiling description in list view.

      my_generating_attempts = 0;
      my_generating_stopwatch.elapsed();
      my_stop_generating = true;

      draw_tiling();
      update_generating_attempts();
      update_generating_time();
      update_toolbar();
   }

   void main_window_t::update_generating_time()
   {
      if (my_generating_attempts)
      {
         my_generating_stopwatch.elapsed();

         my_generating_time_label->setText((std::string("Time: ") + my_generating_time_buffer).c_str());

         if (!my_generating_time_label->isVisible())
            my_generating_time_label->show();
      }
      else
      {
         if (my_generating_time_label->isVisible())
            my_generating_time_label->hide();
      }
   }

   void main_window_t::update_generating_attempts()
   {
      if (my_generating_attempts)
      {
         ostringstream stream;
         if (my_generating_attempts < 2 * 1000)
            stream << "Attempts: " << my_generating_attempts;
         else if (my_generating_attempts < 2 * 1000 * 1000)
            stream << "Attempts: " << (my_generating_attempts / 1000) << " thousands";
         else
            stream << "Attempts: " << (my_generating_attempts / (1000 * 1000)) << " millions";

         my_generating_attempts_label->setText(stream.str().c_str());
         if (!my_generating_attempts_label->isVisible())
            my_generating_attempts_label->show();
      }
      else
      {
         if (my_generating_attempts_label->isVisible())
            my_generating_attempts_label->hide();
      }
   }

   void main_window_t::update_toolbar()
   {
      my_load_tiling_action->setEnabled(true);
      my_save_tiling_action->setEnabled(my_tiling != nullptr);
      my_edit_tiling_action->setEnabled(my_tiling != nullptr);

      my_generate_tiling_action->setEnabled(my_tiling != nullptr);
      my_stop_tiling_action->setEnabled(my_async_generating.valid());
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Asynchornous tiling generating update.

   bool main_window_t::interrupted()
   {
      return my_stop_generating;
   }

   void main_window_t::update_progress(size_t a_total_count_so_far)
   {
      my_generating_attempts = a_total_count_so_far;
      if (my_stop_generating)
         throw std::exception("Stop generating the tiling.");
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Asynchornous tiling generating.

   void main_window_t::generate_tiling()
   {
      my_stop_generating = false;
      my_generating_attempts = 0;
      my_generating_stopwatch.start();
      my_async_generating = std::async(std::launch::async, [self = this]()
      {
         try
         {
            double quasi_bounds[2][tiling_t::MAX_DIM] =
            {
               { -20., -20., -20., -20., -20., -20., -20., -20., },
               {  20.,  20.,  20.,  20.,  20.,  20.,  20.,  20., },
            };
            double quasi_offsets[tiling_t::MAX_DIM] =
            {
               0., 0., 0., 0., 0., 0., 0., 0.,
            };

            const int dim_count = self->my_dimension_count_combo->currentData().toInt();

            auto tiling = std::make_shared<tiling_t>(dim_count);
            auto drawing = std::make_shared<drawing_t>(tiling);

            tiling->init(quasi_offsets);
            tiling->generate(quasi_bounds, *drawing, *self);
            drawing->locate_tiles(*self);

            self->my_tiling = tiling;
            self->my_drawing = drawing;
            self->draw_tiling();

            return 1;
         }
         catch (const std::exception&)
         {
            return 0;
         }
      });
      my_generate_tiling_timer->start(500);
   }

   bool main_window_t::is_async_filtering_ready()
   {
      if (!my_async_generating.valid())
         return false;

      if (my_async_generating.wait_for(1us) != future_status::ready)
         return false;

      return true;
   }

   void main_window_t::verify_async_tiling_generating()
   {
      update_generating_attempts();
      update_generating_time();

      if (!is_async_filtering_ready())
      {
         my_generate_tiling_timer->start(500);
         return;
      }

      try
      {
         int result = my_async_generating.get();
      }
      catch (const std::exception& ex)
      {
         showException("Could not generate the tiling:", ex);
      }
      update_tiling();
   }

   void main_window_t::stop_tiling()
   {
      my_stop_generating = true;
   }
}
