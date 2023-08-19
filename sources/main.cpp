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

private:

};

// This function is invoked internally by calling DispatchMessage(). Note that
// it is executed in the same thread.
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

        EndPaint(hWnd, &ps);
    }
    return 0;
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
    // Handle no window.
  }

  ShowWindow(hWindow, config.cmd_show);
}

class Game
{
public:
  Game() = default;

  void init(Configuration const& config);

  void exec();

private:
  void addObject(float x, float y);

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
  MSG msg = { };
  auto prev_end_time = std::chrono::steady_clock::now();
  auto prev_sleep_time = std::chrono::duration<float, std::milli>(0);
  while (GetMessage(&msg, NULL, 0, 0) > 0)
  {
    const auto begin_time = std::chrono::steady_clock::now();
    const auto prev_overshoot_time = begin_time - prev_end_time - prev_sleep_time;

    TranslateMessage(&msg);
    DispatchMessage(&msg);

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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
  auto config = read_config();

  // Additional Windows specific parameters.
  config.instance = hInstance;
  config.cmd_show = nCmdShow;

  auto game = std::make_unique<Game>();

  game->init(config);

  game->exec();

  return 0;
}
