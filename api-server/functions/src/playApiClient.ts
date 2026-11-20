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

import * as functions from 'firebase-functions/v1';
import { google } from 'googleapis';
import * as billing from './billing';
import * as myconfig from './config';

// Strict input validation patterns to prevent URL path traversal and SSRF.
const PACKAGE_NAME_REGEX = /^[a-zA-Z][a-zA-Z0-9_]*(\.[a-zA-Z][a-zA-Z0-9_]*)+$/;
const PRODUCT_ID_REGEX = /^[a-zA-Z0-9._-]{1,150}$/;
// Tokens must be non-empty and must not contain URL path/query delimiter or traversal sequences.
const SAFE_TOKEN_REGEX = /^[^\s/?#&\\]{8,4096}$/;

export function isValidPackageName(pkg: string): boolean {
  return PACKAGE_NAME_REGEX.test(pkg) && !pkg.includes('..');
}

export function isValidProductId(productId: string): boolean {
  return PRODUCT_ID_REGEX.test(productId) && !productId.includes('..');
}

export function isValidToken(token: string): boolean {
  return SAFE_TOKEN_REGEX.test(token) && !token.includes('..');
}

/**
 * Resolves and validates the target package name. To prevent arbitrary cross-package
 * queries using the server's service account, if PLAY_PACKAGE_NAME is configured
 * (and not the sample placeholder), the requested package must match it.
 */
export function resolvePackageName(requestedPackage?: string): string {
  const defaultPkg = process.env.PLAY_PACKAGE_NAME ?? myconfig.packageName;
  const candidate = (requestedPackage || defaultPkg).trim();
  if (!isValidPackageName(candidate)) {
    throw new Error('Invalid packageName format.');
  }
  if (
    process.env.PLAY_PACKAGE_NAME &&
    candidate !== process.env.PLAY_PACKAGE_NAME.trim()
  ) {
    throw new Error(`Unauthorized packageName '${candidate}'.`);
  }
  return candidate;
}

let cachedJwtClient: InstanceType<typeof google.auth.JWT> | null = null;
let cachedPlayApi: ReturnType<typeof google.androidpublisher> | null = null;

function getPlayClients() {
  if (!cachedJwtClient || !cachedPlayApi) {
    const email = (process.env.SA_EMAIL ?? myconfig.serviceAccountEmail).trim();
    const rawKey = (
      process.env.SA_KEY ?? myconfig.serviceAccountPrivateKey
    ).replace(/\\n/g, '\n');
    cachedJwtClient = new google.auth.JWT(
      email,
      undefined,
      rawKey,
      myconfig.scopes,
    );
    cachedPlayApi = google.androidpublisher({
      version: 'v3',
      auth: cachedJwtClient,
    });
  }
  const activePkg = (
    process.env.PLAY_PACKAGE_NAME ?? myconfig.packageName
  ).trim();
  return {
    jwtClient: cachedJwtClient,
    playApi: cachedPlayApi,
    activePkg,
  };
}

/**
 * Acknowledges a one-time in-app purchase via parameterized androidpublisher v3 SDK.
 */
export async function acknowledgePurchase(
  productId: string,
  token: string,
  requestedPackageName?: string,
): Promise<any> {
  if (!isValidProductId(productId) || !isValidToken(token)) {
    return { result: false, error: 'Invalid productId or purchase token format.' };
  }
  try {
    const targetPkg = resolvePackageName(requestedPackageName);
    const { playApi } = getPlayClients();
    const res = await playApi.purchases.products.acknowledge({
      packageName: targetPkg,
      productId,
      token,
    });
    return res.data ?? { result: true };
  } catch (error) {
    functions.logger.error('Error calling acknowledgePurchase', error);
    return { result: false, error: error instanceof Error ? error.message : String(error) };
  }
}

/**
 * Consumes a repeatable in-app purchase via parameterized androidpublisher v3 SDK.
 */
export async function consumePurchase(
  productId: string,
  token: string,
  requestedPackageName?: string,
): Promise<any> {
  if (!isValidProductId(productId) || !isValidToken(token)) {
    return { result: false, error: 'Invalid productId or purchase token format.' };
  }
  try {
    const targetPkg = resolvePackageName(requestedPackageName);
    const { playApi } = getPlayClients();
    const res = await playApi.purchases.products.consume({
      packageName: targetPkg,
      productId,
      token,
    });
    return res.data ?? { result: true };
  } catch (error) {
    functions.logger.error('Error calling consumePurchase', error);
    return { result: false, error: error instanceof Error ? error.message : String(error) };
  }
}

/**
 * Verifies a one-time product purchase token via parameterized androidpublisher v3 SDK.
 */
export async function verifyPurchaseToken(
  productId: string,
  token: string,
  requestedPackageName?: string,
): Promise<any> {
  if (!isValidProductId(productId) || !isValidToken(token)) {
    return { result: false, error: 'Invalid productId or purchase token format.' };
  }
  try {
    const targetPkg = resolvePackageName(requestedPackageName);
    const { playApi } = getPlayClients();
    const res = await playApi.purchases.products.get({
      packageName: targetPkg,
      productId,
      token,
    });
    return res.data;
  } catch (error) {
    functions.logger.error('Error calling verifyPurchaseToken', error);
    return { result: false, error: error instanceof Error ? error.message : String(error) };
  }
}

/**
 * Fetches and wraps subscription purchase details for Real-Time Developer Notifications (RTDN).
 */
export async function fetchSubscriptionPurchase(
  subscriptionId: string,
  purchaseToken: string,
): Promise<billing.SubscriptionPurchase | null> {
  if (!isValidProductId(subscriptionId) || !isValidToken(purchaseToken)) {
    return null;
  }
  try {
    const { playApi, activePkg } = getPlayClients();
    const res = await playApi.purchases.subscriptions.get({
      packageName: activePkg,
      subscriptionId,
      token: purchaseToken,
    });
    return billing.SubscriptionPurchase.fromApiResponse(res.data, purchaseToken);
  } catch (error) {
    functions.logger.error('Error fetching subscription purchase info', error);
    return null;
  }
}

/**
 * Decodes and verifies a Google Play Games on PC Play Integrity token (`decodePcIntegrityToken`).
 */
export async function verifyIntegrityToken(
  token: string,
  requestedPackageName?: string,
): Promise<any> {
  if (!isValidToken(token)) {
    return { result: false, error: 'Invalid integrity token format.' };
  }

  let targetPackage: string;
  try {
    targetPackage = resolvePackageName(requestedPackageName);
  } catch (err) {
    return { result: false, error: err instanceof Error ? err.message : 'Invalid packageName.' };
  }

  try {
    const { jwtClient } = getPlayClients();
    const endpoint = new URL(
      `https://playintegrity.googleapis.com/v1/${encodeURIComponent(targetPackage)}:decodePcIntegrityToken`,
    );
    const result = await jwtClient.request({
      url: endpoint.toString(),
      method: 'POST',
      data: {
        integrity_token: token,
      },
    });

    const payload = result.data as any;
    const tokenPayload = payload?.tokenPayloadExternal;

    if (!tokenPayload || !tokenPayload.requestDetails) {
      return { result: false, error: 'Invalid response from Play Integrity API', payload };
    }

    if (tokenPayload.requestDetails.requestPackageName !== targetPackage) {
      return { result: false, error: 'Package name mismatch', payload };
    }

    const verdicts: string[] | undefined = tokenPayload.deviceIntegrity?.deviceRecognitionVerdict;
    if (Array.isArray(verdicts) && verdicts.includes('MEETS_PC_INTEGRITY')) {
      return {
        result: true,
        hash: tokenPayload.requestDetails.requestHash || '',
        payload,
      };
    }

    return { result: false, error: 'Device does not meet PC integrity', payload };
  } catch (error) {
    functions.logger.error('Error calling verifyIntegrityToken', error);
    return { result: false, error: error instanceof Error ? error.message : String(error) };
  }
}

/**
 * Legacy REST helper (used by api-client migration reference): lists user purchases.
 */
export async function listPurchases(userAccessToken: string): Promise<any> {
  if (!isValidToken(userAccessToken)) {
    return { result: false, error: 'Invalid userAccessToken format.' };
  }
  try {
    const { jwtClient, activePkg } = getPlayClients();
    const endpoint = new URL(
      `https://www.googleapis.com/androidpublisher/v3/applications/${encodeURIComponent(activePkg)}/purchases/products`,
    );
    endpoint.searchParams.set('userAccessToken', userAccessToken);

    const result = await jwtClient.request({
      url: endpoint.toString(),
      method: 'GET',
    });
    return result.data;
  } catch (error) {
    functions.logger.error('Error calling listPurchases', error);
    return { result: false, error: error instanceof Error ? error.message : String(error) };
  }
}

/**
 * Legacy REST helper (used by api-client migration reference): fetches SKU details & web checkout link.
 */
export async function getSkuDetails(
  userAccessToken: string,
  sku: string,
  skuType: string,
  userIp: string,
  languageCode: string,
): Promise<any> {
  if (!isValidToken(userAccessToken) || !isValidProductId(sku)) {
    return { result: false, error: 'Invalid userAccessToken or sku format.' };
  }
  const normalizedType = skuType === 'subs' ? 'subs' : 'inapp';
  const normalizedLang = /^[a-zA-Z]{2,3}(-[a-zA-Z0-9]{2,8})?$/.test(languageCode)
    ? languageCode
    : 'en-US';
  const normalizedIp = /^[0-9a-fA-F.:]{3,45}$/.test(userIp) ? userIp : '';

  try {
    const { jwtClient, activePkg } = getPlayClients();
    const endpoint = new URL(
      `https://www.googleapis.com/androidpublisher/v3/applications/${encodeURIComponent(activePkg)}/skus`,
    );
    endpoint.searchParams.set('sku', sku);
    endpoint.searchParams.set('skuType', normalizedType);
    if (normalizedIp) {
      endpoint.searchParams.set('userIpAddress', normalizedIp);
    }
    endpoint.searchParams.set('languageCode', normalizedLang);
    endpoint.searchParams.set('userAccessToken', userAccessToken);

    const result = await jwtClient.request({
      url: endpoint.toString(),
      method: 'GET',
    });
    return result.data;
  } catch (error) {
    functions.logger.error('Error calling getSkuDetails', error);
    return { result: false, error: error instanceof Error ? error.message : String(error) };
  }
}
