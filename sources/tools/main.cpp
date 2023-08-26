#include <QApplication>
#include <QMainWindow>
#include <QStackedWidget>

#include "ConfiguringWidgetQt.hpp"
#include "TilingWidgetQt.hpp"

int main(int argc, char* argv[])
{
  QApplication a(argc, argv);

  const auto configuring_widget = new ConfiguringWidget();
  const auto tiling_widget = new TilingWidget();

  const auto main_widget = new QStackedWidget();
  main_widget->addWidget(configuring_widget);
  main_widget->addWidget(tiling_widget);

  QObject::connect(configuring_widget, &ConfiguringWidget::configuringDone,
    [main_widget, tiling_widget](int window_width, int window_height, int tile_size) {
      tiling_widget->setConfiguration(window_width, window_height, tile_size);
      main_widget->setCurrentWidget(tiling_widget);
    });

  QMainWindow main_window;
  main_window.setCentralWidget(main_widget);
  main_window.move(200, 200);
  main_window.show();

  return a.exec();
}