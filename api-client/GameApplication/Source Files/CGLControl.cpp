#include "pch.h"
#include "CGLControl.h"
#include "..\GameApplication.h"
#include "ExistingBillingApp.h"
using namespace System::Diagnostics;
//#include "AuthWindow.h"
//#include "Auth.h"

namespace GameApplication {

    CGLControl::CGLControl()
    {
        bCloudTop = IsRunningInCloud();
        // Set up the control styles
        this->SetStyle(ControlStyles::Opaque, true);
        this->SetStyle(ControlStyles::UserPaint, true);
        this->SetStyle(ControlStyles::AllPaintingInWmPaint, true);
        if (bCloudTop)
            return;
        renderer_ = new TeapotRenderer();
        camera_ = new ndk_helper::TapCamera();
        
    }

    CGLControl::~CGLControl()
    {
        if (hGLRC)
        {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(hGLRC);
        }
        if (hDC)
        {
            ReleaseDC((HWND)this->Handle.ToPointer(), hDC);
        }
        if (bCloudTop)
            return;
        delete renderer_;
        delete camera_;
    }

    bool CGLControl::IsRunningInCloud()
    {
        bool ret = false;
        String^ output = nullptr;

        try {
            Process^ process = gcnew Process();
            process->StartInfo->FileName = "cmd.exe";  // Running cmd.exe
            process->StartInfo->Arguments = "/C wmic computersystem get Manufacturer /value";
            process->StartInfo->RedirectStandardOutput = true;
            process->StartInfo->RedirectStandardError = true;
            process->StartInfo->UseShellExecute = false;
            process->StartInfo->CreateNoWindow = true;

            if (!process->Start()) {
                Console::WriteLine("Failed to start process.");
                return ret;
            }
            output = process->StandardOutput->ReadToEnd();
            process->WaitForExit();
        }
        catch (Exception^ ex) {
            Console::WriteLine("An error occurred: {0}", ex->Message);
            return ret;
        }
        ret = output->Contains("Google");
        return ret;
    }

    // Init_OpenGL() function
    void CGLControl::Init_OpenGL() {

        GLint GlewInitResult = glewInit();
        if (GLEW_OK != GlewInitResult) {
            printf("ERROR: %s", glewGetErrorString(GlewInitResult));
            exit(EXIT_FAILURE);
        }
        // set background color to Black
        glClearColor(0.0, 0.0, 0.0, 0.0);

        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
        renderer_->Init();
        renderer_->Bind(camera_);
        renderer_->UpdateViewport();
        // set shade model to Flat
        glShadeModel(GL_FLAT);
    }

    void CGLControl::InitializeOpenGL()
    {
        Console::WriteLine("InitializeopenGL");
        if (!bCloudTop) {
            hDC = GetDC((HWND)this->Handle.ToPointer());
            if (!hDC)
                throw gcnew Exception("Failed to get device context");
            // Set up pixel format
            PIXELFORMATDESCRIPTOR pfd = {
                sizeof(PIXELFORMATDESCRIPTOR), 1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
                PFD_TYPE_RGBA, // Pixel type
                32,            // Color depth
                0, 0, 0, 0, 0, 0, // Color bits
                0, 0,           // No alpha buffer
                0, 0, 0, 0, 0,  // Accumulation buffer
                24,             // Depth buffer
                8,              // Stencil buffer
                0,              // Aux buffers
                PFD_MAIN_PLANE, // Main layer
                0, 0, 0, 0      // Reserved
            };
            int pixelFormat = ChoosePixelFormat(hDC, &pfd);
            if (!pixelFormat)
                throw gcnew Exception("Failed to choose pixel format");
            if (!SetPixelFormat(hDC, pixelFormat, &pfd))
                throw gcnew Exception("Failed to set pixel format");
            hGLRC = wglCreateContext(hDC);
            if (!hGLRC)
                throw gcnew Exception("Failed to create OpenGL context");
            if (!wglMakeCurrent(hDC, hGLRC))
                throw gcnew Exception("Failed to activate OpenGL context");
            Init_OpenGL();
            // Initialize GLEW
        }
        butBilling = gcnew Button();
        butBilling->Text = "Billing APIs";
        butBilling->Width = 100;
        butBilling->Height = 40;
        butBilling->Location = Drawing::Point(10, 10); // Position over the OpenGL control
        butBilling->Click += gcnew EventHandler(this, &CGLControl::OnBillingButtonClick);
        this->Controls->Add(butBilling);

    }

    void CGLControl::OnBillingButtonClick(Object^ sender, EventArgs^ e)
    {
        GameApplication^ app = gcnew ExistingBillingApp();
    }

    void CGLControl::RenderScene()
    {
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GLfloat teapotColor[] = { 219.0f / 255.0f, 68.0f / 255.0f, 55.0f / 255.0f };
        renderer_->Init();
        renderer_->Bind(camera_);
        renderer_->Render(teapotColor);
        renderer_->UpdateViewport();
        camera_->Update();
        renderer_->Update(0.13f);
        renderer_->UpdateViewport();
        glShadeModel(GL_FLAT);

        SwapBuffers(hDC); // Swap the buffers to display the rendered content
    }

    // Display_Objects() function
    void CGLControl::Display_Objects(void) {
        camera_->Update();
        renderer_->Update(0.13f);
        // clearing the window or remove all drawn objects
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GLfloat teapotColor[] = { 61.0f / 255.0f, 220.0f / 255.0f, 132.0f / 255.0f };

        renderer_->Render(teapotColor);
        glutSwapBuffers();
    }

    // Reshape() function
    void CGLControl::Reshape(int w, int h) {
        glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    }

    void CGLControl::OnPaint(PaintEventArgs^ e)
    {
        if (!bCloudTop)
            RenderScene();
    }

    void CGLControl::DumpDevice(const DISPLAY_DEVICE& dd, int nSpaceCount) {
        wprintf(TEXT("%*sDevice Name: %s\n"), nSpaceCount, TEXT(""), dd.DeviceName);
        wprintf(TEXT("%*sDevice String: %s\n"), nSpaceCount, TEXT(""), dd.DeviceString);
        wprintf(TEXT("%*sState Flags: %x\n"), nSpaceCount, TEXT(""), dd.StateFlags);
        wprintf(TEXT("%*sDeviceID: %s\n"), nSpaceCount, TEXT(""), dd.DeviceID);
        wprintf(TEXT("%*sDeviceKey: ...%s\n\n"), nSpaceCount, TEXT(""), dd.DeviceKey + 42);
    }

    bool CGLControl::InitializeWithFirstDevice(char* program) {
        DISPLAY_DEVICE dd;
        dd.cb = sizeof(DISPLAY_DEVICE);

        DWORD deviceNum = 0;
        while (EnumDisplayDevices(NULL, deviceNum, &dd, 0)) {

            DISPLAY_DEVICE newdd = { 0 };
            newdd.cb = sizeof(DISPLAY_DEVICE);
            DWORD monitorNum = 0;
            while (EnumDisplayDevices(dd.DeviceName, monitorNum, &newdd, 0)) {
                DumpDevice(dd, 0);
                DumpDevice(newdd, 4);
                monitorNum++;
                int argc = 3;
                char* argv[3];
                char hold[250];
                argv[2] = hold;
                size_t size;
                wcstombs_s(&size, hold, dd.DeviceName, 250);
                argv[0] = _strdup(program);
                argv[1] = _strdup("-display");
                glutInit(&argc, argv);
                free(argv[0]);
                free(argv[1]);
                free(argv[2]);
                return true;
            }
            puts("");
            deviceNum++;
        }

        return false;
    }

}
