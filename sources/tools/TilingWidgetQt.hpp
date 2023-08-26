#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QMouseEvent>

struct Tile
{
  bool labeled = false;
};

class TilingWidget : public QWidget
{
  Q_OBJECT

public:
  explicit TilingWidget(QWidget* parent = nullptr);

  void setConfiguration(int window_width, int window_height, int tile_size);

private:
  std::vector<Tile> tiles_;
};

class TilesView : public QGraphicsView
{
  Q_OBJECT

public:
  explicit TilesView(int grid_width, int grid_height, int tile_size);

signals:
  void tileLabeledAt(QPoint pos);

private:
  void labelTileAt(QPoint pos);

  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
};
