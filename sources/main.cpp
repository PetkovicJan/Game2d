#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <uxtheme.h>

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

  float main_actor_v = 2.f;
  const WCHAR* main_actor_bitmap = L"C:\\Jan\\Programiranje\\\C++\\Game2d\\resources\\shooter.jpg";

  float bullet_v = 5.f;
  const WCHAR* bullet_bitmap = L"C:\\Jan\\Programiranje\\\C++\\Game2d\\resources\\bullet.png";
};

Configuration read_config()
{
  return Configuration();
}

class DynamicsHandler;
class InputHandler;
class GraphicsHandler;

enum class KeyState { Up, Down };

struct Object
{
  Object(float x, float y, float vx = 0.f, float vy = 0.f) : x(x), y(y), vx(vx), vy(vy) {}

  void handleInput(KeyState state, int vkey);
  void handleDynamics();
  void handleGraphics(Gdiplus::Graphics& graphics);

  float x = 0.f, y = 0.f;
  float vx = 0.f, vy = 0.f;
  bool remove = false;

  std::unique_ptr<DynamicsHandler> dynamics_handler_;
  std::unique_ptr<InputHandler> input_handler_;
  std::unique_ptr<GraphicsHandler> graphics_handler_;

};

class DynamicsHandler
{
public:
  virtual ~DynamicsHandler() {}

  virtual void handleDynamics(Object& obj) = 0;
};

class StraightMotion : public DynamicsHandler
{
public:
  virtual void handleDynamics(Object& obj)
  {
    obj.x += obj.vx;
    obj.y += obj.vy;
  }
};

class GraphicsHandler
{
public:
  ~GraphicsHandler() {}

  virtual void handleGraphics(Object& obj, Gdiplus::Graphics& graphics) = 0;
};

class BitmapGraphics : public GraphicsHandler
{
public:
  BitmapGraphics(const WCHAR* file);

  void handleGraphics(Object& obj, Gdiplus::Graphics& graphics);

private:
  Gdiplus::Bitmap bitmap_;
};

BitmapGraphics::BitmapGraphics(const WCHAR* file) : bitmap_(file) {}

void BitmapGraphics::handleGraphics(Object& obj, Gdiplus::Graphics& graphics)
{
  const int w = bitmap_.GetWidth();
  const int h = bitmap_.GetHeight();
  const int left = obj.x - w / 2;
  const int top = obj.y - h / 2;
  graphics.DrawImage(&bitmap_, left, top);
}

class InputHandler
{
public:
  virtual ~InputHandler() {}

  virtual void handleInput(Object& obj, KeyState state, int vkey) = 0;
};

class MainActorInput : public InputHandler
{
public:
  explicit MainActorInput(float v, float v_bullet, const WCHAR* bullet_bitmap, std::vector<Object>& objects);

  void handleInput(Object& obj, KeyState state, int vkey);

public:
  void createBullet(Object& obj);

  float v_ = 0.f;
  float v_bullet_ = 0.f;
  const WCHAR* bullet_bitmap_;
  std::vector<Object>& objects_;
};

MainActorInput::MainActorInput(float v, float v_bullet, const WCHAR* bullet_bitmap, std::vector<Object>& objects) :
  v_(v), v_bullet_(v_bullet), bullet_bitmap_(bullet_bitmap), objects_(objects) {}

void MainActorInput::handleInput(Object& obj, KeyState state, int vkey)
{
  // Handle movement.
  if (state == KeyState::Down)
  {
    if (vkey == VK_LEFT)
    {
      obj.vx = -v_;
    }
    else if (vkey == VK_RIGHT)
    {
      obj.vx = v_;
    }
    else if (vkey == VK_UP)
    {
      obj.vy = -v_;
    }
    else if (vkey == VK_DOWN)
    {
      obj.vy = v_;
    }
  }
  else // KeyState::Up
  {
    if (vkey == VK_LEFT && obj.vx < 0.f)
    {
      obj.vx = 0.f;
    }
    else if (vkey == VK_RIGHT && obj.vx > 0.f)
    {
      obj.vx = 0.f;
    }
    else if (vkey == VK_UP && obj.vy < 0.f)
    {
      obj.vy = 0.f;
    }
    else if (vkey == VK_DOWN && obj.vy > 0.f)
    {
      obj.vy = 0.f;
    }
  }

  if (vkey == VK_SPACE && state == KeyState::Down)
  {
    createBullet(obj);
  }
}

void MainActorInput::createBullet(Object& obj)
{
  const float abs_vx = abs(obj.vx);
  const float unit_x = abs_vx > 1e-6f ? obj.vx / abs_vx : 0.f;
  const float abs_vy = abs(obj.vy);
  const float unit_y = abs_vy > 1e-6f ? obj.vy / abs_vy : 0.f;
  Object bullet(obj.x, obj.y, unit_x * v_bullet_, unit_y * v_bullet_);

  bullet.dynamics_handler_ = std::make_unique<StraightMotion>();
  bullet.graphics_handler_ = std::make_unique<BitmapGraphics>(bullet_bitmap_);

  objects_.emplace_back(std::move(bullet));
}

void Object::handleInput(KeyState state, int vkey)
{
  if(input_handler_)
    input_handler_->handleInput(*this, state, vkey);
}

void Object::handleDynamics()
{
  if (dynamics_handler_)
    dynamics_handler_->handleDynamics(*this);
}

void Object::handleGraphics(Gdiplus::Graphics& graphics)
{
  if (graphics_handler_)
    graphics_handler_->handleGraphics(*this, graphics);
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

class Window
{
public:
  Window(Configuration const& config, std::vector<Object>& obj_collection);

  ~Window();

  void render();
private:
  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT CALLBACK WindowProcImpl(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  HWND hWnd_ = NULL;
  std::vector<Object>& obj_collection_;
};

Window::Window(Configuration const& config, std::vector<Object>& obj_collection) : obj_collection_(obj_collection)
{
  BufferedPaintInit();

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
}

Window::~Window()
{
  BufferedPaintUnInit();
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
  switch (uMsg)
  {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);

      RECT rect;
      GetClientRect(hWnd, &rect);
      HDC buff_hdc;
      auto h_buff = BeginBufferedPaint(hdc, &rect, BPBF_COMPATIBLEBITMAP, NULL, &buff_hdc);

      // Fill background.
      FillRect(buff_hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

      Gdiplus::Graphics graphics(buff_hdc);
      for (auto it = obj_collection_.rbegin(); it != obj_collection_.rend(); ++it )
        it->handleGraphics(graphics);

      EndBufferedPaint(h_buff, TRUE);
      EndPaint(hWnd, &ps);
      break;
    }
  case WM_KEYDOWN:
  {
    for (auto& o : obj_collection_)
      o.handleInput(KeyState::Down, wParam);

    break;
  }
  case WM_KEYUP:
  {
    for (auto& o : obj_collection_)
      o.handleInput(KeyState::Up, wParam);

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
  void triggerRender();

  std::unique_ptr<Window> win_;

  std::chrono::duration<float, std::milli> frame_time_;

  std::vector<Object> objects_;
};

void Game::init(Configuration const& config)
{
  win_ = std::make_unique<Window>(config, objects_);
  frame_time_ = std::chrono::milliseconds(1000) / config.fps;

  Object main_actor(config.window_width / 2.f, config.window_height / 2.f);
  main_actor.dynamics_handler_ = std::make_unique<StraightMotion>();
  main_actor.input_handler_ = std::make_unique<MainActorInput>(config.main_actor_v, config.bullet_v, config.bullet_bitmap, objects_);
  main_actor.graphics_handler_ = std::make_unique<BitmapGraphics>(config.main_actor_bitmap);
  objects_.emplace_back(std::move(main_actor));
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

    for (auto& o : objects_)
    {
      o.handleDynamics();
    }

    triggerRender();

    {
      auto& vec = objects_;
      vec.erase(std::remove_if(vec.begin(), vec.end(),
        [](Object const& o) { return o.remove; }), vec.end());
    }

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
  // Initialize GDI+.
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

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

  Gdiplus::GdiplusShutdown(gdiplusToken);

  return 0;
}
