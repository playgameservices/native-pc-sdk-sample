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
 * Zero-dependency preflight environment checker for playgameservices/native-pc-sdk-sample.
 * Inspects local toolchains, Play PC SDK placement, gitignored .local/.env overrides,
 * and Windows Google Play Games runtime / 64-bit Developer Mode registry status without
 * ever printing secret values or GUIDs.
 */

import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRootIdx = process.argv.indexOf('--repo-root');
const REPO_ROOT = repoRootIdx !== -1 && process.argv[repoRootIdx + 1]
  ? path.resolve(process.argv[repoRootIdx + 1])
  : path.resolve(__dirname, '..', '..', '..', '..');
const IS_WIN = process.platform === 'win32';
const JSON_MODE = process.argv.includes('--json');

function tryExec(cmd, args, opts = {}) {
  try {
    const out = execFileSync(cmd, args, {
      encoding: 'utf8',
      stdio: ['ignore', 'pipe', 'pipe'],
      timeout: opts.timeout ?? 10000,
      cwd: opts.cwd ?? REPO_ROOT,
    });
    return { ok: true, stdout: out.trim() };
  } catch (err) {
    return {
      ok: false,
      stdout: (err.stdout ? String(err.stdout) : '').trim(),
      stderr: (err.stderr ? String(err.stderr) : '').trim(),
    };
  }
}

function commandVersion(cmd, args = ['--version']) {
  const candidates = IS_WIN && !cmd.endsWith('.exe') && !cmd.endsWith('.cmd')
    ? [`${cmd}.cmd`, `${cmd}.exe`, cmd]
    : [cmd];
  for (const bin of candidates) {
    const res = tryExec(bin, args);
    if (res.ok && res.stdout) {
      return { installed: true, version: res.stdout.split(/\r?\n/)[0].trim() };
    }
  }
  return { installed: false, version: null };
}

function findMsBuild() {
  if (!IS_WIN) return { installed: false, path: null };
  const vswhere = path.join(
    process.env['ProgramFiles(x86)'] || 'C:\\Program Files (x86)',
    'Microsoft Visual Studio',
    'Installer',
    'vswhere.exe',
  );
  if (fs.existsSync(vswhere)) {
    const res = tryExec(vswhere, [
      '-latest',
      '-requires',
      'Microsoft.Component.MSBuild',
      '-find',
      'MSBuild\\**\\Bin\\MSBuild.exe',
    ]);
    if (res.ok && res.stdout) {
      const found = res.stdout.split(/\r?\n/)[0].trim();
      if (fs.existsSync(found)) return { installed: true, path: found };
    }
  }
  const fallbacks = [
    'C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe',
    'C:\\Program Files\\Microsoft Visual Studio\\18\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe',
    'C:\\Program Files\\Microsoft Visual Studio\\18\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe',
    'C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe',
    'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe',
    'C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe',
    'C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\MSBuild\\Current\\Bin\\MSBuild.exe',
  ];
  for (const p of fallbacks) {
    if (fs.existsSync(p)) return { installed: true, path: p };
  }
  return { installed: false, path: null };
}

function checkWindowsRegistryAndGds() {
  if (!IS_WIN) {
    return { applicable: false, note: 'Non-Windows host (api-server setup supported; Windows required for client builds).' };
  }
  const key64 = tryExec('reg.exe', [
    'query',
    'HKLM\\Software\\Google\\Play Games Services',
    '/v',
    'EarlyAccessPartnerGuid',
    '/reg:64',
  ]);
  const key32 = tryExec('reg.exe', [
    'query',
    'HKLM\\SOFTWARE\\WOW6432Node\\Google\\Play Games Services',
    '/v',
    'EarlyAccessPartnerGuid',
  ]);
  const has64 = key64.ok && /EarlyAccessPartnerGuid\s+REG_SZ\s+\S+/i.test(key64.stdout);
  const has32 = key32.ok && /EarlyAccessPartnerGuid\s+REG_SZ\s+\S+/i.test(key32.stdout);

  const svcRes = tryExec('powershell.exe', [
    '-NoLogo',
    '-NoProfile',
    '-NonInteractive',
    '-Command',
    "Get-Service | Where-Object { $_.Name -match 'PlayGames|GooglePlay' } | ForEach-Object { $_.Name + ':' + $_.Status }",
  ]);
  const services = svcRes.ok && svcRes.stdout
    ? svcRes.stdout.split(/\r?\n/).map((s) => s.trim()).filter(Boolean)
    : [];

  return {
    applicable: true,
    earlyAccessPartnerGuid64Bit: has64 ? 'SET' : 'MISSING',
    earlyAccessPartnerGuid32BitWow64: has32 ? 'SET' : 'MISSING',
    registryWarning: !has64 && has32
      ? 'EarlyAccessPartnerGuid is only set in 32-bit WOW6432Node; 64-bit GooglePlayGamesServices requires /reg:64.'
      : null,
    playGamesServices: services,
  };
}

function inspectManifestSummary(relPath) {
  const full = path.join(REPO_ROOT, relPath);
  if (!fs.existsSync(full)) return { exists: false };
  const txt = fs.readFileSync(full, 'utf8');
  const extract = (tag) => {
    const m = txt.match(new RegExp(`<${tag}>([^<]*)</${tag}>`, 'i'));
    return m ? m[1].trim() : null;
  };
  return {
    exists: true,
    packageName: extract('PackageName'),
    sku: extract('SKU'),
    cloudProjectNumber: extract('CloudProjectNumber'),
    backendBaseUrl: extract('BackendBaseUrl'),
  };
}

function inspectEnvSummary(relPath) {
  const full = path.join(REPO_ROOT, relPath);
  if (!fs.existsSync(full)) return { exists: false };
  const txt = fs.readFileSync(full, 'utf8');
  const keys = new Set();
  let saEmail = null;
  let playPackageName = null;
  for (const line of txt.split(/\r?\n/)) {
    const idx = line.indexOf('=');
    if (idx <= 0 || line.trim().startsWith('#')) continue;
    const k = line.slice(0, idx).trim();
    const v = line.slice(idx + 1).trim().replace(/^"|"$/g, '');
    keys.add(k);
    if (k === 'SA_EMAIL') saEmail = v;
    if (k === 'PLAY_PACKAGE_NAME') playPackageName = v;
  }
  return {
    exists: true,
    hasSaEmail: Boolean(saEmail && !saEmail.includes('<')),
    saEmail: saEmail || null,
    hasSaKey: keys.has('SA_KEY'),
    playPackageName: playPackageName || null,
    hasRtdnTopic: keys.has('RTDN_TOPIC_ID'),
  };
}

function inspectAppConfigSummary(relPath) {
  const full = path.join(REPO_ROOT, relPath);
  if (!fs.existsSync(full)) return { exists: false };
  try {
    const raw = fs.readFileSync(full, 'utf8').replace(/^\uFEFF/, '');
    const obj = JSON.parse(raw);
    const cid = String(obj.client_id || '');
    return {
      exists: true,
      hasClientId: Boolean(cid && !cid.includes('<')),
      clientIdPrefix: cid ? cid.slice(0, 16) + (cid.length > 16 ? '...' : '') : null,
      hasClientSecret: Boolean(obj.client_secret && String(obj.client_secret).length > 5),
      redirectUri: obj.redirect_uri || null,
    };
  } catch {
    return { exists: true, parseError: true };
  }
}

const nodeMajor = Number(process.versions.node.split('.')[0] || 0);
const gitStatusRes = tryExec('git', ['status', '--short']);
const gitBranchRes = tryExec('git', ['rev-parse', '--abbrev-ref', 'HEAD']);

const report = {
  repoRoot: REPO_ROOT,
  platform: `${process.platform} (${os.arch()})`,
  git: {
    branch: gitBranchRes.ok ? gitBranchRes.stdout : null,
    clean: gitStatusRes.ok ? gitStatusRes.stdout === '' : null,
    dirtyFileCount: gitStatusRes.ok && gitStatusRes.stdout
      ? gitStatusRes.stdout.split(/\r?\n/).length
      : 0,
  },
  tools: {
    node: { installed: true, version: process.version, meetsNode20: nodeMajor >= 20, meetsNode22: nodeMajor >= 22 },
    npm: commandVersion('npm', ['--version']),
    firebase: commandVersion('firebase', ['--version']),
    gcloud: commandVersion('gcloud', ['--version']),
    vcpkg: commandVersion('vcpkg', ['version']),
    msbuild: findMsBuild(),
  },
  sdkAndDependencies: {
    playPcSdkIncludes: fs.existsSync(path.join(REPO_ROOT, 'sdk-client', 'play_pc_sdk', 'includes')),
    playPcSdkImportsX64: fs.existsSync(path.join(REPO_ROOT, 'sdk-client', 'play_pc_sdk', 'imports', 'x64')),
    sdkClientVcpkgInstalled: fs.existsSync(path.join(REPO_ROOT, 'sdk-client', 'vcpkg_installed')),
    apiClientVcpkgInstalled: fs.existsSync(path.join(REPO_ROOT, 'api-client', 'vcpkg_installed')),
    apiServerNodeModules: fs.existsSync(path.join(REPO_ROOT, 'api-server', 'functions', 'node_modules')),
    apiServerCompiledLib: fs.existsSync(path.join(REPO_ROOT, 'api-server', 'functions', 'lib', 'index.js')),
  },
  localOverrides: {
    apiServerEnv: inspectEnvSummary(path.join('api-server', 'functions', '.env')),
    sdkClientManifestLocal: inspectManifestSummary(path.join('sdk-client', 'GameApplication', 'manifest.local.xml')),
    sdkClientDebugOutManifest: inspectManifestSummary(path.join('sdk-client', 'x64', 'Debug', 'manifest.xml')),
    apiClientManifestLocal: inspectManifestSummary(path.join('api-client', 'GameApplication', 'Resource Files', 'manifest.local.xml')),
    apiClientAppConfigLocal: inspectAppConfigSummary(path.join('api-client', 'GameApplication', 'Resource Files', 'AppConfig.local.json')),
  },
  windowsRuntime: checkWindowsRegistryAndGds(),
};

if (JSON_MODE) {
  console.log(JSON.stringify(report, null, 2));
  process.exit(0);
}

const mark = (ok) => (ok ? '[OK]  ' : '[MISS]');
console.log('=== Native PC SDK Sample Preflight ===');
console.log(`Repo Root : ${report.repoRoot}`);
console.log(`Platform  : ${report.platform}`);
console.log(`Git       : branch=${report.git.branch || 'unknown'} clean=${report.git.clean} (dirty=${report.git.dirtyFileCount})`);
console.log('\n-- Toolchains --');
console.log(`${mark(report.tools.node.meetsNode20)} Node.js ${report.tools.node.version} (Node 20+ locally; Cloud Functions runtime is nodejs22)`);
console.log(`${mark(report.tools.npm.installed)} npm ${report.tools.npm.version || 'not found'}`);
console.log(`${mark(report.tools.firebase.installed)} firebase CLI ${report.tools.firebase.version || 'not found (install with: npm i -g firebase-tools)'}`);
console.log(`${mark(report.tools.gcloud.installed)} gcloud CLI ${report.tools.gcloud.version || 'not found (optional for automated SA setup)'}`);
if (IS_WIN) {
  console.log(`${mark(report.tools.msbuild.installed)} MSBuild ${report.tools.msbuild.path || 'not found'}`);
  console.log(`${mark(report.tools.vcpkg.installed)} vcpkg ${report.tools.vcpkg.version || 'not found on PATH'}`);
}

console.log('\n-- SDK & Dependencies --');
console.log(`${mark(report.sdkAndDependencies.playPcSdkIncludes && report.sdkAndDependencies.playPcSdkImportsX64)} sdk-client/play_pc_sdk (includes + imports/x64)`);
console.log(`${mark(report.sdkAndDependencies.sdkClientVcpkgInstalled)} sdk-client/vcpkg_installed`);
console.log(`${mark(report.sdkAndDependencies.apiClientVcpkgInstalled)} api-client/vcpkg_installed`);
console.log(`${mark(report.sdkAndDependencies.apiServerNodeModules)} api-server/functions/node_modules`);
console.log(`${mark(report.sdkAndDependencies.apiServerCompiledLib)} api-server/functions/lib/index.js`);

console.log('\n-- Gitignored Local Overrides (No Secrets Displayed) --');
const env = report.localOverrides.apiServerEnv;
console.log(`${mark(env.exists && env.hasSaEmail && env.hasSaKey)} api-server/functions/.env (pkg=${env.playPackageName || 'unset'}, saKey=${env.hasSaKey ? 'SET' : 'MISSING'})`);
const sm = report.localOverrides.sdkClientManifestLocal;
console.log(`${mark(sm.exists)} sdk-client/GameApplication/manifest.local.xml (pkg=${sm.packageName || 'unset'}, skus=${sm.sku || 'unset'}, cloudProject=${sm.cloudProjectNumber || 'unset'})`);
const sdm = report.localOverrides.sdkClientDebugOutManifest;
console.log(`${mark(sdm.exists && sdm.packageName && sdm.packageName !== 'your-package-id')} sdk-client/x64/Debug/manifest.xml (pkg=${sdm.packageName || 'missing'})`);
const am = report.localOverrides.apiClientManifestLocal;
console.log(`${mark(am.exists)} api-client/.../manifest.local.xml (pkg=${am.packageName || 'unset'})`);
const ac = report.localOverrides.apiClientAppConfigLocal;
console.log(`${mark(ac.exists && ac.hasClientId && ac.hasClientSecret)} api-client/.../AppConfig.local.json (clientId=${ac.clientIdPrefix || 'unset'}, secret=${ac.hasClientSecret ? 'SET' : 'MISSING'})`);

if (report.windowsRuntime.applicable) {
  console.log('\n-- Windows Play Games Runtime & Developer Mode --');
  console.log(`${mark(report.windowsRuntime.playGamesServices.length > 0)} Services: ${report.windowsRuntime.playGamesServices.join(', ') || 'none detected'}`);
  console.log(`${mark(report.windowsRuntime.earlyAccessPartnerGuid64Bit === 'SET')} 64-bit HKLM\\Software\\Google\\Play Games Services\\EarlyAccessPartnerGuid: ${report.windowsRuntime.earlyAccessPartnerGuid64Bit}`);
  console.log(`       32-bit WOW6432Node EarlyAccessPartnerGuid: ${report.windowsRuntime.earlyAccessPartnerGuid32BitWow64}`);
  if (report.windowsRuntime.registryWarning) {
    console.log(`[WARN] ${report.windowsRuntime.registryWarning}`);
  }
}
