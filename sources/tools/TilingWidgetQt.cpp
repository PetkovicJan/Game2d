#include "TilingWidgetQt.hpp"

#include <QHBoxLayout>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QComboBox>

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

void TilesView::setActiveTileType(TileType type)
{
  switch (type)
  {
  case TileType::Clear:
    active_brush_ = QBrush(Qt::white);
    break;
  case TileType::Ground:
    active_brush_ = QBrush(Qt::green);
    break;
  case TileType::Wall:
    active_brush_ = QBrush(Qt::gray);
    break;
  case TileType::Water:
    active_brush_ = QBrush(Qt::blue);
    break;
  }
}

void TilesView::labelTileAt(QPoint pos)
{
  if (auto item = this->itemAt(pos); item)
  {
    static_cast<QGraphicsRectItem*>(item)->setBrush(active_brush_);
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

  auto type_combo_box = new QComboBox();
  type_combo_box->addItem(QString("Clear"), (int)TileType::Clear);
  type_combo_box->addItem(QString("Ground"), (int)TileType::Ground);
  type_combo_box->addItem(QString("Wall"), (int)TileType::Wall);
  type_combo_box->addItem(QString("Water"), (int)TileType::Water);

  auto toolbar_layout = new QVBoxLayout();
  toolbar_layout->addWidget(type_combo_box);
  toolbar_layout->addStretch();

  auto top_layout = new QHBoxLayout();
  this->setLayout(top_layout);
  top_layout->addWidget(tiles_view);
  top_layout->addLayout(toolbar_layout);

  QObject::connect(tiles_view, &TilesView::tileLabeledAt, 
    [this, grid_width, grid_height, tile_size](QPoint pos)
    {
      // Compute grid 2d index.
      const auto grid_x = detail::clamp(pos.x() / tile_size, 0, grid_width - 1);
      const auto grid_y = detail::clamp(pos.y() / tile_size, 0, grid_height - 1);

      // Transform into linear index.
      const auto lin_index = grid_y * grid_width + grid_x;

      // Save.
      tiles_.at(lin_index).type = active_type_;
    });

  QObject::connect(type_combo_box, &QComboBox::currentTextChanged,
    [this, type_combo_box, tiles_view](QString const& index)
    {
      const auto current_data = type_combo_box->currentData(Qt::UserRole);
      if (!current_data.isValid()) return;

      const auto active_tile_type = (TileType)current_data.toInt();

      this->active_type_ = active_tile_type;
      tiles_view->setActiveTileType(active_tile_type);
    });
}
