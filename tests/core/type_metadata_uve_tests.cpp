// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/type_metadata_uve.h"

#include <gtest/gtest.h>

namespace UVE::Core {

TEST(TypeMetadataRegistryUVETest, RegisterTypeUVE_OrdersCopiedSnapshotAndAdvancesGeneration) {
    TypeMetadataRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterTypeUVE(TypeMetadataEntryUVE{
        TypeMetadataKindUVE::VisualScriptNode,
        "node.zeta",
        "Zeta Node",
        1U,
        {{"Value", "Value", "Number", true}},
        {{"Execute", "Execute", 1U}},
    }).IsRegisteredUVE());
    ASSERT_TRUE(registry.RegisterTypeUVE(TypeMetadataEntryUVE{
        TypeMetadataKindUVE::Component,
        "component.transform",
        "Transform",
        2U,
        {{"Position", "Position", "Vector3", true}},
        {},
    }).IsRegisteredUVE());

    const TypeMetadataSnapshotUVE snapshot = registry.GetSnapshotUVE();
    ASSERT_EQ(snapshot.entries.size(), 2U);
    EXPECT_EQ(snapshot.generation, 2U);
    EXPECT_EQ(snapshot.entries[0].typeId, "component.transform");
    EXPECT_EQ(snapshot.entries[1].typeId, "node.zeta");
    EXPECT_TRUE(snapshot.entries[1].properties[0].editable);
    EXPECT_EQ(snapshot.entries[1].methods[0].name, "Execute");
}

TEST(TypeMetadataRegistryUVETest, RegisterTypeUVE_RejectsDuplicateAndMalformedMembersWithoutMutation) {
    TypeMetadataRegistryUVE registry;
    const TypeMetadataEntryUVE valid{
        TypeMetadataKindUVE::Resource, "resource.mesh", "Mesh", 1U, {}, {}};
    ASSERT_TRUE(registry.RegisterTypeUVE(valid).IsRegisteredUVE());

    const TypeMetadataRegistrationResultUVE duplicate = registry.RegisterTypeUVE(valid);
    EXPECT_EQ(duplicate.code, TypeMetadataRegistrationCodeUVE::DuplicateType);
    EXPECT_EQ(registry.GetTypeCountUVE(), 1U);
    EXPECT_EQ(registry.GetGenerationUVE(), 1U);

    const TypeMetadataRegistrationResultUVE malformed = registry.RegisterTypeUVE(TypeMetadataEntryUVE{
        TypeMetadataKindUVE::InspectorTarget,
        "target.camera",
        "Camera",
        1U,
        {{"Value", "Value", "Number", true}},
        {{"Value", "Invoke", 0U}},
    });
    EXPECT_EQ(malformed.code, TypeMetadataRegistrationCodeUVE::InvalidEntry);
    EXPECT_EQ(registry.GetTypeCountUVE(), 1U);
    EXPECT_EQ(registry.GetGenerationUVE(), 1U);
}

TEST(TypeMetadataRegistryUVETest, RegisterTypeUVE_RejectsUnboundedIdentityBeforeMutation) {
    TypeMetadataRegistryUVE registry;
    const std::string oversized(TypeMetadataRegistryUVE::kMaximumIdentifierBytesUVE + 1U, 'x');
    const TypeMetadataRegistrationResultUVE result = registry.RegisterTypeUVE(TypeMetadataEntryUVE{
        TypeMetadataKindUVE::Other, oversized, "Oversized", 1U, {}, {}});

    EXPECT_EQ(result.code, TypeMetadataRegistrationCodeUVE::InvalidEntry);
    EXPECT_EQ(registry.GetTypeCountUVE(), 0U);
    EXPECT_EQ(registry.GetGenerationUVE(), 0U);
}

} // namespace UVE::Core
