#include "TilingWidgetQt.hpp"

#include <QHBoxLayout>
#include <QGraphicsScene>
#include <QGraphicsRectItem>

#include <iostream>

namespace detail
{
  template<typename T>
  T clamp(T val, T min, T max)
  {
    return val < min ? min : (val > max ? max : val);
  }
}

TilesView::TilesView(int grid_width, int grid_height, int tile_size)
{
  auto tiles_scene = new QGraphicsScene(this);
  this->setScene(tiles_scene);
  this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  for(int i = 0; i < grid_width; ++i)
    for (int j = 0; j < grid_height; ++j)
    {
      const auto left = i * tile_size;
      const auto top = j * tile_size;
      QRectF tile_rect(left, top, tile_size, tile_size);
      auto tile_item = tiles_scene->addRect(tile_rect);
    }
}

void TilesView::labelTileAt(QPoint pos)
{
  if (auto item = this->itemAt(pos); item)
  {
    static_cast<QGraphicsRectItem*>(item)->setBrush(QBrush(Qt::green));
    emit tileLabeledAt(pos);
  }
}

void TilesView::mousePressEvent(QMouseEvent* event)
{
  labelTileAt(event->pos());
}

void TilesView::mouseMoveEvent(QMouseEvent* event)
{
  labelTileAt(event->pos());
}

TilingWidget::TilingWidget(QWidget* parent) : QWidget(parent)
{
}

void TilingWidget::setConfiguration(int window_width, int window_height, int tile_size)
{
  const auto grid_width = window_width / tile_size;
  const auto grid_height = window_height / tile_size;

  tiles_.resize(grid_width * grid_height);

  const auto actual_window_width = grid_width * tile_size;
  const auto actual_window_height = grid_height * tile_size;

  auto tiles_view = new TilesView(grid_width, grid_height, tile_size);
  tiles_view->setFixedSize(actual_window_width, actual_window_height);

  auto top_layout = new QHBoxLayout();
  this->setLayout(top_layout);
  top_layout->addWidget(tiles_view);

  QObject::connect(tiles_view, &TilesView::tileLabeledAt, 
    [this, grid_width, grid_height, tile_size](QPoint pos)
    {
      // Compute grid 2d index.
      const auto grid_x = detail::clamp(pos.x() / tile_size, 0, grid_width - 1);
      const auto grid_y = detail::clamp(pos.y() / tile_size, 0, grid_height - 1);

      // Transform into linear index.
      const auto lin_index = grid_y * grid_width + grid_x;

      // Save.
      tiles_.at(lin_index).labeled = true;
    });
}
