#include "pch.h"
#include "ExistingBillingApp.h"
namespace GameApplication {
	void ExistingBillingApp::LoadWindow(System::Object^ sender, System::EventArgs^ e)
	{
		Console::WriteLine("ExistingBillingApp::LoadWindow");
	}

	void ExistingBillingApp::OnExecute(System::Object^ sender, System::EventArgs^ e)
	{
		Console::WriteLine("ExistingBillingApp::Execute button clicked");
	}

	void ExistingBillingApp::OnGameListSelected(System::Object^ sender, System::EventArgs^ e)
	{
		Console::WriteLine("BillingSDK::Gamelist clicked");
	}

};