// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/memory/allocator_utils_uve.h"

#include <gtest/gtest.h>

#include "uve/memory/heap_allocator_uve.h"

namespace UVE::Memory::Tests {
namespace {

struct TestObjectUVE {
    TestObjectUVE(int valueIn, bool* destructorFlagIn) : value(valueIn), destructorFlag(destructorFlagIn) {}
    ~TestObjectUVE() {
        if (destructorFlag != nullptr) {
            *destructorFlag = true;
        }
    }

    int value;
    bool* destructorFlag;
};

TEST(AllocatorUtilsUVETest, ConstructUVE_RunsConstructorWithForwardedArgs) {
    HeapAllocatorUVE allocator;
    bool destructorCalled = false;

    TestObjectUVE* const object =
        ConstructUVE<TestObjectUVE>(allocator, __FILE__, __LINE__, 42, &destructorCalled);
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->value, 42);
    EXPECT_FALSE(destructorCalled);

    DestroyUVE(allocator, object);
}

TEST(AllocatorUtilsUVETest, DestroyUVE_CallsDestructorBeforeDeallocating) {
    HeapAllocatorUVE allocator;
    bool destructorCalled = false;
    TestObjectUVE* const object =
        ConstructUVE<TestObjectUVE>(allocator, __FILE__, __LINE__, 7, &destructorCalled);
    ASSERT_EQ(allocator.GetAllocatedBytesUVE(), sizeof(TestObjectUVE));

    DestroyUVE(allocator, object);
    EXPECT_TRUE(destructorCalled);
    EXPECT_EQ(allocator.GetAllocatedBytesUVE(), 0U);
}

TEST(AllocatorUtilsUVETest, DestroyUVE_NullObject_IsNoOp) {
    HeapAllocatorUVE allocator;
    DestroyUVE<TestObjectUVE>(allocator, nullptr); // must not crash
}

TEST(AllocatorUtilsUVETest, UveConstructMacro_CapturesSourceLocationAutomatically) {
    HeapAllocatorUVE allocator;
    bool destructorCalled = false;
    TestObjectUVE* const object = UVE_CONSTRUCT(allocator, TestObjectUVE, 5, &destructorCalled);
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->value, 5);
    DestroyUVE(allocator, object);
}

} // namespace
} // namespace UVE::Memory::Tests
