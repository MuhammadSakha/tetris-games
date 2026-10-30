#include <SFML/Graphics.hpp>
#include <iostream>
#include <ctime>
#include <cstdlib>
using namespace sf;
using namespace std;

const int M = 20;
const int N = 10;
const int TILE = 18;

const int OFFX = 40;
const int OFFY = 46;
const float SC = 0.95f;

const int WIN_W = 460;
const int WIN_H = 480;
const int PANEL_X = 240;

const float NORMAL_DELAY = 0.3f;
const float FAST_DELAY = 0.05f;

const int figures[7][4] {
    1, 3, 5, 7, // I 
    2, 4, 5, 7, // Z
    3, 5, 4, 6, // S
    3, 5, 4, 7, // T
    2, 3, 5, 7, // L
    3, 5, 7, 6, // J
    2, 3, 4, 5, // O
};

struct Point {
    int x, y;
};

class Tetromino {
    int type_;
    int color_;
    Point cells_[4];

public:
    Tetromino(int type = 0, int color = 1) : type_(type), color_(color) {
        spawn();
    }

    // Build random block
    static Tetromino random() {
        return Tetromino(rand() % 7, 1 + rand() % 7);
    }

    void spawn() {
        for (int i = 0; i < 4; i++) {
            cells_[i].x = figures[type_][i] % 2 + N / 2 - 1;
            cells_[i].y = figures[type_][i] / 2;
        }
    }

    void move(int dx, int dy) {
        for (int i = 0; i < 4; i++) {
            cells_[i].x += dx;
            cells_[i].y += dy;
        }
    }

    // Rotate
    void rotate() {
        Point p = cells_[1];
        for (int i = 0; i < 4; i++) {
            int x = cells_[i].y - p.y;
            int y = cells_[i].x - p.x;
            cells_[i].x = p.x - x;
            cells_[i].y = p.y + y;
        }
    }

    //getter
    int type() const { return type_; }
    int color() const { return color_; }
    const Point &cell(int i) const { return cells_[i]; }

    Point shapeCell(int i) const {
        return Point{figures[type_][i] % 2, figures[type_][i] / 2};
    }
};


class Board {
    int field[M][N];

public:
    Board() { reset(); }

    void reset() {
        for (int i = 0; i < M; i++)
            for (int j = 0; j < N; j++) field[i][j] = 0;
    }

    int at(int y, int x) const { return field[y][x]; }

    bool isValid(const Tetromino &t) const {
        for (int i = 0; i < 4; i++){
            int x = t.cell(i).x;
            int y = t.cell(i).y;
            if (x < 0 || x >= N || y < 0 || y >= M) return false;
            if (field[y][x]) return false;
        }
    return true;
    }

    void lock(const Tetromino &t) {
        for (int i = 0; i < 4; i++)
            field[t.cell(i).y][t.cell(i).x] = t.color();
    }

    // Delete full rows and return the number of rows deleted
    int clearLines() {
        int k = M - 1;
        int cleared = 0;
        for (int i = M - 1; i >= 0; i--) {
            int count = 0;
            for (int j = 0; j < N; j++)
                if (field[i][j]) count++;

            if (count < N) {
                if (k != i )
                    for (int j = 0; j < N; j++) field[k][j] = field[i][j];
                k--;
            } else {
                cleared++;
            }
        }
        for (int i = 0; i <= k; i++)
            for (int j = 0; j < N; j++) field[i][j] = 0;
        return cleared;
    }
};

class Game {
    Board board;
    Tetromino current;
    Tetromino next[2];
    int score;

    float timer;
    int dx;
    bool rotateReq, skipReq;

    // Graphic assets
    Texture tTiles, tBackground, tFrame;
    Font font;
    Sprite sTile, sBackground, sFrame;
    RectangleShape panel;

    // Logic Game
    void reset() {
        board.reset();
        score = 0;
        timer = 0;
        current = Tetromino::random();
        next[0] = Tetromino::random();
        next[1] = Tetromino::random();
    }

    void advanceQueue() {
        current = next[0];
        next[0] = next[1];
        next[1] = Tetromino::random();
    }

    void trySkip() {
        Tetromino candidate = next[0];
        if (!board.isValid(candidate)) return;
        advanceQueue();
    }

    void tick() {
        Tetromino t = current;
        t.move(0, 1);
        if (board.isValid(t)) {
            current = t;
            return;
        }

        board.lock(current);
        score += 10;

        int cleared = board.clearLines();
        static const int points[5] = {0, 100, 300, 500, 800};
        if (cleared >= 1 && cleared <= 4) score += points[cleared];

        advanceQueue();
        if (!board.isValid(current)) reset();
    }

    void drawCell(RenderWindow &w, int color, float x, float y, float scale) {
        sTile.setTextureRect(IntRect(color * TILE, 0, TILE, TILE));
        sTile.setScale(scale, scale);
        sTile.setPosition(x, y);
        w.draw(sTile);
    }

    void drawPreview(RenderWindow &w, const Tetromino &t, float px, float py) {
        for (int i = 0; i < 4; i++) {
            Point c = t.shapeCell(i);
            drawCell(w, t.color(), px + c.x * TILE, py + c.y * TILE, 1.f);
        }
    }

    void label(RenderWindow &w, const string &str, int size, float x, float y, Color c = Color::White) {
        Text txt(str, font, size);
        txt.setFillColor(c);
        txt.setPosition(x, y);
        w.draw(txt);
    }

    bool loadFont() {
        const char *paths[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/mnt/c/Windows/Fonts/arial.ttf",
            "images/font.ttf"
        };
        for (const char *p : paths)
            if (font.loadFromFile(p)) return true;
        return false;
    }

public:
    Game() : score(0), timer(0), dx(0), rotateReq(false), skipReq(false) {}

    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;

    bool load() {
        if (!tTiles.loadFromFile("images/tiles.png")) {
            cout << "Failed to load images/tiles.png" << endl;
            return false;
        }
        if (!tBackground.loadFromFile("images/background.png")) {
            cout << "Failed to load images/background.png" << endl;
            return false;
        }
        if (!tFrame.loadFromFile("images/frame.png")) {
            cout << "Failed to load images/frame.png" << endl;
            return false;
        }
        if (!loadFont()) {
            cout << "Font not found. Run: sudo apt install fonts-dejavu-core" << endl;
            return false;
        }

        sTile.setTexture(tTiles);
        sBackground.setTexture(tBackground);
        sBackground.setScale((float)WIN_W / tBackground.getSize().x,
                             (float)WIN_H / tBackground.getSize().y);
        sFrame.setTexture(tFrame);

        panel.setSize(Vector2f(WIN_W - PANEL_X - 10, 450));
        panel.setPosition(PANEL_X, 15);
        panel.setFillColor(Color(0, 0, 0, 200));

        reset();
        return true;
    }

    void handleEvent(const Event &e) {
        if (e.type != Event::KeyPressed) return;
        switch (e.key.code) {
            case Keyboard::Up:    rotateReq = true; break;
            case Keyboard::Left:  dx = -1;          break;
            case Keyboard::Right: dx = 1;           break;
            case Keyboard::Space: skipReq = true;   break;
            default: break;
        }
    }

    void update(float dt) {
        timer += dt;

        // Geser kiri/kanan: coba pada salinan, terapkan hanya jika valid
        if (dx != 0) {
            Tetromino t = current;
            t.move(dx, 0);
            if (board.isValid(t)) current = t;
        }

        // Rotasi
        if (rotateReq) {
            Tetromino t = current;
            t.rotate();
            if (board.isValid(t)) current = t;
        }

        if (skipReq) trySkip();

        // Tombol Down menurunkan jeda agar blok jatuh lebih cepat
        float delay = Keyboard::isKeyPressed(Keyboard::Down) ? FAST_DELAY : NORMAL_DELAY;
        if (timer > delay) {
            tick();
            timer = 0;
        }

        dx = 0;
        rotateReq = false;
        skipReq = false;
    }

    void draw(RenderWindow &w) {
        w.clear(Color::White);
        w.draw(sBackground);

        for (int i = 0; i < M; i++)
            for (int j = 0; j < N; j++)
                if (board.at(i, j))
                    drawCell(w, board.at(i, j),
                             j * TILE * SC + OFFX, i * TILE * SC + OFFY, SC);

        
        for (int i = 0; i < 4; i++)
            drawCell(w, current.color(),
                     current.cell(i).x * TILE * SC + OFFX,
                     current.cell(i).y * TILE * SC + OFFY, SC);

        w.draw(sFrame);   

        // Right panle
        w.draw(panel);
        const float X = PANEL_X + 12;

        label(w, "SCORE", 18, X, 25, Color::Yellow);
        label(w, to_string(score), 28, X, 48);

        label(w, "CURRENT BLOCK", 15, X, 110, Color::Yellow);
        drawPreview(w, current, X, 140);

        label(w, "NEXT", 15, X, 235, Color::Yellow);
        drawPreview(w, next[0], X, 265);
        drawPreview(w, next[1], X + 70, 265);
        label(w, "1", 14, X + 10, 345);
        label(w, "2", 14, X + 80, 345);

        label(w, "Space : change the block", 14, X, 385);
        label(w, "Up : Rotate", 14, X, 408);
        label(w, "Down : down quickly", 14, X, 431);

        w.display();
    }
};

int main() {
    srand(time(0));

    RenderWindow window(VideoMode(WIN_W, WIN_H), "Tetris");
    window.setFramerateLimit(60);

    Game game;
    if (!game.load()) return 1;

    Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) window.close();
            game.handleEvent(e);
        }

        game.update(dt);
        game.draw(window);
    }
    return 0;
}
