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
