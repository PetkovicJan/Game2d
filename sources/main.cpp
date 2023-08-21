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
  float vx = 0.f, vy = 0.f;
};

class ObjectCollection
{
public:
  void addObject(float x, float y);

  std::vector<Object>& getObjects();
  std::vector<Object> const& getObjects() const;

private:
  int object_unique_id_ = 0;
  std::vector<Object> objects_;
};

void ObjectCollection::addObject(float x, float y)
{
  objects_.emplace_back(object_unique_id_, x, y);
  ++object_unique_id_;
}

std::vector<Object>& ObjectCollection::getObjects() 
{
  return objects_;
}

std::vector<Object> const& ObjectCollection::getObjects() const
{
  return objects_;
}

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

class Command
{
public:
  virtual ~Command() {};
  virtual void execute(Object& obj) = 0;
};

class SetVelocityX : public Command
{
public:
  SetVelocityX(float vx) : vx_(vx) {}

  void execute(Object& obj) override
  {
    obj.vx = vx_;
  }

private:
  float vx_{ 0.f };
};

class SetVelocityY : public Command
{
public:
  SetVelocityY(float vy) : vy_(vy) {}

  void execute(Object& obj) override
  {
    obj.vy = vy_;
  }

private:
  float vy_{ 0.f };
};

class AddVelocityCommand : public Command
{
public:
  AddVelocityCommand(float vx, float vy) : vx_(vx), vy_(vy) {}

  void execute(Object& obj) override
  {
    obj.vx += vx_;
    obj.vy += vy_;
  }

private:
  float vx_{ 0.f }, vy_{ 0.f };
};

class Window
{
public:
  Window(Configuration const& config, ObjectCollection& obj_collection);

  void render();
private:
  enum class KeyState { Up, Down };

  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT CALLBACK WindowProcImpl(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  template<typename T, typename... Args>
  void registerKeyCommand(int key, KeyState state, Args... args);

  HWND hWnd_ = NULL;
  ObjectCollection& obj_collection_;

  std::unordered_map<int, std::unique_ptr<Command>> up_commands_;
  std::unordered_map<int, std::unique_ptr<Command>> down_commands_;
};

Window::Window(Configuration const& config, ObjectCollection& obj_collection) : obj_collection_(obj_collection)
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

  // Set a value at the specified offset (user data) in the extra window memory.
  // Used to enable wrapping the member function into a static method.
  SetWindowLongPtr(hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  ShowWindow(hWindow, config.cmd_show);

  hWnd_ = hWindow;

  registerKeyCommand<SetVelocityX>(VK_LEFT, KeyState::Down, -1.f);
  registerKeyCommand<SetVelocityX>(VK_RIGHT, KeyState::Down, 1.f);
  registerKeyCommand<SetVelocityY>(VK_UP, KeyState::Down, -1.f);
  registerKeyCommand<SetVelocityY>(VK_DOWN, KeyState::Down, 1.f);
  registerKeyCommand<SetVelocityX>(VK_LEFT, KeyState::Up, 0.f);
  registerKeyCommand<SetVelocityX>(VK_RIGHT, KeyState::Up, 0.f);
  registerKeyCommand<SetVelocityY>(VK_UP, KeyState::Up, 0.f);
  registerKeyCommand<SetVelocityY>(VK_DOWN, KeyState::Up, 0.f);
}

void Window::render()
{
  InvalidateRect(hWnd_, NULL, TRUE);
  UpdateWindow(hWnd_);
}

// This function is invoked internally by calling DispatchMessage(). Note that
// it is executed in the same thread.
LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  // Standard pattern for wrapping the member function, so that it can be
  // used as a callback (Windows expects a plain function, but member functions
  // have an additional implicit this argument).
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

LRESULT CALLBACK Window::WindowProcImpl(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  Object* main_actor = nullptr;
  if (obj_collection_.getObjects().size() > 0)
    main_actor = &(obj_collection_.getObjects().at(0));

  Command* cmd = nullptr;

  switch (uMsg)
  {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);

      // Fill background.
      FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

      for(const auto& obj : obj_collection_.getObjects())
      {
        const auto w = 20;
        const auto h = 20;
        SolidBrushRaii red_brush(hdc, 255, 0, 0);
        Rectangle(hdc, obj.x, obj.y, obj.x + w, obj.y + h);
      }

      EndPaint(hWnd, &ps);
      break;
    }
  case WM_KEYDOWN:
  {
    if (const auto it = down_commands_.find(wParam); it != down_commands_.end())
      cmd = it->second.get();

    break;
  }
  case WM_KEYUP:
  {
    if (const auto it = up_commands_.find(wParam); it != up_commands_.end())
      cmd = it->second.get();

    break;
  }

    return 0;
  }

  if (cmd && main_actor)
  {
    cmd->execute(*main_actor);
  }

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

template<typename T, typename... Args>
void Window::registerKeyCommand(int key, KeyState state, Args... args)
{
  auto cmd = std::make_unique<T>(args...);
  if (state == KeyState::Up)
  {
    up_commands_.emplace(key, std::move(cmd));
  }
  else // KeyState::Down
  {
    down_commands_.emplace(key, std::move(cmd));
  }
}

class Game
{
public:
  Game() = default;

  void init(Configuration const& config);

  void exec();

private:
  void triggerRender();

  std::unique_ptr<Window> win_;

  std::chrono::duration<float, std::milli> frame_time_;

  ObjectCollection object_collection_;
};

void Game::init(Configuration const& config)
{
  win_ = std::make_unique<Window>(config, object_collection_);
  frame_time_ = std::chrono::milliseconds(1000) / config.fps;
  object_collection_.addObject(config.window_width / 2.f, config.window_height / 2.f);
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

    for (auto& o : object_collection_.getObjects())
    {
      o.x += o.vx;
      o.y += o.vy;
    }

    triggerRender();

    const auto end_time = std::chrono::steady_clock::now();
    const auto body_duration = end_time - begin_time;

    std::this_thread::sleep_for(frame_time_ - body_duration - prev_overshoot_time);

    prev_end_time = end_time;
  }
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
