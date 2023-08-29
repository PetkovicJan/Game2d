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

class LinearMotion : public DynamicsHandler
{
public:
  LinearMotion() = default;

  void handleDynamics(Object& obj) override
  {
    obj.x += obj.vx;
    obj.y += obj.vy;
  }
};

class GravitationalMotion : public DynamicsHandler
{
public:
  explicit GravitationalMotion(float gravity) : gravity_(gravity) {}

  void handleDynamics(Object& obj) override
  {
    obj.x += obj.vx;
    obj.y += obj.vy;

    obj.vy += gravity_;
  }

private:
  float gravity_ = 0.f;
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
      player.vy = 0.f;
    }
    else
    {
      // Move player in the x-direction away from the tile center.
      player.x = tile.x + (dx > 0.f ? 1.f : -1.f) * min_x_dist;
      player.vx = 0.f;
    }
  }

  Object& player;
};

void TileCollisionHandler::handleCollision(PlayerCollisionHandler& handler)
{
  handler.handleCollision(*this);
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

struct AnimationConfiguration
{
  struct SingleAnimationConfiguration
  {
    std::string name;
    float fps = 1.f;
    std::vector<const WCHAR*> frame_files;
  };

  std::vector<SingleAnimationConfiguration> single_animation_configs;
};

class AnimationGraphics : public GraphicsHandler
{
public:
  AnimationGraphics(AnimationConfiguration const& config);

  void handleGraphics(Object& obj, Gdiplus::Graphics& graphics) override;

  void play();
  void stop();

  void setAnimation(std::string const& animation);

  void flipHorizontally(bool flip);
  void flipVertically(bool flip);

private:
  struct SingleAnimationData
  {
    std::vector<std::unique_ptr<Gdiplus::Bitmap>> frames;
    float frame_time_ms;
    int current_frame_idx_ = 0;
  };

  class FrameTimeout
  {
  public:
    explicit FrameTimeout(float ms) : timeout_ms_(ms) 
    {
      start_ = std::chrono::high_resolution_clock::now();
    }

    bool is_out() const
    {
      const auto current = std::chrono::high_resolution_clock::now();
      const auto diff = std::chrono::duration<float>(current - start_);

      return std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() > timeout_ms_;
    }

    void reset()
    {
      start_ = std::chrono::high_resolution_clock::now();
    }

  private:
    float timeout_ms_;
    std::chrono::high_resolution_clock::time_point start_;
  };

  std::unordered_map<std::string, SingleAnimationData> animation_data_;
  SingleAnimationData* current_animation_data_ = nullptr;

  std::unique_ptr<FrameTimeout> frame_timeout_;
};

AnimationGraphics::AnimationGraphics(AnimationConfiguration const& config)
{
  for (const auto& single_animation_config : config.single_animation_configs)
  {
    SingleAnimationData single_data;
    single_data.current_frame_idx_ = 0;
    single_data.frame_time_ms = 1000.f / single_animation_config.fps;

    for (const auto& file : single_animation_config.frame_files)
    {
      single_data.frames.emplace_back(std::make_unique<Gdiplus::Bitmap>(file));
    }

    const auto animation_name = single_animation_config.name;
    animation_data_.emplace(animation_name, std::move(single_data));
  }

  const auto& first_name = config.single_animation_configs.front().name;
  current_animation_data_ = &(animation_data_.at(first_name));
  frame_timeout_ = std::make_unique<FrameTimeout>(current_animation_data_->frame_time_ms);
}

void AnimationGraphics::handleGraphics(Object& obj, Gdiplus::Graphics& graphics)
{
  if (!current_animation_data_) return;

  if (frame_timeout_->is_out())
  {
    auto& data = current_animation_data_;
    data->current_frame_idx_ = (data->current_frame_idx_ + 1) % data->frames.size();

    frame_timeout_->reset();
  }

  const auto& frame = current_animation_data_->frames.at(current_animation_data_->current_frame_idx_);

  const int w = frame->GetWidth();
  const int h = frame->GetHeight();
  const int left = obj.x - w / 2;
  const int top = obj.y - h / 2;
  graphics.DrawImage(frame.get(), left, top);
}

void AnimationGraphics::play()
{
}

void AnimationGraphics::stop()
{
}

void AnimationGraphics::setAnimation(std::string const& animation)
{
  auto it = animation_data_.find(animation);
  if (it == animation_data_.end()) return;

  current_animation_data_ = &(it->second);
  frame_timeout_ = std::make_unique<FrameTimeout>(current_animation_data_->frame_time_ms);
}

void AnimationGraphics::flipHorizontally(bool flip)
{
}

void AnimationGraphics::flipVertically(bool flip)
{
}

struct Configuration
{
  struct
  {
    int window_width;
    int window_height;
    HINSTANCE instance;
    int cmd_show;
    float fps;
  } game;

  struct
  {
    float v;
    float g;
    const WCHAR* bitmap;
    Size size;
    AnimationConfiguration anim_config;
  } player;

  struct
  {
    float v;
    const WCHAR* bitmap;
    Size size;
  } bullet;

  TileConfiguration tile_config;
};

Configuration read_config()
{
  Configuration config;

  config.game.window_width = 1000;
  config.game.window_height = 800;

  config.game.fps = 16;

  config.player.v = .5f;
  config.player.g = 0.1f;
  config.player.bitmap = L"C:\\Jan\\Programiranje\\\C++\\Game2d\\resources\\shooter.jpg";
  config.player.size = Size{ 50.f, 50.f };
  auto& anim = config.player.anim_config;
  AnimationConfiguration::SingleAnimationConfiguration walk_anim;
  walk_anim.fps = 3.f;
  walk_anim.name = "walk";
  walk_anim.frame_files.push_back(L"C:\\Jan\\Programiranje\\\C++\\Game2d\\resources\\walking_1.png");
  walk_anim.frame_files.push_back(L"C:\\Jan\\Programiranje\\\C++\\Game2d\\resources\\walking_2.png");
  anim.single_animation_configs.push_back(walk_anim);

  config.bullet.v = 3.f;
  config.bullet.bitmap = L"C:\\Jan\\Programiranje\\\C++\\Game2d\\resources\\bullet.png";
  config.bullet.size = Size{ 50.f, 50.f };

  float tile_sz = 20.f;
  config.tile_config.tile_size = tile_sz;
  config.tile_config.grid_width = config.game.window_width / tile_sz;
  config.tile_config.grid_height = config.game.window_height / tile_sz;

  // Fill bottom row of the window with tiles.
  const auto grid_y = config.tile_config.grid_height - 5;
  auto& tiles = config.tile_config.tiles;
  for (int i = 3; i < config.tile_config.grid_width - 4; ++i)
    tiles.push_back(Point{ i, grid_y });

  return config;
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

  int last_dir_ = VK_RIGHT;
};

PlayerInput::PlayerInput(Configuration const& config, std::vector<std::unique_ptr<Object>>& objects) :
  v_(config.player.v), v_bullet_(config.bullet.v), bullet_bitmap_(config.bullet.bitmap),
  bullet_size_(config.bullet.size), objects_(objects) {}

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

void PlayerInput::createBullet(Object& obj)
{
  // We only consider left and right direction.
  const auto dir = last_dir_ == VK_LEFT ? -1.f : 1.f;

  auto bullet = std::make_unique<Object>(obj.x, obj.y, dir * v_bullet_, 0.f, bullet_size_);

  bullet->dynamics_handler_ = std::make_unique<LinearMotion>();
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
  window_class.hInstance = config.game.instance;
  window_class.lpszClassName = CLASS_NAME;

  RegisterClass(&window_class);

  HWND hWindow = CreateWindowEx(0, CLASS_NAME, L"Game", WS_OVERLAPPEDWINDOW, 
    CW_USEDEFAULT, CW_USEDEFAULT, config.game.window_width, config.game.window_height,
    NULL, NULL, config.game.instance, NULL);

  if (hWindow == NULL)
  {
    throw std::runtime_error("Creating window failed.");
  }

  // Set a value at the specified offset (user data) in the extra window memory.
  // Used to enable wrapping the member function into a static method.
  SetWindowLongPtr(hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  ShowWindow(hWindow, config.game.cmd_show);

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

  float world_width_ = 0.f;
  float world_height_ = 0.f;
};

void Game::init(Configuration const& config)
{
  world_width_ = config.game.window_width;
  world_height_ = config.game.window_height;

  win_ = std::make_unique<Window>(config, objects_);
  frame_time_ = std::chrono::milliseconds(1000) / config.game.fps;

  auto player = std::make_unique<Object>(
    config.game.window_width / 2.f, config.game.window_height / 2.f, 0.f, 0.f, config.player.size);
  //player->dynamics_handler_ = std::make_unique<LinearMotion>();
  player->dynamics_handler_ = std::make_unique<GravitationalMotion>(config.player.g);
  player->collision_handler_ = std::make_unique<PlayerCollisionHandler>(*player);
  player->input_handler_ = std::make_unique<PlayerInput>(config, objects_);
  //player->graphics_handler_ = std::make_unique<BitmapGraphics>(config.player.bitmap);
  player->graphics_handler_ = std::make_unique<AnimationGraphics>(config.player.anim_config);
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
    
      if (o->x < 0.f || o->x > world_width_ || o->y < 0.f || o->y > world_height_)
        o->remove = true;
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
    config.game.instance = hInstance;
    config.game.cmd_show = nCmdShow;

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

