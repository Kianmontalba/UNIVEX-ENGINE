// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_schema_compatibility_uve.h"

namespace UVE::Plugins {
namespace {
[[nodiscard]] MotionQuerySchemaCompatibilityResultUVE MakeCompatibilityResultUVE(
    const MotionQuerySchemaCompatibilityCodeUVE code, const std::size_t index,
    const char* message) noexcept {
    return MotionQuerySchemaCompatibilityResultUVE{code, index, message};
}
} // namespace

MotionQuerySchemaCompatibilityResultUVE ValidateMotionQueryDerivedDataSchemaUVE(
    const MotionQueryDerivedDataUVE& derivedData,
    const UVE::Core::MotionQueryFeatureSchemaUVE& schema) noexcept {
    const UVE::Core::MotionQueryFeatureValidationResultUVE schemaValidation =
        UVE::Core::ValidateMotionQueryFeatureSchemaUVE(schema);
    if (!schemaValidation.IsValidUVE()) {
        return MakeCompatibilityResultUVE(MotionQuerySchemaCompatibilityCodeUVE::InvalidSchema,
                                          schemaValidation.index,
                                          schemaValidation.message.c_str());
    }
    if (derivedData.key.schemaVersion != schema.version) {
        return MakeCompatibilityResultUVE(
            MotionQuerySchemaCompatibilityCodeUVE::SchemaVersionMismatch, 0U,
            "motion query derived data schema version does not match the authoritative schema");
    }
    if (derivedData.normalizedSamples.empty()) {
        return MakeCompatibilityResultUVE(MotionQuerySchemaCompatibilityCodeUVE::EmptyDerivedData,
                                          0U,
                                          "motion query derived data contains no normalized samples");
    }
    const std::size_t expectedDimension = schema.channels.size();
    for (std::size_t index = 0U; index < derivedData.normalizedSamples.size(); ++index) {
        if (derivedData.normalizedSamples[index].values.size() != expectedDimension) {
            return MakeCompatibilityResultUVE(
                MotionQuerySchemaCompatibilityCodeUVE::FeatureDimensionMismatch, index,
                "motion query derived data dimension does not match the authoritative schema");
        }
    }
    return MakeCompatibilityResultUVE(MotionQuerySchemaCompatibilityCodeUVE::Compatible, 0U,
                                      "motion query derived data is compatible with the schema");
}

} // namespace UVE::Plugins
