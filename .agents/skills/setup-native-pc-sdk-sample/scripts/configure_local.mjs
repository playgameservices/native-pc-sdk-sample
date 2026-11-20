#!/usr/bin/env node
/*
 * Copyright 2026 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Deterministic, secret-safe local override generator for playgameservices/native-pc-sdk-sample.
 *
 * Reads external Service Account JSON (and optional Desktop OAuth 2.0 JSON in either
 * Google Cloud Console {"installed": {...}} format or flat AppConfig.json format) and writes
 * BOM-free UTF-8 gitignored override files without modifying tracked templates or printing secrets.
 */

import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const DEFAULT_REPO_ROOT = path.resolve(__dirname, '..', '..', '..', '..');

const PACKAGE_RE = /^[a-zA-Z][a-zA-Z0-9_]*(\.[a-zA-Z][a-zA-Z0-9_]*)+$/;
const SKU_RE = /^[a-zA-Z0-9._-]{1,150}$/;
const DEFAULT_SCOPES =
  'https://www.googleapis.com/auth/googleplay.purchases%20https://www.googleapis.com/auth/googleplay.products';

function usage() {
  console.log(`Usage:
  node .agents/skills/setup-native-pc-sdk-sample/scripts/configure_local.mjs [options]

Options:
  --target <list>                Comma-separated targets: all, sdk-client, api-client, api-server (default: all)
  --package-name <pkg>           Android/Play Console package name (e.g. com.example.mygame)
  --skus <sku1,sku2>             One or more comma-separated active Play Console product IDs
  --cloud-project-number <num>   Numeric Google Cloud project number (for Play Integrity)
  --backend-url <url>            Deployed api-server base HTTPS URL (e.g. https://my-proj.web.app)
  --sa-json <path>               Path to external GCP Service Account JSON key (required for api-server)
  --oauth-json <path>            Path to external Desktop OAuth 2.0 JSON (required for api-client; supports GCP {"installed":{...}} or flat AppConfig.json)
  --rtdn-topic <topic>           Optional Cloud Pub/Sub RTDN topic ID (written as RTDN_TOPIC_ID in .env)
  --repo-root <path>             Override repository root (default: auto-detected from script location)
  --dry-run                      Validate inputs and print planned target paths without writing files
  --help                         Show this help message
`);
}

function parseArgs(argv) {
  const opts = { target: 'all', dryRun: false, repoRoot: DEFAULT_REPO_ROOT };
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a === '--help' || a === '-h') {
      usage();
      process.exit(0);
    } else if (a === '--dry-run') {
      opts.dryRun = true;
    } else if (a.startsWith('--')) {
      const key = a.slice(2);
      const val = argv[++i];
      if (val === undefined) {
        throw new Error(`Missing value for flag --${key}`);
      }
      opts[key] = val;
    } else {
      throw new Error(`Unexpected positional argument: ${a}`);
    }
  }
  return opts;
}

function xmlEscape(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&apos;');
}

function parseTargets(targetStr) {
  const items = targetStr.split(',').map((s) => s.trim().toLowerCase()).filter(Boolean);
  if (items.includes('all')) return new Set(['sdk-client', 'api-client', 'api-server']);
  const allowed = new Set(['sdk-client', 'api-client', 'api-server']);
  const out = new Set();
  for (const item of items) {
    if (!allowed.has(item)) {
      throw new Error(`Invalid target '${item}'. Allowed: all, sdk-client, api-client, api-server.`);
    }
    out.add(item);
  }
  return out;
}

function loadServiceAccount(saPath) {
  const resolved = path.resolve(saPath);
  if (!fs.existsSync(resolved)) {
    throw new Error(`Service Account JSON not found: ${resolved}`);
  }
  const obj = JSON.parse(fs.readFileSync(resolved, 'utf8').replace(/^\uFEFF/, ''));
  const email = String(obj.client_email || '').trim();
  const rawKey = String(obj.private_key || '').replace(/\r\n/g, '\n').trim();
  if (!email || !email.includes('@')) {
    throw new Error('Service Account JSON is missing a valid client_email.');
  }
  if (!rawKey || !rawKey.includes('PRIVATE KEY') || rawKey.length < 100) {
    throw new Error('Service Account JSON is missing a valid private_key.');
  }
  return { email, keyEscaped: `${rawKey}\n`.replace(/\n/g, '\\n') };
}

function loadOAuthDesktopConfig(oauthPath) {
  const resolved = path.resolve(oauthPath);
  if (!fs.existsSync(resolved)) {
    throw new Error(`Desktop OAuth JSON not found: ${resolved}`);
  }
  const rawObj = JSON.parse(fs.readFileSync(resolved, 'utf8').replace(/^\uFEFF/, ''));
  const src = rawObj.installed || rawObj.web || rawObj;
  const clientId = String(src.client_id || '').trim();
  const clientSecret = String(src.client_secret || '').trim();
  if (!clientId || clientId.includes('<')) {
    throw new Error('OAuth JSON is missing a valid client_id.');
  }
  if (!clientSecret) {
    throw new Error('OAuth JSON is missing client_secret.');
  }
  return {
    client_id: clientId,
    client_secret: clientSecret,
    redirect_uri: 'http://127.0.0.1:8080/',
    authorization_Endpoint: String(
      src.authorization_Endpoint || src.auth_uri || 'https://accounts.google.com/o/oauth2/v2/auth',
    ).trim(),
    authorization_code: '',
    Scopes: String(src.Scopes || DEFAULT_SCOPES).trim(),
    tokenEndpoint: String(
      src.tokenEndpoint || src.token_Endpoint || src.token_uri || 'https://oauth2.googleapis.com/token',
    ).trim(),
  };
}

function buildManifestXml({ packageName, skuList, cloudProjectNumber, backendUrl }) {
  const lines = [
    '<?xml version="1.0" encoding="utf-8"?>',
    '<Manifest version="1">',
    '  <Application>',
    `    <PackageName>${xmlEscape(packageName)}</PackageName>`,
    '    <IsDeveloperMode>true</IsDeveloperMode>',
    `    <SKU>${xmlEscape(skuList.join(','))}</SKU>`,
    `    <CloudProjectNumber>${xmlEscape(cloudProjectNumber)}</CloudProjectNumber>`,
    `    <BackendBaseUrl>${xmlEscape(backendUrl)}</BackendBaseUrl>`,
    '  </Application>',
    '</Manifest>',
    '',
  ];
  return lines.join('\n');
}

function assertGitIgnored(repoRoot, relPath) {
  if (!fs.existsSync(path.join(repoRoot, '.git'))) return;
  try {
    execFileSync('git', ['check-ignore', '-q', relPath], { cwd: repoRoot, stdio: 'ignore' });
  } catch {
    throw new Error(
      `Refusing to write '${relPath}': path is NOT matched by .gitignore and could leak credentials.`,
    );
  }
}

function main() {
  const opts = parseArgs(process.argv.slice(2));
  const repoRoot = path.resolve(opts['repo-root'] || opts.repoRoot);
  if (!fs.existsSync(path.join(repoRoot, '.gitignore'))) {
    throw new Error(`Invalid --repo-root '${repoRoot}': expected repository root containing .gitignore.`);
  }
  const targets = parseTargets(opts.target);

  const packageName = String(opts['package-name'] || '').trim();
  if (!PACKAGE_RE.test(packageName)) {
    throw new Error('--package-name is required and must be a valid reverse-DNS package name (e.g. com.example.mygame).');
  }

  const skuList = String(opts.skus || '')
    .split(',')
    .map((s) => s.trim())
    .filter(Boolean);
  if ((targets.has('sdk-client') || targets.has('api-client')) && skuList.length === 0) {
    throw new Error('--skus is required for sdk-client / api-client (e.g. --skus coins_100,starter_pack).');
  }
  for (const s of skuList) {
    if (!SKU_RE.test(s)) {
      throw new Error(`Invalid SKU '${s}'. Must match ${SKU_RE}.`);
    }
  }

  const cloudProjectNumber = String(opts['cloud-project-number'] || '0').trim();
  if (!/^\d+$/.test(cloudProjectNumber)) {
    throw new Error('--cloud-project-number must be numeric (e.g. 123456789012).');
  }

  const backendUrl = String(opts['backend-url'] || '').trim().replace(/\/+$/, '');
  if ((targets.has('sdk-client') || targets.has('api-client')) && !/^https:\/\/[^/\s]+/i.test(backendUrl)) {
    throw new Error('--backend-url must be an HTTPS base URL (e.g. https://my-project.web.app).');
  }

  const plannedWrites = new Map();

  if (targets.has('api-server')) {
    if (!opts['sa-json']) {
      throw new Error('--sa-json is required when targeting api-server.');
    }
    const sa = loadServiceAccount(opts['sa-json']);
    const envLines = [
      `SA_EMAIL=${sa.email}`,
      `SA_KEY="${sa.keyEscaped}"`,
      `PLAY_PACKAGE_NAME=${packageName}`,
    ];
    if (opts['rtdn-topic']) {
      envLines.push(`RTDN_TOPIC_ID=${String(opts['rtdn-topic']).trim()}`);
    }
    plannedWrites.set(
      path.join('api-server', 'functions', '.env'),
      `${envLines.join('\n')}\n`,
    );
  }

  if (targets.has('sdk-client')) {
    const manifestXml = buildManifestXml({ packageName, skuList, cloudProjectNumber, backendUrl });
    plannedWrites.set(
      path.join('sdk-client', 'GameApplication', 'manifest.local.xml'),
      manifestXml,
    );
  }

  if (targets.has('api-client')) {
    if (!opts['oauth-json']) {
      throw new Error('--oauth-json is required when targeting api-client (or pass --target sdk-client,api-server to skip api-client).');
    }
    const manifestXml = buildManifestXml({ packageName, skuList, cloudProjectNumber, backendUrl });
    const appCfgJson = `${JSON.stringify(loadOAuthDesktopConfig(opts['oauth-json']), null, 2)}\n`;
    plannedWrites.set(path.join('api-client', 'GameApplication', 'manifest.local.xml'), manifestXml);
    plannedWrites.set(path.join('api-client', 'GameApplication', 'Resource Files', 'manifest.local.xml'), manifestXml);
    plannedWrites.set(path.join('api-client', 'GameApplication', 'AppConfig.local.json'), appCfgJson);
    plannedWrites.set(path.join('api-client', 'GameApplication', 'Resource Files', 'AppConfig.local.json'), appCfgJson);
  }

  // Enforce gitignore safety BEFORE touching disk
  for (const relPath of plannedWrites.keys()) {
    assertGitIgnored(repoRoot, relPath);
  }

  if (opts.dryRun) {
    console.log('Dry-run validation PASSED (no files written). Planned gitignored writes:');
    for (const [relPath, content] of plannedWrites.entries()) {
      console.log(`  - ${relPath} (${Buffer.byteLength(content, 'utf8')} bytes, BOM-free UTF-8)`);
    }
    return;
  }

  for (const [relPath, content] of plannedWrites.entries()) {
    const dest = path.join(repoRoot, relPath);
    fs.mkdirSync(path.dirname(dest), { recursive: true });
    fs.writeFileSync(dest, content, { encoding: 'utf8' });
    console.log(`Wrote gitignored override: ${relPath} (${Buffer.byteLength(content, 'utf8')} bytes)`);
  }

  if (fs.existsSync(path.join(repoRoot, '.git'))) {
    const status = execFileSync('git', ['status', '--short'], { cwd: repoRoot, encoding: 'utf8' }).trim();
    console.log(`Git working tree status after configuration: ${status === '' ? 'CLEAN (0 tracked changes)' : status}`);
  }
}

try {
  main();
} catch (err) {
  console.error(`ERROR: ${err instanceof Error ? err.message : String(err)}`);
  process.exit(1);
}
