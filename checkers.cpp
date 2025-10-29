#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>

class Checkers {
public:
    Checkers() : currentPlayer(1) {
        board = {
            { 0, -1,  0, -1,  0, -1,  0, -1},
            {-1,  0, -1,  0, -1,  0, -1,  0},
            { 0, -1,  0, -1,  0, -1,  0, -1},
            { 0,  0,  0,  0,  0,  0,  0,  0},
            { 0,  0,  0,  0,  0,  0,  0,  0},
            { 1,  0,  1,  0,  1,  0,  1,  0},
            { 0,  1,  0,  1,  0,  1,  0,  1},
            { 1,  0,  1,  0,  1,  0,  1,  0}
        };
        
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* local_tm = std::localtime(&now_time);
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "log_%Y-%m-%d_%H-%M-%S.txt", local_tm);
        logFilename = buffer;

        std::string startTimeStr = std::ctime(&now_time);
        startTimeStr.pop_back(); // Remove trailing newline
        log("Game started at " + startTimeStr, false);
    }

    ~Checkers() {
        std::time_t now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string endTimeStr = std::ctime(&now_time);
        endTimeStr.pop_back(); // Remove trailing newline
        log("Game ended at " + endTimeStr, false);
    }

    void play() {
        while (true) {
            printBoard();
            std::cout << (currentPlayer == 1 ? "White" : "Black") << "\'s turn." << std::endl;

            if (!hasValidMoves(currentPlayer)) {
                std::stringstream msg;
                msg << (currentPlayer == 1 ? "Black" : "White") << " wins! No valid moves.";
                log(msg.str());
                break;
            }

            int fromRow, fromCol, toRow, toCol;
            int moveResult = getPlayerMove(fromRow, fromCol, toRow, toCol);

            if (moveResult == -1) { // Resign
                std::stringstream msg;
                msg << (currentPlayer == 1 ? "Black" : "White") << " wins by resignation.";
                log(msg.str());
                break;
            }
            if (moveResult == 0) { // Invalid move
                continue;
            }

            makeMove(fromRow, fromCol, toRow, toCol);

            int winner = getWinner();
            if (winner != 0) {
                printBoard();
                std::stringstream msg;
                msg << (winner == 1 ? "White" : "Black") << " wins!";
                log(msg.str());
                break;
            }

            switchPlayer();
        }
    }

private:
    std::vector<std::vector<int>> board;
    int currentPlayer;
    std::string logFilename;

    void log(const std::string& message, bool toConsole = true) {
        std::ofstream logFile(logFilename, std::ios_base::app);
        if (logFile.is_open()) {
            logFile << message << std::endl;
            logFile.close();
        }
        if (toConsole) {
            std::cout << message << std::endl;
        }
    }

    void printBoard() const {
        std::cout << "  a b c d e f g h" << std::endl;
        for (int i = 0; i < 8; ++i) {
            std::cout << 8 - i << " ";
            for (int j = 0; j < 8; ++j) {
                switch (board[i][j]) {
                    case 1:  std::cout << "w "; break;
                    case -1: std::cout << "b "; break;
                    case 2:  std::cout << "W "; break;
                    case -2: std::cout << "B "; break;
                    default: std::cout << ". "; break;
                }
            }
            std::cout << std::endl;
        }
    }

    int getPlayerMove(int& fromRow, int& fromCol, int& toRow, int& toCol) {
        std::cout << "Enter your move (e.g., a6 b5) or type 'resign': ";
        std::string line;
        std::getline(std::cin, line);
        std::stringstream ss(line);
        
        std::string firstWord;
        ss >> firstWord;

        if (firstWord == "resign") {
            return -1; // Signal resignation
        }

        if (firstWord.length() != 2) {
            log("Invalid input format: " + line);
            return 0;
        }
        char fromColChar = firstWord[0];
        char fromRowChar = firstWord[1];

        std::string secondWord;
        ss >> secondWord;
        if (secondWord.length() != 2) {
            log("Invalid input format: " + line);
            return 0;
        }
        char toColChar = secondWord[0];
        char toRowChar = secondWord[1];

        if (fromColChar < 'a' || fromColChar > 'h' || toColChar < 'a' || toColChar > 'h' ||
            fromRowChar < '1' || fromRowChar > '8' || toRowChar < '1' || toRowChar > '8') {
            log("Invalid coordinates: " + line);
            return 0;
        }

        fromCol = fromColChar - 'a';
        fromRow = 8 - (fromRowChar - '0');
        toCol = toColChar - 'a';
        toRow = 8 - (toRowChar - '0');

        int jumpedRow, jumpedCol;
        if (!isMoveValid(fromRow, fromCol, toRow, toCol, jumpedRow, jumpedCol)) {
            log("Invalid move attempt: " + line);
            return 0;
        }
        return 1; // Valid move
    }

    bool isMoveValid(int fromRow, int fromCol, int toRow, int toCol, int& jumpedRow, int& jumpedCol) const {
        if (fromRow < 0 || fromRow > 7 || fromCol < 0 || fromCol > 7 ||
            toRow < 0 || toRow > 7 || toCol < 0 || toCol > 7) {
            return false;
        }

        if (board[fromRow][fromCol] * currentPlayer <= 0) {
            return false; // Not player's piece
        }

        if (board[toRow][toCol] != 0) {
            return false; // Destination not empty
        }

        int piece = board[fromRow][fromCol];
        int rowDiff = abs(fromRow - toRow);
        int colDiff = abs(fromCol - toCol);

        if (rowDiff != colDiff || rowDiff == 0) {
            return false; // Not a diagonal move
        }

        jumpedRow = -1;
        jumpedCol = -1;

        // --- King Logic ---
        if (abs(piece) == 2) {
            int r_step = (toRow - fromRow) / rowDiff;
            int c_step = (toCol - fromCol) / colDiff;
            int piecesOnPath = 0;
            int lastPieceRow = -1, lastPieceCol = -1;

            for (int i = 1; i < rowDiff; ++i) {
                int r = fromRow + i * r_step;
                int c = fromCol + i * c_step;
                if (board[r][c] != 0) {
                    piecesOnPath++;
                    lastPieceRow = r;
                    lastPieceCol = c;
                }
            }

            if (piecesOnPath == 0) {
                return true; // Valid move, no jump
            }

            if (piecesOnPath == 1) {
                // Check if the single piece on the path is an opponent's piece
                if (board[lastPieceRow][lastPieceCol] * currentPlayer < 0) {
                    jumpedRow = lastPieceRow;
                    jumpedCol = lastPieceCol;
                    return true; // Valid jump
                }
            }
            
            return false; // Invalid king move (jumping own piece or >1 piece)
        }

        // --- Regular Piece Logic ---
        // Direction check
        if (piece == 1 && fromRow < toRow) return false;
        if (piece == -1 && fromRow > toRow) return false;

        if (rowDiff == 1) {
            return true; // Simple move
        }

        if (rowDiff == 2) {
            jumpedRow = (fromRow + toRow) / 2;
            jumpedCol = (fromCol + toCol) / 2;
            if (board[jumpedRow][jumpedCol] * currentPlayer < 0) {
                return true; // Valid jump
            }
        }

        return false;
    }

    void makeMove(int fromRow, int fromCol, int toRow, int toCol) {
        int jumpedRow, jumpedCol;
        isMoveValid(fromRow, fromCol, toRow, toCol, jumpedRow, jumpedCol);

        std::stringstream moveMsg;
        moveMsg << (currentPlayer == 1 ? "White: " : "Black: ") 
                << (char)(fromCol + 'a') << 8 - fromRow 
                << " -> " 
                << (char)(toCol + 'a') << 8 - toRow;

        board[toRow][toCol] = board[fromRow][fromCol];
        board[fromRow][fromCol] = 0;

        if (jumpedRow != -1) {
            board[jumpedRow][jumpedCol] = 0;
            moveMsg << " (Capture at " << (char)(jumpedCol + 'a') << 8 - jumpedRow << ")";
        }
        log(moveMsg.str());

        // King promotion
        if ((board[toRow][toCol] == 1 && toRow == 0) || (board[toRow][toCol] == -1 && toRow == 7)){
            if(board[toRow][toCol] == 1) board[toRow][toCol] = 2;
            else board[toRow][toCol] = -2;
            std::stringstream promotionMsg;
            promotionMsg << "  -> Promoted to King at " << (char)(toCol + 'a') << 8 - toRow;
            log(promotionMsg.str());
        }
    }

    bool hasValidMoves(int player) const {
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (board[r][c] * player > 0) {
                    for (int dr = -2; dr <= 2; ++dr) {
                        if (dr == 0) continue;
                        for (int dc = -2; dc <= 2; ++dc) {
                            if (dc == 0) continue;
                            int jumpedR, jumpedC;
                            if (isMoveValid(r, c, r + dr, c + dc, jumpedR, jumpedC)) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    int getWinner() const {
        int whitePieces = 0, blackPieces = 0;
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                if (board[i][j] > 0) whitePieces++;
                else if (board[i][j] < 0) blackPieces++;
            }
        }
        if (whitePieces == 0) return -1;
        if (blackPieces == 0) return 1;
        return 0;
    }

    void switchPlayer() {
        currentPlayer *= -1;
    }
};

int main() {
    Checkers game;
    game.play();
    return 0;
}
