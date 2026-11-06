/// @file      event_system.cpp
/// @namespace Chess
/// @brief     Chess Event System (CES) - Event Management Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implementation of event-driven architecture for chess events.

#include "event_system.h"
#include <algorithm>
#include <iostream>

namespace Chess
{

  /// @brief Register a listener for an event type.
  /// @param type Event category to subscribe to.
  /// @param listener Callback invoked whenever that event is emitted.
  /// @return Unique subscription identifier used for unsubscribe().
  size_t ChessEventSystem::subscribe(EventType type, EventListener listener)
  {
    size_t subscription_id = next_subscription_id_++;

    // Create subscription
    subscriptions_.emplace_back(subscription_id, type, listener);

    // Store direct ID -> index mapping for O(1) lookup
    id_to_index_[subscription_id] = subscriptions_.size() - 1;

    // Add to type index
    type_to_subscriptions_[type].push_back(subscription_id);

    return subscription_id;
  }

  /// @brief Remove a listener subscription.
  /// @param subscription_id Identifier returned from subscribe().
  /// @return True when an active subscription was removed, false otherwise.
  bool ChessEventSystem::unsubscribe(size_t subscription_id)
  {
    auto id_it = id_to_index_.find(subscription_id);
    if (id_it == id_to_index_.end())
    {
      return false; // Not found
    }

    const size_t remove_index = id_it->second;
    const EventType type = subscriptions_[remove_index].type;

    // Remove from type index
    auto& type_subs = type_to_subscriptions_[type];
    type_subs.erase(std::remove(type_subs.begin(), type_subs.end(), subscription_id), type_subs.end());

    // Remove from direct map
    id_to_index_.erase(id_it);

    // Keep vector compact by moving last element into removed slot.
    const size_t last_index = subscriptions_.size() - 1;
    if (remove_index != last_index)
    {
      subscriptions_[remove_index] = std::move(subscriptions_[last_index]);
      id_to_index_[subscriptions_[remove_index].id] = remove_index;
    }

    subscriptions_.pop_back();

    return true;
  }

  /// @brief Broadcast an event to all subscribers of the same EventType.
  /// @param event Event payload to dispatch.
  /// @details Listeners are looked up by event type and invoked in subscription
  ///          order. Exceptions thrown by listeners are caught and logged to stderr.
  ///
  /// Mutation safety (single-threaded re-entrancy policy):
  ///  - The active subscription ID list is snapshotted before iteration begins.
  ///  - Listeners that unsubscribe themselves or others during dispatch are removed
  ///    from the live index immediately; any ID no longer present in id_to_index_
  ///    after their turn comes up is silently skipped.
  ///  - New listeners registered by a callback during dispatch are not called in
  ///    the current cycle; they become active for the next emit_event call.
  ///  - This class is NOT thread-safe. External synchronization is required if
  ///    subscribe/unsubscribe/emit are called from multiple threads.
  void ChessEventSystem::emit_event(const EventData& event)
  {
    auto it = type_to_subscriptions_.find(event.type);
    if (it == type_to_subscriptions_.end())
    {
      return;
    }

    // Snapshot the active IDs before dispatch so that subscribe/unsubscribe
    // calls inside a listener do not invalidate the current iteration.
    const std::vector<size_t> ids_snapshot = it->second;

    for (const size_t subscription_id : ids_snapshot)
    {
      // Re-check presence: the listener may have been unsubscribed by an
      // earlier listener in this same dispatch cycle.
      const auto idx_it = id_to_index_.find(subscription_id);
      if (idx_it == id_to_index_.end())
      {
        continue;
      }

      try
      {
        subscriptions_[idx_it->second].listener(event);
      }
      catch (const std::exception& e)
      {
        std::cerr << "[CES] Event listener error: " << e.what() << '\n';
      }
    }
  }

  /// @brief Emit MOVE_EXECUTED after a legal move is applied.
  /// @param grid Current board state snapshot.
  /// @param move Move that was just executed.
  /// @param turn Side to move after emission context.
  void ChessEventSystem::emit_move_executed(const Grid& grid, const Move& move, Color turn)
  {
    EventData event(EventType::MOVE_EXECUTED, &grid, move, turn);
    emit_event(event);
  }

  /// @brief Emit PIECE_CAPTURED after a capture move.
  /// @param grid Current board state snapshot.
  /// @param move Capture move that removed an opponent piece.
  /// @param turn Side to move after emission context.
  void ChessEventSystem::emit_piece_captured(const Grid& grid, const Move& move, Color turn)
  {
    EventData event(EventType::PIECE_CAPTURED, &grid, move, turn);
    emit_event(event);
  }

  /// @brief Emit POSITION_UPDATED for non-move board updates.
  /// @param grid Current board state snapshot.
  /// @param turn Side to move in the updated position.
  void ChessEventSystem::emit_position_updated(const Grid& grid, Color turn)
  {
    Move dummy_move; // Empty move for position updates
    EventData event(EventType::POSITION_UPDATED, &grid, dummy_move, turn);
    emit_event(event);
  }

  /// @brief Emit GAME_STARTED when a new game is initialized.
  /// @param grid Initial game position.
  void ChessEventSystem::emit_game_started(const Grid& grid)
  {
    Move dummy_move; // Empty move for game start
    EventData event(EventType::GAME_STARTED, &grid, dummy_move, Color::WHITE);
    emit_event(event);
  }

  /// @brief Emit GAME_ENDED when terminal state is reached.
  /// @param grid Final board state.
  /// @param reason Human-readable reason string.
  /// @param turn Side associated with terminal-state context.
  void ChessEventSystem::emit_game_ended(const Grid& grid, const std::string& reason, Color turn)
  {
    (void)reason; // reason is carried by the subscriber; no raw I/O side effects here.
    Move dummy_move;
    EventData event(EventType::GAME_ENDED, &grid, dummy_move, turn);
    emit_event(event);
  }

  /// @brief Emit TURN_CHANGED after active color switches.
  /// @param grid Current board state snapshot.
  /// @param new_turn Color whose turn it is now.
  void ChessEventSystem::emit_turn_changed(const Grid& grid, Color new_turn)
  {
    Move dummy_move; // Empty move for turn change
    EventData event(EventType::TURN_CHANGED, &grid, dummy_move, new_turn);
    emit_event(event);
  }

  /// @brief Emit CHECK_DETECTED when a king is under attack.
  /// @param grid Current board state snapshot.
  /// @param checked_color Color of the king in check.
  void ChessEventSystem::emit_check_detected(const Grid& grid, Color checked_color)
  {
    Move dummy_move; // Empty move for check detection
    EventData event(EventType::CHECK_DETECTED, &grid, dummy_move, checked_color);
    emit_event(event);
  }

  /// @brief Emit CHECKMATE_DETECTED when side to move is checkmated.
  /// @param grid Current board state snapshot.
  /// @param checkmated_color Color of the checkmated king.
  void ChessEventSystem::emit_checkmate_detected(const Grid& grid, Color checkmated_color)
  {
    Move dummy_move; // Empty move for checkmate detection
    EventData event(EventType::CHECKMATE_DETECTED, &grid, dummy_move, checkmated_color);
    emit_event(event);
  }

  /// @brief Emit STALEMATE_DETECTED for no-legal-move non-check positions.
  /// @param grid Current board state snapshot.
  /// @param stalemate_color Color that cannot make a legal move.
  void ChessEventSystem::emit_stalemate_detected(const Grid& grid, Color stalemate_color)
  {
    Move dummy_move; // Empty move for stalemate detection
    EventData event(EventType::STALEMATE_DETECTED, &grid, dummy_move, stalemate_color);
    emit_event(event);
  }

  /// @brief Count all active subscriptions.
  /// @return Total number of listeners across all event types.
  size_t ChessEventSystem::get_subscriber_count() const
  {
    return subscriptions_.size();
  }

  /// @brief Count active subscriptions for one event type.
  /// @param type Event type to inspect.
  /// @return Number of listeners registered for that type.
  size_t ChessEventSystem::get_subscriber_count(EventType type) const
  {
    auto it = type_to_subscriptions_.find(type);
    return (it != type_to_subscriptions_.end()) ? it->second.size() : 0;
  }

} // namespace Chess


