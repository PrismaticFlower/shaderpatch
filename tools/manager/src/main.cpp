﻿#include "framework.hpp"

#include "../resource.h"
#include "app_mode_configurator.hpp"
#include "app_mode_installer.hpp"

constexpr auto window_title = L"Shader Patch Configurator";
constexpr auto window_class = window_title;

HWND init_instance(HINSTANCE, int);
void set_xaml_island_window_pos();
LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);

HINSTANCE current_instance;
HWND main_hwnd = nullptr;
HWND xaml_island_hwnd = nullptr;

using namespace std::literals;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Xaml::Hosting;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
   winrt::init_apartment(winrt::apartment_type::multi_threaded);
   current_instance = hInstance;

   UNREFERENCED_PARAMETER(hPrevInstance);
   UNREFERENCED_PARAMETER(lpCmdLine);

   main_hwnd = init_instance(hInstance, nCmdShow);

   if (!main_hwnd) return 1;

   auto xaml_manager = WindowsXamlManager::InitializeForCurrentThread();

   DesktopWindowXamlSource xaml_desktop_source;

   auto xaml_interop = xaml_desktop_source.as<IDesktopWindowXamlSourceNative2>();

   winrt::check_hresult(xaml_interop->AttachToWindow(main_hwnd));

   xaml_interop->get_WindowHandle(&xaml_island_hwnd);

   auto active_mode = make_app_mode_installer();
   active_mode->switch_to(xaml_desktop_source);

   set_xaml_island_window_pos();

   MSG msg;

   // Main message loop:
   while (GetMessageW(&msg, nullptr, 0, 0)) {
      active_mode->update(msg);

      TranslateMessage(&msg);
      DispatchMessageW(&msg);
   }

   return (int)msg.wParam;
}

ATOM register_class(HINSTANCE instance)
{
   WNDCLASSEXW wcex;

   wcex.cbSize = sizeof(WNDCLASSEX);

   wcex.style = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc = wnd_proc;
   wcex.cbClsExtra = 0;
   wcex.cbWndExtra = 0;
   wcex.hInstance = instance;
   wcex.hIcon = LoadIconW(instance, MAKEINTRESOURCE(IDI_MANAGER));
   wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
   wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
   wcex.lpszMenuName = nullptr;
   wcex.lpszClassName = window_class;
   wcex.hIconSm = LoadIconW(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

   return RegisterClassExW(&wcex);
}

HWND init_instance(HINSTANCE instance, int nCmdShow)
{
   register_class(instance);

   HWND hwnd = CreateWindowExW(0, window_class, window_title, WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr,
                               nullptr, instance, nullptr);

   if (!hwnd) {
      return nullptr;
   }

   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   return hwnd;
}

void set_xaml_island_window_pos()
{
   RECT rect;
   GetClientRect(main_hwnd, &rect);
   SetWindowPos(xaml_island_hwnd, 0, rect.left, rect.top, rect.right,
                rect.bottom, SWP_SHOWWINDOW);
}

LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message) {
   case WM_SIZE: {
      set_xaml_island_window_pos();

      return 0;
   }
   case WM_DPICHANGED: {
      auto rect = reinterpret_cast<const RECT*>(lParam);

      SetWindowPos(hWnd, nullptr, rect->left, rect->top, rect->right - rect->left,
                   rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);

      return 0;
   }
   case WM_DESTROY:
      PostQuitMessage(0);
      break;
   default:
      return DefWindowProcW(hWnd, message, wParam, lParam);
   }
   return 0;
}