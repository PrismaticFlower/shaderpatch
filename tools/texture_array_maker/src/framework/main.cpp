// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "roboto_regular.h"
#include <d3d11.h>
#include <math.h>
#include <tchar.h>

#include "../array_maker.hpp"

// Data
const static float g_fontBaseSize = 16.0f;

static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;
static float g_displayScale = 1.0f;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void InitDisplayScale(HWND hWnd);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char**)
{
   if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) return 1;

   FreeConsole();

   // Create application window
   // ImGui_ImplWin32_EnableDpiAwareness();
   WNDCLASSEX wc = {sizeof(WNDCLASSEX),     CS_CLASSDC, WndProc, 0L,   0L,
                    GetModuleHandleW(NULL), NULL,       NULL,    NULL, NULL,
                    L"Texture Array Maker", NULL};
   ::RegisterClassExW(&wc);
   HWND hwnd = ::CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, wc.lpszClassName,
                                 L"Texture Array Maker", WS_OVERLAPPEDWINDOW, 100,
                                 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

   DragAcceptFiles(hwnd, true);

   InitDisplayScale(hwnd);

   // Initialize Direct3D
   if (!CreateDeviceD3D(hwnd)) {
      CleanupDeviceD3D();
      ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
      return 1;
   }

   array_maker_init(*g_pd3dDevice, hwnd);

   // Show the window
   ::ShowWindow(hwnd, SW_SHOWDEFAULT);
   ::UpdateWindow(hwnd);

   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO();
   (void)io;
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
   // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

   // Setup Dear ImGui style
   ImGui::StyleColorsDark();
   // ImGui::StyleColorsClassic();

   // Setup Platform/Renderer backends
   ImGui_ImplWin32_Init(hwnd);
   ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

   // Load Fonts
   // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
   // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
   // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
   // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
   // - Read 'docs/FONTS.md' for more instructions and details.
   // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
   // io.Fonts->AddFontDefault();
   io.Fonts->AddFontFromMemoryCompressedBase85TTF(roboto_regular_compressed_data_base85,
                                                  floorf(g_displayScale * g_fontBaseSize));
   // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
   // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
   // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
   // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
   // IM_ASSERT(font != NULL);

   ImGui::GetStyle().ScaleAllSizes(g_displayScale);

   // Our state
   ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

   // Main loop
   bool done = false;
   while (!done) {
      // Poll and handle messages (inputs, window resize, etc.)
      // See the WndProc() function below for our to dispatch events to the Win32 backend.

      if (GetFocus() != hwnd) WaitMessage();

      MSG msg;
      while (::PeekMessageW(&msg, NULL, 0U, 0U, PM_REMOVE)) {
         ::TranslateMessage(&msg);
         ::DispatchMessageW(&msg);
         if (msg.message == WM_QUIT) done = true;
      }
      if (done) break;

      // Start the Dear ImGui frame
      ImGui_ImplDX11_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();

      array_maker(g_displayScale);

      // Rendering
      ImGui::Render();
      const float clear_color_with_alpha[4] = {0.0f, 0.0f, 0.0f, 1.0f};
      g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
      g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView,
                                                 clear_color_with_alpha);
      ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

      g_pSwapChain->Present(1, 0); // Present with vsync
      // g_pSwapChain->Present(0, 0); // Present without vsync
   }

   // Cleanup
   ImGui_ImplDX11_Shutdown();
   ImGui_ImplWin32_Shutdown();
   ImGui::DestroyContext();

   CleanupDeviceD3D();
   ::DestroyWindow(hwnd);
   ::UnregisterClass(wc.lpszClassName, wc.hInstance);

   return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
   // Setup swap chain
   DXGI_SWAP_CHAIN_DESC sd;
   ZeroMemory(&sd, sizeof(sd));
   sd.BufferCount = 2;
   sd.BufferDesc.Width = 0;
   sd.BufferDesc.Height = 0;
   sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   sd.BufferDesc.RefreshRate.Numerator = 0;
   sd.BufferDesc.RefreshRate.Denominator = 0;
   sd.Flags = 0;
   sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
   sd.OutputWindow = hWnd;
   sd.SampleDesc.Count = 1;
   sd.SampleDesc.Quality = 0;
   sd.Windowed = TRUE;
   sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

   UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
   createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
   D3D_FEATURE_LEVEL featureLevel;
   const D3D_FEATURE_LEVEL featureLevelArray[2] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_0,
   };
   if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                                     createDeviceFlags, featureLevelArray, 2,
                                     D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
                                     &featureLevel, &g_pd3dDeviceContext) != S_OK)
      return false;

   CreateRenderTarget();
   return true;
}

void CleanupDeviceD3D()
{
   CleanupRenderTarget();
   if (g_pSwapChain) {
      g_pSwapChain->Release();
      g_pSwapChain = NULL;
   }
   if (g_pd3dDeviceContext) {
      g_pd3dDeviceContext->Release();
      g_pd3dDeviceContext = NULL;
   }
   if (g_pd3dDevice) {
      g_pd3dDevice->Release();
      g_pd3dDevice = NULL;
   }
}

void CreateRenderTarget()
{
   ID3D11Texture2D* pBackBuffer;
   g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

   D3D11_RENDER_TARGET_VIEW_DESC desc{.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                      .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D};

   g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &desc, &g_mainRenderTargetView);
   pBackBuffer->Release();
}

void CleanupRenderTarget()
{
   if (g_mainRenderTargetView) {
      g_mainRenderTargetView->Release();
      g_mainRenderTargetView = NULL;
   }
}

void InitDisplayScale(HWND hWnd)
{
   const float baseDpi = 96.0f;
   const float currentDpi = (float)GetDpiForWindow(hWnd);

   g_displayScale = currentDpi / baseDpi;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);
// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

   switch (msg) {
   case WM_SIZE:
      if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
         CleanupRenderTarget();
         g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam),
                                     (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
         CreateRenderTarget();
      }
      return 0;
   case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
         return 0;
      break;
   case WM_DESTROY:
      ::PostQuitMessage(0);
      return 0;
   case WM_DPICHANGED: {
      const RECT* rect = reinterpret_cast<const RECT*>(lParam);

      SetWindowPos(hWnd, nullptr, rect->left, rect->top, rect->right,
                   rect->bottom, SWP_ASYNCWINDOWPOS);

      const float newDpi = LOWORD(wParam);
      const float baseDpi = 96.0f;
      const float oldDisplayScale = g_displayScale;

      g_displayScale = newDpi / baseDpi;

      if (ImGui::GetCurrentContext() == NULL) return 0;

      ImGuiIO& io = ImGui::GetIO();

      io.Fonts->Clear();
      io.Fonts->AddFontFromMemoryCompressedBase85TTF(roboto_regular_compressed_data_base85,
                                                     floorf(g_displayScale *
                                                            g_fontBaseSize));
      ImGui::GetStyle().ScaleAllSizes(1.0f / oldDisplayScale);
      ImGui::GetStyle().ScaleAllSizes(g_displayScale);

      ImGui_ImplDX11_InvalidateDeviceObjects();

      return 0;
   }
   case WM_DROPFILES: {
      array_maker_add_drop_files((HDROP)wParam);

      return 0;
   }
   }
   return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
