# Google Play for Native PC Samples Repository

This repository contains sample projects demonstrating various aspects of integrating Google Play APIs for your native game. A description of the high-level directories in this repo follows:

## API Client (Legacy)

For new integrations, refer to [sdk-client](#sdk-client) section.

Do not use this implementation anymore as this implementation method will be replaced by [sdk-client](#sdk-client).

*  **[api-client/](api-client)** (C/C++) - This sample demonstrate integration and use of the [Google Play Developer APIs](https://developers.google.com/android-publisher/). The folder contains C++ project compilable using any recent version of [Microsoft Visual Studio](https://visualstudio.microsoft.com/).

## API Server
See the [api-server](api-server) section for description and setup of the backend needed by the [api-client](api-client) and [sdk-client](sdk-client) samples.
*  **[api-server/](api-server)** (NodeJS/JavaScript) - This sample contain the backend code for the [api-client](api-client) runnable in [Cloud Functions for Firebase](https://firebase.google.com/docs/functions). The folder contains NodeJS project publishable as [Cloud Functions for Firebase](https://firebase.google.com/docs/functions) to [Firebase project](https://firebase.google.com).

### Setup

* Setup your own firebase project from your [Firebase Console](https://console.firebase.google.com/)
* Retrieve the credentials of your service account from your [Google Cloud project](https://cloud.google.com/iam/docs/service-accounts-create)
* Change `<FIREBASE_PROJECT_ID>` in [.firebaserc](/api-server/.firebaserc) and [firebase.json](/api-server/firebase.json) to your firebase-project-id
* Put in your service account credentials in [config.ts](/api-server/functions/src/config.ts)
* Change `<PLAY_PACKAGE_NAME>` in [manifest.json](/api-server/src/manifest.json) to your application package-id retrieved from your [Play Console](https://play.google.com/console)
* Run `npm ci` to download and install all the dependencies
* Run `npm run predeploy` to prepare the code for deployment
* Run `npm run deploy` to actually deploy the code

### Running
Your APIs will be available under the `<firebase-app>/api/*` of your Firebase project deployment URL. 

## SDK Client

See the [sdk-client](sdk-client) section for description and setup of SDK Client sample.
*  **[sdk-client/](sdk-client)** (C/C++) - This sample demonstrate integration and use of the [Google Play Games for PC Native SDK](https://developer.android.com/games/playgames/native-pc/). The folder contains C++ project compilable using any recent version of [Microsoft Visual Studio](https://visualstudio.microsoft.com/).