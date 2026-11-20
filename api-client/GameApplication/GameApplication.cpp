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

#include <windows.h>
#include <gl/glew.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <gl/glut.h>

#pragma comment(lib, "Ws2_32.lib")

#include "CGLControl.h"
#include "GameApplication.h"
#include "GameHelper.h"
#include "TapCamera.h"
#include "TeapotRenderer.h"
#include "resource.h"

namespace GameApplication {

void GameApplication::RunListener(Object^ listenerObj) {
  HttpListenerThread^ listenerThread =
      dynamic_cast<HttpListenerThread^>(listenerObj);
  if (listenerThread != nullptr) {
    listenerThread->StartListening();
  }
}

}  // namespace GameApplication

using namespace GameApplication;

public ref class GameForm : public Form {
 public:
  GameForm() {
    this->Text =
        "Google Play Games on PC — Legacy REST API Sample (Migration Reference)";
    this->Size = System::Drawing::Size(700, 500);
    CGLControl^ openGLControl = gcnew CGLControl();
    openGLControl->Dock = DockStyle::Fill;
    this->Controls->Add(openGLControl);
    openGLControl->InitializeOpenGL();
  }
};

[STAThread]
int main(array<System::String^>^ args) {
  (void)args;
  System::Windows::Forms::Application::EnableVisualStyles();
  System::Windows::Forms::Application::SetCompatibleTextRenderingDefault(false);
  System::Windows::Forms::Application::Run(gcnew GameForm());
  return 0;
}
