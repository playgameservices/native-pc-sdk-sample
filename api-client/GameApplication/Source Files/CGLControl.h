#pragma once

#include <Windows.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "TeapotRenderer.h"
#include "TapCamera.h"
#include <list>
#using <System.dll>

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

namespace GameApplication {

	/// <summary>
	/// Summary for CGLControl
	/// </summary>
	public ref class CGLControl : public System::Windows::Forms::UserControl
    {
    public:
        TeapotRenderer* renderer_;
        ndk_helper::TapCamera* camera_;
        CGLControl();
        ~CGLControl();
        Button^ butBilling;
        void InitializeOpenGL();
        void RenderScene();
        // Display_Objects() function
        void Display_Objects(void);

        // Reshape() function
        void Reshape(int w, int h);
    protected:
        virtual void OnPaint(PaintEventArgs^ e) override;

    private:
        bool bCloudTop = false;
        void OnBillingButtonClick(Object^ sender, EventArgs^ e);
        bool IsRunningInCloud();
        HDC hDC; // Device context handle
        HGLRC hGLRC; // OpenGL rendering context

        // Init_OpenGL() function
        void Init_OpenGL();
        void DumpDevice(const DISPLAY_DEVICE& dd, int nSpaceCount);
        bool InitializeWithFirstDevice(char* program);

    };

}
