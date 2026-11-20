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

// SECURITY: Never commit real service-account keys to source control.
// Configure SA_EMAIL, SA_KEY, and PLAY_PACKAGE_NAME via Firebase Functions
// environment variables / secrets (`firebase functions:secrets:set SA_KEY`).
export const serviceAccountEmail = process.env.SA_EMAIL ?? '<SERVICE_ACCOUNT_EMAIL>';
export const serviceAccountPrivateKey = (
  process.env.SA_KEY ?? '<SERVICE_ACCOUNT_PRIVATE_KEY>'
).replace(/\\n/g, '\n');

// Canonical Play Console package name allowed by this backend.
export const packageName = process.env.PLAY_PACKAGE_NAME ?? 'com.example.yourgame';

// Optional comma-separated CORS origins.
// Defaults to false (CORS disabled) because native Windows C++ / Unity desktop clients do not
// require browser CORS; set ALLOWED_ORIGINS explicitly if browser origins must be permitted.
export function parseAllowedOrigins(rawOrigins?: string): boolean | string[] {
  if (!rawOrigins) {
    return false;
  }
  const origins = rawOrigins
    .split(',')
    .map((origin) => origin.trim())
    .filter((origin) => origin.length > 0);
  return origins.length > 0 ? origins : false;
}

export const allowedOrigins: boolean | string[] = parseAllowedOrigins(
  process.env.ALLOWED_ORIGINS,
);

// Real-Time Developer Notifications (RTDN) Cloud Pub/Sub topic ID.
export const topicID = process.env.RTDN_TOPIC_ID ?? 'your-rtdn-pubsub-topic';

// Required Google API OAuth2 scopes for purchase and Play Integrity verification.
export const scopes: string[] = [
  'https://www.googleapis.com/auth/androidpublisher',
  'https://www.googleapis.com/auth/playintegrity',
];
