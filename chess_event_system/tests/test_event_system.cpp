/// @file      test_event_system.cpp
/// @namespace Chess
/// @brief     Unit tests for Chess Event System (CES).
/// @author    Calileus
/// @date      2026-05-08
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Covers subscription lifecycle, listener dispatch, exception
///            isolation, and mutation-during-emit safety contracts.

#include <gtest/gtest.h>

#include "../include/event_system.h"
#include "../../chess_common_interface/include/grid.h"

using namespace Chess;

/// @class EventSystemTest
/// @brief Fixture that provides a fresh ChessEventSystem and an initialized
///        Grid for every test case.
class EventSystemTest : public ::testing::Test
{
  protected:
    ChessEventSystem ces_;
    Grid             grid_;

    void SetUp() override { grid_.initialize_standard_position(); }
};

// ---------------------------------------------------------------------------
// Subscription lifecycle
// ---------------------------------------------------------------------------

/// @test Subscribing a listener and emitting the matching type invokes it.
TEST_F(EventSystemTest, SubscribeAndReceiveEvent)
{
  int call_count = 0;
  ces_.subscribe(EventType::MOVE_EXECUTED, [&](const EventData&) { ++call_count; });

  Move m;
  ces_.emit_move_executed(grid_, m, Color::WHITE);

  EXPECT_EQ(call_count, 1);
}

/// @test Emitting a type that no listener was subscribed to does not crash.
TEST_F(EventSystemTest, EmitWithNoSubscribersDoesNotCrash)
{
  EXPECT_NO_THROW(ces_.emit_game_started(grid_));
  EXPECT_NO_THROW(ces_.emit_turn_changed(grid_, Color::WHITE));
}

/// @test Unsubscribing stops future delivery of that event.
TEST_F(EventSystemTest, UnsubscribeStopsDelivery)
{
  int call_count = 0;
  const size_t id = ces_.subscribe(EventType::GAME_STARTED, [&](const EventData&) { ++call_count; });

  ces_.emit_game_started(grid_);
  ASSERT_EQ(call_count, 1);

  EXPECT_TRUE(ces_.unsubscribe(id));

  ces_.emit_game_started(grid_);
  EXPECT_EQ(call_count, 1); // Must not increment after unsubscribe
}

/// @test Unsubscribing an unknown ID returns false without crashing.
TEST_F(EventSystemTest, UnsubscribeUnknownIdReturnsFalse)
{
  EXPECT_FALSE(ces_.unsubscribe(9999));
}

/// @test Unsubscribing the same ID twice: first returns true, second false.
TEST_F(EventSystemTest, DoubleUnsubscribeReturnsFalse)
{
  const size_t id = ces_.subscribe(EventType::GAME_ENDED, [](const EventData&) {});
  EXPECT_TRUE(ces_.unsubscribe(id));
  EXPECT_FALSE(ces_.unsubscribe(id));
}

// ---------------------------------------------------------------------------
// Multi-listener dispatch
// ---------------------------------------------------------------------------

/// @test Multiple listeners on the same type all receive the event.
TEST_F(EventSystemTest, MultipleListenersSameTypeAllReceive)
{
  int count_a = 0;
  int count_b = 0;
  ces_.subscribe(EventType::TURN_CHANGED, [&](const EventData&) { ++count_a; });
  ces_.subscribe(EventType::TURN_CHANGED, [&](const EventData&) { ++count_b; });

  ces_.emit_turn_changed(grid_, Color::BLACK);

  EXPECT_EQ(count_a, 1);
  EXPECT_EQ(count_b, 1);
}

/// @test Listeners on different types do not cross-fire.
TEST_F(EventSystemTest, ListenersForDifferentTypesAreIndependent)
{
  int move_count = 0;
  int game_count = 0;
  ces_.subscribe(EventType::MOVE_EXECUTED, [&](const EventData&) { ++move_count; });
  ces_.subscribe(EventType::GAME_ENDED,    [&](const EventData&) { ++game_count; });

  ces_.emit_game_started(grid_); // Neither MOVE_EXECUTED nor GAME_ENDED

  EXPECT_EQ(move_count, 0);
  EXPECT_EQ(game_count, 0);
}

// ---------------------------------------------------------------------------
// Exception isolation
// ---------------------------------------------------------------------------

/// @test A listener that throws does not prevent later listeners from running.
TEST_F(EventSystemTest, ThrowingListenerDoesNotAbortSubsequentDelivery)
{
  int count_after = 0;
  ces_.subscribe(EventType::MOVE_EXECUTED, [](const EventData&)
  {
    throw std::runtime_error("intentional listener failure");
  });
  ces_.subscribe(EventType::MOVE_EXECUTED, [&](const EventData&) { ++count_after; });

  Move m;
  EXPECT_NO_THROW(ces_.emit_move_executed(grid_, m, Color::WHITE));
  EXPECT_EQ(count_after, 1);
}

// ---------------------------------------------------------------------------
// Mutation-during-emit safety
// ---------------------------------------------------------------------------

/// @test A listener that unsubscribes itself does not crash; its peer is
///       still delivered (they were both in the snapshot for that cycle).
TEST_F(EventSystemTest, UnsubscribeSelfDuringEmitDoesNotCrash)
{
  size_t id_self = 0;
  int    peer_count = 0;

  id_self = ces_.subscribe(EventType::TURN_CHANGED, [&](const EventData&)
  {
    ces_.unsubscribe(id_self);
  });
  ces_.subscribe(EventType::TURN_CHANGED, [&](const EventData&) { ++peer_count; });

  EXPECT_NO_THROW(ces_.emit_turn_changed(grid_, Color::WHITE));

  EXPECT_EQ(ces_.get_subscriber_count(), 1); // Only peer remains
  EXPECT_EQ(peer_count, 1);
}

/// @test A listener that unsubscribes a not-yet-dispatched peer causes that
///       peer to be skipped in the current cycle and absent in the next.
TEST_F(EventSystemTest, UnsubscribePendingListenerDuringEmitSkipsIt)
{
  size_t id_b     = 0;
  int    count_b  = 0;

  // A is subscribed first; it fires before B and removes B.
  ces_.subscribe(EventType::GAME_STARTED, [&](const EventData&)
  {
    ces_.unsubscribe(id_b);
  });
  id_b = ces_.subscribe(EventType::GAME_STARTED, [&](const EventData&) { ++count_b; });

  ces_.emit_game_started(grid_);
  EXPECT_EQ(count_b, 0); // B was removed before its turn in this cycle

  ces_.emit_game_started(grid_);
  EXPECT_EQ(count_b, 0); // B absent for subsequent dispatches too
}

/// @test A listener that unsubscribes an already-dispatched peer: the peer ran
///       normally this cycle; removal takes effect for the next cycle.
TEST_F(EventSystemTest, UnsubscribeAlreadyDispatchedListenerTakesEffectNextCycle)
{
  size_t id_a    = 0;
  int    count_a = 0;

  id_a = ces_.subscribe(EventType::GAME_STARTED, [&](const EventData&) { ++count_a; });
  // B is subscribed second; it fires after A and then removes A.
  ces_.subscribe(EventType::GAME_STARTED, [&](const EventData&)
  {
    ces_.unsubscribe(id_a);
  });

  ces_.emit_game_started(grid_);
  EXPECT_EQ(count_a, 1); // A already ran before B removed it

  ces_.emit_game_started(grid_);
  EXPECT_EQ(count_a, 1); // A is gone now
}

/// @test A listener that subscribes a new listener during emit: the new
///       listener is NOT invoked in the current cycle but is active next time.
TEST_F(EventSystemTest, SubscribeDuringEmitDeferredToNextDispatch)
{
  int  deferred_count = 0;
  bool inner_registered = false;

  ces_.subscribe(EventType::GAME_ENDED, [&](const EventData&)
  {
    if (!inner_registered)
    {
      inner_registered = true;
      ces_.subscribe(EventType::GAME_ENDED, [&](const EventData&) { ++deferred_count; });
    }
  });

  ces_.emit_game_ended(grid_, "checkmate", Color::WHITE);
  EXPECT_EQ(deferred_count, 0); // Inner listener not yet active

  ces_.emit_game_ended(grid_, "checkmate", Color::WHITE);
  EXPECT_EQ(deferred_count, 1); // Active from second dispatch onward
}

// ---------------------------------------------------------------------------
// Subscriber count API
// ---------------------------------------------------------------------------

/// @test get_subscriber_count() (total) tracks additions and removals.
TEST_F(EventSystemTest, GetSubscriberCountTotal)
{
  EXPECT_EQ(ces_.get_subscriber_count(), 0u);

  ces_.subscribe(EventType::MOVE_EXECUTED, [](const EventData&) {});
  ces_.subscribe(EventType::MOVE_EXECUTED, [](const EventData&) {});
  const size_t id = ces_.subscribe(EventType::GAME_STARTED, [](const EventData&) {});

  EXPECT_EQ(ces_.get_subscriber_count(), 3u);

  ces_.unsubscribe(id);
  EXPECT_EQ(ces_.get_subscriber_count(), 2u);
}

/// @test get_subscriber_count(EventType) returns per-type totals.
TEST_F(EventSystemTest, GetSubscriberCountByType)
{
  ces_.subscribe(EventType::CHECK_DETECTED,    [](const EventData&) {});
  ces_.subscribe(EventType::CHECK_DETECTED,    [](const EventData&) {});
  ces_.subscribe(EventType::STALEMATE_DETECTED,[](const EventData&) {});

  EXPECT_EQ(ces_.get_subscriber_count(EventType::CHECK_DETECTED),    2u);
  EXPECT_EQ(ces_.get_subscriber_count(EventType::STALEMATE_DETECTED),1u);
  EXPECT_EQ(ces_.get_subscriber_count(EventType::MOVE_EXECUTED),     0u);
}

// ---------------------------------------------------------------------------
// Convenience emit methods fire correct event type
// ---------------------------------------------------------------------------

/// @test emit_move_executed delivers EventType::MOVE_EXECUTED.
TEST_F(EventSystemTest, EmitMoveExecutedFiresCorrectType)
{
  EventType received = EventType::GAME_STARTED;
  ces_.subscribe(EventType::MOVE_EXECUTED, [&](const EventData& e) { received = e.type; });

  Move m;
  ces_.emit_move_executed(grid_, m, Color::WHITE);

  EXPECT_EQ(received, EventType::MOVE_EXECUTED);
}

/// @test emit_game_started delivers EventType::GAME_STARTED.
TEST_F(EventSystemTest, EmitGameStartedFiresCorrectType)
{
  EventType received = EventType::MOVE_EXECUTED;
  ces_.subscribe(EventType::GAME_STARTED, [&](const EventData& e) { received = e.type; });

  ces_.emit_game_started(grid_);

  EXPECT_EQ(received, EventType::GAME_STARTED);
}

/// @test emit_game_ended delivers EventType::GAME_ENDED.
TEST_F(EventSystemTest, EmitGameEndedFiresCorrectType)
{
  EventType received = EventType::MOVE_EXECUTED;
  ces_.subscribe(EventType::GAME_ENDED, [&](const EventData& e) { received = e.type; });

  ces_.emit_game_ended(grid_, "stalemate", Color::BLACK);

  EXPECT_EQ(received, EventType::GAME_ENDED);
}

/// @test emit_turn_changed carries the new active color in EventData.
TEST_F(EventSystemTest, EmitTurnChangedCarriesCorrectColor)
{
  Color received = Color::WHITE;
  ces_.subscribe(EventType::TURN_CHANGED, [&](const EventData& e) { received = e.current_turn; });

  ces_.emit_turn_changed(grid_, Color::BLACK);

  EXPECT_EQ(received, Color::BLACK);
}

/// @test emit_check_detected delivers EventType::CHECK_DETECTED.
TEST_F(EventSystemTest, EmitCheckDetectedFiresCorrectType)
{
  EventType received = EventType::MOVE_EXECUTED;
  ces_.subscribe(EventType::CHECK_DETECTED, [&](const EventData& e) { received = e.type; });

  ces_.emit_check_detected(grid_, Color::WHITE);

  EXPECT_EQ(received, EventType::CHECK_DETECTED);
}

/// @test EventData::grid pointer matches the address of the grid passed to emit.
TEST_F(EventSystemTest, EventDataCarriesGridPointerIdentity)
{
  const Grid* received_grid = nullptr;
  ces_.subscribe(EventType::GAME_STARTED, [&](const EventData& e) { received_grid = e.grid; });

  ces_.emit_game_started(grid_);

  EXPECT_EQ(received_grid, &grid_);
}
