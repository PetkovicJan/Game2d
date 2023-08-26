#include "TilingWidgetQt.hpp"

#include <QHBoxLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QMouseEvent>

#include <iostream>

class TilesView : public QGraphicsView
{
public:
  explicit TilesView(int grid_width, int grid_height, int tile_size)
  {
    auto tiles_scene = new QGraphicsScene();
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

private:
  void labelTileAt(QPoint pos)
  {
    if (auto item = this->itemAt(pos); item)
    {
      static_cast<QGraphicsRectItem*>(item)->setBrush(QBrush(Qt::green));
    }
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    labelTileAt(event->pos());
  }

  void mouseMoveEvent(QMouseEvent* event) override
  {
    labelTileAt(event->pos());
  }
};

TilingWidget::TilingWidget(QWidget* parent) : QWidget(parent)
{
}

void TilingWidget::setConfiguration(int window_width, int window_height, int tile_size)
{
  const auto grid_width = window_width / tile_size;
  const auto grid_height = window_height / tile_size;

  const auto actual_window_width = grid_width * tile_size;
  const auto actual_window_height = grid_height * tile_size;

  auto tiles_view = new TilesView(grid_width, grid_height, tile_size);
  tiles_view->setFixedSize(actual_window_width, actual_window_height);

  auto top_layout = new QHBoxLayout();
  this->setLayout(top_layout);
  top_layout->addWidget(tiles_view);
}
