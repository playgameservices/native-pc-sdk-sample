#ifndef HELPERCLASSES_H
#define HELPERCLASSES_H

namespace GameApplication {
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Net;
	using namespace System::Net::Sockets;
	using namespace System::Collections::Generic;
	using namespace System::Net::Http;
	using namespace System::Text;
	using namespace System::Runtime::Serialization::Json;
	using namespace System::Runtime::Serialization;
	using namespace System::Threading::Tasks;
	using namespace System::IO;
	using namespace System::Text::RegularExpressions;
	using namespace System::Net::NetworkInformation;
	using namespace System::Diagnostics;

	String^ ReadFile(String^ filePath);

	public enum HTTPType {
		GET,
		POST
	};

	public enum HttpOps {
		GLogin,					//0
		GetSKUs,				//1
		GetPurchases,			//2
		VerifyThePurchaseToken,	//3
		AcknowledgeThePurchase,	//4
		ConsumePurchases,		//5
		VerifyIntegrityToken,	//6
	};

	[DataContract]
	public ref class GameApis {
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
	};

	List<GameApis^>^ ReadApis(String^ fileName);

	[DataContract]
	public ref class Config {
	public:
		[DataMember]
		String^ client_id;
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
	};
	Config^ ReadConfig(String^ fileName);

	[DataContract]
	public ref class Response {
	public:
		[DataMember]
			String^ access_token;
		[DataMember]
			String^ expires_in;
		[DataMember]
			String^ refresh_token;
		[DataMember]
			String^ scope;
		[DataMember]
			String^ token_type;
		[DataMember]
			String^ id_token;
	};
	Response^ ReadResponse(String^ jsonString);

	public ref class SharedData {
	public:
		Object^ syncRoot;
		Config^ sdConfig;
		Response^ sdResposne;
		String^ sdAccessToken;
		String^ sdLocalhostAddress;
		String^ sdApiResponse;
		String^ mState;
		String^ codeVerifier;
		SharedData() {
			syncRoot = gcnew Object();
		}
	};

	[DataContract]
	public ref class RequestData
	{
	public:
		[DataMember]
			String^ productId;
		[DataMember]
			String^ sku;
		[DataMember]
			String^ token;
		[DataMember(EmitDefaultValue = false)]
			String^ packageName;
		RequestData();
		RequestData(String^ ssku, String^ stoken);
		RequestData(String^ ssku, String^ stoken, String^ spkg);
	};

	[DataContract]
	public ref class PurchasesRequestData
	{
	public:
		[DataMember]
			String^ userAccessToken;
		PurchasesRequestData(String^ accessToken) : userAccessToken(accessToken) {}
	};

	[DataContract]
	public ref class SkuDetailsRequestData
	{
	public:
		[DataMember]
			String^ userAccessToken;
		[DataMember]
			String^ sku;
		[DataMember]
			String^ skuType;
		[DataMember]
			String^ userIpAddress;
		[DataMember]
			String^ languageCode;
		SkuDetailsRequestData(
			String^ accessToken,
			String^ ssku,
			String^ sType,
			String^ ip,
			String^ lang)
			: userAccessToken(accessToken),
			  sku(ssku),
			  skuType(sType),
			  userIpAddress(ip),
			  languageCode(lang) {}
	};

	[DataContract]
	public ref class IntegrityRequestData
	{
	public:
		[DataMember]
			String^ packageName;
		[DataMember]
			String^ token;
		IntegrityRequestData(String^ pkg, String^ tok) : packageName(pkg), token(tok) {}
	};

	template<typename T>
	String^ Serialize(T^ data)
	{
		MemoryStream^ stream = gcnew MemoryStream();
		DataContractJsonSerializer^ serializer = gcnew DataContractJsonSerializer(T::typeid);
		serializer->WriteObject(stream, data);
		stream->Position = 0;
		StreamReader^ reader = gcnew StreamReader(stream, Encoding::UTF8);
		String^ json = reader->ReadToEnd();
		return json;
	}

	template<typename T>
	T^ Deserialize(String^ json)
	{
		array<Byte>^ jsonBytes = Encoding::UTF8->GetBytes(json);
		MemoryStream^ stream = gcnew MemoryStream(jsonBytes);
		DataContractJsonSerializer^ serializer = gcnew DataContractJsonSerializer(T::typeid);
		T^ data = (T^)serializer->ReadObject(stream);
		return data;
	}

	List<String^>^ ExtractCurlyBracesBlocks(String^ data);

	// Util-Functions
	String^ GetLocalIPAddress();
	bool TryValidateAllowedHttpsUrl(String^ rawUrl, String^ expectedHost, Uri^% outUri);
	String^ RedactTokenForLog(String^ secret);

	[DataContract]
	ref class localizedPrice
	{
	public:
		[DataMember]
			String^ priceMicros;
		[DataMember]
			String^ formattedPrice;
		[DataMember]
			String^ currency;
	};

	[DataContract]
	ref class resources
	{
	public:
		[DataMember]
			String^ packageName;
		[DataMember]
			String^ sku;
		[DataMember]
			String^ deepLink;
		[DataMember]
			localizedPrice^ localizedPrice;
	};

	[DataContract]
	ref class result
	{
	public:
		[DataMember]
			resources^ resource;
	};

	[DataContract]
	ref class SkuResult
	{
	public:
		[DataMember]
			result^ result;
	};

	String^ GetSkuObj(String^ jsonString);

};

#endif // !HELPERCLASSES_H
