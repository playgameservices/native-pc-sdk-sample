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

import { allowedOrigins, topicID } from './config';
import * as rtdn from './notifications';
import * as playApiClient from './playApiClient';

if (!admin.apps.length) {
  admin.initializeApp();
}

export interface RequestWithUser extends express.Request {
  user?: admin.auth.DecodedIdToken;
}

export type IdTokenVerifier = (token: string) => Promise<admin.auth.DecodedIdToken>;

export interface ApiAppDependencies {
  corsOrigin?: boolean | string[];
  verifyIdToken?: IdTokenVerifier;
  client?: Pick<
    typeof playApiClient,
    | 'acknowledgePurchase'
    | 'consumePurchase'
    | 'verifyPurchaseToken'
    | 'verifyIntegrityToken'
    | 'listPurchases'
    | 'getSkuDetails'
  >;
}

/**
 * Fail-closed Firebase Auth middleware: requires a valid `Authorization: Bearer <Firebase ID token>`
 * header on every /api/* endpoint.
 */
export function createVerifyAuthMiddleware(
  verifyIdToken: IdTokenVerifier = (token) => admin.auth().verifyIdToken(token),
): express.RequestHandler {
  return async (
    req: RequestWithUser,
    res: express.Response,
    next: express.NextFunction,
  ): Promise<void> => {
    const authHeader = req.headers?.authorization;
    if (typeof authHeader !== 'string' || !authHeader.startsWith('Bearer ')) {
      res.status(401).json({
        result: false,
        error: 'Unauthorized: missing or malformed Authorization Bearer token.',
      });
      return;
    }

    const bearerToken = authHeader.slice('Bearer '.length).trim();
    if (!bearerToken || /\s/.test(bearerToken)) {
      res.status(401).json({
        result: false,
        error: 'Unauthorized: missing or malformed Authorization Bearer token.',
      });
      return;
    }

    try {
      req.user = await verifyIdToken(bearerToken);
      next();
    } catch (error) {
      functions.logger.warn(
        'Rejected unauthenticated API request',
        playApiClient.sanitizeErrorForLog(error),
      );
      res.status(401).json({
        result: false,
        error: 'Unauthorized: invalid or expired Firebase ID token.',
      });
    }
  };
}

function readRequiredString(body: unknown, key: string): string | null {
  if (!body || typeof body !== 'object' || Array.isArray(body)) {
    return null;
  }
  const value = (body as Record<string, unknown>)[key];
  if (typeof value !== 'string') {
    return null;
  }
  const trimmed = value.trim();
  return trimmed.length > 0 ? trimmed : null;
}

function readOptionalString(
  body: unknown,
  keys: string[],
  defaultValue = '',
): string | null {
  if (!body || typeof body !== 'object' || Array.isArray(body)) {
    return defaultValue;
  }
  const record = body as Record<string, unknown>;
  for (const key of keys) {
    if (Object.prototype.hasOwnProperty.call(record, key)) {
      const value = record[key];
      if (value === undefined || value === null) {
        continue;
      }
      if (typeof value !== 'string') {
        return null;
      }
      return value.trim();
    }
  }
  return defaultValue;
}

export function createApiApp(deps: ApiAppDependencies = {}): express.Express {
  const corsOrigin = deps.corsOrigin !== undefined ? deps.corsOrigin : allowedOrigins;
  const verifyIdToken =
    deps.verifyIdToken ?? ((token: string) => admin.auth().verifyIdToken(token));
  const client = deps.client ?? playApiClient;

  const apiRouter = express.Router();
  apiRouter.use(cors({ origin: corsOrigin }));
  apiRouter.use(express.json({ limit: '64kb' }));
  apiRouter.use(createVerifyAuthMiddleware(verifyIdToken));

  /**
   * POST /api/acknowledgePurchase
   * Acknowledges a non-consumable in-app purchase token using the server's Service Account.
   */
  apiRouter.post('/acknowledgePurchase', async (req: RequestWithUser, res: express.Response) => {
    const productId = readRequiredString(req.body, 'productId');
    const token = readRequiredString(req.body, 'token');
    const requestedPkg = readOptionalString(req.body, ['packageName']);
    if (!productId || !token || requestedPkg === null) {
      res.status(400).json({
        result: false,
        error: 'Request body must include string fields productId and token.',
      });
      return;
    }
    const outcome = await client.acknowledgePurchase(productId, token, requestedPkg);
    res.status(outcome.status).json(outcome.body);
  });

  /**
   * POST /api/consumePurchase
   * Consumes a repeatable in-app purchase token using the server's Service Account.
   */
  apiRouter.post('/consumePurchase', async (req: RequestWithUser, res: express.Response) => {
    const productId = readRequiredString(req.body, 'productId');
    const token = readRequiredString(req.body, 'token');
    const requestedPkg = readOptionalString(req.body, ['packageName']);
    if (!productId || !token || requestedPkg === null) {
      res.status(400).json({
        result: false,
        error: 'Request body must include string fields productId and token.',
      });
      return;
    }
    const outcome = await client.consumePurchase(productId, token, requestedPkg);
    res.status(outcome.status).json(outcome.body);
  });

  /**
   * POST /api/verifyPurchaseToken
   * Validates a purchase token and returns the authoritative ProductPurchase state from Google Play.
   */
  apiRouter.post('/verifyPurchaseToken', async (req: RequestWithUser, res: express.Response) => {
    const productId = readRequiredString(req.body, 'productId');
    const token = readRequiredString(req.body, 'token');
    const requestedPkg = readOptionalString(req.body, ['packageName']);
    if (!productId || !token || requestedPkg === null) {
      res.status(400).json({
        result: false,
        error: 'Request body must include string fields productId and token.',
      });
      return;
    }
    const outcome = await client.verifyPurchaseToken(productId, token, requestedPkg);
    res.status(outcome.status).json(outcome.body);
  });

  /**
   * POST /api/verifyIntegrityToken
   * Decodes and verifies a PC Play Integrity token (`decodePcIntegrityToken`) and returns
   * the device verdict, licensing verdict, and matched requestHash.
   */
  apiRouter.post('/verifyIntegrityToken', async (req: RequestWithUser, res: express.Response) => {
    const token = readRequiredString(req.body, 'token');
    const requestedPkg = readOptionalString(req.body, ['packageName']);
    if (!token || requestedPkg === null) {
      res.status(400).json({
        result: false,
        error: 'Request body must include string field token.',
      });
      return;
    }
    const outcome = await client.verifyIntegrityToken(token, requestedPkg);
    res.status(outcome.status).json(outcome.body);
  });

  /**
   * POST /api/getPurchases
   * Legacy REST endpoint used by api-client migration reference (POST JSON body to avoid tokens in URLs).
   */
  apiRouter.post('/getPurchases', async (req: RequestWithUser, res: express.Response) => {
    const userAccessToken = readRequiredString(req.body, 'userAccessToken');
    if (!userAccessToken) {
      res.status(400).json({
        result: false,
        error: 'Request body must include string field userAccessToken.',
      });
      return;
    }
    const outcome = await client.listPurchases(userAccessToken);
    res.status(outcome.status).json(outcome.body);
  });

  /**
   * POST /api/getSkuDetails
   * Legacy REST endpoint used by api-client migration reference (POST JSON body to avoid tokens in URLs).
   */
  apiRouter.post('/getSkuDetails', async (req: RequestWithUser, res: express.Response) => {
    const userAccessToken = readRequiredString(req.body, 'userAccessToken');
    const sku = readRequiredString(req.body, 'sku');
    const skuType = readOptionalString(req.body, ['skuType', 'skutype'], 'inapp');
    const userIpAddress = readOptionalString(req.body, ['userIpAddress', 'useripaddress'], '');
    const languageCode = readOptionalString(req.body, ['languageCode', 'languagecode'], 'en-US');

    if (
      !userAccessToken ||
      !sku ||
      skuType === null ||
      userIpAddress === null ||
      languageCode === null
    ) {
      res.status(400).json({
        result: false,
        error: 'Request body must include valid string fields userAccessToken and sku.',
      });
      return;
    }

    const outcome = await client.getSkuDetails(
      userAccessToken,
      sku,
      skuType,
      userIpAddress,
      languageCode,
    );
    res.status(outcome.status).json(outcome.body);
  });

  /**
   * POST /api/postSkuDetails
   * Legacy REST helper endpoint preserved for backwards compatibility.
   */
  apiRouter.post('/postSkuDetails', async (req: RequestWithUser, res: express.Response) => {
    const userAccessToken = readRequiredString(req.body, 'userAccessToken');
    const sku = readRequiredString(req.body, 'sku');
    if (!userAccessToken || !sku) {
      res.status(400).json({
        result: false,
        error: 'Request body must include string fields userAccessToken and sku.',
      });
      return;
    }
    const outcome = await client.getSkuDetails(userAccessToken, sku, 'inapp', '', 'en-US');
    res.status(outcome.status).json(outcome.body);
  });

  const mainApp = express();
  mainApp.disable('x-powered-by');
  mainApp.use('/api', apiRouter);
  return mainApp;
}

export const main = functions.https.onRequest(createApiApp());

/**
 * Pub/Sub Real-Time Developer Notification (RTDN) listener.
 */
export const rtdnListener = functions.pubsub.topic(topicID).onPublish(async (message) => {
  try {
    const notification = message.json as rtdn.DeveloperNotification;
    const configuredPkg = playApiClient.getConfiguredPackageName();
    if (!notification?.packageName || notification.packageName !== configuredPkg) {
      functions.logger.warn('Ignoring RTDN event for unexpected packageName', {
        receivedPackageName: notification?.packageName ?? '',
        expectedPackageName: configuredPkg,
      });
      return;
    }

    const subInfo = notification.subscriptionNotification;
    if (!subInfo?.subscriptionId || !subInfo?.purchaseToken) {
      functions.logger.info('Received non-subscription or test RTDN event', {
        version: notification.version,
        packageName: notification.packageName,
        eventTimeMillis: notification.eventTimeMillis,
        hasTestNotification: Boolean(notification.testNotification),
      });
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
    functions.logger.error(
      'Error processing RTDN Pub/Sub message',
      playApiClient.sanitizeErrorForLog(error),
    );
  }
});
