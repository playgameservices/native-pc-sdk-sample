# Teapot Sample

This project demonstrates Windows and Android sharing C++ code.

## Windows Setup

In order to build run this sample on Windows you will need libraries 'freeglut' and 'glew'.

One way to get these is to use vcpkg:
Common reqirement for Game Engine
```
./vcpkg install freeglut:x64-windows
./vcpkg install glew:x64-windows
./vcpkg install freeglut:x86-windows
./vcpkg install glew:x86-windows
```
Integrated vcpkg and moved the play_pc_sdk under root\Library folder.
with this we don't need to copy the vcpkg and play_pc_sdk dll in all the folders

```make sure the librars and dlls are availble in the root\Library and root\vcpkg\installed folder 
and in the solution root folder.```

For SDK version
```
unzip play_pc_sdk_v0.8.0.zip on root folder
Install Google play games beta
```
For legancy billing version
```
    "nlohmann-json",
    {
      "name": "google-cloud-cpp",
      "default-features": false,
      "features": [ "storage" ]
    },
    "tinyxml2",
    "boost-asio",
	"boost-beast",
	"boost-json",
	"restclient-cpp"
```

Run command on the cmd root directory 
```
c:\path-to-vcpkg\vcpkg install
```
The vcpkg tool can be installed using these instructions: https://vcpkg.io/en/getting-started.html 

Update AppConfig.json file
```
Replace   "client_id": "xxxx", & "client_secret": "xxxxx",
with your client_id and client_secret
```

Edit Register run the command
```
reg add "HKLM\Software\Google\Play Games Services" /v EarlyAccessPartnerGuid /t REG_SZ /d c0f10adb-4e19-4cb0-b191-c7b77f0d0b9d
```

Install and run Google Play Games
```
Google Play Games BETA - https://play.google.com/googleplaygames
Login with personal/gamer email ID
```

