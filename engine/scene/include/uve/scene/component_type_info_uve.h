// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <new>
#include <utility>

namespace UVE::Scene {

/// A type-erased descriptor of how to construct/move/destroy a specific component type `T`
/// inside IEntityManagerUVE's archetype storage, generated once per `T` via
/// MakeComponentTypeInfoUVE<T>(). This is the type-erased analog of what
/// ConstructUVE<T>()/DestroyUVE<T>() (engine/memory/include/uve/memory/allocator_utils_uve.h) do
/// for a single, compile-time-known `T` — necessary here because archetype storage is built
/// generically across arbitrary component types identified only by std::type_index at runtime,
/// which a templated ConstructUVE<T>() call site can't express. This struct carries no
/// third-party dependency (unlike ConfigManagerUVE's nlohmann::json), so — unlike that PIMPL —
/// it is intentionally part of the public interface: IEntityManagerUVE's type-safe template
/// methods must be able to name it when calling through to the type-erased virtual primitives.
struct ComponentTypeInfoUVE {
    std::size_t size = 0;
    std::size_t alignment = 0;
    void (*constructDefault)(void* destination) = nullptr;
    void (*moveConstructAndDestroySource)(void* destination, void* source) = nullptr;
    void (*destroy)(void* target) = nullptr;
};

/// Builds the ComponentTypeInfoUVE for `T`. Called once per `T`, the first time
/// IEntityManagerUVE::AddComponentUVE<T>() sees that type on a given manager instance (the
/// concrete implementation caches the result rather than rebuilding it on every call).
template <typename T>
[[nodiscard]] ComponentTypeInfoUVE MakeComponentTypeInfoUVE() {
    return ComponentTypeInfoUVE{
        sizeof(T),
        alignof(T),
        [](void* destination) { ::new (destination) T(); },
        [](void* destination, void* source) {
            T* const sourceObject = static_cast<T*>(source);
            ::new (destination) T(std::move(*sourceObject));
            sourceObject->~T();
        },
        [](void* target) { static_cast<T*>(target)->~T(); },
    };
}

} // namespace UVE::Scene
