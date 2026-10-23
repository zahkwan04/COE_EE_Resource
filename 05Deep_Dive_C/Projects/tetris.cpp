// Console Tetris for Windows (C++11 or newer)
//
// Build (MinGW / MSYS2):  g++ tetris.cpp -o tetris.exe -O2
// Build (Visual Studio):  cl /EHsc /O2 tetris.cpp
//
// Controls: Left/Right = move, Up = rotate, Down = soft drop,
//           Space = hard drop, P = pause, Esc = quit

#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <string>

// ---------- Settings ----------
static const int FW = 12;           // field width incl. walls (10 playable)
static const int FH = 21;           // field height incl. floor (20 playable)
static const int SW = 46;           // screen width in characters
static const int SH = 24;           // screen height in characters

// ---------- Tetromino shapes (4x4 grids) ----------
static const char* SHAPES[7] = {
    "..X...X...X...X.",   // I
    ".....XX..XX.....",   // O
    "..X..XX...X.....",   // T
    ".....XX.XX......",   // S
    ".XX...XX........",   // Z
    ".X...X..XX......",   // J
    "..X...X...XX....",   // L
};

// Colors (background attributes), index 1..7 = pieces, 8 = line flash, 9 = wall
static const WORD COLORS[10] = {
    0,
    BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,   // I cyan
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,    // O yellow
    BACKGROUND_RED | BACKGROUND_BLUE,                            // T purple
    BACKGROUND_GREEN | BACKGROUND_INTENSITY,                     // S green
    BACKGROUND_RED | BACKGROUND_INTENSITY,                       // Z red
    BACKGROUND_BLUE | BACKGROUND_INTENSITY,                      // J blue
    BACKGROUND_RED | BACKGROUND_GREEN,                           // L orange-ish
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY, // flash
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,         // wall
};

static const WORD WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
static const WORD GREY  = FOREGROUND_INTENSITY;

static unsigned char field[FW * FH];
static CHAR_INFO screen[SW * SH];

// ---------- Helpers ----------
static int rotateIndex(int x, int y, int r) {
    switch (r & 3) {
        case 0: return y * 4 + x;
        case 1: return 12 + y - (x * 4);
        case 2: return 15 - (y * 4) - x;
        default: return 3 - y + (x * 4);
    }
}

static bool fits(int piece, int rot, int px, int py) {
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++) {
            if (SHAPES[piece][rotateIndex(x, y, rot)] != 'X') continue;
            int fx = px + x, fy = py + y;
            if (fx < 0 || fx >= FW || fy < 0 || fy >= FH) return false;
            if (field[fy * FW + fx] != 0) return false;
        }
    return true;
}

static void clearScreen() {
    for (int i = 0; i < SW * SH; i++) {
        screen[i].Char.AsciiChar = ' ';
        screen[i].Attributes = WHITE;
    }
}

static void put(int x, int y, char c, WORD attr) {
    if (x < 0 || x >= SW || y < 0 || y >= SH) return;
    screen[y * SW + x].Char.AsciiChar = c;
    screen[y * SW + x].Attributes = attr;
}

static void text(int x, int y, const std::string& s, WORD attr = WHITE) {
    for (size_t i = 0; i < s.size(); i++) put(x + (int)i, y, s[i], attr);
}

// A board cell is 2 characters wide so it looks square
static void cell(int x, int y, int v) {
    int sx = 2 + x * 2, sy = 1 + y;
    if (v == 0) {
        put(sx, sy, '.', GREY);
        put(sx + 1, sy, ' ', GREY);
    } else {
        put(sx, sy, ' ', COLORS[v]);
        put(sx + 1, sy, ' ', COLORS[v]);
    }
}

static void present(HANDLE out) {
    COORD size = { SW, SH }, origin = { 0, 0 };
    SMALL_RECT region = { 0, 0, SW - 1, SH - 1 };
    WriteConsoleOutputA(out, screen, size, origin, &region);
}

static void draw(HANDLE out, int cur, int rot, int px, int py, int next,
                 int score, int lines, int level, bool paused) {
    clearScreen();

    for (int y = 0; y < FH; y++)
        for (int x = 0; x < FW; x++)
            cell(x, y, field[y * FW + x]);

    if (!paused)
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++)
                if (SHAPES[cur][rotateIndex(x, y, rot)] == 'X')
                    cell(px + x, py + y, cur + 1);

    // Side panel
    int px0 = 30;
    text(px0, 1, "T E T R I S", FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    text(px0, 3, "SCORE: " + std::to_string(score));
    text(px0, 4, "LINES: " + std::to_string(lines));
    text(px0, 5, "LEVEL: " + std::to_string(level));
    text(px0, 7, "NEXT:");
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (SHAPES[next][rotateIndex(x, y, 0)] == 'X') {
                put(px0 + x * 2, 8 + y, ' ', COLORS[next + 1]);
                put(px0 + x * 2 + 1, 8 + y, ' ', COLORS[next + 1]);
            }
    text(px0, 13, "Left/Right  move", GREY);
    text(px0, 14, "Up          rotate", GREY);
    text(px0, 15, "Down        soft drop", GREY);
    text(px0, 16, "Space       hard drop", GREY);
    text(px0, 17, "P           pause", GREY);
    text(px0, 18, "Esc         quit", GREY);

    if (paused) text(8, 10, " PAUSED ", FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    present(out);
}

// ---------- Key handling with auto-repeat ----------
struct Key {
    int vk;
    int held;
    // mode 0: single press, 1: repeat (move), 2: continuous
    bool update(int mode) {
        if (GetAsyncKeyState(vk) & 0x8000) {
            held++;
            if (mode == 0) return held == 1;
            if (mode == 1) return held == 1 || (held > 6 && held % 2 == 0);
            return true;
        }
        held = 0;
        return false;
    }
};

// ---------- One game session; returns true to play again ----------
static bool playGame(HANDLE out) {
    for (int y = 0; y < FH; y++)
        for (int x = 0; x < FW; x++)
            field[y * FW + x] = (x == 0 || x == FW - 1 || y == FH - 1) ? 9 : 0;

    int cur = rand() % 7, next = rand() % 7;
    int rot = 0, px = FW / 2 - 2, py = 0;
    int score = 0, lines = 0, level = 1;
    int tick = 0, speed = 20;
    bool paused = false;

    Key kLeft = { VK_LEFT, 0 }, kRight = { VK_RIGHT, 0 }, kDown = { VK_DOWN, 0 };
    Key kUp = { VK_UP, 0 }, kSpace = { VK_SPACE, 0 }, kPause = { 'P', 0 };

    while (true) {
        Sleep(50);

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) return false;
        if (kPause.update(0)) paused = !paused;
        if (paused) {
            draw(out, cur, rot, px, py, next, score, lines, level, true);
            continue;
        }

        // --- Input ---
        if (kLeft.update(1) && fits(cur, rot, px - 1, py)) px--;
        if (kRight.update(1) && fits(cur, rot, px + 1, py)) px++;
        if (kDown.update(2) && fits(cur, rot, px, py + 1)) { py++; tick = 0; }
        if (kUp.update(0)) {
            int nr = rot + 1;
            if (fits(cur, nr, px, py)) rot = nr;
            else if (fits(cur, nr, px - 1, py)) { rot = nr; px--; }   // simple wall kicks
            else if (fits(cur, nr, px + 1, py)) { rot = nr; px++; }
            else if (fits(cur, nr, px - 2, py)) { rot = nr; px -= 2; }
            else if (fits(cur, nr, px + 2, py)) { rot = nr; px += 2; }
        }
        bool forceLock = false;
        if (kSpace.update(0)) {
            while (fits(cur, rot, px, py + 1)) py++;
            forceLock = true;
        }

        // --- Gravity ---
        tick++;
        if (tick >= speed || forceLock) {
            tick = 0;
            if (!forceLock && fits(cur, rot, px, py + 1)) {
                py++;
            } else {
                // Lock piece into the field
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++)
                        if (SHAPES[cur][rotateIndex(x, y, rot)] == 'X')
                            field[(py + y) * FW + (px + x)] = cur + 1;
                score += 25;

                // Find full lines
                int cleared = 0;
                for (int y = 0; y < 4; y++) {
                    int fy = py + y;
                    if (fy >= FH - 1) continue;
                    bool full = true;
                    for (int x = 1; x < FW - 1; x++)
                        if (field[fy * FW + x] == 0) { full = false; break; }
                    if (full) {
                        for (int x = 1; x < FW - 1; x++) field[fy * FW + x] = 8;
                        cleared++;
                    }
                }

                if (cleared > 0) {
                    draw(out, cur, rot, px, py, next, score, lines, level, false);
                    Sleep(350);   // flash
                    for (int y = FH - 2; y >= 0; y--) {
                        if (field[y * FW + 1] == 8) {
                            for (int yy = y; yy > 0; yy--)
                                for (int x = 1; x < FW - 1; x++)
                                    field[yy * FW + x] = field[(yy - 1) * FW + x];
                            for (int x = 1; x < FW - 1; x++) field[x] = 0;
                            y++;  // re-check same row index
                        }
                    }
                    static const int pts[5] = { 0, 100, 300, 500, 800 };
                    score += pts[cleared] * level;
                    lines += cleared;
                    level = lines / 10 + 1;
                    speed = 20 - (level - 1) * 2;
                    if (speed < 3) speed = 3;
                }

                // Next piece
                cur = next;
                next = rand() % 7;
                rot = 0; px = FW / 2 - 2; py = 0;

                if (!fits(cur, rot, px, py)) {
                    // Game over
                    draw(out, cur, rot, px, py, next, score, lines, level, false);
                    text(5, 9,  "                  ", FOREGROUND_RED | FOREGROUND_INTENSITY);
                    text(5, 10, "    GAME  OVER    ", FOREGROUND_RED | FOREGROUND_INTENSITY);
                    text(5, 11, " R = restart       ", WHITE);
                    text(5, 12, " Esc = quit        ", WHITE);
                    present(out);
                    Sleep(500);
                    while (true) {
                        if (GetAsyncKeyState('R') & 0x8000) return true;
                        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) return false;
                        Sleep(50);
                    }
                }
            }
        }

        draw(out, cur, rot, px, py, next, score, lines, level, false);
    }
}

int main() {
    srand((unsigned)time(nullptr));
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO ci = { 1, FALSE };
    SetConsoleCursorInfo(out, &ci);
    SetConsoleTitleA("Tetris");
    system("cls");

    while (playGame(out)) {}

    ci.bVisible = TRUE;
    SetConsoleCursorInfo(out, &ci);
    system("cls");
    return 0;
}