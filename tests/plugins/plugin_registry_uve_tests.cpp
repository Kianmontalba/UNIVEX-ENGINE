// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/plugin_registry_uve.h"

#include <gtest/gtest.h>

namespace UVE::Plugins::Tests {

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
