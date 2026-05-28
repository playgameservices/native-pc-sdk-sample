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

/**
 * This file is so a user can set their desired constants for their account.
 */

// SECURITY: never commit service-account keys. Load from the Functions
// runtime environment (set via `firebase functions:secrets:set SA_KEY`).
export const serviceAccountEmail =
  process.env.SA_EMAIL ?? '<SERVICE_ACCOUNT_EMAIL>';
export const serviceAccountPrivateKey =
  (process.env.SA_KEY ?? '<SERVICE_ACCOUNT_PRIVATE_KEY>').replace(/\\n/g, '\n');

// app package name
export const packageName = 'com.dynasty.pconlygame';

// RTDN pub/sub topic ID
export const topicID = 'teapot-billing';

// needed scopes
export const scopes: string[] = [
  'https://www.googleapis.com/auth/androidpublisher',
  'https://www.googleapis.com/auth/playintegrity',
];
