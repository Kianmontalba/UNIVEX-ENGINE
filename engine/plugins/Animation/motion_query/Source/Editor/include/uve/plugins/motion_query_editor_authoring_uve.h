// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/asset/resource_dependency_graph_uve.h"
#include "uve/plugins/motion_query_database_contract_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UVE::Plugins::Editor {

inline constexpr std::uint32_t kMotionQueryEditorProtocolVersionUVE = 1U;
inline constexpr std::size_t kMotionQueryEditorMaximumDatabasesUVE = 128U;
inline constexpr std::size_t kMotionQueryEditorMaximumDisplayNameBytesUVE = 128U;
inline constexpr std::size_t kMotionQueryEditorMaximumHistoryEntriesUVE = 32U;

struct MotionQueryEditorDatabaseEntryUVE final {
    UVE::Asset::ResourceHandleUVE resource;
    std::string displayName;
    UVE::Core::MotionQueryDatabaseContractUVE contract;
    bool dirty = false;
};

struct MotionQueryEditorPasteTargetUVE final {
    UVE::Asset::ResourceHandleUVE resource;
    std::string displayName;
    UVE::Core::MotionQueryDatabaseContextUVE context;
};

enum class MotionQueryEditorCommandPayloadKindUVE : std::uint8_t {
    None = 0,
    Database,
    Resource,
    Text,
    CandidateIndex,
    Candidate,
    PasteTarget,
};

enum class MotionQueryEditorCommandKindUVE : std::uint8_t {
    ReadSnapshot = 0,
    RegisterDatabase,
    RemoveDatabase,
    SelectDatabase,
    SetDisplayName,
    SetSchemaId,
    SetMaximumCandidates,
    AddCandidate,
    RemoveCandidate,
    ValidateDatabase,
    CopyDatabase,
    PasteDatabase,
    Undo,
    Redo,
};

enum class MotionQueryEditorUtilityValidationCodeUVE : std::uint8_t {
    Valid = 0,
    InvalidCandidateIdentifier,
    DuplicateCandidateIdentifier,
    InvalidResource,
    InvalidDisplayName,
    InvalidDatabaseContract,
};

struct MotionQueryEditorUtilityValidationResultUVE final {
    MotionQueryEditorUtilityValidationCodeUVE code =
        MotionQueryEditorUtilityValidationCodeUVE::InvalidCandidateIdentifier;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == MotionQueryEditorUtilityValidationCodeUVE::Valid;
    }
};

[[nodiscard]] bool IsValidMotionQueryEditorDisplayNameUVE(std::string_view displayName) noexcept;

[[nodiscard]] std::string NormalizeMotionQueryEditorSchemaIdUVE(std::string_view schemaId);

[[nodiscard]] MotionQueryEditorUtilityValidationResultUVE
ValidateMotionQueryEditorCandidateIdentifiersUVE(
    const std::vector<UVE::Core::MotionMatchingCandidateUVE>& candidates) noexcept;

[[nodiscard]] MotionQueryEditorUtilityValidationResultUVE
ValidateMotionQueryEditorDatabaseEntryUVE(
    const MotionQueryEditorDatabaseEntryUVE& entry) noexcept;

struct MotionQueryEditorDatabaseFactoryResultUVE final {
    MotionQueryEditorDatabaseEntryUVE entry;
    MotionQueryEditorUtilityValidationResultUVE validation;

    [[nodiscard]] bool IsCreatedUVE() const noexcept {
        return validation.IsValidUVE();
    }
};

[[nodiscard]] MotionQueryEditorDatabaseFactoryResultUVE CreateMotionQueryEditorDatabaseEntryUVE(
    UVE::Asset::ResourceHandleUVE resource, std::string_view displayName,
    UVE::Core::MotionQueryDatabaseContractUVE contract);

enum class MotionQueryEditorPropertyTypeUVE : std::uint8_t {
    String = 0,
    UnsignedInteger,
    Boolean,
    TrajectoryOffsets,
    FeatureChannels,
    CandidateArray,
};

struct MotionQueryEditorPropertyMetadataUVE final {
    std::string id;
    std::string label;
    MotionQueryEditorPropertyTypeUVE type = MotionQueryEditorPropertyTypeUVE::String;
    bool editable = false;
    bool required = false;
    std::size_t maximumItems = 0U;
    std::size_t maximumBytes = 0U;

    [[nodiscard]] bool operator==(const MotionQueryEditorPropertyMetadataUVE&) const = default;
};

[[nodiscard]] const std::vector<MotionQueryEditorPropertyMetadataUVE>&
GetMotionQueryEditorPropertyMetadataUVE() noexcept;

struct MotionQueryEditorCommandMetadataUVE final {
    MotionQueryEditorCommandKindUVE kind = MotionQueryEditorCommandKindUVE::ReadSnapshot;
    MotionQueryEditorCommandPayloadKindUVE payloadKind = MotionQueryEditorCommandPayloadKindUVE::None;
    std::string name;
    std::string label;
    bool mutatesAuthoring = false;
    bool requiresResource = false;
    bool requiresPayload = false;
    bool supportsUndo = false;

    [[nodiscard]] bool operator==(const MotionQueryEditorCommandMetadataUVE&) const = default;
};

[[nodiscard]] const std::vector<MotionQueryEditorCommandMetadataUVE>&
GetMotionQueryEditorCommandMetadataUVE() noexcept;

struct MotionQueryEditorCommandUVE final {
    std::uint32_t protocolVersion = kMotionQueryEditorProtocolVersionUVE;
    std::uint64_t requestId = 0U;
    std::uint64_t expectedRevision = 0U;
    MotionQueryEditorCommandKindUVE kind = MotionQueryEditorCommandKindUVE::ReadSnapshot;
    std::optional<MotionQueryEditorDatabaseEntryUVE> database;
    std::optional<UVE::Asset::ResourceHandleUVE> resource;
    std::optional<std::string> text;
    std::optional<std::size_t> candidateIndex;
    std::optional<UVE::Core::MotionMatchingCandidateUVE> candidate;
    std::optional<MotionQueryEditorPasteTargetUVE> pasteTarget;
};

struct MotionQueryEditorDatabaseRowUVE final {
    UVE::Asset::ResourceHandleUVE resource;
    std::string displayName;
    std::string databaseId;
    std::uint64_t generation = 0U;
    std::uint32_t schemaVersion = 0U;
    std::string schemaId;
    std::size_t candidateCount = 0U;
    std::size_t maximumCandidates = 0U;
    bool valid = false;
    bool selected = false;
    bool dirty = false;

    [[nodiscard]] bool operator==(const MotionQueryEditorDatabaseRowUVE&) const = default;
};

struct MotionQueryEditorSnapshotUVE final {
    std::uint32_t protocolVersion = kMotionQueryEditorProtocolVersionUVE;
    std::uint64_t revision = 0U;
    bool authoringAvailable = true;
    std::optional<UVE::Asset::ResourceHandleUVE> selectedResource;
    std::vector<MotionQueryEditorDatabaseRowUVE> databases;
    std::vector<MotionQueryEditorCommandMetadataUVE> commandMetadata;
    std::vector<MotionQueryEditorPropertyMetadataUVE> propertyMetadata;
    bool clipboardAvailable = false;
    bool canUndo = false;
    bool canRedo = false;
    std::string diagnostic;

    [[nodiscard]] bool operator==(const MotionQueryEditorSnapshotUVE&) const = default;
};

enum class MotionQueryEditorResponseCodeUVE : std::uint8_t {
    Applied = 0,
    InvalidProtocol,
    StaleRevision,
    InvalidCommand,
    InvalidDatabase,
    DatabaseNotFound,
    DuplicateDatabase,
    CandidateNotFound,
    ValidationFailed,
    ClipboardEmpty,
    InvalidPasteTarget,
    NothingToUndo,
    NothingToRedo,
};

struct MotionQueryEditorResponseUVE final {
    std::uint32_t protocolVersion = kMotionQueryEditorProtocolVersionUVE;
    std::uint64_t requestId = 0U;
    bool applied = false;
    MotionQueryEditorResponseCodeUVE code = MotionQueryEditorResponseCodeUVE::InvalidCommand;
    std::string message;
    MotionQueryEditorSnapshotUVE snapshot;
};

class MotionQueryEditorAuthoringSessionUVE final {
public:
    [[nodiscard]] MotionQueryEditorResponseUVE DispatchUVE(
        const MotionQueryEditorCommandUVE& command) noexcept;

    void ClearUVE() noexcept;

    [[nodiscard]] MotionQueryEditorSnapshotUVE GetSnapshotUVE() const noexcept;
    [[nodiscard]] bool TryGetDatabaseCopyUVE(
        UVE::Asset::ResourceHandleUVE resource,
        UVE::Core::MotionQueryDatabaseContractUVE& destination) const noexcept;

private:
    struct StateUVE final {
        std::vector<MotionQueryEditorDatabaseEntryUVE> databases;
        std::optional<UVE::Asset::ResourceHandleUVE> selectedResource;
        std::optional<MotionQueryEditorDatabaseEntryUVE> clipboard;
    };

    [[nodiscard]] StateUVE CaptureStateUVE() const;
    void RestoreStateUVE(StateUVE state) noexcept;
    void CommitMutationUVE(StateUVE stateBefore);

    [[nodiscard]] MotionQueryEditorResponseUVE MakeResponseUVE(
        const MotionQueryEditorCommandUVE& command, bool applied,
        MotionQueryEditorResponseCodeUVE code, std::string message) const;
    [[nodiscard]] MotionQueryEditorDatabaseEntryUVE* FindDatabaseUVE(
        UVE::Asset::ResourceHandleUVE resource) noexcept;
    [[nodiscard]] const MotionQueryEditorDatabaseEntryUVE* FindDatabaseUVE(
        UVE::Asset::ResourceHandleUVE resource) const noexcept;
    [[nodiscard]] static bool IsValidResourceUVE(
        UVE::Asset::ResourceHandleUVE resource) noexcept;
    [[nodiscard]] static MotionQueryEditorDatabaseRowUVE BuildRowUVE(
        const MotionQueryEditorDatabaseEntryUVE& entry,
        std::optional<UVE::Asset::ResourceHandleUVE> selectedResource);

    std::vector<MotionQueryEditorDatabaseEntryUVE> databases_;
    std::optional<UVE::Asset::ResourceHandleUVE> selectedResource_;
    std::optional<MotionQueryEditorDatabaseEntryUVE> clipboard_;
    std::vector<StateUVE> undoHistory_;
    std::vector<StateUVE> redoHistory_;
    std::uint64_t revision_ = 0U;
};

} // namespace UVE::Plugins::Editor
