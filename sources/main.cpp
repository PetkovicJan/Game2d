#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

#include <iostream>
#include <memory>
#include <functional>

struct Configuration
{
  int window_width = 1000;
  int window_height = 800;
  HINSTANCE instance;
  int cmd_show;
};

Configuration read_config()
{
  return Configuration();
}

class Window
{
public:
  Window(Configuration const& config);

private:

};

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
  std::unique_ptr<Window> win_;
};

void Game::init(Configuration const& config)
{
  win_ = std::make_unique<Window>(config);
}

void Game::exec()
{
  MSG msg = { };
  while (GetMessage(&msg, NULL, 0, 0) > 0)
  {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
  }
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
