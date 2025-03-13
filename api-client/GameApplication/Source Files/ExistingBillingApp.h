#pragma once
#include "..\GameApplication.h"

namespace GameApplication{

    ref class ExistingBillingApp :
        public GameApplication
    {
    public:
        ExistingBillingApp() {
            ShowText();
            //InitializeComponent();
            this->Text = "GameApplication - Billing";
            this->Size = System::Drawing::Size(2700, 1500);
            this->ShowDialog();

        }
        void ShowText() override{
            Console::WriteLine("ExistingBillingApp");
        }
        void OnExecute(System::Object^ sender, System::EventArgs^ e) override;
        void OnGameListSelected(System::Object^ sender, System::EventArgs^ e) override;
        void LoadWindow(System::Object^ sender, System::EventArgs^ e) override;

    };
};
