# Chess Engine Interface (CEI)

## Overview

The Chess Engine Interface (CEI) module provides the artificial intelligence capabilities for the Inheritance Chess Engine. It implements advanced chess algorithms including minimax search with alpha-beta pruning, position evaluation, and move selection logic.

## Architecture

### Core Components

- **ChessEngine**: Main AI engine class that orchestrates search and evaluation
- **SearchLimits**: Configuration structure for search constraints
- **EvaluationResult**: Structure containing search results and statistics

### Key Features

1. **Move Evaluation**
   - Material balance analysis
   - Positional assessment
   - King safety evaluation
   - Piece-square tables

2. **Search Algorithms**
   - Minimax with alpha-beta pruning
   - Quiescence search for tactical positions
   - Configurable search depth and time limits
   - Node counting and time tracking

3. **AI Decision Making**
   - Best move selection
   - Difficulty levels (1-10)
   - Search statistics tracking
   - Draw detection

## File Structure

```
cei/
├── include/
│   └── chess_engine.h          # Main engine interface
├── src/
│   └── chess_engine.cpp        # Engine implementation
├── tests/
│   └── test_chess_engine.cpp    # Comprehensive test suite
└── README.md                   # This documentation
```

## API Reference

### ChessEngine Class

#### Constructor
```cpp
ChessEngine();
```

#### Core Methods

##### find_best_move
```cpp
EvaluationResult find_best_move(const Grid& grid, const SearchLimits& limits);
```
Finds the best move for the current position using AI search.

**Parameters:**
- `grid`: Current board position
- `limits`: Search constraints (depth, time, nodes)

**Returns:** `EvaluationResult` containing best move, score, and statistics

##### evaluate_position
```cpp
int evaluate_position(const Grid& grid, Color perspective);
```
Evaluates the current position from a specific color's perspective.

**Parameters:**
- `grid`: Position to evaluate
- `perspective`: Color to evaluate from (WHITE or BLACK)

**Returns:** Score in centipawns (positive = advantage for perspective)

##### get_legal_moves
```cpp
std::vector<Move> get_legal_moves(const Grid& grid);
```
Gets all legal moves for the current position.

**Parameters:**
- `grid`: Position to analyze

**Returns:** Vector of legal moves

##### is_draw
```cpp
bool is_draw(const Grid& grid);
```
Checks if the current position is a draw.

**Parameters:**
- `grid`: Position to check

**Returns:** True if position is a draw

##### set_difficulty
```cpp
void set_difficulty(int difficulty);
```
Sets the engine difficulty level.

**Parameters:**
- `difficulty`: Difficulty from 1-10 (1=easiest, 10=hardest)

##### get_statistics
```cpp
std::string get_statistics() const;
```
Gets engine performance statistics.

**Returns:** Formatted statistics string

### Supporting Structures

#### SearchLimits
```cpp
struct SearchLimits {
    int max_depth = 5;                                    // Maximum search depth
    std::chrono::milliseconds max_time{5000};             // Maximum search time
    uint64_t max_nodes = 1000000;                         // Maximum nodes to search
    bool infinite = false;                                 // Infinite search mode
};
```

#### EvaluationResult
```cpp
struct EvaluationResult {
    Move best_move;                    // Best move found
    int score = 0;                    // Evaluation score
    int depth = 0;                    // Search depth achieved
    uint64_t nodes_searched = 0;      // Number of nodes searched
    std::chrono::milliseconds time_used{0}; // Time spent searching
};
```

## Usage Examples

### Basic AI Analysis
```cpp
#include "cei/include/chess_engine.h"

Chess::ChessEngine engine;
Chess::Grid grid;
grid.initialize_standard_position();

// Set up search limits
Chess::SearchLimits limits;
limits.max_depth = 4;
limits.max_time = std::chrono::milliseconds(1000);

// Find best move
auto result = engine.find_best_move(grid, limits);
std::cout << "Best move: " << result.score << " centipawns\n";
std::cout << "Depth: " << result.depth << ", Nodes: " << result.nodes_searched << "\n";
```

### Position Evaluation
```cpp
// Evaluate position from both perspectives
int white_score = engine.evaluate_position(grid, Chess::Color::WHITE);
int black_score = engine.evaluate_position(grid, Chess::Color::BLACK);

std::cout << "White advantage: " << white_score << " centipawns\n";
std::cout << "Black advantage: " << black_score << " centipawns\n";
```

### Difficulty Settings
```cpp
// Set engine difficulty
engine.set_difficulty(3);  // Easy
engine.set_difficulty(8);  // Hard

// Get performance statistics
std::cout << engine.get_statistics() << "\n";
```

## Algorithm Details

### Search Algorithm

The engine uses **minimax search with alpha-beta pruning**:

1. **Minimax**: Recursively explores game tree to terminal positions
2. **Alpha-Beta Pruning**: Eliminates branches that won't affect the final decision
3. **Quiescence Search**: Extends search at tactical nodes to avoid horizon effect
4. **Move Ordering**: Searches promising moves first for better pruning

### Evaluation Function

The position evaluation combines multiple factors:

```cpp
int score = material_score + positional_score + king_safety_score;
```

#### Material Evaluation
- Pawn: 100 centipawns
- Knight: 320 centipawns  
- Bishop: 330 centipawns
- Rook: 500 centipawns
- Queen: 900 centipawns
- King: 20000 centipawns (for terminal positions)

#### Positional Factors
- Center control for pawns and pieces
- Piece mobility and development
- Pawn structure weaknesses
- Bishop pair advantage

#### King Safety
- Pawn shield evaluation
- Open files near king
- Attacking piece proximity

## Performance Considerations

### Search Optimization

1. **Transposition Table**: Caches positions to avoid recomputation
2. **Move Ordering**: Prioritizes captures, checks, and threats
3. **Null Move Pruning**: Skips moves in quiet positions
4. **Late Move Reductions**: Reduces search depth for less promising moves

### Memory Management

- Uses RAII principles with smart pointers
- Efficient position representation
- Minimal memory allocation during search

## Testing

The CEI module includes comprehensive tests covering:

- Engine initialization and configuration
- Position evaluation accuracy
- Move generation and validation
- Search algorithm correctness
- Difficulty level behavior
- Statistics tracking
- Integration with other modules

### Running Tests
```bash
# Build and run all tests
cmake -P build_and_run_project.cmake

# Run only CEI tests
cd build
ctest -R "ChessEngine"
```

## Integration

### Dependencies

The CEI module depends on:
- **CCI** (Chess Common Interface): Grid, Move, Position types
- **CGH** (Chess Game Handler): Move validation and game state
- **CPL** (Chess Pieces Logic): Piece movement and properties

### Module Boundaries

- **Input**: Grid positions and search limits
- **Output**: Move recommendations and evaluations
- **No Direct Board Access**: Uses game handler for move validation
- **Stateless**: Each search operates independently

## Future Enhancements

### Planned Features

1. **Advanced Search Techniques**
   - Iterative deepening
   - Principal variation search
   - Multi-cut pruning

2. **Improved Evaluation**
   - Endgame tablebase integration
   - Neural network evaluation
   - Dynamic piece values

3. **Performance Optimizations**
   - Parallel search algorithms
   - SIMD operations
   - Cache-friendly data structures

4. **Opening Book Integration**
   - Polyglot book support
   - Learning from game history
   - Book move statistics

## Configuration

### Compile-time Options

The engine can be configured at compile time:

```cpp
// Enable/disable features
#define ENABLE_QUIESCENCE_SEARCH
#define ENABLE_TRANSPOSITION_TABLE
#define ENABLE_NULL_MOVE_PRUNING
```

### Runtime Parameters

Key runtime parameters:

```cpp
// Search limits
limits.max_depth = 1..20;
limits.max_time = 1ms..10min;
limits.max_nodes = 1000..100000000;

// Engine behavior
engine.set_difficulty(1..10);
engine.reset_statistics();
```

## Troubleshooting

### Common Issues

1. **Poor Move Quality**
   - Increase search depth or time limits
   - Check position evaluation weights
   - Verify move generation correctness

2. **Slow Performance**
   - Reduce search depth
   - Enable aggressive pruning
   - Profile for bottlenecks

3. **Memory Issues**
   - Check for memory leaks in search
   - Reduce transposition table size
   - Monitor stack depth

### Debug Options

Enable debug output for troubleshooting:

```cpp
// Enable verbose logging
#define DEBUG_ENGINE_OUTPUT

// Track search statistics
engine.get_statistics();
```

## Contributing

When contributing to the CEI module:

1. **Follow Existing Patterns**: Use established coding conventions
2. **Add Tests**: Include comprehensive test coverage
3. **Document Changes**: Update API documentation
4. **Benchmark**: Measure performance impact
5. **Review Integration**: Ensure compatibility with other modules

## License

This module is licensed under the Apache License 2.0, same as the main project.

---

**Author:** Calileus  
**Date:** 2026-01-22  
**Version:** 0.1.0  
**Repository:** https://github.com/ObsidianHonorCoders/inheritance-chess
