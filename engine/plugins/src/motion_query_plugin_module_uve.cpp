// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_plugin_module_uve.h"

#include <utility>

namespace UVE::Plugins {

MotionQueryPluginDescriptorUVE MakeMotionQueryPluginDescriptorUVE() {
    MotionQueryPluginDescriptorUVE descriptor;
    descriptor.manifest.pluginId = kMotionQueryPluginIdUVE;
    descriptor.manifest.displayName = kMotionQueryPluginDisplayNameUVE;
    descriptor.manifest.version = NativePluginVersionUVE{1U, 0U, 0U};
    descriptor.manifest.requiredEngineProtocol = kNativePluginProtocolVersionUVE;
    descriptor.manifest.capabilityIds = {
        kMotionQueryCapabilityRuntimeUVE,
        kMotionQueryCapabilityAssetSamplingUVE,
        kMotionQueryCapabilityEditorUVE,
        kMotionQueryCapabilityDiagnosticsUVE,
    };
    descriptor.featureIds = {
        "motion_query.features",
        "motion_query.trajectory",
        "motion_query.history",
        "motion_query.database",
        "motion_query.matching",
    };
    return descriptor;
}

NativePluginRegistryResultUVE RegisterMotionQueryPluginUVE(NativePluginRegistryUVE& registry) {
    return registry.RegisterManifestUVE(MakeMotionQueryPluginDescriptorUVE().manifest);
}

NativePluginRegistryResultUVE RegisterMotionQueryPluginUVE(
    NativePluginRegistryUVE& registry, const NativePluginCapabilityPolicyUVE& policy) {
    return registry.RegisterManifestUVE(MakeMotionQueryPluginDescriptorUVE().manifest, policy);
}

} // namespace UVE::Plugins
