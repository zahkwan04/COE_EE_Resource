#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#define SLEEP(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP(ms) usleep((ms) * 1000)
#endif


#define W 61
#define H 25
#define PLANES 5
#define PI 3.14159265


typedef struct {
    char name[8];
    double x, y, heading, speed;   // heading: 0 = north, 90 = east
} Plane;


static char screen[H][W + 1];
static const char *airlines[] = {"BA", "LH", "AF", "DL", "EK", "QR", "KL", "UA"};


static double cx = W / 2, cy = H / 2;


static void spawn(Plane *p) {
    int side = rand() % 4;
    if (side == 0)      { p->x = rand() % W; p->y = 0; }
    else if (side == 1) { p->x = rand() % W; p->y = H - 1; }
    else if (side == 2) { p->x = 0;          p->y = rand() % H; }
    else                { p->x = W - 1;      p->y = rand() % H; }


    // aim roughly at the airport (x is halved because characters are tall)
    double dx = (cx - p->x) / 2, dy = cy - p->y;
    p->heading = atan2(dx, -dy) * 180 / PI + (rand() % 31 - 15);
    p->speed = 0.2 + (rand() % 5) / 10.0;
    snprintf(p->name, sizeof p->name, "%s%d", airlines[rand() % 8], 100 + rand() % 900);
}


static double dist_to_airport(const Plane *p) {
    double dx = (p->x - cx) / 2, dy = p->y - cy;
    return sqrt(dx * dx + dy * dy);
}


static void put(int x, int y, char c) {
    if (x >= 0 && x < W && y >= 0 && y < H) screen[y][x] = c;
}


static char arrow(double h) {
    h = fmod(h + 360, 360);
    if (h < 45 || h >= 315) return '^';
    if (h < 135) return '>';
    if (h < 225) return 'v';
    return '<';
}


int main(void) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(h, &mode)) SetConsoleMode(h, mode | 0x0004);
#endif
    srand((unsigned)time(NULL));
    Plane planes[PLANES];
    for (int i = 0; i < PLANES; i++) spawn(&planes[i]);
    int landed = 0;


    printf("\033[2J\033[?25l");
    for (int frame = 0; frame < 1000; frame++) {
        // clear screen buffer and draw range rings
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) screen[y][x] = ' ';
        for (int deg = 0; deg < 360; deg += 4) {
            put((int)(cx + 2 * 6 * sin(deg * PI / 180)),  (int)(cy - 6 * cos(deg * PI / 180)),  '.');
            put((int)(cx + 2 * 11 * sin(deg * PI / 180)), (int)(cy - 11 * cos(deg * PI / 180)), '.');
        }
        put((int)cx, (int)cy, '+');   // the airport


        // move planes
        for (int i = 0; i < PLANES; i++) {
            Plane *p = &planes[i];
            p->x += 2 * sin(p->heading * PI / 180) * p->speed;
            p->y -= cos(p->heading * PI / 180) * p->speed;
            if (dist_to_airport(p) < 1.5) { landed++; spawn(p); }
            else if (p->x < -2 || p->x > W + 2 || p->y < -2 || p->y > H + 2) spawn(p);
        }


        // draw planes, flag conflicts
        for (int i = 0; i < PLANES; i++) {
            Plane *p = &planes[i];
            int conflict = 0;
            for (int j = 0; j < PLANES; j++) {
                if (i == j) continue;
                double dx = (p->x - planes[j].x) / 2, dy = p->y - planes[j].y;
                if (sqrt(dx * dx + dy * dy) < 2.5) conflict = 1;
            }
            put((int)p->x, (int)p->y, conflict ? '!' : arrow(p->heading));
            int alt = (int)(dist_to_airport(p) * 300);   // lower as it gets closer
            char label[24];
            snprintf(label, sizeof label, "%s %dft", p->name, alt);
            for (int k = 0; label[k]; k++) put((int)p->x + 2 + k, (int)p->y, label[k]);
        }


        // output everything in one go
        printf("\033[H=== AIR TRAFFIC RADAR ===   Landed: %d\n", landed);
        for (int y = 0; y < H; y++) {
            screen[y][W] = '\0';
            printf("%s\n", screen[y]);
        }
        fflush(stdout);
        SLEEP(150);
    }
    printf("\033[?25h");
    return 0;
}