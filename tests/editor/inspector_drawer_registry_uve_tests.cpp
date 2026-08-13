// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/editor/inspector_drawer_registry_uve.h"

namespace UVE::Editor::Tests {
namespace {

[[nodiscard]] InspectorDrawerEntryUVE MakeAlwaysEligibleDrawerUVE(const std::string id,
                                                                   std::vector<std::string>& invocationOrder) {
    return InspectorDrawerEntryUVE{
        id,
        [](const Scene::EntityUVE) { return true; },
        [&invocationOrder, id](const Scene::EntityUVE) { invocationOrder.push_back(id); },
    };
}

TEST(InspectorDrawerRegistryUVETest, RegisterDrawerUVE_RejectsIncompleteAndDuplicateEntriesWithoutMutation) {
    InspectorDrawerRegistryUVE registry;
    std::vector<std::string> invocationOrder;

    EXPECT_FALSE(registry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "", [](const Scene::EntityUVE) { return true; }, [](const Scene::EntityUVE) {},
    }));
    EXPECT_FALSE(registry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "missing-predicate", {}, [](const Scene::EntityUVE) {},
    }));
    EXPECT_FALSE(registry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "missing-draw", [](const Scene::EntityUVE) { return true; }, {},
    }));
    EXPECT_EQ(registry.GetDrawerCountUVE(), 0U);
    EXPECT_FALSE(registry.HasDrawerUVE("name"));

    ASSERT_TRUE(registry.RegisterDrawerUVE(MakeAlwaysEligibleDrawerUVE("name", invocationOrder)));
    EXPECT_TRUE(registry.HasDrawerUVE("name"));
    EXPECT_EQ(registry.GetDrawerCountUVE(), 1U);

    EXPECT_FALSE(registry.RegisterDrawerUVE(MakeAlwaysEligibleDrawerUVE("name", invocationOrder)));
    EXPECT_EQ(registry.GetDrawerCountUVE(), 1U);
}

TEST(InspectorDrawerRegistryUVETest, DrawEligibleUVE_PreservesRegistrationOrderAndFiltersIneligibleDrawers) {
    InspectorDrawerRegistryUVE registry;
    std::vector<std::string> invocationOrder;
    const Scene::EntityUVE selected{42U, 7U};
    Scene::EntityUVE predicateEntity{};
    Scene::EntityUVE drawEntity{};

    ASSERT_TRUE(registry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "first",
        [&predicateEntity](const Scene::EntityUVE entity) {
            predicateEntity = entity;
            return true;
        },
        [&invocationOrder, &drawEntity](const Scene::EntityUVE entity) {
            drawEntity = entity;
            invocationOrder.emplace_back("first");
        },
    }));
    ASSERT_TRUE(registry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "filtered",
        [](const Scene::EntityUVE) { return false; },
        [&invocationOrder](const Scene::EntityUVE) { invocationOrder.emplace_back("filtered"); },
    }));
    ASSERT_TRUE(registry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "last",
        [](const Scene::EntityUVE) { return true; },
        [&invocationOrder](const Scene::EntityUVE) { invocationOrder.emplace_back("last"); },
    }));

    registry.DrawEligibleUVE(selected);

    EXPECT_EQ(predicateEntity, selected);
    EXPECT_EQ(drawEntity, selected);
    EXPECT_EQ(invocationOrder, (std::vector<std::string>{"first", "last"}));
}

TEST(InspectorDrawerRegistryUVETest, DrawEligibleUVE_EmptyRegistryDoesNotInvokeCallbacks) {
    InspectorDrawerRegistryUVE registry;

    registry.DrawEligibleUVE(Scene::EntityUVE{1U, 0U});

    EXPECT_EQ(registry.GetDrawerCountUVE(), 0U);
}

TEST(InspectorDrawerRegistryUVETest, DrawEligibleUVE_DefersDrawerRegisteredByCallbackUntilNextPass) {
    InspectorDrawerRegistryUVE registry;
    std::vector<std::string> invocationOrder;
    bool registeredDeferredDrawer = false;

    ASSERT_TRUE(registry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "first",
        [](const Scene::EntityUVE) { return true; },
        [&registry, &invocationOrder, &registeredDeferredDrawer](const Scene::EntityUVE) {
            invocationOrder.emplace_back("first");
            if (registeredDeferredDrawer) {
                return;
            }
            registeredDeferredDrawer = true;
            ASSERT_TRUE(registry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
                "deferred",
                [](const Scene::EntityUVE) { return true; },
                [&invocationOrder](const Scene::EntityUVE) { invocationOrder.emplace_back("deferred"); },
            }));
        },
    }));

    registry.DrawEligibleUVE(Scene::EntityUVE{5U, 3U});
    EXPECT_EQ(invocationOrder, (std::vector<std::string>{"first"}));
    EXPECT_EQ(registry.GetDrawerCountUVE(), 2U);

    registry.DrawEligibleUVE(Scene::EntityUVE{5U, 3U});
    EXPECT_EQ(invocationOrder, (std::vector<std::string>{"first", "first", "deferred"}));
}

} // namespace
} // namespace UVE::Editor::Tests
