//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/core/engine_services_uve.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>

#include <gtest/gtest.h>

#include "uve/debug/i_logger_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/utilities/i_timer_uve.h"

// These hand-written fakes exist to prove that EngineServicesUVE (and, by
// extension, anything that consumes ILoggerUVE/ITimerUVE/IEventSystemUVE)
// works against ANY conforming implementation, independent of the concrete
// LoggerUVE/TimerUVE/EventSystemUVE classes used by EngineCoreUVE — this is
// the whole point of introducing the interfaces.

namespace UVE::Core::Tests {
namespace {

class FakeLoggerUVE final : public Debug::ILoggerUVE {
public:
    void Init(Debug::LogLevelUVE) override {}
    void Shutdown() override {}
    void AddSink(std::unique_ptr<Debug::ILogSinkUVE>) override {}
    void SetMinLevel(Debug::LogLevelUVE level) override { minLevel = level; }
    [[nodiscard]] Debug::LogLevelUVE GetMinLevel() const override { return minLevel; }
    void LogFormatted(Debug::LogLevelUVE, std::string_view, const char*, int, std::string) override {}

    Debug::LogLevelUVE minLevel = Debug::LogLevelUVE::Trace;
};

class FakeTimerUVE final : public Utilities::ITimerUVE {
public:
    void Tick() override { ++tickCount; }
    [[nodiscard]] double GetDeltaTimeUVE() const override { return 0.0; }
    [[nodiscard]] float GetDeltaTimeFloatUVE() const override { return 0.0F; }
    [[nodiscard]] double GetTotalTimeUVE() const override { return 0.0; }
    void Reset() override {}
    void SetMaxDeltaTimeUVE(double) override {}
    void SetFixedTimestepUVE(double) override {}
    Utilities::FixedStepResultUVE AdvanceFixedStepUVE() override { return {}; }

    int tickCount = 0;
};

class FakeEventSystemUVE final : public Events::IEventSystemUVE {
public:
    void Unsubscribe(const Events::EventSubscriptionUVE&) override {}
    void DispatchQueuedUVE() override { ++dispatchCount; }
    void Clear() override {}

    int dispatchCount = 0;

protected:
    std::uint64_t SubscribeErased(std::type_index, std::function<void(const void*)>) override { return 1; }
    void PublishErased(std::type_index, const void*) override {}
    void QueueErased(std::type_index, Events::EventPriorityUVE, std::function<void()>) override {}
};

TEST(EngineServicesUVETest, Accessors_ReturnExactSameInstancesPassedIn) {
    FakeLoggerUVE logger;
    FakeTimerUVE timer;
    FakeEventSystemUVE eventSystem;

    const EngineServicesUVE services(logger, timer, eventSystem);

    EXPECT_EQ(&services.GetLoggerUVE(), &logger);
    EXPECT_EQ(&services.GetTimerUVE(), &timer);
    EXPECT_EQ(&services.GetEventSystemUVE(), &eventSystem);
}

TEST(EngineServicesUVETest, Accessors_ProveInterfacesAreGenuinelySubstitutable) {
    FakeLoggerUVE logger;
    FakeTimerUVE timer;
    FakeEventSystemUVE eventSystem;
    const EngineServicesUVE services(logger, timer, eventSystem);

    services.GetTimerUVE().Tick();
    services.GetEventSystemUVE().DispatchQueuedUVE();

    EXPECT_EQ(timer.tickCount, 1);
    EXPECT_EQ(eventSystem.dispatchCount, 1);
}

} // namespace
} // namespace UVE::Core::Tests
