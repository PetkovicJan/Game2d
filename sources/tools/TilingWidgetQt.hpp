#pragma once

#include <QWidget>

class TilingWidget : public QWidget
{
  Q_OBJECT

public:
  explicit TilingWidget(QWidget* parent = nullptr);

  void setConfiguration(int window_width, int window_height, int tile_size);
};
