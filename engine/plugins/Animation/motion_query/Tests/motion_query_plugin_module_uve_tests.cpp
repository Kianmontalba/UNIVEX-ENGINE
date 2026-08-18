// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_plugin_module_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins {

TEST(MotionQueryPluginModuleUVETest, MakeDescriptorUVE_UsesStableManifestAndFeatureIds) {
    const MotionQueryPluginDescriptorUVE descriptor = MakeMotionQueryPluginDescriptorUVE();

    EXPECT_EQ(descriptor.manifest.pluginId, kMotionQueryPluginIdUVE);
    EXPECT_EQ(descriptor.manifest.displayName, kMotionQueryPluginDisplayNameUVE);
    EXPECT_EQ(descriptor.manifest.version, (NativePluginVersionUVE{1U, 0U, 0U}));
    EXPECT_EQ(descriptor.manifest.requiredEngineProtocol, kNativePluginProtocolVersionUVE);
    EXPECT_EQ(descriptor.manifest.capabilityIds.size(), 4U);
    EXPECT_EQ(descriptor.featureIds.size(), 5U);
}

TEST(MotionQueryPluginModuleUVETest, RegisterMotionQueryPluginUVE_UsesExistingRegistryLifecycle) {
    NativePluginRegistryUVE registry;
    const NativePluginRegistryResultUVE registration = RegisterMotionQueryPluginUVE(registry);

    ASSERT_TRUE(registration.IsAcceptedUVE()) << registration.message;
    ASSERT_NE(registry.FindManifestUVE(kMotionQueryPluginIdUVE), nullptr);
    EXPECT_EQ(registry.GetManifestCountUVE(), 1U);

    const auto scope = registry.OpenScopeUVE(kMotionQueryPluginIdUVE);
    ASSERT_TRUE(scope.has_value());
    EXPECT_TRUE(registry.IsScopeOpenUVE(kMotionQueryPluginIdUVE));
    EXPECT_TRUE(registry.CloseScopeUVE(*scope).IsAcceptedUVE());
    EXPECT_FALSE(registry.IsScopeOpenUVE(kMotionQueryPluginIdUVE));
}

TEST(MotionQueryPluginModuleUVETest, RegisterMotionQueryPluginUVE_RejectsDuplicateAndRestrictedPolicy) {
    NativePluginRegistryUVE registry;
    ASSERT_TRUE(RegisterMotionQueryPluginUVE(registry).IsAcceptedUVE());
    EXPECT_FALSE(RegisterMotionQueryPluginUVE(registry).IsAcceptedUVE());

    NativePluginRegistryUVE restrictedRegistry;
    NativePluginCapabilityPolicyUVE policy;
    policy.allowAllCapabilities = false;
    policy.allowedCapabilityIds = {kMotionQueryCapabilityRuntimeUVE};
    const NativePluginRegistryResultUVE restricted =
        RegisterMotionQueryPluginUVE(restrictedRegistry, policy);
    EXPECT_FALSE(restricted.IsAcceptedUVE());
    EXPECT_EQ(restrictedRegistry.GetManifestCountUVE(), 0U);
}

} // namespace UVE::Plugins

namespace UVE::Plugins {
namespace {

constexpr std::string_view kMotionQueryPluginConfigurationJsonUVE = R"json({
  "copyright": "Copyright (c) 2026 UniVex Studios. All Rights Reserved.",
  "pluginId": "uve.motion_query",
  "displayName": "UniVex Motion Query",
  "version": "1.0.0",
  "requiredEngineProtocol": 1,
  "capabilities": [
    "motion_query.runtime",
    "motion_query.asset_sampling",
    "motion_query.editor",
    "motion_query.diagnostics"
  ],
  "layout": {
    "runtime": "Source/Runtime",
    "editor": "Source/Editor",
    "shared": "Source/Shared",
    "tests": "Tests"
  }
})json";

} // namespace

TEST(MotionQueryPluginModuleUVETest, ConfigurationUVE_ParsesAndMatchesNativeDescriptor) {
    const MotionQueryPluginConfigurationResultUVE parsed =
        ParseMotionQueryPluginConfigurationUVE(kMotionQueryPluginConfigurationJsonUVE);
    ASSERT_TRUE(parsed.IsValidUVE()) << parsed.message;
    ASSERT_TRUE(parsed.configuration.has_value());

    const MotionQueryPluginConfigurationResultUVE parity =
        ValidateMotionQueryPluginConfigurationParityUVE(*parsed.configuration,
                                                        MakeMotionQueryPluginDescriptorUVE());
    EXPECT_TRUE(parity.IsValidUVE()) << parity.message;
}

TEST(MotionQueryPluginModuleUVETest, ConfigurationUVE_RejectsMalformedVersionAndUnsafeLayout) {
    const std::string malformedVersion = R"json({
      "copyright":"Copyright (c) 2026 UniVex Studios. All Rights Reserved.",
      "pluginId":"uve.motion_query", "displayName":"UniVex Motion Query", "version":"1.0",
      "requiredEngineProtocol":1,
      "capabilities":["motion_query.runtime"],
      "layout":{"runtime":"Source/Runtime","editor":"Source/Editor","shared":"Source/Shared","tests":"Tests"}
    })json";
    EXPECT_EQ(ParseMotionQueryPluginConfigurationUVE(malformedVersion).code,
              MotionQueryPluginConfigurationCodeUVE::InvalidVersion);

    const std::string unsafeLayout = R"json({
      "copyright":"Copyright (c) 2026 UniVex Studios. All Rights Reserved.",
      "pluginId":"uve.motion_query", "displayName":"UniVex Motion Query", "version":"1.0.0",
      "requiredEngineProtocol":1,
      "capabilities":["motion_query.runtime"],
      "layout":{"runtime":"../Runtime","editor":"Source/Editor","shared":"Source/Shared","tests":"Tests"}
    })json";
    EXPECT_EQ(ParseMotionQueryPluginConfigurationUVE(unsafeLayout).code,
              MotionQueryPluginConfigurationCodeUVE::InvalidLayout);
}

TEST(MotionQueryPluginModuleUVETest, ConfigurationUVE_RejectsNativeManifestDrift) {
    const MotionQueryPluginConfigurationResultUVE parsed =
        ParseMotionQueryPluginConfigurationUVE(kMotionQueryPluginConfigurationJsonUVE);
    ASSERT_TRUE(parsed.IsValidUVE());
    ASSERT_TRUE(parsed.configuration.has_value());

    MotionQueryPluginDescriptorUVE descriptor = MakeMotionQueryPluginDescriptorUVE();
    descriptor.manifest.version.patch = 1U;
    EXPECT_EQ(ValidateMotionQueryPluginConfigurationParityUVE(*parsed.configuration, descriptor).code,
              MotionQueryPluginConfigurationCodeUVE::ManifestMismatch);
}

} // namespace UVE::Plugins
