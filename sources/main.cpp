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

class DynamicsHandler;
class CollisionHandler;
class GraphicsHandler;
class InputHandler;

enum class KeyState { Up, Down };

struct Size
{
  float width = 0.f, height = 0.f;
};

struct Object
{
  Object(float x, float y, float vx, float vy, Size size) : 
    x(x), y(y), vx(vx), vy(vy), size(size) {}

  void handleDynamics();
  void handleCollision(Object& other);
  void handleGraphics(Gdiplus::Graphics& graphics);
  void handleInput(KeyState state, int vkey);

  float x = 0.f, y = 0.f;
  float vx = 0.f, vy = 0.f;
  Size size;
  bool remove = false;

  std::unique_ptr<DynamicsHandler> dynamics_handler_ = nullptr;
  std::unique_ptr<CollisionHandler> collision_handler_ = nullptr;
  std::unique_ptr<GraphicsHandler> graphics_handler_ = nullptr;
  std::unique_ptr<InputHandler> input_handler_ = nullptr;
};

class DynamicsHandler
{
public:
  virtual ~DynamicsHandler() {}

  virtual void handleDynamics(Object& obj) = 0;
};

class CollisionHandler
{
public:
  virtual ~CollisionHandler() {}

  virtual void acceptCollision(CollisionHandler& handler) = 0;

  virtual void handleCollision(class PlayerCollisionHandler& handler) = 0;
  virtual void handleCollision(class TileCollisionHandler& handler) = 0;
};

class GraphicsHandler
{
public:
  ~GraphicsHandler() {}

  virtual void handleGraphics(Object& obj, Gdiplus::Graphics& graphics) = 0;
};

class InputHandler
{
public:
  virtual ~InputHandler() {}

  virtual void handleInput(Object& obj, KeyState state, int vkey) = 0;
};

void Object::handleDynamics()
{
  if (dynamics_handler_)
    dynamics_handler_->handleDynamics(*this);
}

void Object::handleCollision(Object& other)
{
  if (collision_handler_ && other.collision_handler_)
    other.collision_handler_->acceptCollision(*collision_handler_);
}

void Object::handleGraphics(Gdiplus::Graphics& graphics)
{
  if (graphics_handler_)
    graphics_handler_->handleGraphics(*this, graphics);
}

void Object::handleInput(KeyState state, int vkey)
{
  if(input_handler_)
    input_handler_->handleInput(*this, state, vkey);
}

struct Point
{
  int x = 0, y = 0;
};

struct TileConfiguration
{
  int grid_width;
  int grid_height;

  float tile_size;
  const WCHAR* tile_bitmap;
  std::vector<Point> tiles;
};

struct Configuration
{
  int window_width = 1000;
  int window_height = 800;
  HINSTANCE instance;
  int cmd_show;

  float fps = 16;

  struct
  {
    float v = 2.f;
    const WCHAR* bitmap = L"C:\\Jan\\Programiranje\\\C++\\Game2d\\resources\\shooter.jpg";
    Size size{ 50.f, 50.f };
  } actor;

  struct
  {
    float v = 5.f;
    const WCHAR* bitmap = L"C:\\Jan\\Programiranje\\\C++\\Game2d\\resources\\bullet.png";
    Size size{ 50.f, 50.f };
  } bullet;

  TileConfiguration tile_config;
};

Configuration read_config()
{
  Configuration config;

  config.window_width = 1000;
  config.window_height = 800;

  config.fps = 16;

  config.actor.v = 3.f;
  config.actor.bitmap = L"C:\\Jan\\Programiranje\\\C++\\Game2d\\resources\\shooter.jpg";
  config.actor.size = Size{ 50.f, 50.f };

  config.bullet.v = 7.f;
  config.bullet.bitmap = L"C:\\Jan\\Programiranje\\\C++\\Game2d\\resources\\bullet.png";
  config.bullet.size = Size{ 50.f, 50.f };

  float tile_sz = 20.f;
  config.tile_config.tile_size = tile_sz;
  config.tile_config.grid_width = config.window_width / tile_sz;
  config.tile_config.grid_height = config.window_height / tile_sz;

  // Fill bottom row of the window with tiles.
  const auto grid_y = config.tile_config.grid_height - 5;
  auto& tiles = config.tile_config.tiles;
  for (int i = 3; i < config.tile_config.grid_width - 4; ++i)
    tiles.push_back(Point{ i, grid_y });

  return config;
}

class LinearMotion : public DynamicsHandler
{
public:
  LinearMotion(float world_width, float world_height) :
    world_width_(world_width), world_height_(world_height) {}

  virtual void handleDynamics(Object& obj) override
  {
    obj.x += obj.vx;
    obj.y += obj.vy;

    if (obj.x < 0.f || obj.x > world_width_ || obj.y < 0.f || obj.y > world_height_)
      obj.remove = true;
  }

private:
  float world_width_;
  float world_height_;
};

class TileCollisionHandler : public CollisionHandler
{
public:
  explicit TileCollisionHandler(Object& tile) : tile(tile) {}

  void acceptCollision(CollisionHandler& handler) override
  {
    handler.handleCollision(*this);
  }

  void handleCollision(PlayerCollisionHandler& handler) override;

  void handleCollision(TileCollisionHandler& handler) override {}

  Object& tile;
};

class PlayerCollisionHandler : public CollisionHandler
{
public:
  explicit PlayerCollisionHandler(Object& player) : player(player) {}

  void acceptCollision(CollisionHandler& handler) override
  {
    handler.handleCollision(*this);
  }

  void handleCollision(PlayerCollisionHandler& handler) override {}

  void handleCollision(TileCollisionHandler& handler) override
  {
    const auto& tile = handler.tile;
    const auto dx = player.x - tile.x;
    const auto dy = player.y - tile.y;

    const float min_x_dist = 0.5f * (player.size.width + tile.size.width);
    const float min_y_dist = 0.5f * (player.size.height + tile.size.height);
    const float overlap_x =  min_x_dist - abs(dx);
    const float overlap_y = min_y_dist - abs(dy);

    if (overlap_x > overlap_y)
    {
      // Move player in the y-direction away from the tile center.
      player.y = tile.y + (dy > 0.f ? 1.f : -1.f) * min_y_dist;
    }
    else
    {
      // Move player in the x-direction away from the tile center.
      player.x = tile.x + (dx > 0.f ? 1.f : -1.f) * min_x_dist;
    }
  }

  Object& player;
};

void TileCollisionHandler::handleCollision(PlayerCollisionHandler& handler)
{
  handler.handleCollision(*this);
}

class BitmapGraphics : public GraphicsHandler
{
public:
  BitmapGraphics(const WCHAR* file);

  void handleGraphics(Object& obj, Gdiplus::Graphics& graphics) override;

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
class RectGraphics : public GraphicsHandler
{
public:
  RectGraphics(int width, int height);

  void handleGraphics(Object& obj, Gdiplus::Graphics& graphics) override;

private:
  int w_ = 0, h_ = 0;
  Gdiplus::Pen pen_;
};

RectGraphics::RectGraphics(int width, int height) : 
  w_(width), h_(height), pen_(Gdiplus::Color(255, 0, 0, 0)) {}

void RectGraphics::handleGraphics(Object& obj, Gdiplus::Graphics& graphics)
{
  const auto x = int(obj.x) - w_ / 2;
  const auto y = int(obj.y) - h_ / 2;

  Gdiplus::Rect rect(x, y, w_, h_);
  graphics.DrawRectangle(&pen_, rect);
}

class PlayerInput : public InputHandler
{
public:
  explicit PlayerInput(Configuration const& config, std::vector<std::unique_ptr<Object>>& objects);

  void handleInput(Object& obj, KeyState state, int vkey) override;

public:
  void createBullet(Object& obj);

  float v_ = 0.f;
  float v_bullet_ = 0.f;
  Size bullet_size_;
  const WCHAR* bullet_bitmap_;
  std::vector<std::unique_ptr<Object>>& objects_;

  float world_width_;
  float world_height_;

  int last_dir_ = VK_RIGHT;
};

PlayerInput::PlayerInput(Configuration const& config, std::vector<std::unique_ptr<Object>>& objects) :
  v_(config.actor.v), v_bullet_(config.bullet.v), bullet_bitmap_(config.bullet.bitmap), 
  bullet_size_(config.bullet.size), objects_(objects), 
  world_width_(config.window_width), world_height_(config.window_height) {}

void PlayerInput::handleInput(Object& obj, KeyState state, int vkey)
{
  // Handle movement.
  if (state == KeyState::Down)
  {
    if (vkey == VK_LEFT)
    {
      last_dir_ = vkey;
      obj.vx = -v_;
    }
    else if (vkey == VK_RIGHT)
    {
      last_dir_ = vkey;
      obj.vx = v_;
    }
    else if (vkey == VK_UP)
    {
      last_dir_ = vkey;
      obj.vy = -v_;
    }
    else if (vkey == VK_DOWN)
    {
      last_dir_ = vkey;
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

void PlayerInput::createBullet(Object& obj)
{
  float unit_x = 0.f, unit_y = 0.f;
  if (last_dir_ == VK_LEFT) unit_x = -1.f;
  else if (last_dir_ == VK_RIGHT) unit_x = 1.f;
  else if (last_dir_ == VK_UP) unit_y = -1.f;
  else if (last_dir_ == VK_DOWN) unit_y = 1.f;

  auto bullet = std::make_unique<Object>(obj.x, obj.y, unit_x * v_bullet_, unit_y * v_bullet_, bullet_size_);

  bullet->dynamics_handler_ = std::make_unique<LinearMotion>(world_width_, world_height_);
  bullet->graphics_handler_ = std::make_unique<BitmapGraphics>(bullet_bitmap_);

  objects_.emplace_back(std::move(bullet));
}

void addTiles(TileConfiguration const& config, std::vector<std::unique_ptr<Object>>& objects)
{
  const auto tile_size = config.tile_size;
  const auto offset = 0.5f * config.tile_size;
  for (auto pt : config.tiles)
  {
    // Create a tile object.
    const auto x = pt.x * tile_size + offset;
    const auto y = pt.y * tile_size + offset;
    auto tile = std::make_unique<Object>(x, y, 0.f, 0.f, Size{ tile_size, tile_size });
    tile->collision_handler_ = std::make_unique<TileCollisionHandler>(*tile);
    tile->graphics_handler_ = std::make_unique<RectGraphics>(tile_size, tile_size);

    // Add a tile to object collection.
    objects.emplace_back(std::move(tile));
  }
}

class Window
{
public:
  Window(Configuration const& config, std::vector<std::unique_ptr<Object>>& obj_collection);

  ~Window();

  void render();
private:
  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT CALLBACK WindowProcImpl(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  HWND hWnd_ = NULL;
  std::vector<std::unique_ptr<Object>>& obj_collection_;
};

Window::Window(Configuration const& config, std::vector<std::unique_ptr<Object>>& obj_collection) : obj_collection_(obj_collection)
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
        (*it)->handleGraphics(graphics);

      EndBufferedPaint(h_buff, TRUE);
      EndPaint(hWnd, &ps);
      break;
    }
  case WM_KEYDOWN:
  {
    std::vector<Object*> current_objects;
    current_objects.reserve(obj_collection_.size());
    for (auto& obj : obj_collection_)
      current_objects.push_back(obj.get());

    for (auto obj : current_objects)
      obj->handleInput(KeyState::Down, wParam);

    break;
  }
  case WM_KEYUP:
  {
    std::vector<Object*> current_objects;
    current_objects.reserve(obj_collection_.size());
    for (auto& obj : obj_collection_)
      current_objects.push_back(obj.get());

    for (auto obj : current_objects)
      obj->handleInput(KeyState::Up, wParam);

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

  bool areObjectsColliding(Object& obj_1, Object& obj_2);

  std::unique_ptr<Window> win_;

  std::chrono::duration<float, std::milli> frame_time_;

  std::vector<std::unique_ptr<Object>> objects_;
};

void Game::init(Configuration const& config)
{
  win_ = std::make_unique<Window>(config, objects_);
  frame_time_ = std::chrono::milliseconds(1000) / config.fps;

  auto player = std::make_unique<Object>(config.window_width / 2.f, config.window_height / 2.f, 0.f, 0.f, config.actor.size);
  player->dynamics_handler_ = std::make_unique<LinearMotion>(config.window_width, config.window_height);
  player->collision_handler_ = std::make_unique<PlayerCollisionHandler>(*player);
  player->input_handler_ = std::make_unique<PlayerInput>(config, objects_);
  player->graphics_handler_ = std::make_unique<BitmapGraphics>(config.actor.bitmap);
  objects_.emplace_back(std::move(player));

  // Add tiles.
  addTiles(config.tile_config, objects_);
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
      o->handleDynamics();
    }

    // Collision detection.
    // Loop over unique pairs of objects.
    for (int i = 0; i < objects_.size(); ++i)
    {
      auto& obj_1 = objects_.at(i);
      for (int j = 0; j < i; ++j)
      {
        auto& obj_2 = objects_.at(j);

        if (areObjectsColliding(*obj_1, *obj_2))
        {
          obj_1->handleCollision(*obj_2);
        }
      }
    }

    triggerRender();

    {
      auto& vec = objects_;
      vec.erase(std::remove_if(vec.begin(), vec.end(),
        [](auto const& o) { return o->remove; }), vec.end());
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

bool Game::areObjectsColliding(Object& obj_1, Object& obj_2)
{
  const float dpos_x = obj_1.x - obj_2.x;
  const float dpos_y = obj_1.y - obj_2.y;

  const bool overlap_x = abs(dpos_x) < 0.5f * (obj_1.size.width + obj_2.size.width);
  const bool overlap_y = abs(dpos_y) < 0.5f * (obj_1.size.height + obj_2.size.height);
  if (overlap_x && overlap_y)
  {
    // Due to finite time steps in game engine the collision might have already 
    // been handled in the previous step and the objects are moving apart from 
    // each other, even though they are still close enough. Therefore it is 
    // crucial to also check the relative velocity with respect to the distance 
    // between them.
    const auto dv_x = obj_1.vx - obj_2.vx;
    const auto dv_y = obj_1.vy - obj_2.vy;
    return (dpos_x * dv_x + dpos_y * dv_y) < 0.f;
  }

  return false;
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
