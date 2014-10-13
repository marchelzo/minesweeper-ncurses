#include <curses.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define KEY_SPACE ' '
#define KEY_RETURN '\r'

#define SQ_FLAGGED_CHAR   '#'
#define SQ_UNCLICKED_CHAR '*'

#define BEGINNER_NUM_MINES   10
#define BEGINNER_BOARD_SIZE 9

#define INTERMEDIATE_NUM_MINES   10
#define INTERMEDIATE_BOARD_SIZE 9

#define EXPERT_NUM_MINES   10
#define EXPERT_BOARD_SIZE 9

typedef enum { FLAGGED, CLICKED, UNCLICKED } SquareState;

typedef enum { BEGINNER, INTERMEDIATE, EXPERT } Difficulty;

typedef struct square_t {
    struct square_t **touching;
    SquareState state;
    uint8_t numTouching;
    uint8_t surroundingMines;
    bool hasMine;
} Square;

typedef struct {
    Square *squares;
    int x, y;
    uint16_t boardSize;
    uint16_t numMines;
    bool gameOver;
} Game;

void setUp(void);

void quitWithError(const char *msg);

void drawGame(Game*);

int moveLeft(Game*);

int moveRight(Game*);

int moveUp(Game*);

int moveDown(Game*);

void pollInput(Game*);

void clickSquare(Game*);

void flagSquare(Game*);

Game *newGame(Difficulty);

int main(int argc, char *argv[])
{
    setUp();
    Game *g = newGame(BEGINNER);
    while (!g->gameOver) {
        drawGame(g);
        pollInput(g);
    }
    free(g);
    return 0;
}

Game *newGame(Difficulty d)
{
    uint16_t numMines;
    uint16_t boardSize;
    Game *g = malloc(sizeof(Game));
    g->x = 0;
    g->y = 0;

    switch (d) {
    case BEGINNER:
        numMines = BEGINNER_NUM_MINES;
        boardSize = BEGINNER_BOARD_SIZE;
        break;
    case INTERMEDIATE:
        numMines = INTERMEDIATE_NUM_MINES;
        boardSize = INTERMEDIATE_BOARD_SIZE;
        break;
    case EXPERT:
        numMines = EXPERT_NUM_MINES;
        boardSize = EXPERT_BOARD_SIZE;
        break;
    default:
        quitWithError("Invalid difficulty specified");
    }

    /* calculate the number of total squares using the side length of the board */
    uint16_t numSquares = boardSize * boardSize;

    g->boardSize = boardSize;
    g->numMines   = numMines;

    g->squares = malloc(sizeof(Square) * numSquares);

    for (int i = 0; i < numSquares; ++i) {
        g->squares[i].state = UNCLICKED;
        g->squares[i].surroundingMines = 0;
        g->squares[i].hasMine = false;
        g->squares[i].touching = malloc(sizeof(Square*) * 8);
        g->squares[i].numTouching = 0;
    }

    /* create the links between touching squares */
    for (int i = 0; i < numSquares; ++i) {
        uint16_t surrounding[] = {i + 1,
                                  i - 1,
                                  i + boardSize,
                                  i + boardSize + 1,
                                  i + boardSize - 1,
                                  i - boardSize, i - boardSize + 1, i - boardSize - 1
                                  };
        for (int j = 0; j < 8; ++j) {
            if (surrounding[j] >= 0 && surrounding[j] < numSquares) {
                g->squares[i].touching[g->squares[i].numTouching++] = (g->squares + surrounding[j]);
            }
        }
    }
    
    int minesAdded = 0;
    while (minesAdded < numMines) {
        uint16_t pos = rand() % numSquares;
        if (!(g->squares[pos].hasMine)) {
            g->squares[pos].hasMine = true;
            /* update the surrounding squares' minesSurrounding counts */
            uint16_t surroundingSquares[] = {pos + 1, pos - 1, pos + boardSize, pos + boardSize + 1, pos + boardSize - 1,
                                            pos - boardSize, pos - boardSize + 1, pos - boardSize - 1};
            for (int i = 0; i < 8; ++i) {
                if (surroundingSquares[i] >= 0 && surroundingSquares[i] < numSquares) {
                    g->squares[surroundingSquares[i]].surroundingMines += 1;
                }
            }
            ++minesAdded;
        }
    }
    return g;
}

void drawGame(Game *g)
{
    clear();
    for (uint16_t r = 0; r < g->boardSize; ++r) {
        for (uint16_t c = 0; c < g->boardSize; ++c) {
            char sqChar;
            uint16_t sqPos = g->boardSize * r + c;
            switch (g->squares[sqPos].state) {
            case FLAGGED:
                sqChar = SQ_FLAGGED_CHAR;
                break;
            case CLICKED:
                sqChar = g->squares[sqPos].surroundingMines + '0';
                break;
            case UNCLICKED:
                sqChar = SQ_UNCLICKED_CHAR;
                break;
            }
            mvaddch(r, c, sqChar);
        }
        addch('\n');
    }
    /* debug stuff */
    printw("\nx: %d\ny: %d\n", g->x, g->y);
    move(g->y, g->x);
}

void quitWithError(const char *msg)
{
    fputs(msg, stderr);
    exit(EXIT_FAILURE);
}

void setUp(void)
{
    srand(time(NULL));
    initscr();
    noecho();
    raw();
    cbreak();
    keypad(stdscr, true);
}

int moveLeft(Game *g)
{
    if (g->x > 0) {
        g->x -= 1;
        return 1;
    } else {
        return 0;
    }
}

int moveRight(Game *g)
{
    if (g->x + 1 < g->boardSize) {
        g->x += 1;
        return 1;
    } else {
        return 0;
    }
}

int moveUp(Game *g)
{
    if (g->y > 0) {
        g->y -= 1;
        return 1;
    } else {
        return 0;
    }
}

int moveDown(Game *g)
{
    if (g->y + 1 < (int) g->boardSize) {
        g->y += 1;
        return 1;
    } else {
        return 0;
    }
}

void pollInput(Game *g)
{
    int pressed = getch();
    switch (pressed) {
    case KEY_UP:
        moveUp(g);
        break;
    case KEY_DOWN:
        moveDown(g);
        break;
    case KEY_RIGHT:
        moveRight(g);
        break;
    case KEY_LEFT:
        moveLeft(g);
        break;
    case KEY_RETURN:
        clickSquare(g);
        break;
    case KEY_SPACE:
        flagSquare(g);
        break;
    case 27:
        quitWithError("game over");
        break;
    }
    flushinp();
}

void clickSquare(Game *g)
{
    Square *sq = g->squares + (g->y * g->boardSize + g->x);
    if (sq->state == UNCLICKED) {
        if (sq->hasMine)
            g->gameOver = true;
        else {
            sq->state = CLICKED;
        }
    }
}

void flagSquare(Game *g)
{
    Square *sq = g->squares + (g->y * g->boardSize + g->x);
    if (sq->state == FLAGGED)
        sq->state = UNCLICKED;
    else if (sq->state == UNCLICKED)
        sq->state = FLAGGED;
}
