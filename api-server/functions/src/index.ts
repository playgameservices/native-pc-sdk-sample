/*
 *  Copyright 2021 Google LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

import * as admin from 'firebase-admin';
import * as cors from 'cors';
import * as express from 'express';
import * as functions from 'firebase-functions/v1';

import { allowedOrigins, packageName, topicID } from './config';
import * as rtdn from './notifications';
import * as playApiClient from './playApiClient';

admin.initializeApp();

const app = express();
app.use(cors({ origin: allowedOrigins }));
app.use(express.json());

/**
 * POST /api/acknowledgePurchase
 * Acknowledges a non-consumable in-app purchase token using the server's Service Account.
 */
app.post('/acknowledgePurchase', async (req: express.Request, res: express.Response) => {
  const productId = String(req.body?.productId ?? req.query?.productId ?? '');
  const token = String(req.body?.token ?? req.query?.token ?? '');
  const requestedPkg = String(req.body?.packageName ?? req.query?.packageName ?? '');
  const result = await playApiClient.acknowledgePurchase(productId, token, requestedPkg);
  res.json(result);
});

/**
 * POST /api/consumePurchase
 * Consumes a repeatable in-app purchase token using the server's Service Account.
 */
app.post('/consumePurchase', async (req: express.Request, res: express.Response) => {
  const productId = String(req.body?.productId ?? req.query?.productId ?? '');
  const token = String(req.body?.token ?? req.query?.token ?? '');
  const requestedPkg = String(req.body?.packageName ?? req.query?.packageName ?? '');
  const result = await playApiClient.consumePurchase(productId, token, requestedPkg);
  res.json(result);
});

/**
 * GET|POST /api/verifyPurchaseToken
 * Validates a purchase token and returns the authoritative ProductPurchase state from Google Play.
 */
async function handleVerifyPurchaseToken(req: express.Request, res: express.Response) {
  const productId = String(req.query?.productId ?? req.body?.productId ?? '');
  const token = String(req.query?.token ?? req.body?.token ?? '');
  const requestedPkg = String(req.query?.packageName ?? req.body?.packageName ?? '');
  const result = await playApiClient.verifyPurchaseToken(productId, token, requestedPkg);
  res.json(result);
}
app.get('/verifyPurchaseToken', handleVerifyPurchaseToken);
app.post('/verifyPurchaseToken', handleVerifyPurchaseToken);

/**
 * POST /api/verifyIntegrityToken
 * Decodes and verifies a PC Play Integrity token (`decodePcIntegrityToken`) and returns
 * the device verdict, licensing verdict, and matched requestHash.
 */
app.post('/verifyIntegrityToken', async (req: express.Request, res: express.Response) => {
  const token = String(req.body?.token ?? req.query?.token ?? '');
  const requestedPkg = String(req.body?.packageName ?? req.query?.packageName ?? packageName);
  const result = await playApiClient.verifyIntegrityToken(token, requestedPkg);
  res.json(result);
});

/**
 * GET /api/getPurchases
 * Legacy REST endpoint used by api-client migration reference.
 */
app.get('/getPurchases', async (req: express.Request, res: express.Response) => {
  const userAccessToken = String(req.query?.userAccessToken ?? '');
  const result = await playApiClient.listPurchases(userAccessToken);
  res.json({ purchasesresult: result });
});

/**
 * GET /api/getSkuDetails
 * Legacy REST endpoint used by api-client migration reference.
 */
app.get('/getSkuDetails', async (req: express.Request, res: express.Response) => {
  const userAccessToken = String(req.query?.userAccessToken ?? '');
  const sku = String(req.query?.sku ?? '');
  const skutype = String(req.query?.skutype ?? req.query?.skuType ?? 'inapp');
  const useripaddress = String(req.query?.useripaddress ?? req.query?.userIpAddress ?? '');
  const languagecode = String(req.query?.languagecode ?? req.query?.languageCode ?? 'en-US');

  const result = await playApiClient.getSkuDetails(
    userAccessToken,
    sku,
    skutype,
    useripaddress,
    languagecode,
  );
  res.json({ result });
});

/**
 * POST /api/postSkuDetails
 * Legacy REST helper endpoint preserved for backwards compatibility.
 */
app.post('/postSkuDetails', async (req: express.Request, res: express.Response) => {
  const userAccessToken = String(req.body?.userAccessToken ?? '');
  const sku = String(req.body?.sku ?? '');
  const result = await playApiClient.getSkuDetails(userAccessToken, sku, 'inapp', '', 'en-US');
  res.json({ result });
});

const mainApp = express();
mainApp.use(cors({ origin: allowedOrigins }));
mainApp.use('/api', app);

export const main = functions.https.onRequest(mainApp);

/**
 * Pub/Sub Real-Time Developer Notification (RTDN) listener.
 */
export const rtdnListener = functions.pubsub.topic(topicID).onPublish(async (message) => {
  try {
    const notification = message.json as rtdn.DeveloperNotification;
    const subInfo = notification?.subscriptionNotification;
    if (!subInfo?.subscriptionId || !subInfo?.purchaseToken) {
      functions.logger.info('Received non-subscription or test RTDN event', notification);
      return;
    }

    const subPurchase = await playApiClient.fetchSubscriptionPurchase(
      subInfo.subscriptionId,
      subInfo.purchaseToken,
    );
    functions.logger.info('Verified RTDN subscription state', {
      subscriptionId: subInfo.subscriptionId,
      notificationType: rtdn.NotificationType[subInfo.notificationType],
      entitlementActive: subPurchase?.isEntitlementActive() ?? false,
    });
  } catch (error) {
    functions.logger.error('Error processing RTDN Pub/Sub message', error);
  }
});
