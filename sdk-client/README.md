# Teapot Sample

This project demonstrates the use of [Google Play Games for PC Native SDK](https://developer.android.com/games/playgames/native-pc/) to [seamlessly sign-in](https://developer.android.com/games/playgames/native-pc/pgs) and [sell digital content](https://developer.android.com/games/playgames/native-pc/billing).

## Windows Setup

In order to build run this sample on Windows you will need to fulfil 3P dependencies.

One way to get these is to use [vcpkg](https://vcpkg.io/en/getting-started.html):

```
vcpkg install
```

You will also need the [play_pc_sdk](https://developer.android.com/games/playgames/native-pc/downloads/cpp) in the solution folder.

```
unzip play_pc_sdk.zip in the solution folder
```

Of course, install [Google Play Games for PC client application](https://play.google.com/googleplaygames).

## Play Console Setup

You will need to setup billing from your [Play Console Application](https://developer.android.com/distribute/console) and [Create in-app products](https://developer.android.com/guide/playpoints/create-products) for purchase in this sample.

## Building the application

* Open [Teapot.sln](Teapot.sln) file using any recent version of Microsoft Visual Studio.

* Open [AppConfig.json](GameApplication/AppConfig.json) and replace the same _client_id_ and _client_secret_ with your Google API client-id and secret.

* Press F5 to build and run your application.

## Running the application

* Once the application is running correctly, you will be greeted by an OpenGL [scene](/docs/screenshots/sdk-client/00.png) with a teapot and "Billing SDK" button.

![Teapot Scene](/docs/screenshots/sdk-client/00.png?raw=true "Teapot Scene")

* Click on the "Billing SDK" button and a [debug panel](/docs/screenshots/sdk-client/01.png) will pop up. Using this panel, you will be able to see a list of the operations you can perform, the [SDK interfaces](https://developer.android.com/games/playgames/native-pc/reference) and parameters that will be sent to the server as well as the result once you execute the SDK function.

![Debug Panel](/docs/screenshots/sdk-client/01.png?raw=true "Debug Panel")

* In the *Select the operation* menu, click on the first menu item "Initialize Billing SDK". The parameters will automatically change to reflect the parameters that will be passed when you click on the *Run* button.

* Click on the *Run* button and upon the initialization function call return, the result will be updated in the *Result* area. Once the initialization is completed successfully, you will be able to call the rest of the functions in the SDK.

* There are steps that you need to adhere to such as checking for purchases completed outside of your app whenever your application re-enters background. For the most updated list of requirements, checkout the [documentation](https://developer.android.com/games/playgames/native-pc/billing).

![Initialization and Result](/docs/screenshots/sdk-client/02.png?raw=true "Initialization and Result")

* To get the list of products users can purchase and the localized information that can be used to be displayed to the user, you can issue call to [QueryProductDetails](https://developer.android.com/games/playgames/native-pc/reference/class/google/play/billing/billing-client#queryproductdetails).

![Query Product Details](/docs/screenshots/sdk-client/03.png?raw=true "Query Product Details")

* Once the user select a product and decide to purchase, you need to call [LaunchPurchaseFlow](https://developer.android.com/games/playgames/native-pc/reference/class/google/play/billing/billing-client#launchpurchaseflow) to start the purchase flow process.

![Launch Purchase Flow](/docs/screenshots/sdk-client/04.png?raw=true "Launch Purchase Flow")

* The user may be directed to external browser application to complete the purchase.

![User Default Browser Application](/docs/screenshots/sdk-client/05.png?raw=true "User Default Browser Application")

* The entire process will be completed safely, including adding/choosing payment method, fraud prevention, etc. Once the purchase flow is completed, your application can check the [LaunchPurchaseFlowResult](https://developer.android.com/games/playgames/native-pc/reference/class/google/play/result) and retrieve [LaunchPurchaseFlowResultValue](https://developer.android.com/games/playgames/native-pc/reference/struct/google/play/billing/launch-purchase-flow-result-value#structgoogle_1_1play_1_1billing_1_1_launch_purchase_flow_result_value).

![Purchase Completed](/docs/screenshots/sdk-client/06.png?raw=true "Purchase Completed")

* Acknowledge the purchased item by calling [AcknowledgePurchase](https://developer.android.com/games/playgames/native-pc/reference/class/google/play/billing/billing-client#acknowledgepurchase).

![Acknowledge Purchase](/docs/screenshots/sdk-client/07.png?raw=true "Acknowledge Purchase")

* Consume the purchased item by calling [ConsumePurchase](https://developer.android.com/games/playgames/native-pc/reference/class/google/play/billing/billing-client#consumepurchase) if you intend to let the user purchase the same product again.

![Consume Purchase](/docs/screenshots/sdk-client/08.png?raw=true "Consume Purchase")
