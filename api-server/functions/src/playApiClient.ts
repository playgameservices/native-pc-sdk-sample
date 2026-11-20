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

// Strict input validation patterns to prevent URL path traversal and injection.
const PACKAGE_NAME_REGEX = /^[a-zA-Z][a-zA-Z0-9_]*(\.[a-zA-Z][a-zA-Z0-9_]*)+$/;
const PRODUCT_ID_REGEX = /^[a-zA-Z0-9._-]{1,150}$/;
// Positive character allowlist for Play purchase tokens, Play Integrity tokens,
// and Google OAuth access tokens (excludes '/', '?', '#', '&', '\\', ',', '%', ';', whitespace, and control chars).
const SAFE_TOKEN_REGEX = /^[A-Za-z0-9._~:+=-]{8,4096}$/;

export interface ApiCallResult<T = Record<string, any>> {
  status: number;
  body: T;
}

export class PackageAuthorizationError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'PackageAuthorizationError';
  }
}

export class InvalidPackageNameError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'InvalidPackageNameError';
  }
}

export function isValidPackageName(pkg: string): boolean {
  return typeof pkg === 'string' && PACKAGE_NAME_REGEX.test(pkg) && !pkg.includes('..');
}

export function isValidProductId(productId: string): boolean {
  return typeof productId === 'string' && PRODUCT_ID_REGEX.test(productId) && !productId.includes('..');
}

export function isValidToken(token: string): boolean {
  return typeof token === 'string' && SAFE_TOKEN_REGEX.test(token) && !token.includes('..');
}

/**
 * Strips HTTP request/response config objects (including Authorization Bearer headers)
 * from Gaxios / googleapis errors before writing to Cloud Logging.
 */
export function sanitizeErrorForLog(error: unknown): Record<string, unknown> {
  if (error instanceof Error) {
    const errObj = error as Error & {
      code?: unknown;
      status?: unknown;
      response?: { status?: unknown; statusText?: unknown };
    };
    const sanitized: Record<string, unknown> = {
      name: errObj.name,
      message: errObj.message,
    };
    if (errObj.code !== undefined) {
      sanitized.code = errObj.code;
    }
    const httpStatus = errObj.status ?? errObj.response?.status;
    if (httpStatus !== undefined) {
      sanitized.status = httpStatus;
    }
    if (errObj.response?.statusText !== undefined) {
      sanitized.statusText = errObj.response.statusText;
    }
    return sanitized;
  }
  return { message: String(error) };
}

/**
 * Returns the server-configured Play Console package name.
 */
export function getConfiguredPackageName(): string {
  return (process.env.PLAY_PACKAGE_NAME ?? myconfig.packageName).trim();
}

/**
 * Resolves and validates the target package name. Always enforces single-package
 * isolation against the server-configured package name (whether supplied via
 * PLAY_PACKAGE_NAME or config.ts) so callers cannot query other apps with the
 * backend Service Account.
 */
export function resolvePackageName(requestedPackage?: string): string {
  const configuredPkg = getConfiguredPackageName();
  if (!isValidPackageName(configuredPkg)) {
    throw new InvalidPackageNameError('Server packageName is not configured with a valid format.');
  }
  const candidate = (requestedPackage ? requestedPackage.trim() : '') || configuredPkg;
  if (!isValidPackageName(candidate)) {
    throw new InvalidPackageNameError('Invalid packageName format.');
  }
  if (candidate !== configuredPkg) {
    throw new PackageAuthorizationError(`Unauthorized packageName '${candidate}'.`);
  }
  return configuredPkg;
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
  const activePkg = getConfiguredPackageName();
  return {
    jwtClient: cachedJwtClient,
    playApi: cachedPlayApi,
    activePkg,
  };
}

function mapErrorToResult(context: string, error: unknown): ApiCallResult {
  if (error instanceof InvalidPackageNameError) {
    return { status: 400, body: { result: false, error: error.message } };
  }
  if (error instanceof PackageAuthorizationError) {
    return { status: 403, body: { result: false, error: error.message } };
  }
  functions.logger.error(context, sanitizeErrorForLog(error));
  return {
    status: 502,
    body: {
      result: false,
      error: error instanceof Error ? error.message : String(error),
    },
  };
}

/**
 * Acknowledges a one-time in-app purchase via parameterized androidpublisher v3 SDK.
 */
export async function acknowledgePurchase(
  productId: string,
  token: string,
  requestedPackageName?: string,
): Promise<ApiCallResult> {
  if (!isValidProductId(productId) || !isValidToken(token)) {
    return {
      status: 400,
      body: { result: false, error: 'Invalid productId or purchase token format.' },
    };
  }
  try {
    const targetPkg = resolvePackageName(requestedPackageName);
    const { playApi } = getPlayClients();
    const res = await playApi.purchases.products.acknowledge({
      packageName: targetPkg,
      productId,
      token,
    });
    return { status: 200, body: res.data ?? { result: true } };
  } catch (error) {
    return mapErrorToResult('Error calling acknowledgePurchase', error);
  }
}

/**
 * Consumes a repeatable in-app purchase via parameterized androidpublisher v3 SDK.
 */
export async function consumePurchase(
  productId: string,
  token: string,
  requestedPackageName?: string,
): Promise<ApiCallResult> {
  if (!isValidProductId(productId) || !isValidToken(token)) {
    return {
      status: 400,
      body: { result: false, error: 'Invalid productId or purchase token format.' },
    };
  }
  try {
    const targetPkg = resolvePackageName(requestedPackageName);
    const { playApi } = getPlayClients();
    const res = await playApi.purchases.products.consume({
      packageName: targetPkg,
      productId,
      token,
    });
    return { status: 200, body: res.data ?? { result: true } };
  } catch (error) {
    return mapErrorToResult('Error calling consumePurchase', error);
  }
}

/**
 * Verifies a one-time product purchase token via parameterized androidpublisher v3 SDK.
 */
export async function verifyPurchaseToken(
  productId: string,
  token: string,
  requestedPackageName?: string,
): Promise<ApiCallResult> {
  if (!isValidProductId(productId) || !isValidToken(token)) {
    return {
      status: 400,
      body: { result: false, error: 'Invalid productId or purchase token format.' },
    };
  }
  try {
    const targetPkg = resolvePackageName(requestedPackageName);
    const { playApi } = getPlayClients();
    const res = await playApi.purchases.products.get({
      packageName: targetPkg,
      productId,
      token,
    });
    return { status: 200, body: res.data };
  } catch (error) {
    return mapErrorToResult('Error calling verifyPurchaseToken', error);
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
    functions.logger.error('Error fetching subscription purchase info', sanitizeErrorForLog(error));
    return null;
  }
}

/**
 * Decodes and verifies a Google Play Games on PC Play Integrity token (`decodePcIntegrityToken`).
 */
export async function verifyIntegrityToken(
  token: string,
  requestedPackageName?: string,
): Promise<ApiCallResult> {
  if (!isValidToken(token)) {
    return {
      status: 400,
      body: { result: false, error: 'Invalid integrity token format.' },
    };
  }

  let targetPackage: string;
  try {
    targetPackage = resolvePackageName(requestedPackageName);
  } catch (err) {
    return mapErrorToResult('Invalid packageName in verifyIntegrityToken', err);
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
      return {
        status: 502,
        body: { result: false, error: 'Invalid response from Play Integrity API', payload },
      };
    }

    if (tokenPayload.requestDetails.requestPackageName !== targetPackage) {
      return {
        status: 403,
        body: { result: false, error: 'Package name mismatch', payload },
      };
    }

    const verdicts: string[] | undefined = tokenPayload.deviceIntegrity?.deviceRecognitionVerdict;
    if (Array.isArray(verdicts) && verdicts.includes('MEETS_PC_INTEGRITY')) {
      return {
        status: 200,
        body: {
          result: true,
          hash: tokenPayload.requestDetails.requestHash || '',
          payload,
        },
      };
    }

    return {
      status: 200,
      body: { result: false, error: 'Device does not meet PC integrity', payload },
    };
  } catch (error) {
    return mapErrorToResult('Error calling verifyIntegrityToken', error);
  }
}

/**
 * Legacy REST helper (used by api-client migration reference): lists user purchases.
 */
export async function listPurchases(userAccessToken: string): Promise<ApiCallResult> {
  if (!isValidToken(userAccessToken)) {
    return {
      status: 400,
      body: { result: false, error: 'Invalid userAccessToken format.' },
    };
  }
  try {
    const targetPkg = resolvePackageName();
    const { jwtClient } = getPlayClients();
    const endpoint = new URL(
      `https://www.googleapis.com/androidpublisher/v3/applications/${encodeURIComponent(targetPkg)}/purchases/products`,
    );
    endpoint.searchParams.set('userAccessToken', userAccessToken);

    const result = await jwtClient.request({
      url: endpoint.toString(),
      method: 'GET',
    });
    return { status: 200, body: { purchasesresult: result.data } };
  } catch (error) {
    return mapErrorToResult('Error calling listPurchases', error);
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
): Promise<ApiCallResult> {
  if (!isValidToken(userAccessToken) || !isValidProductId(sku)) {
    return {
      status: 400,
      body: { result: false, error: 'Invalid userAccessToken or sku format.' },
    };
  }
  const normalizedType = skuType === 'subs' ? 'subs' : 'inapp';
  const normalizedLang = /^[a-zA-Z]{2,3}(-[a-zA-Z0-9]{2,8})?$/.test(languageCode)
    ? languageCode
    : 'en-US';
  const normalizedIp = /^[0-9a-fA-F.:]{3,45}$/.test(userIp) ? userIp : '';

  try {
    const targetPkg = resolvePackageName();
    const { jwtClient } = getPlayClients();
    const endpoint = new URL(
      `https://www.googleapis.com/androidpublisher/v3/applications/${encodeURIComponent(targetPkg)}/skus`,
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
    return { status: 200, body: { result: result.data } };
  } catch (error) {
    return mapErrorToResult('Error calling getSkuDetails', error);
  }
}
