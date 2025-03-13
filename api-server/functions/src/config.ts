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

// service account credentials
export const serviceAccountEmail = '101904050147-compute@developer.gserviceaccount.com';
export const serviceAccountPrivateKey =
  '-----BEGIN PRIVATE KEY-----\nMIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQCfoPgekvxGfYnA\nuu/+gr89FulT3LoVbsLU+V7J+v3BjhNEmBhXVL2MTTHO9XxvVrJFThXT9M/gQBnb\nTHjAnwwgGfZt9LwWKMWtjoLyQfr9DYFCtzJZLTLuMPxmUXRN8DBdRMnx7e1qYtU/\nEi1wwdz4dABzS7oB8VvpQEuXOtbZ2e7GIW9xUuUWNC+AjDVy5A2VGIjEhN4IrsJM\nhcB4uPI+iRnT9uiAJnJbqJsvUjjVuSZ/4BYmijDdacXQ2k+MeTzA3hOTv+Z6wCu+\njWg5Cblz5/9NI7Au3EoT1UJ/66sV5S63eLamPWgrx0PMuBao+XqpmiLUAPw5jbOb\nIRv2zJ5lAgMBAAECggEAGx1ocCcXD7E84D5QzGpwqiOVbD+a40+ebDdUvNnz1zg3\nOShdwB3embG2I3nLEo2HOCMIramQ0v6IddoSS+PoVx6Z37kZ5uizlWTXBOgkxm3Q\nFmFQfdnh5Bhxkz7EUwXY7UrlQ8Fe2fqiTvWbPuNLUe/AcNU86h4RtvgLLwvZWYU4\npt6ZO14OU2F6ceskZphVgayEomhX87Wub+h6/aqvtTB/mS6FyrZeKrbpC2HU5ESj\nADYd6YajeEewiGT+9ryn8q5/uuj2hqbgx1Jj7PuWPeAEzmD7HlJNCwP49eGZcnp+\nHBTaGXTZZUWDnW70/1qivNM49zDQLsnZwE32WZo1YQKBgQC4poAZiFKFYYcpKnhi\n8Wjp3rZA46j9RyUmk92GaJqUMNXRzfRUIFM/nKlaDWLAGaRfsAm88TJPZD6CwhCN\ngq/mNfSyp5lQHpHiH0BXyqyhy9XJJY2dXX9cgqTuxW+WPHDzyyjovG9zHMOk+226\nzUekR+OPzUCMkpb3xWHqpTsnYQKBgQDdT1jw6VKRywVwFjzi4SDmXqiM23j388aI\nkbKYPZ/GozeZ1BgMFOZIOnq7imYUeXjMYWUlhtox4a7rdo7bODImlPlFYvTXmQlu\nlBG48GVJEJmwlVFWeLnOpawdzjyyO+WH2X71sgowFlWzzYXupE+W75nYZv2HUZnY\nYB8yzObJhQKBgQCO9+PhYQq72Vqkhgy2V0KrKdSlg2AZ01cxenWgFyfwq6syiiuu\nY2TxsOPzJMAcCv6ZVB0GHR6eF/jKl2Xnm5OERKyVE22Celjr4tSX5QYrND7wQlJW\n0MjHISSA93/BrESsjtW69gO49kGJYV+BrmXVDhBlDD/5V5rqdvqsioi1oQKBgQCL\noVWHYl7DN6mtp4owx0PftVd4z7xKDNd1MR/SaAcX58Db4HJvpPwKFEN9cfun7WAm\n60hK3gNqt26Kh1novFL//rwraBz8rkTCLM/ltF7jM4MUY5FXEoKA/AxVHdszxRCz\nnqk40WUWI/FcqU5YW/kPTQru+0P4jaaYW0Uz4w21lQKBgQCJadR7KEwDXHRYmicJ\n7YHYkraWC4Gwj0LZ/nkkTsHLUYQCdXDWl4Vf0HgqQx+LROsIM5kMdVU7n+pTLMBd\nY6+XCmC+ec0VusU8vuVxuvbI8fOAInivJSVMM4bPS7QDmCJyIXu38VK70b2nHtLk\nN/bUqkHB+ylrW8BmArkpuSTB2g==\n-----END PRIVATE KEY-----\n';

// app package name
export const packageName = 'com.dynasty.pconlygame';

// RTDN pub/sub topic ID
export const topicID = 'teapot-billing';

// needed scopes
export const scopes: string[] = ['https://www.googleapis.com/auth/androidpublisher'];
