# Google Play for Native PC Samples Repository

This repository contains sample projects demonstrating various aspects of integrating Google Play APIs for your native game. A description of the high-level directories in this repo follows:

## API Client

See the [api-client](#api-client) section for description and setup of API Client sample.
*  **[api-client/](api-client)** (C/C++) - This sample demonstrate integration and use of the [Google Play Developer APIs](https://developers.google.com/android-publisher/).

## API Server
See the [api-server](#api-server) section for description and setup of the backend needed by the [api-client](api-client)  sample.
*  **[api-server/](api-server)** (NodeJS/JavaScript) - This sample contain the backend code for the [api-client](api-client) runnable in [Cloud Functions for Firebase](https://firebase.google.com/docs/functions).

# API Client

[api-client/](api-client) folder contains C++ project compilable using any recent version of [Microsoft Visual Studio](https://visualstudio.microsoft.com/).

## Setup

You will need to setup billing from your [Play Console Application](https://developer.android.com/distribute/console) and [Create in-app products](https://developer.android.com/guide/playpoints/create-products) for purchase in this sample.

Also, to sign-in using Google Identity API, you will need to retrieve your [Google API client-id and secret](https://developers.google.com/identity/oauth2/web/guides/get-google-api-clientid)

* Open [Teapot-Sdk.sln](api-client/Teapot-Sdk.sln) file using any recent version of Microsoft Visual Studio

* Open [app.config](api-client/GameApplication/app.config) and replace the _client_id_ and _client_secret_ with your Google API client-id and secret.

* Open [AppConfig.json](api-client/GameApplication/AppConfig.json) and replace the same _client_id_ and _client_secret_ with your Google API client-id and secret.

* Press F5 to build and run your application

## Running the application

* Once the application is running correctly, you will be greeted by an OpenGL [scene](docs/screenshots/00.png) with a teapot and "Billing APIs" button

![Teapot Scene](/docs/00.png?raw=true "Teapot Scene")

* Click on the "Billing APIs" button and a [debug panel](docs/screenshots/01.png) will pop up. Using this panel, you will be able to see a list of the operations you can perform, the API and parameters that will be sent to the server as well as the result once you execute the API request.

![Debug Panel](/docs/01.png?raw=true "Debug Panel")

* In the *Select the operation* menu, click on the first menu item "Google Login". The URLs and parameters will automatically change to reflect the backend APIs that will be invoked when you click on the *Execute* button.

![Google Login](/docs/02.png?raw=true "Google Login")

* Click on the *Execute* button and follow the login process, make sure that you give the permissions requested during the OAuth steps that follows

![Oauth Steps](/docs/03.png?raw=true "OAuth Steps")

![Oauth Steps](/docs/04.png?raw=true "OAuth Steps")

* Upon returning, you will get the appropriate *user access token* that can be used to invoke purchase and other subsequent APIs

![User Access Token](/docs/05.png?raw=true "User Access Token")

* With the *user access token* loaded, proceed to *Get SKUs* API which will allow you to get a list of all available IAP items and launch the purchase flow on a hardcoded item

![Get SKUs API](/docs/06.png?raw=true "Get SKUs API")

* The application will launch the purchase flow using your default browser

![Purchase Flow](/docs/07.png?raw=true "Purchase Flow")

![Purchase Completed](/docs/08.png?raw=true "Purchase Completed")

* Get all purchased items using the [Get Purchase API](https://developers.google.com/android-publisher/api-ref/rest/v3/purchases.products/get)

![Get Purchased Items](/docs/09.png?raw=true "Get Purchased Items")

![Purchase Token](/docs/10.png?raw=true "Purchase Token")

* Validate the purchased item using [Validate Purchase API](https://developers.google.com/android-publisher/api-ref/rest/v3/purchases.products)

![Validate Purchase](/docs/11.png?raw=true "Validate Purchase")

![Validate Purchase Result](/docs/12.png?raw=true "Validate Purchase Result")

* Acknowledge the purchased item using [Acknowledge Purchase API](https://developers.google.com/android-publisher/api-ref/rest/v3/purchases.products/acknowledge)

![Acknowledge Purchase](/docs/13.png?raw=true "Acknowledge Purchase")

![Acknowledge Purchase Result](/docs/14.png?raw=true "Acknowledge Purchase Result")
  
* Consume the purchased item using [Consume Purchase API](https://developers.google.com/android-publisher/api-ref/rest/v3/purchases.products/consume)

![Consume Purchase](/docs/15.png?raw=true "Consume Purchase")

![Consume Purchase Result](/docs/16.png?raw=true "Consume Purchase Result")
  
 * Invoke get all purchased items using the [Get Purchase API](https://developers.google.com/android-publisher/api-ref/rest/v3/purchases.products/get) and you will see that the API now returns no item as it has been consumed

![Get Purchased Items Result](/docs/17.png?raw=true "Get Purchased Items Result")

Note that some of the API may return empty string upon success, this is intended and you can modify it in the [API Server](api-server)
  
# API Server

[api-server/](api-server) folder contains NodeJS project publishable as [Cloud Functions for Firebase](https://firebase.google.com/docs/functions) to [Firebase project](https://firebase.google.com).

## Setup

* Setup your own firebase project from your [Firebase Console](https://console.firebase.google.com/)
* Retrieve the credentials of your service account from your [Google Cloud project](https://cloud.google.com/iam/docs/service-accounts-create)
* Change `<FIREBASE_PROJECT_ID>` in [.firebaserc](/api-server/.firebaserc) and [firebase.json](/api-server/firebase.json) to your firebase-project-id
* Put in your service account credentials in [config.ts](/api-server/functions/src/config.ts)
* Change `<PLAY_PACKAGE_NAME>` in [manifest.json](/api-server/src/manifest.json) to your application package-id retrieved from your [Play Console](https://play.google.com/console)
* Run `npm ci` to download and install all the dependencies
* Run `npm run predeploy` to prepare the code for deployment
* Run `npm run deploy` to actually deploy the code

## Running
Your APIs will be available under the `<firebase-app>/api/*` of your Firebase project deployment URL. 