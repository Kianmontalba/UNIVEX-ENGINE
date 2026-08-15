// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/plugin_manifest_validation_uve.h"
#include "uve/plugins/plugin_registry_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins::Tests {

TEST(NativePluginManifestValidationUVETest, ValidateNativePluginManifestUVE_ReportsBoundedStructuredDiagnostics) {
    NativePluginManifestUVE valid{"uve.valid", "Valid", {1U, 0U, 0U},
                                 kNativePluginProtocolVersionUVE, {"node.types", "editor.window"}};
    EXPECT_TRUE(ValidateNativePluginManifestUVE(valid).IsValidUVE());

    NativePluginManifestUVE duplicate = valid;
    duplicate.capabilityIds = {"node.types", "node.types"};
    const NativePluginManifestValidationResultUVE duplicateResult = ValidateNativePluginManifestUVE(duplicate);
    ASSERT_FALSE(duplicateResult.IsValidUVE());
    ASSERT_EQ(duplicateResult.diagnostics.size(), 1U);
    EXPECT_EQ(duplicateResult.diagnostics[0].code,
              NativePluginManifestValidationCodeUVE::DuplicateCapabilityId);
    EXPECT_EQ(duplicateResult.diagnostics[0].capabilityIndex, 1U);

    NativePluginManifestUVE invalidProtocol = valid;
    invalidProtocol.requiredEngineProtocol = 99U;
    const NativePluginManifestValidationResultUVE protocolResult =
        ValidateNativePluginManifestUVE(invalidProtocol);
    ASSERT_FALSE(protocolResult.IsValidUVE());
    EXPECT_EQ(protocolResult.diagnostics[0].code,
              NativePluginManifestValidationCodeUVE::UnsupportedProtocol);

    NativePluginManifestUVE tooManyCapabilities = valid;
    tooManyCapabilities.capabilityIds.assign(
        NativePluginRegistryUVE::kMaximumCapabilitiesPerPluginUVE + 1U, "capability");
    const NativePluginManifestValidationResultUVE limitResult =
        ValidateNativePluginManifestUVE(tooManyCapabilities);
    ASSERT_FALSE(limitResult.IsValidUVE());
    EXPECT_EQ(limitResult.diagnostics[0].code,
              NativePluginManifestValidationCodeUVE::TooManyCapabilities);
}

TEST(NativePluginManifestValidationUVETest, ValidateNativePluginManifestUVE_AppliesExplicitCapabilityPolicy) {
    const NativePluginManifestUVE manifest{"uve.policy", "Policy", {1U, 0U, 0U},
                                           kNativePluginProtocolVersionUVE, {"node.types", "editor.window"}};
    const NativePluginCapabilityPolicyUVE allowPolicy{false, {"node.types", "editor.window"}};
    EXPECT_TRUE(ValidateNativePluginManifestUVE(manifest, allowPolicy).IsValidUVE());

    const NativePluginCapabilityPolicyUVE denyPolicy{false, {"node.types"}};
    const NativePluginManifestValidationResultUVE denied =
        ValidateNativePluginManifestUVE(manifest, denyPolicy);
    ASSERT_FALSE(denied.IsValidUVE());
    ASSERT_EQ(denied.diagnostics.size(), 1U);
    EXPECT_EQ(denied.diagnostics[0].code, NativePluginManifestValidationCodeUVE::CapabilityNotAllowed);
    EXPECT_EQ(denied.diagnostics[0].capabilityIndex, 1U);

    const NativePluginCapabilityPolicyUVE duplicatePolicy{false, {"node.types", "node.types"}};
    const NativePluginManifestValidationResultUVE duplicate =
        ValidateNativePluginManifestUVE(manifest, duplicatePolicy);
    ASSERT_FALSE(duplicate.IsValidUVE());
    EXPECT_EQ(duplicate.diagnostics[0].code,
              NativePluginManifestValidationCodeUVE::DuplicatePolicyCapabilityId);
    EXPECT_EQ(duplicate.diagnostics[0].capabilityIndex, 1U);

    const NativePluginCapabilityPolicyUVE invalidPolicy{false, {"bad id"}};
    const NativePluginManifestValidationResultUVE invalid =
        ValidateNativePluginManifestUVE(manifest, invalidPolicy);
    ASSERT_FALSE(invalid.IsValidUVE());
    EXPECT_EQ(invalid.diagnostics[0].code, NativePluginManifestValidationCodeUVE::InvalidCapabilityId);
}

TEST(NativePluginRegistryUVETest, RegisterManifestUVE_ValidatesIdentityProtocolAndCapabilities) {
    NativePluginRegistryUVE registry;
    NativePluginManifestUVE manifest{"uve.terrain", "Terrain", {1U, 2U, 0U},
                                     kNativePluginProtocolVersionUVE, {"node.types", "editor.window"}};
    const NativePluginRegistryResultUVE registered = registry.RegisterManifestUVE(manifest);
    ASSERT_TRUE(registered.IsAcceptedUVE()) << registered.message;
    EXPECT_EQ(registry.GetManifestCountUVE(), 1U);
    ASSERT_NE(registry.FindManifestUVE("uve.terrain"), nullptr);
    EXPECT_EQ(registry.FindManifestUVE("uve.terrain")->displayName, "Terrain");
    EXPECT_FALSE(registry.RegisterManifestUVE(manifest).IsAcceptedUVE());
    EXPECT_FALSE(registry.RegisterManifestUVE({"bad id", "Bad", {}, 1U, {}}).IsAcceptedUVE());
    EXPECT_FALSE(registry.RegisterManifestUVE({"uve.future", "Future", {}, 99U, {}}).IsAcceptedUVE());
}

TEST(NativePluginRegistryUVETest, ScopesAreGenerationCheckedAndExplicitlyClosed) {
    NativePluginRegistryUVE registry;
    const NativePluginRegistryResultUVE registered = registry.RegisterManifestUVE({"uve.test", "Test", {}, 1U, {}});
    ASSERT_TRUE(registered.IsAcceptedUVE()) << registered.message;
    const auto scope = registry.OpenScopeUVE("uve.test");
    ASSERT_TRUE(scope.has_value());
    EXPECT_TRUE(scope->IsValidUVE());
    EXPECT_TRUE(registry.IsScopeOpenUVE("uve.test"));
    EXPECT_FALSE(registry.OpenScopeUVE("uve.test").has_value());
    ASSERT_TRUE(registry.CloseScopeUVE(*scope).IsAcceptedUVE());
    EXPECT_FALSE(registry.IsScopeOpenUVE("uve.test"));
    EXPECT_EQ(registry.CloseScopeUVE(*scope).code, NativePluginRegistryCodeUVE::InvalidScope);
    EXPECT_EQ(registry.GetOpenScopeCountUVE(), 0U);
}

TEST(NativePluginRegistryUVETest, InvalidAndUnknownScopesAreRejectedWithoutMutation) {
    NativePluginRegistryUVE registry;
    EXPECT_FALSE(registry.OpenScopeUVE("missing").has_value());
    EXPECT_EQ(registry.CloseScopeUVE({}).code, NativePluginRegistryCodeUVE::InvalidScope);
    EXPECT_EQ(registry.GetManifestCountUVE(), 0U);
}

} // namespace UVE::Plugins::Tests
