// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/plugins/motion_query_feature_channels_uve.h"
#include "uve/plugins/motion_query_asset_sampling_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace UVE::Plugins {

enum class MotionQuerySchemaCompatibilityCodeUVE : std::uint8_t {
    Compatible = 0,
    InvalidSchema,
    SchemaVersionMismatch,
    FeatureDimensionMismatch,
    EmptyDerivedData,
};

struct MotionQuerySchemaCompatibilityResultUVE final {
    MotionQuerySchemaCompatibilityCodeUVE code =
        MotionQuerySchemaCompatibilityCodeUVE::InvalidSchema;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsCompatibleUVE() const noexcept {
        return code == MotionQuerySchemaCompatibilityCodeUVE::Compatible;
    }
};

[[nodiscard]] MotionQuerySchemaCompatibilityResultUVE ValidateMotionQueryDerivedDataSchemaUVE(
    const MotionQueryDerivedDataUVE& derivedData,
    const UVE::Core::MotionQueryFeatureSchemaUVE& schema) noexcept;

} // namespace UVE::Plugins
