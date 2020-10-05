#include <dimension_editor.h>

#include <dak/ui/qt/convert.h>

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qcolordialog.h>


namespace dak::quasitiler_app
{
   dimension_editor_t::dimension_editor_t(
      int a_dim,
      double a_low_limit, double a_high_limit,
      double an_offset)
   {
      my_dimension = a_dim;

      my_limits[0] = a_low_limit;
      my_limits[1] = a_high_limit;

      my_offset = an_offset;

      build_ui();
      fill_ui();
      connect_ui();
   }

   // Create the UI elements.
   void dimension_editor_t::build_ui()
   {
      auto layout = new QVBoxLayout(this);

      my_dimension_name_label = new QLabel;
      layout->addWidget(my_dimension_name_label);

      for (int i = 0; i < sizeof(my_limits) / sizeof(my_limits[0]); ++i)
      {
         my_limit_editors[i] = new ui::qt::double_editor_t(this, i == 0 ? L"Low" : L"High", my_limits[i]);
         my_limit_editors[i]->set_limits(-100., 100., 1.);
         layout->addWidget(my_limit_editors[i]);
      }

      my_offset_editor = new ui::qt::double_editor_t(this, L"Offset", my_offset);
      my_offset_editor->set_limits(-100., 100., 1.);
      layout->addWidget(my_offset_editor);
   }

   // Connect the signals of the UI elements.
   void dimension_editor_t::connect_ui()
   {
      for (int i = 0; i < sizeof(my_limits) / sizeof(my_limits[0]); ++i)
      {
         my_limit_editors[i]->value_changed = [self = this, index = i](double a_value, bool is_interacting)
         {
            //if (is_interacting)
            //   return;

            self->my_limits[index] = a_value;
            if (self->on_limits_changed)
               self->on_limits_changed(self->my_dimension, self->my_limits[0], self->my_limits[1]);
         };
      }

      my_offset_editor->value_changed = [self = this](double a_value, bool is_interacting)
      {
         //if (is_interacting)
         //   return;

         self->my_offset = a_value;
         if (self->on_offset_changed)
            self->on_offset_changed(self->my_dimension, self->my_offset);
      };

   }

   // Fill the UI with the intial data.
   void dimension_editor_t::fill_ui()
   {
      my_dimension_name_label->setText(QString::asprintf("Dimension #%d", my_dimension + 1));

      for (int i = 0; i < sizeof(my_limits) / sizeof(my_limits[0]); ++i)
      {
         my_limit_editors[i]->set_value(my_limits[i]);
      }

      my_offset_editor->set_value(my_offset);
   }
}

