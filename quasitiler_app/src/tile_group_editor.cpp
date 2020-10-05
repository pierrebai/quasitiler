#include <tile_group_editor.h>

#include <dak/ui/qt/convert.h>

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qcolordialog.h>


namespace dak::quasitiler_app
{
   tile_group_editor_t::tile_group_editor_t(
      int a_tile_group_index,
      ui::color_t a_color_1, ui::color_t a_color_2)
   {
      my_tile_group = a_tile_group_index;

      build_ui(a_color_1, a_color_2);
      connect_ui();
   }

   // Create the UI elements.
   void tile_group_editor_t::build_ui(ui::color_t a_color_1, ui::color_t a_color_2)
   {
      auto layout = new QHBoxLayout(this);

      my_tile_group_name_label = new QLabel(QString::asprintf("Tile Group #%d", my_tile_group + 1));
      layout->addWidget(my_tile_group_name_label);

      for (int i = 0; i < sizeof(my_color_editors) / sizeof(my_color_editors[0]); ++i)
      {
         my_color_editors[i] = new ui::qt::color_editor_t(this);
         my_color_editors[i]->set_color(i == 0 ? a_color_1 : a_color_2);
         layout->addWidget(my_color_editors[i]);
      }

      layout->addStretch();
   }

   // Connect the signals of the UI elements.
   void tile_group_editor_t::connect_ui()
   {
      for (int i = 0; i < sizeof(my_color_editors) / sizeof(my_color_editors[0]); ++i)
      {
         my_color_editors[i]->on_color_changed = [self = this, index = i](ui::color_t a_color)
         {
            if (self->on_color_changed)
               self->on_color_changed(a_color, self->my_tile_group, index);
         };
      }
   }
}

