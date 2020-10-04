#pragma once

#ifndef DAK_QUASITILER_APP_DIMENSION_EDITOR_H
#define DAK_QUASITILER_APP_DIMENSION_EDITOR_H

#include <dak/QtAdditions/QWidgetListItem.h>
#include <dak/ui/color.h>
#include <dak/ui/qt/double_editor.h>

class QLabel;
class QPushButton;

namespace dak::quasitiler_app
{
   using QWidgetListItem = QtAdditions::QWidgetListItem;

   /////////////////////////////////////////////////////////////////////////
   //
   // Main window of the quasitiler app.

   class dimension_editor_t : public QWidgetListItem
   {
   public:
      // Create the main window.
      dimension_editor_t(
         int a_dim_index,
         double a_low_limit, double a_high_limit);

      std::function<void(int a_dim_index, double a_low_limit, double a_high_limit)> on_limits_changed;

   protected:
      // Create the UI elements.
      void build_ui();

      // Connect the signals of the UI elements.
      void connect_ui();

      // Fill the UI with the intial data.
      void fill_ui();

      // UI.
      QLabel*                    my_dimension_name_label = nullptr;
      ui::qt::double_editor_t*   my_limit_editors[2] = { nullptr, nullptr };

      // Data.
      int            my_dimension = 0;
      double         my_limits[2] = { 0., 0. };

      Q_OBJECT;
   };
}

#endif /* DAK_QUASITILER_APP_DIMENSION_EDITOR_H */

