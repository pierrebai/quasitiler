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

      my_colors[0] = a_color_1;
      my_colors[1] = a_color_2;

      build_ui();
      fill_ui();
      connect_ui();
   }

   // Create the UI elements.
   void tile_group_editor_t::build_ui()
   {
      auto layout = new QHBoxLayout(this);

      my_tile_group_name_label = new QLabel;
      layout->addWidget(my_tile_group_name_label);

      for (int i = 0; i < sizeof(my_color_buttons) / sizeof(my_color_buttons[0]); ++i)
      {
         my_color_buttons[i] = new QPushButton;
         layout->addWidget(my_color_buttons[i]);
      }
   }

   // Connect the signals of the UI elements.
   void tile_group_editor_t::connect_ui()
   {
      for (int i = 0; i < sizeof(my_color_buttons) / sizeof(my_color_buttons[0]); ++i)
      {
         my_color_buttons[i]->connect(my_color_buttons[i], &QPushButton::clicked, [self = this, index = i]()
         {
            self->select_color(index);
         });
      }

   }

   // Fill the UI with the intial data.
   void tile_group_editor_t::fill_ui()
   {
      my_tile_group_name_label->setText(QString::asprintf("Tile Group #%d", my_tile_group + 1));

      for (int i = 0; i < sizeof(my_color_buttons) / sizeof(my_color_buttons[0]); ++i)
      {
         QPixmap color_pixmap(16, 16);
         color_pixmap.fill(ui::qt::convert(my_colors[i]));
         my_color_buttons[i]->setIcon(QIcon(color_pixmap));
      }
   }

   void tile_group_editor_t::select_color(int a_color_index)
   {
      const QColor new_color = QColorDialog::getColor(ui::qt::convert(my_colors[a_color_index]), this, tr("Select Color"), QColorDialog::ColorDialogOption::ShowAlphaChannel);

      my_colors[a_color_index] = ui::qt::convert(new_color);
      fill_ui();

      if (on_color_changed)
         on_color_changed(my_colors[a_color_index], my_tile_group, a_color_index);
   }
}

