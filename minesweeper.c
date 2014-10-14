#include <curses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define KEY_SPACE ' '
#define KEY_RETURN '\r'

#define SQ_FLAGGED_CHAR   '@'
#define SQ_UNCLICKED_CHAR ' '

#define BEGINNER_NUM_MINES  10
#define BEGINNER_BOARD_SIZE 9

#define INTERMEDIATE_NUM_MINES  40
#define INTERMEDIATE_BOARD_SIZE 16

#define EXPERT_NUM_MINES  100
#define EXPERT_BOARD_SIZE 22

/* COLOR MACROS */
#define SET_RED (attron(COLOR_PAIR(1)))
#define SET_NORED (attroff(COLOR_PAIR(1)))

#define SET_BLUE (attron(COLOR_PAIR(2)))
#define SET_NOBLUE (attroff(COLOR_PAIR(2)))

#define SET_NORMAL (attron(COLOR_PAIR(3)))
#define SET_NONORMAL (attroff(COLOR_PAIR(3)))

#define SET_GREEN (attron(COLOR_PAIR(4)))
#define SET_NOGREEN (attroff(COLOR_PAIR(4)))

/* GLOBALS */
FILE *errlog;

typedef enum { FLAGGED, CLICKED, UNCLICKED } SquareState;

typedef enum { BEGINNER, INTERMEDIATE, EXPERT } Difficulty;

typedef struct square_t {
    struct square_t **touching;
    bool alreadyChecked;
    SquareState state;
    int numTouching;
    int surroundingMines;
    bool hasMine;
} Square;

typedef struct {
    Square *squares;
    char *horizLine;
    int x, y;
    int boardSize;
    int numMines;
    bool gameOver;
} Game;

typedef struct {
    int *positions;
    int numSurrounding;
} SurroundingSquaresResult;

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

void recursivelyClick(Square*);

char *makeHorizLine(int);

void logSquareInfo(Game*);

SurroundingSquaresResult *getSurroundingSquares(int, int);

Game *newGame(Difficulty);

int main(int argc, char *argv[])
{
    errlog = fopen("errlog.txt", "a");
    Difficulty d;
    /* check command line args */
    if (argc != 3)
        exit(1);
    if (strcmp(argv[1], "-d"))
        exit(1);
    switch (argv[2][0]) {
    case '1':
        d = BEGINNER;
        break;
    case '2':
        d = INTERMEDIATE;
        break;
    case '3':
        d = EXPERT;
        break;
    default:
        exit(1);
    }
    setUp();
    Game *g = newGame(d);
    while (!(g->gameOver)) {
        drawGame(g);
        pollInput(g);
    }
    free(g);
    quitWithError("Game over");
    return 0;
}

Game *newGame(Difficulty d)
{
    int numMines;
    int boardSize;
    Game *g = malloc(sizeof(Game));
    g->gameOver = false;
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
    int numSquares = boardSize * boardSize;

    g->boardSize = boardSize;
    g->numMines = numMines;
    g->horizLine = makeHorizLine(boardSize);

    g->squares = malloc(sizeof(Square) * numSquares);

    for (int i = 0; i < numSquares; ++i) {
        g->squares[i].state = UNCLICKED;
        g->squares[i].surroundingMines = 0;
        g->squares[i].hasMine = false;
        g->squares[i].alreadyChecked = false;
    }

    /* create the links between touching squares */
    for (int i = 0; i < numSquares; ++i) {
        SurroundingSquaresResult *r = getSurroundingSquares(i, boardSize);
        g->squares[i].numTouching = r->numSurrounding;
        g->squares[i].touching = malloc(sizeof(Square*) * r->numSurrounding);
        for (int j = 0; j < r->numSurrounding; ++j) {
            g->squares[i].touching[j] = (g->squares + r->positions[j]);
        }
    }
    
    int minesAdded = 0;
    while (minesAdded < numMines) {
        int pos = rand() % numSquares;
        if (!(g->squares[pos].hasMine)) {
            g->squares[pos].hasMine = true;
            /* update the surrounding squares' minesSurrounding counts */
            for (int i = 0; i < g->squares[pos].numTouching; ++i) {
                Square *adjSq = g->squares[pos].touching[i];
                adjSq->surroundingMines++;
            }
            ++minesAdded;
        }
    }
    return g;
}

void drawGame(Game *g)
{
    SET_BLUE;
    mvaddstr(0, 0, g->horizLine);
    SET_NOBLUE;
    for (int r = 0; r < g->boardSize; ++r) {
        SET_BLUE;
        mvaddch(r + 1, 0, '|');
        SET_NOBLUE;
        for (int c = 0; c < g->boardSize; ++c) {
            char sqChar;
            int sqPos = g->boardSize * r + c;
            switch (g->squares[sqPos].state) {
            case FLAGGED:
                sqChar = SQ_FLAGGED_CHAR;
                SET_NORMAL;
                break;
            case CLICKED:
                SET_GREEN;
                sqChar = (g->squares[sqPos].surroundingMines == 0) ? ' ' : (g->squares[sqPos].surroundingMines + '0');
                break;
            case UNCLICKED:
                sqChar = SQ_UNCLICKED_CHAR;
                SET_NORMAL;
                break;
            }
            mvaddch(r + 1, c + 1, sqChar);
        }
        SET_BLUE;
        addch('|');
        addch('\n');
    }
    addstr(g->horizLine);
    SET_NOBLUE;
    /* debug stuff */
    printw("\nx: %d\ny: %d\n", g->x, g->y);

    move(g->y + 1, g->x + 1);
}

void quitWithError(const char *msg)
{
    clear();
    endwin();
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
    start_color();
    /* initialize the color pairs */
    init_pair(1, COLOR_BLACK, COLOR_RED);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLUE);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);
    clear();
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
    case 'l':
        logSquareInfo(g);
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
            recursivelyClick(sq);
            for (int i = 0; i < g->boardSize * g->boardSize; ++i)
                g->squares[i].alreadyChecked = false;
        }
    } else if (sq->state == CLICKED && sq->surroundingMines > 0) {
        int surroundingFlags = 0;
        for (int i = 0; i < sq->numTouching; ++i) {
            if (sq->touching[i]->state == FLAGGED)
                ++surroundingFlags;
        }
        if (surroundingFlags == sq->surroundingMines)
            for (int i = 0; i < sq->numTouching; ++i) {
                if (sq->touching[i]->hasMine && sq->touching[i]->state != FLAGGED)
                    g->gameOver = true;
                else if (sq->touching[i]->state == UNCLICKED)
                    recursivelyClick(sq->touching[i]);
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

void recursivelyClick(Square *sq)
{
    if (sq->state != FLAGGED)
        sq->state = CLICKED;
    if (sq->surroundingMines > 0 || sq->alreadyChecked) {
        return;
    } else {
        sq->alreadyChecked = true;
        for (int i = 0; i < sq->numTouching; ++i) {
            if (!(sq->touching[i]->alreadyChecked)) {
                if (sq->touching[i]->state != FLAGGED)
                    sq->touching[i]->state = CLICKED;
                recursivelyClick(sq->touching[i]);
                sq->touching[i]->alreadyChecked = true;
            }
        }
    }
}

SurroundingSquaresResult *getSurroundingSquares(int pos, int boardSize)
{
    SurroundingSquaresResult *res = malloc(sizeof(SurroundingSquaresResult));

    if (pos == 0) {
        /* top left corner */
        res->numSurrounding = 3;
        res->positions = malloc(sizeof(int) * 3);
        res->positions[0] = pos + 1;
        res->positions[1] = pos + boardSize;
        res->positions[2] = pos + boardSize + 1;
    } else if (pos == boardSize * boardSize - 1) {
        /* bottom right corner */
        res->numSurrounding = 3;
        res->positions = malloc(sizeof(int) * 3);
        res->positions[0] = pos - 1;
        res->positions[1] = pos - boardSize;
        res->positions[2] = pos - boardSize - 1;
    } else if (pos % boardSize == 0) {
        /* left edge */
        if (pos + boardSize == boardSize * boardSize) {
            /* bottom left corner */
            res->numSurrounding = 3;
            res->positions = malloc(sizeof(int) * 3);
            res->positions[0] = pos + 1;
            res->positions[1] = pos - boardSize;
            res->positions[2] = pos - boardSize + 1;
        } else {
            /* left side */
            res->numSurrounding = 5;
            res->positions = malloc(sizeof(int) * 5);
            res->positions[0] = pos + 1;
            res->positions[1] = pos + boardSize;
            res->positions[2] = pos + boardSize + 1;
            res->positions[3] = pos - boardSize;
            res->positions[4] = pos - boardSize + 1;
        }
    } else if (pos % boardSize == boardSize - 1) {
        /* right edge */
        if (pos - boardSize == -1) {
            /* top right corner */
            res->numSurrounding = 3;
            res->positions = malloc(sizeof(int) * 3);
            res->positions[0] = pos - 1;
            res->positions[1] = pos + boardSize;
            res->positions[2] = pos + boardSize - 1;
        } else {
            /* right side */
            res->numSurrounding = 5;
            res->positions = malloc(sizeof(int) * 5);
            res->positions[0] = pos - 1;
            res->positions[1] = pos + boardSize;
            res->positions[2] = pos + boardSize - 1;
            res->positions[3] = pos - boardSize;
            res->positions[4] = pos - boardSize - 1;
        }
    } else if (pos + boardSize > boardSize * boardSize) {
        /* bottom edge */
        res->numSurrounding = 5;
        res->positions = malloc(sizeof(int) * 5);
        res->positions[0] = pos - 1;
        res->positions[1] = pos + 1;
        res->positions[2] = pos - boardSize;
        res->positions[3] = pos - boardSize - 1;
        res->positions[4] = pos - boardSize + 1;
    } else if (pos - boardSize < 0) {
        /* top edge */
        res->numSurrounding = 5;
        res->positions = malloc(sizeof(int) * 5);
        res->positions[0] = pos - 1;
        res->positions[1] = pos + 1;
        res->positions[2] = pos + boardSize;
        res->positions[3] = pos + boardSize - 1;
        res->positions[4] = pos + boardSize + 1;
    } else {
        /* not an edge square */
        res->numSurrounding = 8;
        res->positions = malloc(sizeof(int) * 8);
        res->positions[0] = pos + 1;
        res->positions[1] = pos - 1;
        res->positions[2] = pos + boardSize;
        res->positions[3] = pos - boardSize;
        res->positions[4] = pos + boardSize + 1;
        res->positions[5] = pos - boardSize + 1;
        res->positions[6] = pos + boardSize - 1;
        res->positions[7] = pos - boardSize - 1;
    }
    return res;
}

char *makeHorizLine(int n)
{
    char *result = malloc(n + 3);
    result[0] = '+';
    for (int i = 1; i <= n; ++i)
        result[i] = '-';
    result[n + 1] = '+';
    result[n + 2] = '\0';
    return result;
}

void logSquareInfo(Game *g)
{
    Square *sq = g->squares + g->y * g->boardSize + g->x;
    fprintf(errlog, "Square at (%d, %d):\n\tSurrounding Mines: %d\n\tTouching: %d squares\n\tState: %d\n\tHas mine: %d\n",
            g->x, g->y, sq->surroundingMines, sq->numTouching, sq->state), sq->hasMine;
    fflush(errlog);
}
