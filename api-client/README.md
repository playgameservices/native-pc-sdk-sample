# Deprecation Notice

API way of accessing will be replaced by SDK way of accessing, refer to [sdk-client](/sdk-client/) for the sample of similar functionality accessed through [SDK](https://developer.android.com/games/playgames/native-pc/reference).

# Teapot Sample

This project demonstrates Windows and Android sharing C++ code.
(Work in progress, please report bugs and suggestions)

## Windows Setup

In order to build run this sample on Windows you will need libraries 'freeglut' and 'glew'.

One way to get these is to use vcpkg:

```
./vcpkg install freeglut:x64-windows
./vcpkg install glew:x64-windows
./vcpkg install freeglut:x86-windows
./vcpkg install glew:x86-windows
```

## Play Console Setup

You will need to setup billing from your [Play Console Application](https://developer.android.com/distribute/console) and [Create in-app products](https://developer.android.com/guide/playpoints/create-products) for purchase in this sample.

Also, to sign-in using Google Identity API, you will need to retrieve your [Google API client-id and secret](https://developers.google.com/identity/oauth2/web/guides/get-google-api-clientid)

## Building the application

* Open [Teapot-Sdk.sln](Teapot-Sdk.sln) file using any recent version of Microsoft Visual Studio

* Open [app.config](GameApplication/app.config) and replace the _client_id_ and _client_secret_ with your Google API client-id and secret.

* Open [AppConfig.json](GameApplication/AppConfig.json) and replace the same _client_id_ and _client_secret_ with your Google API client-id and secret.

* Press F5 to build and run your application

## Running the application

* Once the application is running correctly, you will be greeted by an OpenGL [scene](docs/screenshots/api-client/00.png) with a teapot and "Billing APIs" button

![Teapot Scene](/docs/screenshots/api-client/00.png?raw=true "Teapot Scene")

* Click on the "Billing APIs" button and a [debug panel](docs/screenshots/api-client/01.png) will pop up. Using this panel, you will be able to see a list of the operations you can perform, the API and parameters that will be sent to the server as well as the result once you execute the API request.

![Debug Panel](/docs/screenshots/api-client/01.png?raw=true "Debug Panel")

* In the *Select the operation* menu, click on the first menu item "Google Login". The URLs and parameters will automatically change to reflect the backend APIs that will be invoked when you click on the *Execute* button.

![Google Login](/docs/screenshots/api-client/02.png?raw=true "Google Login")

* Click on the *Execute* button and follow the login process, make sure that you give the permissions requested during the OAuth steps that follows

![Oauth Steps](/docs/screenshots/api-client/03.png?raw=true "OAuth Steps")

![Oauth Steps](/docs/screenshots/api-client/04.png?raw=true "OAuth Steps")

* Upon returning, you will get the appropriate *user access token* that can be used to invoke purchase and other subsequent APIs

![User Access Token](/docs/screenshots/api-client/05.png?raw=true "User Access Token")

* With the *user access token* loaded, proceed to *Get SKUs* API which will allow you to get a list of all available IAP items and launch the purchase flow on a hardcoded item

![Get SKUs API](/docs/screenshots/api-client/06.png?raw=true "Get SKUs API")

* The application will launch the purchase flow using your default browser

![Purchase Flow](/docs/screenshots/api-client/07.png?raw=true "Purchase Flow")

![Purchase Completed](/docs/screenshots/api-client/08.png?raw=true "Purchase Completed")

* Get all purchased items using the [Get Purchase API](https://developers.google.com/android-publisher/api-ref/rest/v3/purchases.products/get)

![Get Purchased Items](/docs/screenshots/api-client/09.png?raw=true "Get Purchased Items")

![Purchase Token](/docs/screenshots/api-client/10.png?raw=true "Purchase Token")

* Validate the purchased item using [Validate Purchase API](https://developers.google.com/android-publisher/api-ref/rest/v3/purchases.products)

![Validate Purchase](/docs/screenshots/api-client/11.png?raw=true "Validate Purchase")

![Validate Purchase Result](/docs/screenshots/api-client/12.png?raw=true "Validate Purchase Result")

* Acknowledge the purchased item using [Acknowledge Purchase API](https://developers.google.com/android-publisher/api-ref/rest/v3/purchases.products/acknowledge)

![Acknowledge Purchase](/docs/screenshots/api-client/13.png?raw=true "Acknowledge Purchase")

![Acknowledge Purchase Result](/docs/screenshots/api-client/14.png?raw=true "Acknowledge Purchase Result")
  
* Consume the purchased item using [Consume Purchase API](https://developers.google.com/android-publisher/api-ref/rest/v3/purchases.products/consume)

![Consume Purchase](/docs/screenshots/api-client/15.png?raw=true "Consume Purchase")

![Consume Purchase Result](/docs/screenshots/api-client/16.png?raw=true "Consume Purchase Result")
  
 * Invoke get all purchased items using the [Get Purchase API](https://developers.google.com/android-publisher/api-ref/rest/v3/purchases.products/get) and you will see that the API now returns no item as it has been consumed

![Get Purchased Items Result](/docs/screenshots/api-client/17.png?raw=true "Get Purchased Items Result")

Note that some of the API may return empty string upon success, this is intended and you can modify it in the [API Server](api-server)
  