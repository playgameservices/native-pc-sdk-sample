using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using UnityEngine;

namespace PlayPcSdkManaged.Core
{
    public class Result<TError> where TError : struct, Enum
    {
        public bool IsOk { get; set; }
        public TError Code { get; set; }
        public string ErrorMessage { get; set; }

        public static Result<TError> Success(TError okCode = default) =>
            new Result<TError> { IsOk = true, Code = okCode, ErrorMessage = string.Empty };

        public static Result<TError> Failure(TError code, string message) =>
            new Result<TError> { IsOk = false, Code = code, ErrorMessage = message };
    }

    public class Result<TValue, TError> : Result<TError> where TError : struct, Enum
    {
        public TValue Value { get; set; }

        public static Result<TValue, TError> Success(TValue value, TError okCode = default) =>
            new Result<TValue, TError> { IsOk = true, Value = value, Code = okCode, ErrorMessage = string.Empty };

        public new static Result<TValue, TError> Failure(TError code, string message) =>
            new Result<TValue, TError> { IsOk = false, Value = default, Code = code, ErrorMessage = message };
    }
}

namespace PlayPcSdkManaged.Initialization
{
    using PlayPcSdkManaged.Core;

    public enum InitializationError
    {
        Ok = 0,
        ActionRequiredShutdownClientProcess = 1,
        InternalError = 2
    }

    public interface IInitializationHandler { }

    public static class GooglePlayInitialization
    {
        public static async Task<Result<InitializationError>> InitializeAsync(IInitializationHandler handler)
        {
            await Task.Delay(50);
            if (handler == null)
            {
                return Result<InitializationError>.Failure(
                    InitializationError.InternalError,
                    "InitializationHandler cannot be null.");
            }
            return Result<InitializationError>.Success(InitializationError.Ok);
        }
    }
}

namespace PlayPcSdkManaged.Billing
{
    using PlayPcSdkManaged.Core;

    public enum BillingError
    {
        Ok = 0,
        UserCanceled = 1,
        ServiceUnavailable = 2,
        DeveloperError = 3
    }

    public enum ProductType
    {
        InApp = 0,
        Subs = 1
    }

    public class BillingClientParams
    {
        public bool EnablePendingPurchases { get; set; }
    }

    public class ProductId
    {
        public string Id { get; set; }
        public ProductType ProductType { get; set; }
    }

    public class QueryProductDetailsParams
    {
        public List<ProductId> ProductIds { get; set; }
    }

    public class ProductOfferDetails
    {
        public string OfferToken { get; set; }
        public string FormattedPrice { get; set; }
    }

    public class ProductDetails
    {
        public string ProductId { get; set; }
        public string Title { get; set; }
        public List<ProductOfferDetails> ProductOffers { get; set; }
    }

    public class QueryProductDetailsResultValue
    {
        public List<ProductDetails> ProductDetailsList { get; set; }
    }

    public class LaunchPurchaseFlowParams
    {
        public string OfferToken { get; set; }
        public int Quantity { get; set; } = 1;
    }

    public class ProductPurchaseDetails
    {
        public string OrderId { get; set; }
    }

    public class LaunchPurchaseFlowResultValue
    {
        public ProductPurchaseDetails ProductPurchaseDetails { get; set; }
    }

    public class BillingClient : IDisposable
    {
        private readonly BillingClientParams _params;

        public BillingClient(BillingClientParams clientParams = null)
        {
            _params = clientParams ?? new BillingClientParams();
        }

        public async Task<Result<QueryProductDetailsResultValue, BillingError>> QueryProductDetailsAsync(
            QueryProductDetailsParams queryParams)
        {
            await Task.Delay(50);
            var list = new List<ProductDetails>();
            if (queryParams?.ProductIds != null)
            {
                foreach (var pid in queryParams.ProductIds)
                {
                    list.Add(new ProductDetails
                    {
                        ProductId = pid.Id,
                        Title = $"Sample Item ({pid.Id})",
                        ProductOffers = new List<ProductOfferDetails>
                        {
                            new ProductOfferDetails
                            {
                                OfferToken = $"offer_token_{pid.Id}",
                                FormattedPrice = "$1.99"
                            }
                        }
                    });
                }
            }

            return Result<QueryProductDetailsResultValue, BillingError>.Success(
                new QueryProductDetailsResultValue { ProductDetailsList = list });
        }

        public async Task<Result<LaunchPurchaseFlowResultValue, BillingError>> LaunchPurchaseFlowAsync(
            LaunchPurchaseFlowParams purchaseParams)
        {
            await Task.Delay(50);
            if (string.IsNullOrEmpty(purchaseParams?.OfferToken))
            {
                return Result<LaunchPurchaseFlowResultValue, BillingError>.Failure(
                    BillingError.DeveloperError,
                    "OfferToken is required.");
            }

            return Result<LaunchPurchaseFlowResultValue, BillingError>.Success(
                new LaunchPurchaseFlowResultValue
                {
                    ProductPurchaseDetails = new ProductPurchaseDetails
                    {
                        OrderId = $"GPA.{Guid.NewGuid().ToString("N").Substring(0, 12).ToUpperInvariant()}"
                    }
                });
        }

        public void Dispose() { }
    }
}

namespace PlayPcSdkManaged.Integrity
{
    using PlayPcSdkManaged.Core;

    public enum IntegrityError
    {
        Ok = 0,
        InvalidProjectNumber = 1,
        InternalError = 2
    }

    public class PrepareIntegrityTokenParams
    {
        public long CloudProjectNumber { get; set; }
    }

    public class PrepareIntegrityTokenResultValue
    {
        public byte[] RequestTokenData { get; set; }
    }

    public class RequestIntegrityTokenParams
    {
        public byte[] RequestTokenData { get; set; }
        public string RequestHash { get; set; }
    }

    public class RequestIntegrityTokenResultValue
    {
        public byte[] TokenBytes { get; set; }
    }

    public class IntegrityClient : IDisposable
    {
        public async Task<Result<PrepareIntegrityTokenResultValue, IntegrityError>> PrepareIntegrityTokenAsync(
            PrepareIntegrityTokenParams prepareParams)
        {
            await Task.Delay(40);
            return Result<PrepareIntegrityTokenResultValue, IntegrityError>.Success(
                new PrepareIntegrityTokenResultValue
                {
                    RequestTokenData = Encoding.UTF8.GetBytes($"prepared_token_{prepareParams?.CloudProjectNumber ?? 0}")
                });
        }

        public async Task<Result<RequestIntegrityTokenResultValue, IntegrityError>> RequestIntegrityTokenAsync(
            RequestIntegrityTokenParams requestParams)
        {
            await Task.Delay(40);
            return Result<RequestIntegrityTokenResultValue, IntegrityError>.Success(
                new RequestIntegrityTokenResultValue
                {
                    TokenBytes = Encoding.UTF8.GetBytes($"verdict_token_{requestParams?.RequestHash ?? "none"}")
                });
        }

        public void Dispose() { }
    }
}

namespace PlayPcSdkManaged.CloudSave
{
    using PlayPcSdkManaged.Core;

    /// <summary>
    /// Error / status codes defined in [DD] Cloud Save MVP for the Play PC Games (Pages 7, 9, 10, 23).
    /// </summary>
    public enum CloudSaveError
    {
        kOk = 0,
        INVALID_ARGUMENT = 3,       // Unspecified conflict policy (0) or invalid/unsafe file_path
        NOT_FOUND = 5,              // Snapshot does not exist when create_if_not_found = false
        QUOTA_EXCEEDED = 8,         // Save file exceeds the strict 3 MB MVP limit
        kCloudSaveUnavailable = 14  // Phenotype flag games_cloud_save_service is false
    }

    /// <summary>
    /// Auto-resolution conflict policies defined in [DD] Cloud Save MVP (Page 7 & Page 15).
    /// </summary>
    public enum SnapshotConflictPolicy
    {
        Unspecified = 0,          // Default if omitted -> Invalid for auto-resolution (returns INVALID_ARGUMENT)
        LastKnownGood = 1,        // Falls back to the last stable, uncontested save file (USE_HEAD)
        LongestPlaytime = 2,      // Selects the save where the player has the most playtime (kLongestPlaytime)
        MostRecentlyModified = 3, // Selects the save that was modified last (kMostRecentlyModified)
        HighestProgress = 4       // Selects the save with the highest developer-defined progress value (kHighestProgress)
    }

    /// <summary>
    /// C++ SDK SnapshotMetadata structure (Pages 14, 17, 22).
    /// Optional numerical fields return -1 when unpopulated.
    /// </summary>
    [Serializable]
    public class SnapshotMetadata
    {
        public string SnapshotName { get; set; }
        public string Description { get; set; }
        public long LastModifiedTimeMillis { get; set; }
        public long PlayedTimeMillis { get; set; } = -1;
        public long ProgressValue { get; set; } = -1;
        public string DeviceName { get; set; } = "Windows PC";

        public override string ToString()
        {
            return $"{{ \"snapshot_name\": \"{SnapshotName}\", \"description\": \"{Description}\", " +
                   $"\"last_modified_time_millis\": {LastModifiedTimeMillis}, \"played_time_millis\": {PlayedTimeMillis}, " +
                   $"\"progress_value\": {ProgressValue}, \"device_name\": \"{DeviceName}\" }}";
        }
    }

    /// <summary>
    /// Metadata change payload for CommitSnapshot (Page 2 & Page 18).
    /// Uses "Commit update" semantics: replaces the entire metadata state and resets unprovided fields.
    /// </summary>
    [Serializable]
    public class SnapshotMetadataChange
    {
        public string Description { get; set; }
        public long? PlayedTimeMillis { get; set; }
        public long? ProgressValue { get; set; }
    }

    public class ListSnapshotsParams { }

    public class ListSnapshotsResultValue
    {
        public List<SnapshotMetadata> Metadata { get; set; } = new List<SnapshotMetadata>();
    }

    public class OpenSnapshotParams
    {
        public string SnapshotName { get; set; }
        public bool CreateIfNotFound { get; set; } = true;
        public SnapshotConflictPolicy ConflictPolicy { get; set; } = SnapshotConflictPolicy.LongestPlaytime;
        public string FilePath { get; set; }
    }

    public class OpenSnapshotResultValue
    {
        public string SnapshotName { get; set; }
        public bool HasConflictingRevisions { get; set; }
        public SnapshotMetadata Metadata { get; set; }
        public string FilePath { get; set; }
        public string RevisionId { get; set; }
    }

    public class CommitSnapshotParams
    {
        public string SnapshotName { get; set; }
        public string RevisionId { get; set; }
        public string FilePath { get; set; }
        public SnapshotMetadataChange MetadataChange { get; set; }
    }

    public class CommitSnapshotResultValue
    {
        public string SnapshotName { get; set; }
        public SnapshotMetadata Metadata { get; set; }
        public string RevisionId { get; set; }
    }

    public class DeleteSnapshotParams
    {
        public string SnapshotName { get; set; }
    }

    public class DeleteSnapshotResultValue
    {
        public string Status { get; set; } = "kOk";
    }

    /// <summary>
    /// GDS SnapshotFilePathValidator implementation (Pages 3, 6, 9, 10).
    /// Strictly validates that file_path is an absolute path, contains no directory traversal (".."),
    /// and is not a symlink or Windows junction.
    /// </summary>
    public static class SnapshotFilePathValidator
    {
        private static readonly Regex WindowsDriveAbsoluteRegex =
            new Regex(@"^[a-zA-Z]:\\(?:[^<>:""/|?*]+\\)*[^<>:""/|?*]+$", RegexOptions.Compiled);

        private static readonly Regex PosixAbsoluteRegex =
            new Regex(@"^/(?:[^/\0]+/)*[^/\0]+$", RegexOptions.Compiled);

        private static readonly Regex SafeFileNameRegex =
            new Regex(@"^[a-zA-Z0-9._-]+$", RegexOptions.Compiled);

        public static bool Validate(string filePath, out string errorReason)
        {
            if (string.IsNullOrWhiteSpace(filePath))
            {
                errorReason = "SnapshotFilePathValidator: file_path must not be empty.";
                return false;
            }

            string normalized = filePath.Replace('/', '\\');
            string[] parts = normalized.Split(new[] { '\\' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (string part in parts)
            {
                if (part == ".." || part == "." || part.Contains(".."))
                {
                    errorReason = $"SnapshotFilePathValidator: Directory traversal ('{part}') is blocked.";
                    return false;
                }
            }

            if (filePath.StartsWith(@"\\", StringComparison.Ordinal) || filePath.StartsWith("//", StringComparison.Ordinal))
            {
                errorReason = "SnapshotFilePathValidator: Network UNC paths are blocked; local absolute path required.";
                return false;
            }

            if (!WindowsDriveAbsoluteRegex.IsMatch(filePath) && !PosixAbsoluteRegex.IsMatch(filePath))
            {
                errorReason = $"SnapshotFilePathValidator: '{filePath}' is not a valid absolute path.";
                return false;
            }

            try
            {
                if (File.Exists(filePath))
                {
                    FileAttributes attr = File.GetAttributes(filePath);
                    if ((attr & FileAttributes.ReparsePoint) != 0)
                    {
                        errorReason = $"SnapshotFilePathValidator: Symlinks and junctions are blocked ('{filePath}').";
                        return false;
                    }
                }
            }
            catch
            {
                // Ignore cross-OS path attribute lookup differences when testing Windows paths on macOS
            }

            errorReason = null;
            return true;
        }

        /// <summary>
        /// Maps a validated game file path (such as "C:\Saves\save_slot_1.dat") into a strictly
        /// bounded local sandbox folder under Application.persistentDataPath/CloudSaveSandbox.
        /// </summary>
        public static string ResolveSandboxedDiskPath(string sandboxDir, string filePath)
        {
            string normalized = (filePath ?? "save_slot_1.dat").Replace('\\', '/');
            string fileName = Path.GetFileName(normalized);
            if (string.IsNullOrWhiteSpace(fileName) || !SafeFileNameRegex.IsMatch(fileName))
            {
                fileName = "save_slot_default.dat";
            }

            string fullRoot = Path.GetFullPath(sandboxDir);
            if (!Directory.Exists(fullRoot))
            {
                Directory.CreateDirectory(fullRoot);
            }

            string rootWithSep = fullRoot.EndsWith(Path.DirectorySeparatorChar.ToString(), StringComparison.Ordinal)
                ? fullRoot
                : fullRoot + Path.DirectorySeparatorChar;

            string resolved = Path.GetFullPath(Path.Combine(fullRoot, fileName));
            if (!resolved.StartsWith(rootWithSep, StringComparison.Ordinal))
            {
                throw new InvalidOperationException("Sandbox boundary validation failed.");
            }

            return resolved;
        }
    }

    /// <summary>
    /// Client for the Google Play Games on PC Cloud Save MVP (`GamesCloudSaveClient`).
    /// Simulates the GDS IPC proxy (`play.desktop.services.api.games.v1.GamesCloudSave/*`) and
    /// PGS Spanner/Scotty backend (`google.play.games.games.v1.SnapshotsExtended/*`) when the
    /// native runtime is not yet live.
    /// </summary>
    public class GamesCloudSaveClient : IDisposable
    {
        public const int MaxMvpPayloadSizeBytes = 3 * 1024 * 1024; // 3 MB strict MVP limit

        private sealed class SpannerRevision
        {
            public string RevisionId;
            public string Description;
            public long LastModifySeconds;
            public long? GameplayDurationMillis;
            public long? ProgressValue;
            public string DeviceName;
            public byte[] BlobBytes;
            public long CreatedSequence;

            public SpannerRevision Clone() => new SpannerRevision
            {
                RevisionId = RevisionId,
                Description = Description,
                LastModifySeconds = LastModifySeconds,
                GameplayDurationMillis = GameplayDurationMillis,
                ProgressValue = ProgressValue,
                DeviceName = DeviceName,
                BlobBytes = BlobBytes != null ? (byte[])BlobBytes.Clone() : Array.Empty<byte>(),
                CreatedSequence = CreatedSequence
            };
        }

        private sealed class SpannerSlot
        {
            public string SnapshotName;
            public bool IsRemoved;
            public SpannerRevision HeadRevision;
            public SpannerRevision LastKnownGoodRevision;
            public readonly List<SpannerRevision> ConflictingRevisions = new List<SpannerRevision>();
        }

        private readonly Dictionary<string, SpannerSlot> _spannerSlots =
            new Dictionary<string, SpannerSlot>(StringComparer.Ordinal);

        private readonly string _sandboxDir;
        private long _sequence = 100;

        /// <summary>
        /// Simulates the GDS `games_cloud_save_service` Phenotype flag (Page 6 & Page 10).
        /// </summary>
        public bool PhenotypeFlagEnabled { get; set; } = true;

        public GamesCloudSaveClient()
        {
            _sandboxDir = Path.GetFullPath(Path.Combine(Application.persistentDataPath, "CloudSaveSandbox"));
            if (!Directory.Exists(_sandboxDir))
            {
                Directory.CreateDirectory(_sandboxDir);
            }
            SeedInitialSpannerData();
        }

        /// <summary>
        /// Seeds the two initial snapshots shown in the Design Doc (Pages 13-14):
        /// "save_slot_1" and "new_player_save".
        /// </summary>
        private void SeedInitialSpannerData()
        {
            var slot1Rev = new SpannerRevision
            {
                RevisionId = "backend_spanner_rev_id_987654321",
                Description = "Chapter 3 - The Forest",
                LastModifySeconds = 1689253452L,
                GameplayDurationMillis = 14400000L,
                ProgressValue = 3L,
                DeviceName = "Windows PC",
                BlobBytes = Encoding.UTF8.GetBytes("{\"chapter\":3,\"area\":\"The Forest\",\"hp\":100}"),
                CreatedSequence = ++_sequence
            };

            _spannerSlots["save_slot_1"] = new SpannerSlot
            {
                SnapshotName = "save_slot_1",
                IsRemoved = false,
                HeadRevision = slot1Rev,
                LastKnownGoodRevision = slot1Rev.Clone()
            };

            var tutorialRev = new SpannerRevision
            {
                RevisionId = "backend_spanner_rev_id_111222333",
                Description = "Tutorial Started",
                LastModifySeconds = 1689260000L,
                GameplayDurationMillis = null, // Unpopulated -> SDK maps to -1 (Page 14)
                ProgressValue = null,          // Unpopulated -> SDK maps to -1 (Page 14)
                DeviceName = "Windows PC",
                BlobBytes = Encoding.UTF8.GetBytes("{\"chapter\":1,\"area\":\"Tutorial\"}"),
                CreatedSequence = ++_sequence
            };

            _spannerSlots["new_player_save"] = new SpannerSlot
            {
                SnapshotName = "new_player_save",
                IsRemoved = false,
                HeadRevision = tutorialRev,
                LastKnownGoodRevision = tutorialRev.Clone()
            };
        }

        /// <summary>
        /// Helper to write local save file bytes to the sandboxed path before calling CommitSnapshotAsync.
        /// </summary>
        public string WriteLocalSaveFile(string virtualFilePath, byte[] bytes)
        {
            string diskPath = SnapshotFilePathValidator.ResolveSandboxedDiskPath(_sandboxDir, virtualFilePath);
            File.WriteAllBytes(diskPath, bytes ?? Array.Empty<byte>());
            return diskPath;
        }

        public byte[] ReadLocalSaveFile(string virtualFilePath)
        {
            string diskPath = SnapshotFilePathValidator.ResolveSandboxedDiskPath(_sandboxDir, virtualFilePath);
            return File.Exists(diskPath) ? File.ReadAllBytes(diskPath) : Array.Empty<byte>();
        }

        /// <summary>
        /// 1. ListSnapshots (Pages 7, 12-14):
        /// Retrieves metadata for all active saves belonging to the player without downloading binary blobs.
        /// </summary>
        public async Task<Result<ListSnapshotsResultValue, CloudSaveError>> ListSnapshotsAsync(
            ListSnapshotsParams request = null)
        {
            await Task.Delay(40);

            if (!PhenotypeFlagEnabled)
            {
                return Result<ListSnapshotsResultValue, CloudSaveError>.Failure(
                    CloudSaveError.kCloudSaveUnavailable,
                    "Cloud Save is disabled via the games_cloud_save_service Phenotype flag.");
            }

            var output = new ListSnapshotsResultValue();
            foreach (var kvp in _spannerSlots)
            {
                SpannerSlot slot = kvp.Value;
                if (slot.IsRemoved || slot.HeadRevision == null)
                {
                    continue;
                }

                output.Metadata.Add(ToSdkMetadata(slot.SnapshotName, slot.HeadRevision));
            }

            return Result<ListSnapshotsResultValue, CloudSaveError>.Success(output, CloudSaveError.kOk);
        }

        /// <summary>
        /// 2. OpenSnapshot (Pages 8, 15-18):
        /// Locates a save slot in Spanner, silently auto-resolves conflicts using the requested policy,
        /// streams the binary blob directly to local disk, and returns the authoritative revision_id.
        /// </summary>
        public async Task<Result<OpenSnapshotResultValue, CloudSaveError>> OpenSnapshotAsync(
            OpenSnapshotParams request)
        {
            await Task.Delay(50);

            if (!PhenotypeFlagEnabled)
            {
                return Result<OpenSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.kCloudSaveUnavailable,
                    "Cloud Save is disabled via the games_cloud_save_service Phenotype flag.");
            }

            if (request == null || string.IsNullOrWhiteSpace(request.SnapshotName))
            {
                return Result<OpenSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.INVALID_ARGUMENT,
                    "snapshot_name is required.");
            }

            if (request.ConflictPolicy == SnapshotConflictPolicy.Unspecified)
            {
                return Result<OpenSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.INVALID_ARGUMENT,
                    "ConflictPolicy.Unspecified (0) is an invalid state for auto-resolution.");
            }

            if (!SnapshotFilePathValidator.Validate(request.FilePath, out string pathError))
            {
                return Result<OpenSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.INVALID_ARGUMENT,
                    pathError);
            }

            if (!_spannerSlots.TryGetValue(request.SnapshotName, out SpannerSlot slot) || slot.IsRemoved)
            {
                if (!request.CreateIfNotFound)
                {
                    return Result<OpenSnapshotResultValue, CloudSaveError>.Failure(
                        CloudSaveError.NOT_FOUND,
                        $"Snapshot '{request.SnapshotName}' was not found and create_if_not_found is false.");
                }

                // Provision initial empty head revision in Spanner (Scenario 1, Page 43)
                var initialRev = new SpannerRevision
                {
                    RevisionId = $"backend_spanner_rev_id_{++_sequence}",
                    Description = string.Empty,
                    LastModifySeconds = DateTimeOffset.UtcNow.ToUnixTimeSeconds(),
                    GameplayDurationMillis = null,
                    ProgressValue = null,
                    DeviceName = "Windows PC",
                    BlobBytes = Array.Empty<byte>(),
                    CreatedSequence = _sequence
                };

                slot = new SpannerSlot
                {
                    SnapshotName = request.SnapshotName,
                    IsRemoved = false,
                    HeadRevision = initialRev,
                    LastKnownGoodRevision = initialRev.Clone()
                };
                _spannerSlots[request.SnapshotName] = slot;
            }
            else if (slot.ConflictingRevisions.Count > 0)
            {
                // Auto-resolve conflicts using policy & tie-breaking rules (Page 7)
                SpannerRevision winner = ResolveConflicts(slot, request.ConflictPolicy);
                slot.HeadRevision = winner;
                slot.LastKnownGoodRevision = winner.Clone();
                slot.ConflictingRevisions.Clear();
            }

            // Stream blob bytes directly to the validated file_path on local disk
            string diskPath = SnapshotFilePathValidator.ResolveSandboxedDiskPath(_sandboxDir, request.FilePath);
            File.WriteAllBytes(diskPath, slot.HeadRevision.BlobBytes ?? Array.Empty<byte>());

            return Result<OpenSnapshotResultValue, CloudSaveError>.Success(
                new OpenSnapshotResultValue
                {
                    SnapshotName = slot.SnapshotName,
                    HasConflictingRevisions = false,
                    Metadata = ToSdkMetadata(slot.SnapshotName, slot.HeadRevision),
                    FilePath = request.FilePath,
                    RevisionId = slot.HeadRevision.RevisionId
                },
                CloudSaveError.kOk);
        }

        /// <summary>
        /// 3. CommitSnapshot (Pages 8-9, 18-22):
        /// Streams the local save file (up to 3 MB) to Scotty and commits the new revision metadata in Spanner.
        /// Uses "Commit update" (full replacement) semantics for metadata.
        /// </summary>
        public async Task<Result<CommitSnapshotResultValue, CloudSaveError>> CommitSnapshotAsync(
            CommitSnapshotParams request)
        {
            await Task.Delay(60);

            if (!PhenotypeFlagEnabled)
            {
                return Result<CommitSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.kCloudSaveUnavailable,
                    "Cloud Save is disabled via the games_cloud_save_service Phenotype flag.");
            }

            if (request == null || string.IsNullOrWhiteSpace(request.SnapshotName) || string.IsNullOrWhiteSpace(request.RevisionId))
            {
                return Result<CommitSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.INVALID_ARGUMENT,
                    "snapshot_name and revision_id are required. Call OpenSnapshotAsync first.");
            }

            if (!SnapshotFilePathValidator.Validate(request.FilePath, out string pathError))
            {
                return Result<CommitSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.INVALID_ARGUMENT,
                    pathError);
            }

            string diskPath = SnapshotFilePathValidator.ResolveSandboxedDiskPath(_sandboxDir, request.FilePath);
            if (!File.Exists(diskPath))
            {
                return Result<CommitSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.NOT_FOUND,
                    $"Local save file does not exist at '{diskPath}'.");
            }

            byte[] payloadBytes = File.ReadAllBytes(diskPath);
            if (payloadBytes.Length > MaxMvpPayloadSizeBytes)
            {
                return Result<CommitSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.QUOTA_EXCEEDED,
                    $"Save file size ({payloadBytes.Length:N0} bytes) exceeds the 3 MB ({MaxMvpPayloadSizeBytes:N0} bytes) MVP limit.");
            }

            if (!_spannerSlots.TryGetValue(request.SnapshotName, out SpannerSlot slot) || slot.IsRemoved || slot.HeadRevision == null)
            {
                return Result<CommitSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.NOT_FOUND,
                    $"Snapshot '{request.SnapshotName}' is not open. Call OpenSnapshotAsync first.");
            }

            string committedRevId = $"backend_spanner_rev_id_new_{++_sequence}";
            var newRev = new SpannerRevision
            {
                RevisionId = committedRevId,
                Description = request.MetadataChange?.Description ?? string.Empty,
                LastModifySeconds = DateTimeOffset.UtcNow.ToUnixTimeSeconds(),
                GameplayDurationMillis = request.MetadataChange?.PlayedTimeMillis,
                ProgressValue = request.MetadataChange?.ProgressValue,
                DeviceName = "Windows PC",
                BlobBytes = (byte[])payloadBytes.Clone(),
                CreatedSequence = _sequence
            };

            if (string.Equals(slot.HeadRevision.RevisionId, request.RevisionId, StringComparison.Ordinal))
            {
                slot.HeadRevision = newRev;
                if (slot.ConflictingRevisions.Count == 0)
                {
                    slot.LastKnownGoodRevision = newRev.Clone();
                }
            }
            else
            {
                // Base revision_id is outdated -> Backend accepts upload and flags it in conflicting_revisions (Pages 6, 20-21)
                slot.ConflictingRevisions.Add(newRev);
            }

            return Result<CommitSnapshotResultValue, CloudSaveError>.Success(
                new CommitSnapshotResultValue
                {
                    SnapshotName = slot.SnapshotName,
                    Metadata = ToSdkMetadata(slot.SnapshotName, newRev),
                    RevisionId = committedRevId
                },
                CloudSaveError.kOk);
        }

        /// <summary>
        /// 4. DeleteSnapshot (Pages 9, 22-23):
        /// Permanently removes a specific save from the cloud and reclaims storage capacity.
        /// </summary>
        public async Task<Result<DeleteSnapshotResultValue, CloudSaveError>> DeleteSnapshotAsync(
            DeleteSnapshotParams request)
        {
            await Task.Delay(40);

            if (!PhenotypeFlagEnabled)
            {
                return Result<DeleteSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.kCloudSaveUnavailable,
                    "Cloud Save is disabled via the games_cloud_save_service Phenotype flag.");
            }

            if (request == null || string.IsNullOrWhiteSpace(request.SnapshotName))
            {
                return Result<DeleteSnapshotResultValue, CloudSaveError>.Failure(
                    CloudSaveError.INVALID_ARGUMENT,
                    "snapshot_name is required.");
            }

            if (_spannerSlots.TryGetValue(request.SnapshotName, out SpannerSlot slot))
            {
                slot.IsRemoved = true;
                slot.HeadRevision = null;
                slot.ConflictingRevisions.Clear();
            }

            return Result<DeleteSnapshotResultValue, CloudSaveError>.Success(
                new DeleteSnapshotResultValue { Status = "kOk" },
                CloudSaveError.kOk);
        }

        /// <summary>
        /// Simulates another device (e.g. Android phone) advancing the cloud head revision while this PC
        /// still holds an older revision_id, allowing testing of Scenario 5 (Cross-Device Conflict Auto-Resolution).
        /// </summary>
        public string SimulateOtherDeviceSave(
            string snapshotName,
            string description,
            long playedTimeMillis,
            long progressValue,
            string deviceName = "Android Phone")
        {
            if (!_spannerSlots.TryGetValue(snapshotName, out SpannerSlot slot) || slot.IsRemoved || slot.HeadRevision == null)
            {
                return null;
            }

            string newHeadId = $"backend_spanner_rev_id_remote_{++_sequence}";
            var remoteRev = new SpannerRevision
            {
                RevisionId = newHeadId,
                Description = description,
                LastModifySeconds = DateTimeOffset.UtcNow.ToUnixTimeSeconds() + 30,
                GameplayDurationMillis = playedTimeMillis,
                ProgressValue = progressValue,
                DeviceName = deviceName,
                BlobBytes = Encoding.UTF8.GetBytes($"{{\"device\":\"{deviceName}\",\"description\":\"{description}\",\"progress\":{progressValue}}}"),
                CreatedSequence = _sequence
            };

            slot.HeadRevision = remoteRev;
            slot.LastKnownGoodRevision = remoteRev.Clone();
            return newHeadId;
        }

        private static SpannerRevision ResolveConflicts(SpannerSlot slot, SnapshotConflictPolicy policy)
        {
            if (policy == SnapshotConflictPolicy.LastKnownGood)
            {
                return (slot.LastKnownGoodRevision ?? slot.HeadRevision).Clone();
            }

            // Tie-breaking rules (Page 7):
            // 1. Tie between head and conflict -> head wins (evaluated first).
            // 2. Tie between two conflicts -> earlier created snapshot wins.
            var candidates = new List<SpannerRevision> { slot.HeadRevision };
            var sortedConflicts = new List<SpannerRevision>(slot.ConflictingRevisions);
            sortedConflicts.Sort((a, b) => a.CreatedSequence.CompareTo(b.CreatedSequence));
            candidates.AddRange(sortedConflicts);

            SpannerRevision winner = candidates[0];
            for (int i = 1; i < candidates.Count; i++)
            {
                long winnerScore = GetMetric(winner, policy);
                long challengerScore = GetMetric(candidates[i], policy);
                if (challengerScore > winnerScore)
                {
                    winner = candidates[i];
                }
            }
            return winner.Clone();
        }

        private static long GetMetric(SpannerRevision rev, SnapshotConflictPolicy policy)
        {
            switch (policy)
            {
                case SnapshotConflictPolicy.LongestPlaytime:
                    return rev.GameplayDurationMillis ?? -1L;
                case SnapshotConflictPolicy.MostRecentlyModified:
                    return rev.LastModifySeconds;
                case SnapshotConflictPolicy.HighestProgress:
                    return rev.ProgressValue ?? -1L;
                default:
                    return 0L;
            }
        }

        private static SnapshotMetadata ToSdkMetadata(string snapshotName, SpannerRevision rev)
        {
            return new SnapshotMetadata
            {
                SnapshotName = snapshotName,
                Description = rev.Description ?? string.Empty,
                LastModifiedTimeMillis = rev.LastModifySeconds * 1000L,
                PlayedTimeMillis = rev.GameplayDurationMillis ?? -1L,
                ProgressValue = rev.ProgressValue ?? -1L,
                DeviceName = rev.DeviceName ?? "Windows PC"
            };
        }

        public void Dispose() { }
    }
}

namespace PlayPcSdkManaged.Unity
{
    using PlayPcSdkManaged.Billing;
    using PlayPcSdkManaged.CloudSave;
    using PlayPcSdkManaged.Initialization;
    using PlayPcSdkManaged.Integrity;

    public static class PlayPcSdkFactory
    {
        private sealed class DefaultUnityInitializationHandler : IInitializationHandler { }

        public static IInitializationHandler InitializationHandler { get; } = new DefaultUnityInitializationHandler();

        public static BillingClient CreateBillingClient(BillingClientParams clientParams = null) =>
            new BillingClient(clientParams);

        public static IntegrityClient CreateIntegrityClient() =>
            new IntegrityClient();

        public static GamesCloudSaveClient CreateCloudSaveClient() =>
            new GamesCloudSaveClient();
    }
}
