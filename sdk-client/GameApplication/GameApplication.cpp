/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pch.h"

#include <Windows.h>
#include <GL/glew.h>
#include <GL/glext.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <gl/glut.h>

#include <cstdlib>
#include <iostream>

#include "GameAppBillingSDKDialog.hpp"
#include "TapCamera.h"
#include "TeapotRenderer.h"
#include "resource.h"

using namespace GameApp;

namespace {
float kTeapotColor[3] = {0.2f, 0.5f, 0.85f};
}  // namespace

class GameApplication {
  enum GameAppIDs { IDR_BTN_BILLING_SDK = 11002 };

 public:
  HDC hDC = nullptr;
  HGLRC hGLRC = nullptr;
  HWND hwndMain = nullptr;
  HWND btnBillingSDK = nullptr;
  TeapotRenderer renderer_;
  ndk_helper::TapCamera camera_;
  HINSTANCE hInstance = nullptr;

  GameApplication() {
    hInstance = GetModuleHandle(nullptr);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OpenGLGameWindow";
    RegisterClassW(&wc);

    hwndMain = CreateWindowExW(
        0, L"OpenGLGameWindow", L"Google Play Games on PC — Native SDK Sample",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800,
        600, nullptr, nullptr, hInstance, this);
    if (!hwndMain) {
      std::cerr << "ERROR: Failed to create window!" << std::endl;
      std::exit(EXIT_FAILURE);
    }

    btnBillingSDK = CreateWindowExW(
        0, L"BUTTON", L"Billing SDK", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20,
        12, 120, 38, hwndMain, reinterpret_cast<HMENU>(IDR_BTN_BILLING_SDK),
        hInstance, nullptr);
    InitOpenGL();
  }

  ~GameApplication() {
    if (hGLRC) {
      wglMakeCurrent(nullptr, nullptr);
      wglDeleteContext(hGLRC);
      hGLRC = nullptr;
    }
    if (hDC && hwndMain) {
      ReleaseDC(hwndMain, hDC);
      hDC = nullptr;
    }
  }

  void InitOpenGL() {
    hDC = GetDC(hwndMain);
    if (!hDC) {
      std::cerr << "ERROR: Failed to get device context!" << std::endl;
      return;
    }

    PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR),
                                 1,
                                 PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
                                     PFD_DOUBLEBUFFER,
                                 PFD_TYPE_RGBA,
                                 32,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 24,
                                 8,
                                 0,
                                 PFD_MAIN_PLANE,
                                 0,
                                 0,
                                 0,
                                 0};

    int pixelFormat = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, pixelFormat, &pfd);
    hGLRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hGLRC);

    if (glewInit() != GLEW_OK) {
      std::cerr << "ERROR: Failed to initialize GLEW!" << std::endl;
      return;
    }

    glEnable(GL_DEPTH_TEST);
    renderer_.Init();
    renderer_.Bind(&camera_);
  }

  void RenderScene() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera_.Update();
    renderer_.Update(0.13f);
    renderer_.Render(kTeapotColor);
    SwapBuffers(hDC);
  }

  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam) {
    static GameApplication* app = nullptr;

    switch (uMsg) {
      case WM_CREATE: {
        auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = static_cast<GameApplication*>(pCreate->lpCreateParams);
        return 0;
      }
      case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        if (app != nullptr) {
          app->RenderScene();
        }
        EndPaint(hwnd, &ps);
        return 0;
      }
      case WM_COMMAND: {
        if (LOWORD(wParam) == IDR_BTN_BILLING_SDK && app != nullptr) {
          GameAppBillingSdkDialog sdkDialog(app->hInstance);
          sdkDialog.ShowDialog();
        }
        return 0;
      }
      case WM_CLOSE: {
        DestroyWindow(hwnd);
        return 0;
      }
      case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
      }
      default:
        break;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }
};

int main() {
  GameApplication app;
  MSG msg = {};
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return static_cast<int>(msg.wParam);
}
