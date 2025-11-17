#ifndef CHECKERS_H
#define CHECKERS_H

#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>

// Represents a piece on the board.
// We use an enum for this, as it's a simple and type-safe way to represent the
// different piece types. This is more efficient than using a class for something
// with no behavior, just state.
enum class Piece {
    EMPTY,
    WHITE_MAN,
    BLACK_MAN,
    WHITE_KING,
    BLACK_KING
};

// Represents a player's color.
enum class Player {
    WHITE,
    BLACK
};

// Represents a single move on the board.
// A struct is used here for simplicity. It's a plain old data (POD) structure.
struct Move {
    int from_row, from_col;
    int to_row, to_col;
    bool is_capture;
};

// The Board class represents the 8x8 checkerboard.
// It's responsible for storing the state of the board and providing methods
// to manipulate it. This class doesn't contain any game logic, making it a
// reusable component.
class Board {
public:
    Board();
    void initialize();
    Piece get_piece(int row, int col) const;
    void set_piece(int row, int col, Piece piece);
    void move_piece(const Move& move);
    static bool is_valid_coord(int row, int col);

private:
    std::vector<std::vector<Piece>> grid;
};

// The Game class encapsulates the entire logic of the checkers game.
// It manages the board, players, turns, and game rules.
// This is the core class that will be used to interact with the game.
// For the AI integration, you'll primarily interact with this class.
class Game {
public:
    Game();
    const Board& get_board() const;
    Player get_current_player() const;
    bool make_move(const Move& move);
    std::vector<Move> get_valid_moves(int row, int col) const;
    bool is_game_over() const;
    Player get_winner() const;

private:
    void switch_player();
    void find_captures(int row, int col, std::vector<Move>& moves) const;
    void find_simple_moves(int row, int col, std::vector<Move>& moves) const;
    bool can_player_capture(Player player) const;

    Board board;
    Player current_player;
    bool game_over;
    Player winner;
    std::ofstream log_file;
};

#endif // CHECKERS_H
