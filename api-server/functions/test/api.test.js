'use strict';

const assert = require('node:assert/strict');
const http = require('node:http');
const { test } = require('node:test');

const { parseAllowedOrigins } = require('../lib/config');
const { createApiApp } = require('../lib/index');
const {
  PackageAuthorizationError,
  isValidProductId,
  isValidToken,
  resolvePackageName,
  sanitizeErrorForLog,
} = require('../lib/playApiClient');

function httpRequest(server, { method, path, headers = {}, body }) {
  return new Promise((resolve, reject) => {
    const addr = server.address();
    const payload = body === undefined ? undefined : JSON.stringify(body);
    const reqHeaders = { ...headers };
    if (payload !== undefined) {
      reqHeaders['Content-Type'] = 'application/json';
      reqHeaders['Content-Length'] = Buffer.byteLength(payload);
    }
    const req = http.request(
      {
        hostname: '127.0.0.1',
        port: addr.port,
        method,
        path,
        headers: reqHeaders,
      },
      (res) => {
        let raw = '';
        res.setEncoding('utf8');
        res.on('data', (chunk) => {
          raw += chunk;
        });
        res.on('end', () => {
          let json = null;
          try {
            json = JSON.parse(raw);
          } catch {
            // Non-JSON response
          }
          resolve({ status: res.statusCode, headers: res.headers, raw, json });
        });
      },
    );
    req.on('error', reject);
    if (payload !== undefined) {
      req.write(payload);
    }
    req.end();
  });
}

function makeStubClient() {
  return {
    acknowledgePurchase: async (productId, token, pkg) => ({
      status: 200,
      body: { acknowledged: true, productId, token, pkg },
    }),
    consumePurchase: async (productId, token, pkg) => ({
      status: 200,
      body: { consumed: true, productId, token, pkg },
    }),
    verifyPurchaseToken: async (productId, token, pkg) => ({
      status: 200,
      body: { purchaseState: 0, productId, token, pkg },
    }),
    verifyIntegrityToken: async (token, pkg) => ({
      status: 200,
      body: { result: true, hash: 'abc123', token, pkg },
    }),
    listPurchases: async (userAccessToken) => ({
      status: 200,
      body: { purchasesresult: { resources: [], seen: Boolean(userAccessToken) } },
    }),
    getSkuDetails: async (userAccessToken, sku) => ({
      status: 200,
      body: { result: { sku, seen: Boolean(userAccessToken) } },
    }),
  };
}

test('CORS defaults to disabled (false) and only enables explicit allowlist', () => {
  assert.equal(parseAllowedOrigins(undefined), false);
  assert.equal(parseAllowedOrigins(''), false);
  assert.equal(parseAllowedOrigins(' ,  '), false);
  assert.deepEqual(
    parseAllowedOrigins('https://a.example.com, https://b.example.com'),
    ['https://a.example.com', 'https://b.example.com'],
  );
});

test('resolvePackageName enforces single-package isolation even when PLAY_PACKAGE_NAME is unset', () => {
  const prev = process.env.PLAY_PACKAGE_NAME;
  delete process.env.PLAY_PACKAGE_NAME;
  try {
    assert.equal(resolvePackageName(), 'com.example.yourgame');
    assert.equal(resolvePackageName('com.example.yourgame'), 'com.example.yourgame');
    assert.throws(
      () => resolvePackageName('com.other.unauthorized'),
      PackageAuthorizationError,
    );
  } finally {
    if (prev !== undefined) {
      process.env.PLAY_PACKAGE_NAME = prev;
    }
  }
});

test('token and productId validators reject traversal, query chars, arrays/coercion delimiters, and control chars', () => {
  assert.equal(isValidProductId('coins_100'), true);
  assert.equal(isValidProductId('../voidedpurchases?x='), false);
  assert.equal(isValidProductId('sku/../../other'), false);

  assert.equal(isValidToken('sample_oauth_token.abc_def-123456'), true);
  assert.equal(isValidToken('token_part1,token_part2'), false);
  assert.equal(isValidToken('token/../../evil'), false);
  assert.equal(isValidToken('token%3Fx=1'), false);
  assert.equal(isValidToken('token\x00nullbyte'), false);
  assert.equal(isValidToken('short'), false);
});

test('sanitizeErrorForLog strips Authorization header and Gaxios config from errors', () => {
  const fakeGaxiosError = new Error('Upstream Play API failure');
  fakeGaxiosError.code = '403';
  fakeGaxiosError.config = {
    url: 'https://www.googleapis.com/androidpublisher/v3/applications/com.example.yourgame/purchases/products/sku/tokens/tok',
    headers: {
      Authorization: 'Bearer synthetic_bearer_placeholder_value',
    },
  };
  fakeGaxiosError.response = {
    status: 403,
    statusText: 'Forbidden',
    config: fakeGaxiosError.config,
  };

  const sanitized = sanitizeErrorForLog(fakeGaxiosError);
  const serialized = JSON.stringify(sanitized);
  assert.equal(sanitized.status, 403);
  assert.equal(sanitized.statusText, 'Forbidden');
  assert.equal(serialized.includes('synthetic_bearer_placeholder_value'), false);
  assert.equal(serialized.includes('Authorization'), false);
});

test('all /api/* routes reject missing or invalid Bearer tokens with 401 and reflect no CORS header by default', async () => {
  const app = createApiApp({
    corsOrigin: false,
    verifyIdToken: async (tok) => {
      if (tok === 'valid-firebase-id-token') {
        return { uid: 'test-user' };
      }
      throw new Error('Invalid token');
    },
    client: makeStubClient(),
  });

  const server = http.createServer(app);
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  try {
    const routes = [
      '/api/verifyPurchaseToken',
      '/api/acknowledgePurchase',
      '/api/consumePurchase',
      '/api/verifyIntegrityToken',
      '/api/getPurchases',
      '/api/getSkuDetails',
      '/api/postSkuDetails',
    ];

    for (const route of routes) {
      const noAuth = await httpRequest(server, {
        method: 'POST',
        path: route,
        headers: { Origin: 'https://evil.example' },
        body: { productId: 'sku1', token: 'valid_token_123456', userAccessToken: 'valid_token_123456', sku: 'sku1' },
      });
      assert.equal(noAuth.status, 401, `${route} without auth must return 401`);
      assert.equal(noAuth.headers['access-control-allow-origin'], undefined);

      const badAuth = await httpRequest(server, {
        method: 'POST',
        path: route,
        headers: { Authorization: 'Bearer invalid-token' },
        body: { productId: 'sku1', token: 'valid_token_123456', userAccessToken: 'valid_token_123456', sku: 'sku1' },
      });
      assert.equal(badAuth.status, 401, `${route} with invalid auth must return 401`);
    }

    // Legacy GET routes with query-string tokens must no longer be routed.
    const legacyGetVerify = await httpRequest(server, {
      method: 'GET',
      path: '/api/verifyPurchaseToken?productId=sku1&token=valid_token_123456',
      headers: { Authorization: 'Bearer valid-firebase-id-token' },
    });
    assert.equal(legacyGetVerify.status, 404);

    const legacyGetPurchases = await httpRequest(server, {
      method: 'GET',
      path: '/api/getPurchases?userAccessToken=valid_token_123456',
      headers: { Authorization: 'Bearer valid-firebase-id-token' },
    });
    assert.equal(legacyGetPurchases.status, 404);

    // Non-string / array inputs in JSON body must be rejected with 400.
    const arrayInjection = await httpRequest(server, {
      method: 'POST',
      path: '/api/verifyPurchaseToken',
      headers: { Authorization: 'Bearer valid-firebase-id-token' },
      body: { productId: ['a', 'b'], token: 'valid_token_123456' },
    });
    assert.equal(arrayInjection.status, 400);

    // Query-string fallback on POST routes must be ignored (returns 400 when body omits token).
    const queryFallback = await httpRequest(server, {
      method: 'POST',
      path: '/api/verifyPurchaseToken?productId=sku1&token=valid_token_123456',
      headers: { Authorization: 'Bearer valid-firebase-id-token' },
      body: {},
    });
    assert.equal(queryFallback.status, 400);

    // Valid authenticated POST request succeeds.
    const validRes = await httpRequest(server, {
      method: 'POST',
      path: '/api/verifyIntegrityToken',
      headers: { Authorization: 'Bearer valid-firebase-id-token' },
      body: { packageName: 'com.example.yourgame', token: 'valid_token_123456' },
    });
    assert.equal(validRes.status, 200);
    assert.equal(validRes.json.result, true);
  } finally {
    await new Promise((resolve) => server.close(resolve));
  }
});
