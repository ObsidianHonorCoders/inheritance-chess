/// @file      event_system.h
/// @namespace Chess
/// @brief     Chess Event System (CES) - The Notifier.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Provides event-driven architecture for board state changes,
///            enabling components to react to moves, captures, and game events.

#ifndef ICHESS_CES_EVENT_SYSTEM
#define ICHESS_CES_EVENT_SYSTEM

#include "grid.h"
#include "move.h"
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>

namespace Chess
{

  /// @enum  EventType
  /// @brief Types of chess events that can be emitted.
  enum class EventType
  {
      MOVE_EXECUTED,          ///< A move was successfully executed
      PIECE_CAPTURED,         ///< A piece was captured
      POSITION_UPDATED,       ///< Board position was updated
      GAME_STARTED,           ///< A new game was started
      GAME_ENDED,             ///< Game ended (checkmate, stalemate, draw)
      TURN_CHANGED,           ///< Current player turn changed
      CASTLING_PERFORMED,     ///< Castling move was executed
      EN_PASSANT_CAPTURE,     ///< En passant capture was executed
      PROMOTION_OCCURRED,     ///< Pawn promotion occurred
      CHECK_DETECTED,         ///< King is in check
      CHECKMATE_DETECTED,     ///< Checkmate detected
      STALEMATE_DETECTED,     ///< Stalemate detected
      DRAW_DETECTED           ///< Draw condition detected
  };

  /// @struct  EventData
  /// @brief   Base structure for event data.
  struct EventData
  {
      EventType type;                    ///< Type of event
      const Grid* grid;                  ///< Current board state (pointer to avoid copying)
      Move move;                         ///< Move that triggered the event (if applicable)
      Color current_turn;               ///< Current player turn
      std::chrono::system_clock::time_point timestamp; ///< When event occurred
      
      EventData(EventType t, const Grid* g, const Move& m, Color turn)
          : type(t), grid(g), move(m), current_turn(turn), 
            timestamp(std::chrono::system_clock::now()) {}
  };

  /// @using   EventListener
  /// @brief   Function type for event listeners.
  using EventListener = std::function<void(const EventData&)>;

  /// @class   ChessEventSystem
  /// @brief   Manages event subscription and emission for chess events.
  /// @details Implements observer pattern for loose coupling between components.
  ///
  /// ### Mutation safety policy (single-threaded)
  ///
  /// The following guarantees apply when subscribe(), unsubscribe(), or
  /// a new subscribe() is called from inside a running listener:
  ///
  /// - **Unsubscribe self**: the listener is removed from the live index
  ///   immediately; no further calls to it will occur for subsequent events,
  ///   but the current invocation completes normally.
  ///
  /// - **Unsubscribe another listener**: the target is removed immediately.
  ///   If it has not yet been dispatched in the current emit cycle it will
  ///   be skipped; if it has already been dispatched it is simply deregistered.
  ///
  /// - **Subscribe a new listener during emit**: the new listener is NOT
  ///   invoked in the current emit cycle; it becomes active for the next call.
  ///
  /// - **Exception in a listener**: caught and logged to stderr; remaining
  ///   listeners for the same event continue to receive the dispatch.
  ///
  /// - **Thread safety**: this class is NOT thread-safe.  External
  ///   synchronization (mutex, strand, etc.) is required when
  ///   subscribe/unsubscribe/emit_event are called from multiple threads.
  class ChessEventSystem
  {
    public:
      /// @brief Constructor.
      ChessEventSystem() = default;

      /// @brief Destructor.
      virtual ~ChessEventSystem() = default;

      /// @brief Subscribe to specific event type.
      /// @param type Event type to listen for.
      /// @param listener Function to call when event occurs.
      /// @return Subscription ID for unsubscribing.
      size_t subscribe(EventType type, EventListener listener);

      /// @brief Unsubscribe from events.
      /// @param subscription_id ID returned from subscribe().
      /// @return True if successfully unsubscribed.
      bool unsubscribe(size_t subscription_id);

      /// @brief Emit an event to all subscribers.
      /// @param event Event data to broadcast.
      void emit_event(const EventData& event);

      /// @brief Emit a move executed event.
      /// @param grid Current board state.
      /// @param move Move that was executed.
      /// @param turn Current player turn.
      void emit_move_executed(const Grid& grid, const Move& move, Color turn);

      /// @brief Emit a piece captured event.
      /// @param grid Current board state.
      /// @param move Capture move.
      /// @param turn Current player turn.
      void emit_piece_captured(const Grid& grid, const Move& move, Color turn);

      /// @brief Emit a position updated event.
      /// @param grid Current board state.
      /// @param turn Current player turn.
      void emit_position_updated(const Grid& grid, Color turn);

      /// @brief Emit a game started event.
      /// @param grid Initial board state.
      void emit_game_started(const Grid& grid);

      /// @brief Emit a game ended event.
      /// @param grid Final board state.
      /// @param reason Reason for game ending.
      /// @param turn Current player turn.
      void emit_game_ended(const Grid& grid, const std::string& reason, Color turn);

      /// @brief Emit a turn changed event.
      /// @param grid Current board state.
      /// @param new_turn New player turn.
      void emit_turn_changed(const Grid& grid, Color new_turn);

      /// @brief Emit a check detected event.
      /// @param grid Current board state.
      /// @param checked_color Color of king in check.
      void emit_check_detected(const Grid& grid, Color checked_color);

      /// @brief Emit a checkmate detected event.
      /// @param grid Current board state.
      /// @param checkmated_color Color of checkmated king.
      void emit_checkmate_detected(const Grid& grid, Color checkmated_color);

      /// @brief Emit a stalemate detected event.
      /// @param grid Current board state.
      /// @param stalemate_color Color that cannot move.
      void emit_stalemate_detected(const Grid& grid, Color stalemate_color);

      /// @brief Get number of active subscribers.
      /// @return Total number of subscribers.
      size_t get_subscriber_count() const;

      /// @brief Get subscribers for specific event type.
      /// @param type Event type.
      /// @return Number of subscribers for this type.
      size_t get_subscriber_count(EventType type) const;

    private:
      /// @struct  Subscription
      /// @brief   Stores subscription information.
      struct Subscription
      {
          size_t id;
          EventType type;
          EventListener listener;
          
          Subscription(size_t i, EventType t, EventListener l)
              : id(i), type(t), listener(l) {}
      };

      std::vector<Subscription> subscriptions_; ///< All active subscriptions
      std::unordered_map<EventType, std::vector<size_t>> type_to_subscriptions_; ///< Index by event type
        std::unordered_map<size_t, size_t> id_to_index_; ///< Direct mapping from subscription ID to subscriptions_ index
      size_t next_subscription_id_ = 1; ///< Next available subscription ID
  };

} // namespace Chess

#endif // ICHESS_CES_EVENT_SYSTEM
