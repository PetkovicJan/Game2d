#include "TilingWidgetQt.hpp"

#include <QHBoxLayout>
#include <QGraphicsView>
#include <QGraphicsScene>

TilingWidget::TilingWidget(QWidget* parent) : QWidget(parent)
{
}

void TilingWidget::setConfiguration(int window_width, int window_height, int tile_size)
{
  const auto grid_width = window_width / tile_size;
  const auto grid_height = window_height / tile_size;

  const auto actual_window_width = grid_width * tile_size;
  const auto actual_window_height = grid_height * tile_size;

  auto tiles_view = new QGraphicsView();
  auto tiles_scene = new QGraphicsScene();
  tiles_view->setScene(tiles_scene);
  tiles_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  tiles_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  tiles_view->setFixedSize(actual_window_width, actual_window_height);

  for(int i = 0; i < grid_width; ++i)
    for (int j = 0; j < grid_height; ++j)
    {
      const auto left = i * tile_size;
      const auto top = j * tile_size;
      QRectF tile_rect(left, top, tile_size, tile_size);
      tiles_scene->addRect(tile_rect);
    }

  auto top_layout = new QHBoxLayout();
  this->setLayout(top_layout);
  top_layout->addWidget(tiles_view);
}
