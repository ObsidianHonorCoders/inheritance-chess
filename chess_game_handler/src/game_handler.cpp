/// @file      game_handler.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Game Handler (CGH).
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements game orchestration and lifecycle management.

#include "game_handler.h"
#include <ctime>
#include <iostream>
#include <algorithm>
#include <sstream>

namespace Chess
{

  namespace
  {
    // Threefold repetition compares board placement, side to move, castling rights,
    // and en passant target; halfmove/fullmove counters are ignored.
    std::string fen_to_repetition_key(const std::string& fen)
    {
      std::istringstream iss(fen);
      std::string board;
      std::string turn;
      std::string castling;
      std::string en_passant;
      if (!(iss >> board >> turn >> castling >> en_passant))
      {
        return fen;
      }
      return board + " " + turn + " " + castling + " " + en_passant;
    }
  } // namespace

  ChessGameHandler::ChessGameHandler()
      : board_manager_(std::make_unique<ChessBoardManager>()), pieces_logic_(std::make_unique<ChessPiecesLogic>()),
        translation_unit_(std::make_unique<ChessTranslationUnit>()), event_system_(std::make_unique<ChessEventSystem>())
  {
  }

  /// @brief Start new game.
  void ChessGameHandler::start_new_game()
  {
    current_grid_.initialize_standard_position();
    session_.game_id = "game_" + std::to_string(std::time(nullptr));
    session_.move_history.clear();
    session_.state      = GameState::ONGOING;
    session_.white_time = std::chrono::milliseconds(600000); // 10 minutes
    session_.black_time = std::chrono::milliseconds(600000); // 10 minutes
    timer_running_      = false;
    timer_start_time_.reset();
    clear_draw_offers();
    reset_position_history();

    // Emit game started event
    event_system_->emit_game_started(current_grid_);
  }

  /// @brief Load game from fen.
  void ChessGameHandler::load_game_from_fen(const std::string& fen)
  {
    current_grid_ = translation_unit_->fen_to_internal(fen);
    session_.move_history.clear();
    session_.state = GameState::ONGOING;
    clear_draw_offers();
    reset_position_history();
  }

  /// @brief Apply move.
  bool ChessGameHandler::make_move(const std::string& algebraic_move)
  {
    try
    {
      Move move = translation_unit_->algebraic_to_move(algebraic_move, current_grid_);
      return make_move(move);
    }
    catch (const std::exception& e)
    {
      std::cerr << "Invalid move: " << algebraic_move << " - " << e.what() << std::endl;
      return false;
    }
  }

  /// @brief Apply move.
  bool ChessGameHandler::make_move(const Move& move)
  {
    if (session_.state != GameState::ONGOING)
    {
      return false;
    }

    // Validate move using board manager
    if (!board_manager_->validate_move(current_grid_, move))
    {
      return false;
    }

    // Execute move on grid
    if (!execute_move_on_grid(move))
    {
      return false;
    }

    // Add to move history
    session_.move_history.push_back(move);
    clear_draw_offers();

    // Emit move executed event
    event_system_->emit_move_executed(current_grid_, move, current_grid_.current_turn);

    // Switch turns
    Color old_turn = current_grid_.current_turn;
    current_grid_.switch_turn();

    // If a clock is running, hand over timing to the opponent.
    if (timer_running_)
    {
      stop_timer(old_turn);
      start_timer(current_grid_.current_turn);
    }

    // Update fullmove counter (increments after Black's move)
    if (old_turn == Color::BLACK)
    {
      current_grid_.flags.fullmove_number++;
    }

    // Emit turn changed event
    event_system_->emit_turn_changed(current_grid_, current_grid_.current_turn);

    // Record the resulting position for threefold-repetition checks.
    record_current_position();

    // Check for game end conditions
    update_game_state();

    return true;
  }

  /// @brief Get current fen.
  std::string ChessGameHandler::get_current_fen() const { return translation_unit_->internal_to_fen(current_grid_); }

  /// @brief Get legal moves.
  std::vector<Move> ChessGameHandler::get_legal_moves() const { return get_legal_moves(current_grid_); }

  std::vector<Move> ChessGameHandler::get_legal_moves(const Grid& grid) const
  {
    // Delegate to board manager for consistent legal move generation
    return board_manager_->get_legal_moves(grid);
  }

  /// @brief Get game state.
  GameState ChessGameHandler::get_game_state() const { return session_.state; }

  bool ChessGameHandler::resign(Color color)
  {
    const GameState resigned_state = (color == Color::WHITE) ? GameState::WHITE_RESIGNED : GameState::BLACK_RESIGNED;
    const char* reason = (color == Color::WHITE) ? "White resigned" : "Black resigned";
    return apply_terminal_state(resigned_state, reason);
  }

  bool ChessGameHandler::offer_draw(Color color)
  {
    if (session_.state != GameState::ONGOING)
    {
      return false;
    }

    if (color == Color::WHITE)
    {
      draw_offered_by_white_ = true;
    }
    else
    {
      draw_offered_by_black_ = true;
    }
    return true;
  }

  bool ChessGameHandler::accept_draw(Color color)
  {
    if (session_.state != GameState::ONGOING)
    {
      return false;
    }

    const bool opponent_offered = (color == Color::WHITE) ? draw_offered_by_black_ : draw_offered_by_white_;
    if (!opponent_offered)
    {
      return false;
    }

    return apply_terminal_state(GameState::DRAW_BY_AGREEMENT, "Draw agreed by both players");
  }

  bool ChessGameHandler::is_draw_offered_by(Color color) const
  {
    return (color == Color::WHITE) ? draw_offered_by_white_ : draw_offered_by_black_;
  }

  bool ChessGameHandler::declare_timeout(Color color)
  {
    const GameState timeout_state = (color == Color::WHITE) ? GameState::WHITE_TIMEOUT : GameState::BLACK_TIMEOUT;
    const char* reason = (color == Color::WHITE) ? "White timeout" : "Black timeout";
    return apply_terminal_state(timeout_state, reason);
  }

  void ChessGameHandler::start_timer(Color color)
  {
    if (timer_running_)
    {
      if (running_timer_color_ == color)
      {
        return;
      }
      stop_timer(running_timer_color_);
    }

    timer_running_ = true;
    running_timer_color_ = color;
    timer_start_time_ = std::chrono::steady_clock::now();
  }

  /// @brief Stop timer.
  void ChessGameHandler::stop_timer(Color color)
  {
    if (!timer_running_ || !timer_start_time_.has_value() || running_timer_color_ != color)
    {
      return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *timer_start_time_);

    auto& remaining = (color == Color::WHITE) ? session_.white_time : session_.black_time;
    const bool timed_out = elapsed >= remaining;
    remaining = timed_out ? std::chrono::milliseconds(0) : (remaining - elapsed);

    timer_running_ = false;
    timer_start_time_.reset();

    if (timed_out)
    {
      declare_timeout(color);
    }
  }

  /// @brief Get time remaining.
  std::chrono::milliseconds ChessGameHandler::get_time_remaining(Color color) const
  {
    auto remaining = (color == Color::WHITE) ? session_.white_time : session_.black_time;

    if (timer_running_ && timer_start_time_.has_value() && running_timer_color_ == color)
    {
      const auto now = std::chrono::steady_clock::now();
      const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *timer_start_time_);
      remaining = (elapsed >= remaining) ? std::chrono::milliseconds(0) : (remaining - elapsed);
    }

    return remaining;
  }

  /// @brief Implement display board.
  void ChessGameHandler::display_board() const
  {
    std::cout << translation_unit_->render_board_ascii(current_grid_);
  }

  /// @brief Get current grid.
  const Grid& ChessGameHandler::get_current_grid() const { return current_grid_; }

  /// @brief Get event system.
  ChessEventSystem& ChessGameHandler::get_event_system() { return *event_system_; }

  bool ChessGameHandler::execute_move_on_grid(const Move& move)
  {
    // Get piece at start position
    auto piece_opt = current_grid_.get_piece(move.start_pos);
    if (!piece_opt.has_value())
    {
      return false;
    }

    // Handle special moves
    if (move.flags == SpecialFlags::CASTLE_KINGSIDE || move.flags == SpecialFlags::CASTLE_QUEENSIDE)
    {
      return execute_castling(move);
    }
    else if (move.flags == SpecialFlags::EN_PASSANT)
    {
      return execute_en_passant(move);
    }
    else if (move.flags == SpecialFlags::PROMOTION)
    {
      return execute_promotion(move);
    }

    // Regular move
    return execute_regular_move(move);
  }

  /// @brief Implement execute regular move.
  bool ChessGameHandler::execute_regular_move(const Move& move)
  {
    // Get piece properties at start position BEFORE clearing
    const auto& piece_opt = current_grid_.get_piece(move.start_pos);
    if (!piece_opt.has_value())
    {
      return false; // No piece at source
    }

    const auto& piece_props = piece_opt.value();

    // Clear start position
    current_grid_.clear_square(move.start_pos);

    // Set piece at end position with updated properties
    current_grid_.set_piece(
        move.end_pos, Chess::PieceProperties{piece_props.type, piece_props.color, move.end_pos, true, false});

    return true;
  }

  /// @brief Implement execute castling.
  bool ChessGameHandler::execute_castling(const Move& move)
  {
    // Get piece properties at start position (should be king)
    const auto& king_opt = current_grid_.get_piece(move.start_pos);
    if (!king_opt.has_value() || king_opt->type != Chess::PieceType::KING)
    {
      return false; // No king at start position
    }

    const auto& king_props = king_opt.value();

    // Clear king's starting position
    current_grid_.clear_square(move.start_pos);

    // Move king to end position
    current_grid_.set_piece(
        move.end_pos, Chess::PieceProperties{king_props.type, king_props.color, move.end_pos, true, false});

    // Determine rook movement based on castling type
    Position rook_start;
    Position rook_end;
    int      rank = move.start_pos.rank; // Same rank as king

    if (move.flags == SpecialFlags::CASTLE_KINGSIDE)
    {
      // Kingside castling: Rook moves from h-file to f-file
      rook_start = Position(7, rank); // h-file
      rook_end   = Position(5, rank); // f-file
    }
    else // CASTLE_QUEENSIDE
    {
      // Queenside castling: Rook moves from a-file to d-file
      rook_start = Position(0, rank); // a-file
      rook_end   = Position(3, rank); // d-file
    }

    // Move the rook
    const auto& rook_opt = current_grid_.get_piece(rook_start);
    if (rook_opt.has_value())
    {
      const auto& rook_props = rook_opt.value();
      current_grid_.clear_square(rook_start);
      current_grid_.set_piece(
          rook_end, Chess::PieceProperties{rook_props.type, rook_props.color, rook_end, true, false});
    }

    return true;
  }

  /// @brief Implement execute en passant.
  bool ChessGameHandler::execute_en_passant(const Move& move)
  {
    // Get pawn properties at start position
    const auto& pawn_opt = current_grid_.get_piece(move.start_pos);
    if (!pawn_opt.has_value() || pawn_opt->type != Chess::PieceType::PAWN)
    {
      return false; // No pawn at start position
    }

    const auto& pawn_props = pawn_opt.value();

    // Clear pawn's starting position
    current_grid_.clear_square(move.start_pos);

    // Move pawn to end position (empty square)
    current_grid_.set_piece(
        move.end_pos, Chess::PieceProperties{pawn_props.type, pawn_props.color, move.end_pos, true, false});

    // Capture the enemy pawn on the original rank
    // The captured pawn is at (end_pos.file, start_pos.rank)
    Position captured_pawn_pos(move.end_pos.file, move.start_pos.rank);
    current_grid_.clear_square(captured_pawn_pos);

    return true;
  }

  /// @brief Implement execute promotion.
  bool ChessGameHandler::execute_promotion(const Move& move)
  {
    const auto& pawn_opt = current_grid_.get_piece(move.start_pos);
    if (!pawn_opt.has_value() || pawn_opt->type != Chess::PieceType::PAWN)
    {
      return false;
    }

    if (move.promotion_piece == Chess::PieceType::PAWN || move.promotion_piece == Chess::PieceType::KING)
    {
      return false;
    }

    const auto& pawn_props = pawn_opt.value();

    current_grid_.clear_square(move.start_pos);
    current_grid_.clear_square(move.end_pos);
    current_grid_.set_piece(
        move.end_pos,
        Chess::PieceProperties{move.promotion_piece, pawn_props.color, move.end_pos, true, false});

    return true;
  }

  /// @brief Update game state.
  void ChessGameHandler::update_game_state()
  {
    Chess::Color current_color = current_grid_.current_turn;

    if (session_.state != GameState::ONGOING)
    {
      return;
    }

    // Check for checkmate (side to move has no legal escape while in check).
    if (board_manager_->is_checkmate(current_grid_, current_color))
    {
      session_.state = GameState::CHECKMATE;
      return;
    }

    // Check for stalemate (side to move has no legal moves while not in check).
    if (board_manager_->is_stalemate(current_grid_, current_color))
    {
      session_.state = GameState::STALEMATE;
      return;
    }

    // Check board-rule draw reasons.
    const DrawReason draw_reason = board_manager_->get_draw_reason(current_grid_, current_color);
    if (draw_reason == DrawReason::FIFTY_MOVE_RULE)
    {
      session_.state = GameState::DRAW_FIFTY_MOVE_RULE;
      return;
    }
    if (draw_reason == DrawReason::INSUFFICIENT_MATERIAL)
    {
      session_.state = GameState::DRAW_INSUFFICIENT_MATERIAL;
      return;
    }

    // Check for threefold repetition.
    if (is_threefold_repetition())
    {
      session_.state = GameState::DRAW_THREEFOLD_REPETITION;
      return;
    }

    // Game continues
    session_.state = GameState::ONGOING;
  }

  /// @brief Build repetition key.
  std::string ChessGameHandler::build_repetition_key(const Grid& grid) const
  {
    return fen_to_repetition_key(translation_unit_->internal_to_fen(grid));
  }

  /// @brief Reset position history.
  void ChessGameHandler::reset_position_history()
  {
    position_occurrence_counts_.clear();
    record_current_position();
  }

  /// @brief Record current position.
  void ChessGameHandler::record_current_position()
  {
    const std::string key = build_repetition_key(current_grid_);
    ++position_occurrence_counts_[key];
  }

  /// @brief Return whether threefold repetition.
  bool ChessGameHandler::is_threefold_repetition() const
  {
    const std::string key = build_repetition_key(current_grid_);
    const auto it = position_occurrence_counts_.find(key);
    return it != position_occurrence_counts_.end() && it->second >= 3;
  }

  bool ChessGameHandler::apply_terminal_state(GameState state, const std::string& reason)
  {
    if (session_.state != GameState::ONGOING)
    {
      return false;
    }

    session_.state = state;
    clear_draw_offers();
    (void)reason;
    return true;
  }

  void ChessGameHandler::clear_draw_offers()
  {
    draw_offered_by_white_ = false;
    draw_offered_by_black_ = false;
  }

} // namespace Chess


