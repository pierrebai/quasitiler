#pragma once

#ifndef DAK_QUASITILER_APP_TILE_GROUP_EDITOR_H
#define DAK_QUASITILER_APP_TILE_GROUP_EDITOR_H

#include <dak/QtAdditions/QWidgetListItem.h>
#include <dak/ui/color.h>
#include <dak/ui/qt/color_editor.h>

class QLabel;
class QPushButton;

namespace dak::quasitiler_app
{
   using QWidgetListItem = QtAdditions::QWidgetListItem;

   /////////////////////////////////////////////////////////////////////////
   //
   // Main window of the quasitiler app.

   class tile_group_editor_t : public QWidgetListItem
   {
   public:
      // Create the main window.
      tile_group_editor_t(
         int a_tile_group_index,
         ui::color_t a_color_1, ui::color_t a_color_2);

      std::function<void(ui::color_t a_color, int a_dim_index, int a_color_index)> on_color_changed;

   protected:
      // Create the UI elements.
      void build_ui(ui::color_t a_color_1, ui::color_t a_color_2);

      // Connect the signals of the UI elements.
      void connect_ui();

      void enterEvent(QEvent* event) override { QWidget::enterEvent(event); }
      void leaveEvent(QEvent* event) override { QWidget::leaveEvent(event); }

      // UI.
      QLabel*                 my_tile_group_name_label = nullptr;
      ui::qt::color_editor_t* my_color_editors[2]      = { nullptr, nullptr };

      // Data.
      int my_tile_group = 0;

      Q_OBJECT;
   };
}

#endif /* DAK_QUASITILER_APP_TILE_GROUP_EDITOR_H */

