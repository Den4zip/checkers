#include "checkers.h"
#include <ncurses/ncurses.h>
#include <vector>
#include <string>
#include <algorithm>

// --- Board Class Implementation ---

Board::Board() : grid(8, std::vector<Piece>(8, Piece::EMPTY)) {}

void Board::initialize() {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 8; ++j) {
            if ((i + j) % 2 != 0) {
                grid[i][j] = Piece::BLACK_MAN;
            }
        }
    }
    for (int i = 5; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if ((i + j) % 2 != 0) {
                grid[i][j] = Piece::WHITE_MAN;
            }
        }
    }
}

Piece Board::get_piece(int row, int col) const {
    if (!is_valid_coord(row, col)) {
        throw std::out_of_range("Coordinates out of bounds");
    }
    return grid[row][col];
}

void Board::set_piece(int row, int col, Piece piece) {
    if (!is_valid_coord(row, col)) {
        throw std::out_of_range("Coordinates out of bounds");
    }
    grid[row][col] = piece;
}

void Board::move_piece(const Move& move) {
    Piece piece = get_piece(move.from_row, move.from_col);
    set_piece(move.to_row, move.to_col, piece);
    set_piece(move.from_row, move.from_col, Piece::EMPTY);
}

bool Board::is_valid_coord(int row, int col) {
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

// --- Game Class Implementation ---

Game::Game() : current_player(Player::WHITE), game_over(false) {
    board.initialize();
    log_file.open("log.txt", std::ios::app);
}

const Board& Game::get_board() const {
    return board;
}

Player Game::get_current_player() const {
    return current_player;
}

void Game::switch_player() {
    current_player = (current_player == Player::WHITE) ? Player::BLACK : Player::WHITE;
}

bool Game::make_move(const Move& move) {
    std::vector<Move> valid_moves = get_valid_moves(move.from_row, move.from_col);
    auto it = std::find_if(valid_moves.begin(), valid_moves.end(), [&](const Move& m) {
        return m.to_row == move.to_row && m.to_col == move.to_col;
    });

    if (it == valid_moves.end()) {
        return false; // Invalid move
    }

    Piece piece = board.get_piece(it->from_row, it->from_col);
    board.move_piece(*it);
    log_file << (current_player == Player::WHITE ? "W: " : "B: ")
             << (char)('a' + move.from_col) << 8 - move.from_row << " -> "
             << (char)('a' + move.to_col) << 8 - move.to_row << std::endl;

    if (it->is_capture) {
        int dr = (it->to_row > it->from_row) ? 1 : -1;
        int dc = (it->to_col > it->from_col) ? 1 : -1;
        int r = it->from_row + dr;
        int c = it->from_col + dc;
        while (r != it->to_row || c != it->to_col) {
            if (board.get_piece(r, c) != Piece::EMPTY) {
                log_file << "   Captured piece at " << (char)('a' + c) << 8 - r << std::endl;
                board.set_piece(r, c, Piece::EMPTY);
                break;
            }
            r += dr;
            c += dc;
        }
    }

    if (piece == Piece::WHITE_MAN && it->to_row == 0) {
        board.set_piece(it->to_row, it->to_col, Piece::WHITE_KING);
        log_file << "   Promoted to King at " << (char)('a' + it->to_col) << 8 - it->to_row << std::endl;
    }
    if (piece == Piece::BLACK_MAN && it->to_row == 7) {
        board.set_piece(it->to_row, it->to_col, Piece::BLACK_KING);
        log_file << "   Promoted to King at " << (char)('a' + it->to_col) << 8 - it->to_row << std::endl;
    }

    // If it was a capture and more captures are possible, don't switch player
    if (it->is_capture) {
        std::vector<Move> next_captures;
        find_captures(move.to_row, move.to_col, next_captures);
        if (!next_captures.empty()) {
            return true; // More captures available
        }
    }

    switch_player();
    
    // Check for game over condition
    int white_pieces = 0, black_pieces = 0;
    for(int r=0; r<8; ++r) {
        for(int c=0; c<8; ++c) {
            Piece p = board.get_piece(r, c);
            if (p == Piece::WHITE_MAN || p == Piece::WHITE_KING) {
                white_pieces++;
            } else if (p == Piece::BLACK_MAN || p == Piece::BLACK_KING) {
                black_pieces++;
            }
        }
    }

    bool current_player_can_move = false;
    for(int r=0; r<8; ++r) {
        for(int c=0; c<8; ++c) {
            Piece p = board.get_piece(r, c);
            if (p != Piece::EMPTY) {
                Player piece_color = (p == Piece::WHITE_MAN || p == Piece::WHITE_KING) ? Player::WHITE : Player::BLACK;
                if (piece_color == current_player) {
                    if (!get_valid_moves(r, c).empty()) {
                        current_player_can_move = true;
                        break;
                    }
                }
            }
        }
        if (current_player_can_move) break;
    }


    if (white_pieces == 0) {
        game_over = true;
        winner = Player::BLACK;
    } else if (black_pieces == 0) {
        game_over = true;
        winner = Player::WHITE;
    } else if (!current_player_can_move) {
        game_over = true;
        winner = (current_player == Player::WHITE) ? Player::BLACK : Player::WHITE;
    }

    return true;
}

std::vector<Move> Game::get_valid_moves(int row, int col) const {
    std::vector<Move> moves;
    Piece piece = board.get_piece(row, col);
    Player piece_color = (piece == Piece::WHITE_MAN || piece == Piece::WHITE_KING) ? Player::WHITE : Player::BLACK;

    if (piece_color != current_player) {
        return moves;
    }

    // Russian checkers rule: if a capture is available, it must be taken.
    if (can_player_capture(current_player)) {
        find_captures(row, col, moves);
    } else {
        find_simple_moves(row, col, moves);
    }

    return moves;
}

bool Game::is_game_over() const {
    return game_over;
}

Player Game::get_winner() const {
    return winner;
}

void Game::find_captures(int row, int col, std::vector<Move>& moves) const {
    Piece piece = board.get_piece(row, col);
    int directions[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

    if (piece == Piece::WHITE_MAN || piece == Piece::BLACK_MAN) {
        for (auto& dir : directions) {
            int opponent_row = row + dir[0];
            int opponent_col = col + dir[1];
            int dest_row = row + 2 * dir[0];
            int dest_col = col + 2 * dir[1];

            if (Board::is_valid_coord(dest_row, dest_col) && board.get_piece(dest_row, dest_col) == Piece::EMPTY) {
                Piece opponent_piece = board.get_piece(opponent_row, opponent_col);
                if (opponent_piece != Piece::EMPTY) {
                    Player piece_color = (piece == Piece::WHITE_MAN || piece == Piece::WHITE_KING) ? Player::WHITE : Player::BLACK;
                    Player opponent_color = (opponent_piece == Piece::WHITE_MAN || opponent_piece == Piece::WHITE_KING) ? Player::WHITE : Player::BLACK;
                    if (piece_color != opponent_color) {
                        moves.push_back({row, col, dest_row, dest_col, true});
                    }
                }
            }
        }
    } else if (piece == Piece::WHITE_KING || piece == Piece::BLACK_KING) {
        for (auto& dir : directions) {
            int opponent_row = -1, opponent_col = -1;
            // Find the first piece in the direction
            for (int i = 1; i < 8; ++i) {
                int current_row = row + i * dir[0];
                int current_col = col + i * dir[1];
                if (!Board::is_valid_coord(current_row, current_col)) {
                    break;
                }
                if (board.get_piece(current_row, current_col) != Piece::EMPTY) {
                    opponent_row = current_row;
                    opponent_col = current_col;
                    break;
                }
            }

            if (opponent_row != -1) {
                Piece opponent_piece = board.get_piece(opponent_row, opponent_col);
                Player piece_color = (piece == Piece::WHITE_KING || piece == Piece::BLACK_KING) ? Player::WHITE : Player::BLACK;
                Player opponent_color = (opponent_piece == Piece::WHITE_MAN || opponent_piece == Piece::WHITE_KING) ? Player::WHITE : Player::BLACK;

                if (piece_color != opponent_color) {
                    // Find empty squares after the opponent
                    for (int i = 1; i < 8; ++i) {
                        int dest_row = opponent_row + i * dir[0];
                        int dest_col = opponent_col + i * dir[1];
                        if (!Board::is_valid_coord(dest_row, dest_col)) {
                            break;
                        }
                        if (board.get_piece(dest_row, dest_col) == Piece::EMPTY) {
                            moves.push_back({row, col, dest_row, dest_col, true});
                        } else {
                            break;
                        }
                    }
                }
            }
        }
    }
}

void Game::find_simple_moves(int row, int col, std::vector<Move>& moves) const {
    Piece piece = board.get_piece(row, col);
    int forward_dir = (current_player == Player::WHITE) ? -1 : 1;

    int directions[2][2] = {{forward_dir, -1}, {forward_dir, 1}};
    int king_directions[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

    if (piece == Piece::WHITE_MAN || piece == Piece::BLACK_MAN) {
        for (auto& dir : directions) {
            int dest_row = row + dir[0];
            int dest_col = col + dir[1];
            if (Board::is_valid_coord(dest_row, dest_col) && board.get_piece(dest_row, dest_col) == Piece::EMPTY) {
                moves.push_back({row, col, dest_row, dest_col, false});
            }
        }
    } else if (piece == Piece::WHITE_KING || piece == Piece::BLACK_KING) {
        for (auto& dir : king_directions) {
            for (int i = 1; i < 8; ++i) {
                int dest_row = row + i * dir[0];
                int dest_col = col + i * dir[1];
                if (Board::is_valid_coord(dest_row, dest_col)) {
                    if (board.get_piece(dest_row, dest_col) == Piece::EMPTY) {
                        moves.push_back({row, col, dest_row, dest_col, false});
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
        }
    }
}

bool Game::can_player_capture(Player player) const {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.get_piece(r, c);
            if (p != Piece::EMPTY) {
                Player piece_color = (p == Piece::WHITE_MAN || p == Piece::WHITE_KING) ? Player::WHITE : Player::BLACK;
                if (piece_color == player) {
                    std::vector<Move> captures;
                    find_captures(r, c, captures);
                    if (!captures.empty()) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


// --- UI Class ---
class UI {
public:
    UI() {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_BLUE, COLOR_BLACK);
    }

    ~UI() {
        endwin();
    }

    void draw_board(const Board& board, int selected_row = -1, int selected_col = -1, const std::vector<Move>& valid_moves = {}) {
        clear();

        // Draw column letters (a-h)
        for (int j = 0; j < 8; ++j) {
            mvprintw(0, 2 + j * 4 + 1, "%c", 'a' + j);
        }

        for (int i = 0; i < 8; ++i) {
            // Draw row numbers (8-1)
            mvprintw(1 + i * 2, 0, "%d", 8 - i);
            for (int j = 0; j < 8; ++j) {
                bool is_valid_move_dest = false;
                for(const auto& move : valid_moves) {
                    if (move.to_row == i && move.to_col == j) {
                        is_valid_move_dest = true;
                        break;
                    }
                }

                if (i == selected_row && j == selected_col) attron(COLOR_PAIR(2));
                else if (is_valid_move_dest) attron(COLOR_PAIR(3));
                
                if ((i + j) % 2 == 0) { // White square
                    mvprintw(1 + i * 2, 2 + j * 4, "    ");
                    mvprintw(1 + i * 2 + 1, 2 + j * 4, "    ");
                } else { // Black square
                    attron(A_REVERSE);
                    mvprintw(1 + i * 2, 2 + j * 4, "    ");
                    mvprintw(1 + i * 2 + 1, 2 + j * 4, "    ");
                    attroff(A_REVERSE);
                }

                switch (board.get_piece(i, j)) {
                    case Piece::WHITE_MAN:  mvprintw(1 + i * 2, 2 + j * 4 + 1, "w"); break;
                    case Piece::BLACK_MAN:  mvprintw(1 + i * 2, 2 + j * 4 + 1, "b"); break;
                    case Piece::WHITE_KING: mvprintw(1 + i * 2, 2 + j * 4 + 1, "W"); break;
                    case Piece::BLACK_KING: mvprintw(1 + i * 2, 2 + j * 4 + 1, "B"); break;
                    default: break;
                }
                if (i == selected_row && j == selected_col) attroff(COLOR_PAIR(2));
                else if (is_valid_move_dest) attroff(COLOR_PAIR(3));
            }
        }
        refresh();
    }

    Move get_move_input(const Game& game) {
        int from_row = -1, from_col = -1;
        std::vector<Move> valid_moves;
        char input_chars[3];

        while (true) {
            draw_board(game.get_board(), from_row, from_col, valid_moves);
            if (from_row == -1) {
                mvprintw(17, 0, "Enter piece to move (e.g., a3): ");
            } else {
                mvprintw(17, 0, "Enter destination (e.g., b4):   ");
            }
            mvprintw(18, 0, "Player %s's turn.                     ", game.get_current_player() == Player::WHITE ? "White" : "Black");
            mvprintw(19, 0, "                                   "); // Clear previous messages
            move(17, 32); // Move cursor to input position

            echo();
            getnstr(input_chars, 2);
            noecho();

            std::string input_str(input_chars);
            if (input_str.length() != 2) {
                mvprintw(19, 0, "Invalid input. Use format 'a3'.");
                continue;
            }

            int col = input_str[0] - 'a';
            int row = 7 - (input_str[1] - '1');

            if (!Board::is_valid_coord(row, col)) {
                mvprintw(19, 0, "Invalid coordinates. Try again.");
                continue;
            }

            if (from_row == -1) { // Selecting a piece
                Piece p = game.get_board().get_piece(row, col);
                if (p != Piece::EMPTY) {
                    Player piece_color = (p == Piece::WHITE_MAN || p == Piece::WHITE_KING) ? Player::WHITE : Player::BLACK;
                    if (piece_color == game.get_current_player()) {
                        from_row = row;
                        from_col = col;
                        valid_moves = game.get_valid_moves(from_row, from_col);
                        if (valid_moves.empty()) {
                            mvprintw(19, 0, "This piece has no valid moves.");
                            from_row = -1; from_col = -1;
                            valid_moves.clear();
                        }
                    } else {
                        mvprintw(19, 0, "Not your piece. Try again.");
                    }
                } else {
                    mvprintw(19, 0, "Empty square. Try again.");
                }
            } else { // Selecting a destination
                auto it = std::find_if(valid_moves.begin(), valid_moves.end(), [&](const Move& m) {
                    return m.to_row == row && m.to_col == col;
                });

                if (it != valid_moves.end()) {
                    return *it;
                }
                
                // If the user selected the same piece, deselect it
                if (row == from_row && col == from_col) {
                     from_row = -1;
                     from_col = -1;
                     valid_moves.clear();
                } else {
                    mvprintw(19, 0, "Invalid destination. Try again.");
                }
            }
        }
    }
};

// --- Main Game Loop ---

int main() {
    Game game;
    UI ui;

    while (!game.is_game_over()) {
        Move move = ui.get_move_input(game);
        game.make_move(move);
    }

    ui.draw_board(game.get_board());
    mvprintw(18, 0, "Game Over! Player %s wins.", game.get_winner() == Player::WHITE ? "White" : "Black");
    mvprintw(19, 0, "Press any key to exit.");
    getch();

    return 0;
}