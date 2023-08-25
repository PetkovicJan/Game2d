#pragma once

#include <QWidget>

#include <optional>

class ConfiguringWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ConfiguringWidget(QWidget* parent = nullptr);

signals:
  void configuringDone(int window_width, int window_height, int tile_size);

private:
  struct
  {
    std::optional<int> window_width;
    std::optional<int> window_height;
    std::optional<int> tile_size;

    bool valid() const
    {
      return window_width.has_value() && window_height.has_value() && tile_size.has_value();
    }
  } state_;
};

