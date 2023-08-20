#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

#include <iostream>
#include <memory>
#include <functional>
#include <thread>
#include <filesystem>
#include <fstream>

class Logger
{
public:

private:
  Logger(std::filesystem::path const& output);
  ~Logger();

  friend Logger& file_logger();

template<typename T>
friend Logger& operator<<(Logger& logger, T&& src);

friend Logger& operator<<(Logger& logger, decltype(std::endl<char, std::char_traits<char>>));

  std::ofstream file_;
};

Logger::Logger(std::filesystem::path const& output)
{
  file_.open(output);
}

Logger::~Logger()
{
  file_.close();
}

template<typename T>
Logger& operator<<(Logger& logger, T&& src)
{
  logger.file_ << src;
  return logger;
}

Logger& operator<<(Logger& logger, decltype(std::endl<char, std::char_traits<char>>))
{
  logger.file_ << std::endl;
  return logger;
}

Logger& file_logger()
{
  std::filesystem::path file_path = "log.txt";
  static Logger logger(file_path);

  return logger;
}

Logger& logger = file_logger();

struct Configuration
{
  int window_width = 1000;
  int window_height = 800;
  HINSTANCE instance;
  int cmd_show;

  float fps = 16;
};

Configuration read_config()
{
  return Configuration();
}

struct Object
{
  Object(int id, float x, float y) : id(id), x(x), y(y) {}

  int id = 0;
  float x = 0.f, y = 0.f;
};

class Window
{
public:
  Window(Configuration const& config);

  void render();
private:
  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT CALLBACK WindowProcImpl(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  HWND hWnd_ = NULL;
};

class SolidBrushRaii
{
public:
  SolidBrushRaii(HDC dc, int r, int g, int b);

  ~SolidBrushRaii();

private:
  HDC dc_ = NULL;
  HBRUSH new_brush_ = NULL;
  HBRUSH old_brush_ = NULL;
};

SolidBrushRaii::SolidBrushRaii(HDC dc, int r, int g, int b) : dc_(dc)
{
  new_brush_ = CreateSolidBrush(RGB(r, g, b));
  old_brush_ = (HBRUSH)SelectObject(dc, new_brush_);
}

SolidBrushRaii::~SolidBrushRaii()
{
  SelectObject(dc_, old_brush_);
  DeleteObject(new_brush_);
}

// This function is invoked internally by calling DispatchMessage(). Note that
// it is executed in the same thread.
LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  Window* this_ptr = nullptr;
  if (uMsg == WM_NCCREATE)
  {
      this_ptr = reinterpret_cast<Window*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
      SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this_ptr));
  }
  else
  {
      this_ptr = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
  }

  if (this_ptr)
  {
      return this_ptr->WindowProcImpl(hWnd, uMsg, wParam, lParam);
  }

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

Window::Window(Configuration const& config)
{
  const wchar_t CLASS_NAME[] = L"Game Window Class";
  WNDCLASS window_class = {};
  window_class.lpfnWndProc = WindowProc;
  window_class.hInstance = config.instance;
  window_class.lpszClassName = CLASS_NAME;

  RegisterClass(&window_class);

  HWND hWindow = CreateWindowEx(0, CLASS_NAME, L"Game", WS_OVERLAPPEDWINDOW, 
    CW_USEDEFAULT, CW_USEDEFAULT, config.window_width, config.window_height,
    NULL, NULL, config.instance, NULL);

  if (hWindow == NULL)
  {
    throw std::runtime_error("Creating window failed.");
  }

  // Standard pattern for wrapping the member function, so that it can be
  // used as a callback (Windows expects a plain function, but member functions
  // have an additional implicit this argument).

  // Set a value at the specified offset (user data) in the extra window memory.
  SetWindowLongPtr(hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  ShowWindow(hWindow, config.cmd_show);

  hWnd_ = hWindow;
}

void Window::render()
{
  InvalidateRect(hWnd_, NULL, TRUE);
  UpdateWindow(hWnd_);
}

LRESULT CALLBACK Window::WindowProcImpl(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);

      FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

      {
        SolidBrushRaii red_brush(hdc, 255, 0, 0);
        Rectangle(hdc, 300, 300, 350, 350);
      }

      {
        SolidBrushRaii blue_brush(hdc, 0, 0, 255);
        Rectangle(hdc, 600, 600, 650, 650);
      }

      EndPaint(hWnd, &ps);
      break;
    }
    return 0;
  }

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

class Game
{
public:
  Game() = default;

  void init(Configuration const& config);

  void exec();

private:
  void addObject(float x, float y);

  void triggerRender();

  std::unique_ptr<Window> win_;

  std::chrono::duration<float, std::milli> frame_time_;

  int object_unique_id_ = 0;
  std::vector<Object> objects_;
};

void Game::init(Configuration const& config)
{
  win_ = std::make_unique<Window>(config);
  frame_time_ = std::chrono::milliseconds(1000) / config.fps;
  addObject(config.window_width / 2.f, config.window_height / 2.f);
}

void Game::exec()
{
  auto prev_end_time = std::chrono::steady_clock::now();
  auto prev_sleep_time = std::chrono::duration<float, std::milli>(0);
  while (true)
  {
    const auto begin_time = std::chrono::steady_clock::now();
    const auto prev_overshoot_time = begin_time - prev_end_time - prev_sleep_time;

    MSG msg;
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
      if (msg.message == WM_QUIT) break;

      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    triggerRender();

    const auto end_time = std::chrono::steady_clock::now();
    const auto body_duration = end_time - begin_time;

    std::this_thread::sleep_for(frame_time_ - body_duration - prev_overshoot_time);

    prev_end_time = end_time;
  }
}

void Game::addObject(float x, float y)
{
  objects_.emplace_back(object_unique_id_, x, y);
  ++object_unique_id_;
}

void Game::triggerRender()
{
  win_->render();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
  try
  {
    auto config = read_config();

    // Additional Windows specific parameters.
    config.instance = hInstance;
    config.cmd_show = nCmdShow;

    auto game = std::make_unique<Game>();

    game->init(config);

    game->exec();
  }
  catch (std::exception const& e)
  {
    logger << e.what() << std::endl;
  }

  return 0;
}
