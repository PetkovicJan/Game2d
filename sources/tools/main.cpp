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

  QMainWindow main_window;
  main_window.setCentralWidget(main_widget);
  main_window.move(200, 200);
  main_window.show();

  return a.exec();
}