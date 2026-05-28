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
	using namespace System::Runtime::Serialization::Json;
	using namespace System::Runtime::Serialization;
	using namespace System::Threading::Tasks;
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
	};

	List<GameApis^>^ ReadApis(String^ fileName);

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
	};
	Config^ ReadConfig(String^ fileName);

	[DataContract]
	ref class Response {
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

	ref class SharedData {
	public:
		Config^ sdConfig;
		Response^ sdResposne;
		String^ sdAccessToken;
		String^ sdLocalhostAddress;
		String^ sdApiResponse;
		String^ mState;
		String^ sdServiceAccessToken;
	};

	ref class AppConfig {
	public:
		//[JsonProperty("client_id")]
		String^ client_id;
		//[JsonProperty("client_secret")]
		String^ client_secret;
		//[JsonProperty("redirect_uri")]
		String^ redirect_uri;
		//[JsonProperty("authorization_Endpoint")]
		String^ authorization_Endpoint;
		//[JsonProperty("authorization_code")]
		String^ authorization_code;
		//[JsonProperty("Scopes")]
		String^ Scopes;
		//[JsonProperty("tokenEndpoint")]
		String^ tokenEndpoint;
		AppConfig() {}
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
		RequestData();
		RequestData(String^ ssku, String^ stoken);
	};

	//template<typename T>
	//String^ Serialize(T^ data);

	//template<typename T>
	//T^ Deserialize(String^ json);
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

	template <typename T>
	T ParseJsonFile(String^ filePath) {
		// Read the JSON content
		String^ json = System::IO::File::ReadAllText(filePath);

		// Parse JSON into the specified type
		T result = nullptr;//Newtonsoft::Json::JsonConvert::DeserializeObject<T>(json);

		return result;
	}

	List<String^>^ ExtractCurlyBracesBlocks(String^ data);

	//Util-Functions
	String^ GetLocalIPAddress();
	Dictionary<String^, String^>^ ParseQueryParameters(String^ url);



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

	//template <typename T^>
	String^ GetSkuObj(String^ jsonString);

};


#endif // !HELPERCLASSES_H
