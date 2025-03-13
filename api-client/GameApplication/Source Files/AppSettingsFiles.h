#pragma once
#include "HelperClasses.h"

namespace GameApplication {
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Data;
	using namespace System::Text;
	using namespace System::Runtime::Serialization::Json;
	using namespace System::Runtime::Serialization;
	using namespace System::Net;
	using namespace System::Net::Sockets;
	using namespace System::Collections::Generic;
	using namespace System::Net::Http;
	/*
	[DataContract]
	ref class Config {
	public:
		[DataMember]
			String^ client_id;
		[DataMember]
			String^ client_secret;
		[DataMember]
			String^ redirect_uri;
		[DataMember]
			String^ authorization_Endpoint;
		[DataMember]
			String^ authorization_code;
		[DataMember]
			String^ Scopes;
		[DataMember]
			String^ tokenEndpoint;
	};*/

	/* [DataContract]
	ref class GameApis {
	public:
		[DataMember]
			int ID;
		[DataMember]
			String^ Method;
		[DataMember]
			String^ MethodName;
		[DataMember]
			String^ URL;
		[DataMember]
			String^ Parameter;
		[DataMember]
			String^ Bearer;
		[DataMember]
			String^ PostData;
	};*/

	[DataContract]
		public ref class Purchase
	{
	public:
		[DataMember]
			String^ purchaseTimeMillis;
		[DataMember]
			int purchaseState;
		[DataMember]
			int consumptionState;
		[DataMember]
			String^ orderId;
		[DataMember]
			int purchaseType;
		[DataMember]
			int acknowledgementState;
		[DataMember]
			String^ purchaseToken;
		[DataMember]
			String^ productId;
		[DataMember]
			String^ kind;
		[DataMember]
			String^ regionCode;
	};

	[DataContract]
	public ref class PurchasesResult
	{
	public:
		[DataMember]
		array<Purchase^>^ resources;
	};

	[DataContract]
	public ref class RootObject
	{
	public:
		[DataMember]
			PurchasesResult^ purchasesresult;
	};

	ref class AppSettingsFiles
	{
	public:
		String^ mPackageName;
		String^ mSku;
		String^ mIPAddress;
		Config^ mConfig;
		String^ mPurchaseToken;
		List<GameApis^>^ mGameApiList;
		String^ mState;
		String^ mUserAccessToken;
		String^ codeChallenge;
		String^ mRedirectUri;
	public:
		AppSettingsFiles();

	protected:
		void ParseManifestXml();
		String^ GetLocalIPAddress();
		String^ ReadFile(String^ filePath);
		Config^ ReadConfig(String^ fileName);
		List<GameApis^>^ ReadApis(String^ fileName);
		//PKCE Functions start
		String^ GenerateRandomBase64Url(int length);
		array<Byte>^ ComputeSha256(String^ input);
		String^ Base64UrlEncodeNoPadding(array<Byte>^ bytes);
		String^ ComputeCodeChallenge(String^ verifier);


	};
}
