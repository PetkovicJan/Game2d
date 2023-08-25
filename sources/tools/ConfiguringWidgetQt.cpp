#include "ConfiguringWidgetQt.hpp"

#include <QLineEdit>
#include <QIntValidator>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>

ConfiguringWidget::ConfiguringWidget(QWidget* parent) : QWidget(parent)
{
  // Setup widgets & layouts.

  auto window_width_edit = new QLineEdit();
  auto window_width_validator = new QIntValidator(100, 1500, this);

  auto window_height_edit = new QLineEdit();
  auto window_height_validator = new QIntValidator(100, 1500, this);

  auto tile_size_edit = new QLineEdit();
  auto tile_size_validator = new QIntValidator(5, 50, this);

  auto form_layout = new QFormLayout();
  form_layout->addRow("Window width:", window_width_edit);
  form_layout->addRow("Window height:", window_height_edit);
  form_layout->addRow("Tile size:", tile_size_edit);

  auto configure_button = new QPushButton("Configure");
  auto button_layout = new QHBoxLayout();
  button_layout->addStretch();
  button_layout->addWidget(configure_button);

  auto top_layout = new QVBoxLayout();
  this->setLayout(top_layout);

  top_layout->addLayout(form_layout);
  top_layout->addLayout(button_layout);

  // Wire up the logic.
  QObject::connect(window_width_edit, &QLineEdit::textChanged, 
    [this, window_width_validator, configure_button](QString const& new_string) 
    {
      int pos = 0;
      auto copy = new_string;
      if (window_width_validator->validate(copy, pos) == QValidator::State::Acceptable)
        state_.window_width = new_string.toInt();
      else
        state_.window_width = std::nullopt;

      configure_button->setEnabled(state_.valid());
    });

  QObject::connect(window_height_edit, &QLineEdit::textChanged, 
    [this, window_height_validator, configure_button](QString const& new_string) 
    {
      int pos = 0;
      auto copy = new_string;
      if (window_height_validator->validate(copy, pos) == QValidator::State::Acceptable)
        state_.window_height = new_string.toInt();
      else
        state_.window_height = std::nullopt;

      configure_button->setEnabled(state_.valid());
    });

  QObject::connect(tile_size_edit, &QLineEdit::textChanged, 
    [this, tile_size_validator, configure_button](QString const& new_string) 
    {
      int pos = 0;
      auto copy = new_string;
      if (tile_size_validator->validate(copy, pos) == QValidator::State::Acceptable)
        state_.tile_size = new_string.toInt();
      else
        state_.tile_size = std::nullopt;

      configure_button->setEnabled(state_.valid());
    });

  QObject::connect(configure_button, &QPushButton::clicked, [this]() 
    {
      if (!state_.valid()) return;

      emit this->configuringDone(*state_.window_width, *state_.window_height, *state_.tile_size);
    });

  configure_button->setDisabled(true);
}
