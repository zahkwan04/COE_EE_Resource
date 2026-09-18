#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <sstream>

// ============================================================
// Board representation
// ============================================================
struct Board {
    char sq[64];          // a1=0, b1=1, ... h8=63
    bool whiteToMove;
    bool wK, wQ, bK, bQ;
    int epSquare;

    Board() { reset(); }

    void reset() {
        const char* start =
            "RNBQKBNR"     // sq[0..7]   = rank 1  (White back rank)
            "PPPPPPPP"     // sq[8..15]  = rank 2  (White pawns)
            "........"     // sq[16..23] = rank 3
            "........"     // sq[24..31] = rank 4
            "........"     // sq[32..39] = rank 5
            "........"     // sq[40..47] = rank 6
           "pppppppp"     // sq[48..55] = rank 7  (Black pawns)
           "rnbqkbnr";    // sq[56..63] = rank 8  (Black back rank)
       for (int i = 0; i < 64; ++i) sq[i] = start[i];
       whiteToMove = true;
       wK = wQ = bK = bQ = true;
      epSquare = -1;
    }

    static bool isWhite(char p) { return p >= 'A' && p <= 'Z'; }
    static bool isBlack(char p) { return p >= 'a' && p <= 'z'; }
    static bool sameSide(char a, char b) {
        return (isWhite(a) && isWhite(b)) || (isBlack(a) && isBlack(b));
    }
};

struct Move {
    int from, to;
    char promotion;
    char captured;
    bool isEnPassant;
    bool isCastle;

    Move() : from(-1), to(-1), promotion(0), captured(0),
             isEnPassant(false), isCastle(false) {}
};

static const int KNIGHT_DELTAS[8][2] = {
    { 1, 2}, { 2, 1}, { 2,-1}, { 1,-2},
    {-1,-2}, {-2,-1}, {-2, 1}, {-1, 2}
};
static const int BISHOP_DELTAS[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
static const int ROOK_DELTAS[4][2]   = {{1,0},{-1,0},{0,1},{0,-1}};
static const int KING_DELTAS[8][2]   = {
    {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}
};

static int rankOf(int s) { return s / 8; }
static int fileOf(int s) { return s % 8; }
static bool onBoard(int r, int f) { return r >= 0 && r < 8 && f >= 0 && f < 8; }

bool isAttacked(const Board& b, int s, bool byWhite) {
    int r = rankOf(s), f = fileOf(s);

    int pr = byWhite ? r - 1 : r + 1;
    for (int df : {-1, 1}) {
        int pf = f + df;
        if (onBoard(pr, pf)) {
            char p = b.sq[pr * 8 + pf];
            if (byWhite && p == 'P') return true;
            if (!byWhite && p == 'p') return true;
        }
    }

    for (auto& d : KNIGHT_DELTAS) {
        int nr = r + d[0], nf = f + d[1];
        if (onBoard(nr, nf)) {
            char p = b.sq[nr * 8 + nf];
            if (byWhite && p == 'N') return true;
            if (!byWhite && p == 'n') return true;
        }
    }

    for (auto& d : KING_DELTAS) {
        int nr = r + d[0], nf = f + d[1];
        if (onBoard(nr, nf)) {
            char p = b.sq[nr * 8 + nf];
            if (byWhite && p == 'K') return true;
            if (!byWhite && p == 'k') return true;
        }
    }

    for (auto& d : BISHOP_DELTAS) {
        int nr = r + d[0], nf = f + d[1];
        while (onBoard(nr, nf)) {
            char p = b.sq[nr * 8 + nf];
            if (p != '.') {
                if (byWhite && (p == 'B' || p == 'Q')) return true;
                if (!byWhite && (p == 'b' || p == 'q')) return true;
                break;
            }
            nr += d[0]; nf += d[1];
        }
    }

    for (auto& d : ROOK_DELTAS) {
        int nr = r + d[0], nf = f + d[1];
        while (onBoard(nr, nf)) {
            char p = b.sq[nr * 8 + nf];
            if (p != '.') {
                if (byWhite && (p == 'R' || p == 'Q')) return true;
                if (!byWhite && (p == 'r' || p == 'q')) return true;
                break;
            }
            nr += d[0]; nf += d[1];
        }
    }
    return false;
}

int findKing(const Board& b, bool white) {
    char target = white ? 'K' : 'k';
    for (int i = 0; i < 64; ++i)
        if (b.sq[i] == target) return i;
    return -1;
}

bool inCheck(const Board& b, bool white) {
    int k = findKing(b, white);
    return k != -1 && isAttacked(b, k, !white);
}

// ============================================================
// Move generation
// ============================================================
void addPawnMoves(const Board& b, int s, std::vector<Move>& out) {
    bool white = Board::isWhite(b.sq[s]);
    int r = rankOf(s), f = fileOf(s);
    int dir = white ? 1 : -1;
    int startRank = white ? 1 : 6;
    int promoRank = white ? 7 : 0;

    auto push = [&](int to, bool ep) {
        Move m; m.from = s; m.to = to;
        m.captured = b.sq[to];
        m.isEnPassant = ep;
        if (rankOf(to) == promoRank) {
            for (char p : {'q','r','b','n'}) {
                Move pm = m;
                pm.promotion = white ? std::toupper(p) : p;
                out.push_back(pm);
            }
        } else {
            out.push_back(m);
        }
    };

    int nr = r + dir;
    if (onBoard(nr, f) && b.sq[nr * 8 + f] == '.') {
        push(nr * 8 + f, false);
        int nr2 = r + 2 * dir;
        if (r == startRank && b.sq[nr2 * 8 + f] == '.') {
            push(nr2 * 8 + f, false);
        }
    }

    for (int df : {-1, 1}) {
        int nf = f + df;
        if (!onBoard(nr, nf)) continue;
        int to = nr * 8 + nf;
        char target = b.sq[to];
        if (target != '.' && !Board::sameSide(b.sq[s], target))
            push(to, false);
        else if (to == b.epSquare)
            push(to, true);
    }
}

void addStepMoves(const Board& b, int s, const int deltas[][2], int n, std::vector<Move>& out) {
    int r = rankOf(s), f = fileOf(s);
    for (int i = 0; i < n; ++i) {
        int nr = r + deltas[i][0], nf = f + deltas[i][1];
        if (!onBoard(nr, nf)) continue;
        int to = nr * 8 + nf;
        if (b.sq[to] == '.' || !Board::sameSide(b.sq[s], b.sq[to])) {
            Move m; m.from = s; m.to = to; m.captured = b.sq[to];
            out.push_back(m);
        }
    }
}

void addSlideMoves(const Board& b, int s, const int deltas[][2], int n, std::vector<Move>& out) {
    int r = rankOf(s), f = fileOf(s);
    for (int i = 0; i < n; ++i) {
        int nr = r + deltas[i][0], nf = f + deltas[i][1];
        while (onBoard(nr, nf)) {
            int to = nr * 8 + nf;
            char target = b.sq[to];
            if (target == '.') {
                Move m; m.from = s; m.to = to;
                out.push_back(m);
            } else {
                if (!Board::sameSide(b.sq[s], target)) {
                    Move m; m.from = s; m.to = to; m.captured = target;
                    out.push_back(m);
                }
                break;
            }
            nr += deltas[i][0]; nf += deltas[i][1];
        }
    }
}

void addCastleMoves(const Board& b, std::vector<Move>& out) {
    bool white = b.whiteToMove;
    if (inCheck(b, white)) return;

    int rank = white ? 0 : 7;
    char king = white ? 'K' : 'k';
    if (b.sq[rank * 8 + 4] != king) return;

    bool canK = white ? b.wK : b.bK;
    bool canQ = white ? b.wQ : b.bQ;

    if (canK
        && b.sq[rank * 8 + 5] == '.' && b.sq[rank * 8 + 6] == '.'
        && !isAttacked(b, rank * 8 + 5, !white)
        && !isAttacked(b, rank * 8 + 6, !white)) {
        Move m; m.from = rank * 8 + 4; m.to = rank * 8 + 6;
        m.isCastle = true;
        out.push_back(m);
    }

    if (canQ
        && b.sq[rank * 8 + 1] == '.' && b.sq[rank * 8 + 2] == '.'
        && b.sq[rank * 8 + 3] == '.'
        && !isAttacked(b, rank * 8 + 3, !white)
        && !isAttacked(b, rank * 8 + 2, !white)) {
        Move m; m.from = rank * 8 + 4; m.to = rank * 8 + 2;
        m.isCastle = true;
        out.push_back(m);
    }
}

std::vector<Move> generateMoves(const Board& b) {
    std::vector<Move> moves;
    for (int s = 0; s < 64; ++s) {
        char p = b.sq[s];
        if (p == '.') continue;
        if (b.whiteToMove != Board::isWhite(p)) continue;

        switch (std::tolower(p)) {
            case 'p': addPawnMoves(b, s, moves); break;
            case 'n': addStepMoves(b, s, KNIGHT_DELTAS, 8, moves); break;
            case 'b': addSlideMoves(b, s, BISHOP_DELTAS, 4, moves); break;
            case 'r': addSlideMoves(b, s, ROOK_DELTAS, 4, moves); break;
            case 'q':
                addSlideMoves(b, s, BISHOP_DELTAS, 4, moves);
                addSlideMoves(b, s, ROOK_DELTAS, 4, moves);
                break;
            case 'k': addStepMoves(b, s, KING_DELTAS, 8, moves); break;
        }
    }
    addCastleMoves(b, moves);
    return moves;
}

// ============================================================
// Make / unmake
// ============================================================
struct UndoInfo {
    bool wK, wQ, bK, bQ;
    int epSquare;
    char capturedAt;
    int rookFrom, rookTo;
    bool movedRook;
};

UndoInfo makeMove(Board& b, const Move& m) {
    UndoInfo u{b.wK, b.wQ, b.bK, b.bQ, b.epSquare, '.', -1, -1, false};

    char piece = b.sq[m.from];
    bool white = Board::isWhite(piece);

    if (m.isEnPassant) {
        int capturedSq = m.to + (white ? -8 : 8);
        u.capturedAt = b.sq[capturedSq];
        b.sq[capturedSq] = '.';
    }

    b.sq[m.to] = m.promotion ? m.promotion : piece;
    b.sq[m.from] = '.';

    if (m.isCastle) {
        int rank = white ? 0 : 7;
        if (m.to == rank * 8 + 6) {
            u.rookFrom = rank * 8 + 7;
            u.rookTo   = rank * 8 + 5;
        } else {
            u.rookFrom = rank * 8 + 0;
            u.rookTo   = rank * 8 + 3;
        }
        b.sq[u.rookTo] = b.sq[u.rookFrom];
        b.sq[u.rookFrom] = '.';
        u.movedRook = true;
    }

    if (piece == 'K') { b.wK = b.wQ = false; }
    if (piece == 'k') { b.bK = b.bQ = false; }
    if (m.from == 0 || m.to == 0) b.wQ = false;
    if (m.from == 7 || m.to == 7) b.wK = false;
    if (m.from == 56 || m.to == 56) b.bQ = false;
    if (m.from == 63 || m.to == 63) b.bK = false;

    b.epSquare = -1;
    if (std::tolower(piece) == 'p' && std::abs(m.to - m.from) == 16) {
        b.epSquare = (m.from + m.to) / 2;
    }

    b.whiteToMove = !b.whiteToMove;
    return u;
}

void unmakeMove(Board& b, const Move& m, const UndoInfo& u) {
    b.whiteToMove = !b.whiteToMove;
    b.wK = u.wK; b.wQ = u.wQ; b.bK = u.bK; b.bQ = u.bQ;
    b.epSquare = u.epSquare;

    char moved = b.sq[m.to];
    if (m.promotion) moved = b.whiteToMove ? 'P' : 'p';

    b.sq[m.from] = moved;
    b.sq[m.to]   = '.';

    if (m.isEnPassant) {
        int capturedSq = m.to + (b.whiteToMove ? -8 : 8);
        b.sq[capturedSq] = u.capturedAt;
    } else if (m.captured) {
        b.sq[m.to] = m.captured;
    }

    if (m.isCastle && u.movedRook) {
        b.sq[u.rookFrom] = b.sq[u.rookTo];
        b.sq[u.rookTo] = '.';
    }
}

std::vector<Move> legalMoves(Board& b) {
    std::vector<Move> result;
    bool mover = b.whiteToMove;

    for (const Move& m : generateMoves(b)) {
        UndoInfo u = makeMove(b, m);
        if (!inCheck(b, mover)) result.push_back(m);
        unmakeMove(b, m, u);
    }
    return result;
}

// ============================================================
// Evaluation
// ============================================================
int evaluate(const Board& b) {
    int score = 0;
    for (int i = 0; i < 64; ++i) {
        switch (b.sq[i]) {
            case 'P': score += 100; break;
            case 'N': score += 320; break;
            case 'B': score += 330; break;
            case 'R': score += 500; break;
            case 'Q': score += 900; break;
            case 'K': score += 20000; break;
            case 'p': score -= 100; break;
            case 'n': score -= 320; break;
            case 'b': score -= 330; break;
            case 'r': score -= 500; break;
            case 'q': score -= 900; break;
            case 'k': score -= 20000; break;
        }
    }
    return score;
}

// ============================================================
// Search
// ============================================================
int negamax(Board& b, int depth, int alpha, int beta) {
    if (depth == 0) {
        int e = evaluate(b);
        return b.whiteToMove ? e : -e;
    }

    auto moves = legalMoves(b);
    if (moves.empty()) {
        return inCheck(b, b.whiteToMove) ? -100000 + (10 - depth) : 0;
    }

    int best = -1000000;
    for (const Move& m : moves) {
        UndoInfo u = makeMove(b, m);
        int score = -negamax(b, depth - 1, -beta, -alpha);
        unmakeMove(b, m, u);

        if (score > best) best = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }
    return best;
}

struct ScoredMove {
    Move move;
    int score;
};

std::vector<ScoredMove> analyzeAllMoves(Board& b, int depth) {
    std::vector<ScoredMove> results;
    auto moves = legalMoves(b);

    for (const Move& m : moves) {
        UndoInfo u = makeMove(b, m);
        int score = -negamax(b, depth - 1, -1000000, 1000000);
        unmakeMove(b, m, u);
        results.push_back({m, score});
    }

    std::sort(results.begin(), results.end(),
              [](const ScoredMove& a, const ScoredMove& c) {
                  return a.score > c.score;
              });
    return results;
}

// ============================================================
// Helpers
// ============================================================
bool parseSquare(const std::string& s, int& out) {
    if (s.size() != 2) return false;
    int f = s[0] - 'a';
    int r = s[1] - '1';
    if (!onBoard(r, f)) return false;
    out = r * 8 + f;
    return true;
}

std::string moveToStr(const Move& m) {
    std::string s;
    s += char('a' + fileOf(m.from));
    s += char('1' + rankOf(m.from));
    s += char('a' + fileOf(m.to));
    s += char('1' + rankOf(m.to));
    if (m.promotion) s += char(std::tolower(m.promotion));
    return s;
}

bool findMove(Board& b, const std::string& input, Move& out) {
    if (input.size() < 4) return false;
    int from, to;
    if (!parseSquare(input.substr(0, 2), from)) return false;
    if (!parseSquare(input.substr(2, 2), to)) return false;

    char promoWanted = 0;
    if (input.size() >= 5) promoWanted = std::tolower(input[4]);

    for (const Move& m : legalMoves(b)) {
        if (m.from != from || m.to != to) continue;
        if (m.promotion) {
            char want = promoWanted ? promoWanted : 'q';
            if (std::tolower(m.promotion) != want) continue;
        }
        out = m;
        return true;
    }
    return false;
}

// ------------------------------------------------------------
// Board display — FIXED orientation, never flips.
//   rank 1 at the bottom, rank 8 at the top
//   a-file on the left,  h-file on the right
// ------------------------------------------------------------
void printBoard(const Board& b) {
    std::cout << "DEBUG: sq[0]=" << b.sq[0]
              << " sq[7]=" << b.sq[7]
              << " sq[56]=" << b.sq[56]
              << " sq[63]=" << b.sq[63] << "\n";
    std::cout << "\n    a  b  c  d  e  f  g  h\n";
    
    std::cout << "\n    a  b  c  d  e  f  g  h\n";
    std::cout << "  +------------------------+\n";
    for (int r = 7; r >= 0; --r) {
        std::cout << (r + 1) << " |";
        for (int f = 0; f < 8; ++f) {
            std::cout << " " << b.sq[r * 8 + f] << " ";
        }
        std::cout << "| " << (r + 1) << "\n";
    }
    std::cout << "  +------------------------+\n";
    std::cout << "    a  b  c  d  e  f  g  h\n\n";
}

std::string evalStr(int cp) {
    std::ostringstream oss;
    if (cp > 90000) oss << "+M" << (100000 - cp);
    else if (cp < -90000) oss << "-M" << (100000 + cp);
    else oss << (cp >= 0 ? "+" : "") << (cp / 100.0);
    return oss.str();
}

// ============================================================
// Analysis loop
// ============================================================
int main() {
    Board board;
    std::vector<std::pair<Move, UndoInfo>> history;

    const int SEARCH_DEPTH = 4;

    // ------------------------------------------------------------
    // Ask which side you're playing (only used for the prompt)
    // ------------------------------------------------------------
    std::cout << "===========================================\n";
    std::cout << "         Chess Analyzer\n";
    std::cout << "===========================================\n\n";
    std::cout << "Which side are you playing?\n";
    std::cout << "  1. White\n";
    std::cout << "  2. Black\n\n";

    bool mySideIsWhite = true;
    while (true) {
        std::cout << "Enter 1 or 2: ";
        std::string choice;
        std::getline(std::cin, choice);

        while (!choice.empty() && std::isspace(choice.front())) choice.erase(choice.begin());
        while (!choice.empty() && std::isspace(choice.back()))  choice.pop_back();

        if (choice == "1") { mySideIsWhite = true;  break; }
        if (choice == "2") { mySideIsWhite = false; break; }
        std::cout << "Invalid choice. Try again.\n";
    }

    std::cout << "\nYou are playing " << (mySideIsWhite ? "White" : "Black") << ".\n";
    std::cout << "Board is drawn from White's perspective at all times\n";
    std::cout << "(rank 1 at the bottom, rank 8 at the top).\n\n";
    std::cout << "Commands:\n";
    std::cout << "  <move>   register a move (e.g. e2e4, e7e8q)\n";
    std::cout << "  hint     ask the engine for the best move for the side to move\n";
    std::cout << "  eval     show current evaluation\n";
    std::cout << "  board    redraw the board\n";
    std::cout << "  undo     take back the last move\n";
    std::cout << "  quit     exit\n\n";

    printBoard(board);

    while (true) {
        auto moves = legalMoves(board);
        if (moves.empty()) {
            printBoard(board);
            if (inCheck(board, board.whiteToMove)) {
                std::cout << "Checkmate. "
                          << (board.whiteToMove ? "Black" : "White") << " wins.\n";
            } else {
                std::cout << "Stalemate. Draw.\n";
            }
            break;
        }

        // Label the turn. "YOUR turn" if it's your side's move.
        bool isMyTurn = (board.whiteToMove == mySideIsWhite);
        std::cout << (isMyTurn ? ">> YOUR turn. " : ">> Opponent's turn. ");
        std::cout << "(" << (board.whiteToMove ? "White" : "Black") << " to move) ";
        std::cout << "Enter move or command: ";

        std::string input;
        std::getline(std::cin, input);

        while (!input.empty() && std::isspace(input.front())) input.erase(input.begin());
        while (!input.empty() && std::isspace(input.back()))  input.pop_back();

        if (input.empty()) continue;

        if (input == "quit" || input == "exit") break;

        if (input == "board") {
            printBoard(board);
            continue;
        }

        if (input == "eval") {
            int e = evaluate(board);
            std::cout << "Static eval (from White's view): " << evalStr(e) << "\n";
            int n = negamax(board, SEARCH_DEPTH, -1000000, 1000000);
            int fromMover = board.whiteToMove ? n : -n;
            std::cout << "Search depth " << SEARCH_DEPTH << " (from "
                      << (board.whiteToMove ? "White" : "Black") << "'s view): "
                      << evalStr(fromMover) << "\n";
            continue;
        }

        if (input == "hint" || input == "best") {
            std::cout << "Analyzing...\n";
            auto ranked = analyzeAllMoves(board, SEARCH_DEPTH);

            std::cout << "\nTop moves for "
                      << (board.whiteToMove ? "White" : "Black") << ":\n";
            int count = std::min<int>(5, ranked.size());
            for (int i = 0; i < count; ++i) {
                std::cout << "  " << moveToStr(ranked[i].move)
                          << "   eval: " << evalStr(ranked[i].score) << "\n";
            }
            std::cout << "\n>>> Suggested move: "
                      << moveToStr(ranked[0].move) << " <<<\n";
            continue;
        }

        if (input == "undo") {
            if (history.empty()) {
                std::cout << "Nothing to undo.\n";
                continue;
            }
            auto [m, u] = history.back();
            history.pop_back();
            unmakeMove(board, m, u);
            std::cout << "Undid " << moveToStr(m) << "\n";
            printBoard(board);
            continue;
        }

        Move m;
        if (!findMove(board, input, m)) {
            std::cout << "Illegal or unparseable move: " << input << "\n";
            std::cout << "Use form like 'e2e4', or 'e7e8q' for promotion.\n";
            continue;
        }

        UndoInfo u = makeMove(board, m);
        history.push_back({m, u});
        std::cout << "Played: " << moveToStr(m) << "\n";
        printBoard(board);
    }

    std::cout << "Goodbye.\n";
    return 0;
}