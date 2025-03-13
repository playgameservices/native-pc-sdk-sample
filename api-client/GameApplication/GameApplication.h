#pragma once
#include <Logger.h>
#include "AppSettingsFiles.h"

#include <DevServer.h>

namespace GameApplication {
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace CLogging;

	/// <summary>
	/// Summary for GameApplication
	/// </summary>
	public ref class GameApplication : public System::Windows::Forms::Form
	{
	public:
		GameApplication(void);
		Logger* logger = NULL;
	protected:
		AppSettingsFiles^ appSettingFiles;
		String^ mPackageName;
		
		 
	private: System::Windows::Forms::Button^ RebuildUrlBtn;
	protected:
		String^ mSku;
		virtual void ShowText();
		virtual void LoadWindow(System::Object^ sender, System::EventArgs^ e){}
		virtual void OnExecute(System::Object^ sender, System::EventArgs^ e) {}
		virtual void OnGameListSelected(System::Object^ sender, System::EventArgs^ e){}
	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~GameApplication()
		{
			if (components)
			{
				delete components;
			}
			delete logger;
		}
	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;
	private: System::Windows::Forms::Label^ lblGoogleTitle;
	private: System::Windows::Forms::Label^ lblAccessToken;
	private: System::Windows::Forms::TextBox^ txtAccessToken;



	private: System::Windows::Forms::Label^ lblSelectOpr;
	protected: System::Windows::Forms::ListBox^ lstBoxOperations;
	private:

	private:

	private: System::Windows::Forms::Label^ lblHttpMethod;
	private: System::Windows::Forms::Label^ lblDefultUrl;
	private: System::Windows::Forms::TextBox^ txtDefaulUrl;
	private: System::Windows::Forms::TextBox^ txtBuiltUrl;



	private: System::Windows::Forms::Label^ lblBuiltUrl;
	private: System::Windows::Forms::CheckBox^ checkBtnBearerToken;
	public protected: System::Windows::Forms::Panel^ scrollPanel;
	private:
	public protected: System::Windows::Forms::TableLayoutPanel^ tableParameters;
	private: System::Windows::Forms::Label^ lblResult;
	private: System::Windows::Forms::Button^ btnExecute;
	private: System::Windows::Forms::TextBox^ txtResult;
	private: System::Windows::Forms::Label^ lblParameter;
	public protected:

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->lblGoogleTitle = (gcnew System::Windows::Forms::Label());
			this->lblAccessToken = (gcnew System::Windows::Forms::Label());
			this->txtAccessToken = (gcnew System::Windows::Forms::TextBox());
			this->lblSelectOpr = (gcnew System::Windows::Forms::Label());
			this->lstBoxOperations = (gcnew System::Windows::Forms::ListBox());
			this->lblHttpMethod = (gcnew System::Windows::Forms::Label());
			this->lblDefultUrl = (gcnew System::Windows::Forms::Label());
			this->txtDefaulUrl = (gcnew System::Windows::Forms::TextBox());
			this->txtBuiltUrl = (gcnew System::Windows::Forms::TextBox());
			this->lblBuiltUrl = (gcnew System::Windows::Forms::Label());
			this->checkBtnBearerToken = (gcnew System::Windows::Forms::CheckBox());
			this->scrollPanel = (gcnew System::Windows::Forms::Panel());
			this->tableParameters = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->lblResult = (gcnew System::Windows::Forms::Label());
			this->btnExecute = (gcnew System::Windows::Forms::Button());
			this->txtResult = (gcnew System::Windows::Forms::TextBox());
			this->lblParameter = (gcnew System::Windows::Forms::Label());
			this->RebuildUrlBtn = (gcnew System::Windows::Forms::Button());
			this->scrollPanel->SuspendLayout();
			this->SuspendLayout();
			// 
			// lblGoogleTitle
			// 
			this->lblGoogleTitle->AutoSize = true;
			this->lblGoogleTitle->Location = System::Drawing::Point(8, 4);
			this->lblGoogleTitle->Margin = System::Windows::Forms::Padding(1, 0, 1, 0);
			this->lblGoogleTitle->Name = L"lblGoogleTitle";
			this->lblGoogleTitle->Size = System::Drawing::Size(80, 13);
			this->lblGoogleTitle->TabIndex = 1;
			this->lblGoogleTitle->Text = L"Google OAuth2";
			// 
			// lblAccessToken
			// 
			this->lblAccessToken->AutoSize = true;
			this->lblAccessToken->Location = System::Drawing::Point(103, 8);
			this->lblAccessToken->Margin = System::Windows::Forms::Padding(1, 0, 1, 0);
			this->lblAccessToken->Name = L"lblAccessToken";
			this->lblAccessToken->Size = System::Drawing::Size(79, 13);
			this->lblAccessToken->TabIndex = 2;
			this->lblAccessToken->Text = L"Access Token:";
			// 
			// txtAccessToken
			// 
			this->txtAccessToken->Location = System::Drawing::Point(177, 7);
			this->txtAccessToken->Margin = System::Windows::Forms::Padding(1);
			this->txtAccessToken->Multiline = true;
			this->txtAccessToken->Name = L"txtAccessToken";
			this->txtAccessToken->Size = System::Drawing::Size(651, 61);
			this->txtAccessToken->TabIndex = 3;
			// 
			// lblSelectOpr
			// 
			this->lblSelectOpr->AutoSize = true;
			this->lblSelectOpr->Location = System::Drawing::Point(12, 31);
			this->lblSelectOpr->Margin = System::Windows::Forms::Padding(1, 0, 1, 0);
			this->lblSelectOpr->Name = L"lblSelectOpr";
			this->lblSelectOpr->Size = System::Drawing::Size(105, 13);
			this->lblSelectOpr->TabIndex = 4;
			this->lblSelectOpr->Text = L"Select the operation:";
			// 
			// lstBoxOperations
			// 
			this->lstBoxOperations->FormattingEnabled = true;
			this->lstBoxOperations->Location = System::Drawing::Point(15, 45);
			this->lstBoxOperations->Margin = System::Windows::Forms::Padding(1);
			this->lstBoxOperations->Name = L"lstBoxOperations";
			this->lstBoxOperations->Size = System::Drawing::Size(158, 290);
			this->lstBoxOperations->TabIndex = 5;
			this->lstBoxOperations->SelectedIndexChanged += gcnew System::EventHandler(this, &GameApplication::lstBox_SelectedIndexChanged);
			// 
			// lblHttpMethod
			// 
			this->lblHttpMethod->AutoSize = true;
			this->lblHttpMethod->Location = System::Drawing::Point(449, 71);
			this->lblHttpMethod->Margin = System::Windows::Forms::Padding(1, 0, 1, 0);
			this->lblHttpMethod->Name = L"lblHttpMethod";
			this->lblHttpMethod->Size = System::Drawing::Size(75, 13);
			this->lblHttpMethod->TabIndex = 6;
			this->lblHttpMethod->Text = L"Http Method - ";
			// 
			// lblDefultUrl
			// 
			this->lblDefultUrl->AutoSize = true;
			this->lblDefultUrl->Location = System::Drawing::Point(175, 71);
			this->lblDefultUrl->Margin = System::Windows::Forms::Padding(1, 0, 1, 0);
			this->lblDefultUrl->Name = L"lblDefultUrl";
			this->lblDefultUrl->Size = System::Drawing::Size(60, 13);
			this->lblDefultUrl->TabIndex = 7;
			this->lblDefultUrl->Text = L"Default Url:";
			// 
			// txtDefaulUrl
			// 
			this->txtDefaulUrl->Location = System::Drawing::Point(177, 86);
			this->txtDefaulUrl->Margin = System::Windows::Forms::Padding(1);
			this->txtDefaulUrl->Multiline = true;
			this->txtDefaulUrl->Name = L"txtDefaulUrl";
			this->txtDefaulUrl->Size = System::Drawing::Size(656, 40);
			this->txtDefaulUrl->TabIndex = 8;
			// 
			// txtBuiltUrl
			// 
			this->txtBuiltUrl->Location = System::Drawing::Point(177, 139);
			this->txtBuiltUrl->Margin = System::Windows::Forms::Padding(1);
			this->txtBuiltUrl->Multiline = true;
			this->txtBuiltUrl->Name = L"txtBuiltUrl";
			this->txtBuiltUrl->Size = System::Drawing::Size(656, 62);
			this->txtBuiltUrl->TabIndex = 10;
			// 
			// lblBuiltUrl
			// 
			this->lblBuiltUrl->AutoSize = true;
			this->lblBuiltUrl->Location = System::Drawing::Point(175, 125);
			this->lblBuiltUrl->Margin = System::Windows::Forms::Padding(1, 0, 1, 0);
			this->lblBuiltUrl->Name = L"lblBuiltUrl";
			this->lblBuiltUrl->Size = System::Drawing::Size(43, 13);
			this->lblBuiltUrl->TabIndex = 9;
			this->lblBuiltUrl->Text = L"Built Url";
			// 
			// checkBtnBearerToken
			// 
			this->checkBtnBearerToken->AutoSize = true;
			this->checkBtnBearerToken->Checked = true;
			this->checkBtnBearerToken->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkBtnBearerToken->Location = System::Drawing::Point(746, 69);
			this->checkBtnBearerToken->Margin = System::Windows::Forms::Padding(1);
			this->checkBtnBearerToken->Name = L"checkBtnBearerToken";
			this->checkBtnBearerToken->Size = System::Drawing::Size(88, 17);
			this->checkBtnBearerToken->TabIndex = 11;
			this->checkBtnBearerToken->Text = L"BearerToken";
			this->checkBtnBearerToken->UseVisualStyleBackColor = true;
			// 
			// scrollPanel
			// 
			this->scrollPanel->AutoScroll = true;
			this->scrollPanel->BackColor = System::Drawing::SystemColors::ControlLight;
			this->scrollPanel->Controls->Add(this->tableParameters);
			this->scrollPanel->Location = System::Drawing::Point(177, 214);
			this->scrollPanel->Margin = System::Windows::Forms::Padding(1);
			this->scrollPanel->Name = L"scrollPanel";
			this->scrollPanel->Size = System::Drawing::Size(653, 118);
			this->scrollPanel->TabIndex = 28;
			// 
			// tableParameters
			// 
			this->tableParameters->AutoScroll = true;
			this->tableParameters->AutoSize = true;
			this->tableParameters->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
			this->tableParameters->ColumnCount = 2;
			this->tableParameters->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableParameters->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				80)));
			this->tableParameters->Location = System::Drawing::Point(0, 0);
			this->tableParameters->Margin = System::Windows::Forms::Padding(1);
			this->tableParameters->Name = L"tableParameters";
			this->tableParameters->RowCount = 2;
			this->tableParameters->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 5)));
			this->tableParameters->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 100)));
			this->tableParameters->Size = System::Drawing::Size(0, 5);
			this->tableParameters->TabIndex = 0;
			// 
			// lblResult
			// 
			this->lblResult->AutoSize = true;
			this->lblResult->Location = System::Drawing::Point(12, 342);
			this->lblResult->Margin = System::Windows::Forms::Padding(1, 0, 1, 0);
			this->lblResult->Name = L"lblResult";
			this->lblResult->Size = System::Drawing::Size(37, 13);
			this->lblResult->TabIndex = 29;
			this->lblResult->Text = L"Result";
			// 
			// btnExecute
			// 
			this->btnExecute->Location = System::Drawing::Point(758, 335);
			this->btnExecute->Margin = System::Windows::Forms::Padding(1);
			this->btnExecute->Name = L"btnExecute";
			this->btnExecute->Size = System::Drawing::Size(57, 21);
			this->btnExecute->TabIndex = 30;
			this->btnExecute->Text = L"Execute";
			this->btnExecute->UseVisualStyleBackColor = true;
			this->btnExecute->Click += gcnew System::EventHandler(this, &GameApplication::btnExecute_Click);
			// 
			// txtResult
			// 
			this->txtResult->Location = System::Drawing::Point(10, 359);
			this->txtResult->Margin = System::Windows::Forms::Padding(1);
			this->txtResult->Multiline = true;
			this->txtResult->Name = L"txtResult";
			this->txtResult->ScrollBars = System::Windows::Forms::ScrollBars::Vertical;
			this->txtResult->Size = System::Drawing::Size(824, 130);
			this->txtResult->TabIndex = 31;
			// 
			// lblParameter
			// 
			this->lblParameter->AutoSize = true;
			this->lblParameter->Location = System::Drawing::Point(178, 201);
			this->lblParameter->Margin = System::Windows::Forms::Padding(1, 0, 1, 0);
			this->lblParameter->Name = L"lblParameter";
			this->lblParameter->Size = System::Drawing::Size(55, 13);
			this->lblParameter->TabIndex = 33;
			this->lblParameter->Text = L"Parameter";
			// 
			// RebuildUrlBtn
			// 
			this->RebuildUrlBtn->Location = System::Drawing::Point(181, 336);
			this->RebuildUrlBtn->Margin = System::Windows::Forms::Padding(1);
			this->RebuildUrlBtn->Name = L"RebuildUrlBtn";
			this->RebuildUrlBtn->Size = System::Drawing::Size(86, 21);
			this->RebuildUrlBtn->TabIndex = 34;
			this->RebuildUrlBtn->Text = L"Rebuild Url";
			this->RebuildUrlBtn->UseVisualStyleBackColor = true;
			this->RebuildUrlBtn->Click += gcnew System::EventHandler(this, &GameApplication::RebuildUrlBtn_Click);
			// 
			// GameApplication
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(822, 490);
			this->Controls->Add(this->RebuildUrlBtn);
			this->Controls->Add(this->lblParameter);
			this->Controls->Add(this->txtResult);
			this->Controls->Add(this->btnExecute);
			this->Controls->Add(this->lblResult);
			this->Controls->Add(this->scrollPanel);
			this->Controls->Add(this->checkBtnBearerToken);
			this->Controls->Add(this->txtBuiltUrl);
			this->Controls->Add(this->lblBuiltUrl);
			this->Controls->Add(this->txtDefaulUrl);
			this->Controls->Add(this->lblDefultUrl);
			this->Controls->Add(this->lblHttpMethod);
			this->Controls->Add(this->lstBoxOperations);
			this->Controls->Add(this->lblSelectOpr);
			this->Controls->Add(this->txtAccessToken);
			this->Controls->Add(this->lblAccessToken);
			this->Controls->Add(this->lblGoogleTitle);
			this->Margin = System::Windows::Forms::Padding(1);
			this->MaximumSize = System::Drawing::Size(864, 552);
			this->MinimumSize = System::Drawing::Size(415, 263);
			this->Name = L"GameApplication";
			this->Text = L"GameApplication";
			this->Load += gcnew System::EventHandler(this, &GameApplication::GameApplication_Load);
			this->scrollPanel->ResumeLayout(false);
			this->scrollPanel->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

	protected: 
		System::Void GameApplication_Load(System::Object^ sender, System::EventArgs^ e);
		System::Void btnExecute_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void lstBox_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e);
		System::Void RebuildUrlBtn_Click(System::Object^ sender, System::EventArgs^ e) {
			txtBuiltUrl->Text = UpdateUrl();
		}

		String^ ParseXmlWithReader(String^ filePath);
		void PopulateTable();
		void SetTableData();
		List<String^>^ ExtractCurlyBracesBlocks(String^ data);
		String^ SetValueForKey(String^ key);
		int SetValueForKey(String^ key, String^ value);
		String^ UpdateUrl();
		////////////////////////
		String^ mRedirectUri;
		//PKCE - member varibles
		String^ codeVerifier;
		String^ codeChallenge;
		SharedData^ mSd;

		HttpListenerThread^ listenerFuncThread;
		Thread^ listenerBackgroundThread;
		static void RunListener(Object^ listenerObj);
		void AccessToken();
		String^ BuildAuthorizationUrl(String^ state, String^ codeChallenge);

		void GetSkusDetails();
		void GetPurchasesDetails();
		void GetValidatePurchase();
		void GetAcknowledgePurchase();
		void PostValidatePurchase();
		void PostConsumePurchase();
};
}
