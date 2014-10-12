#include <curses.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

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

typedef struct {
    SquareState state;
    uint8_t surroundingMines;
    bool hasMine;
} Square;

typedef struct {
    Square *squares;
    uint16_t boardSize;
    uint16_t numMines;
    bool gameOver;
} Game;

void quitWithError(const char *msg);

void drawGame(Game*);

Game *newGame(Difficulty);

int main(int argc, char *argv[])
{
    /* seed the random number generator */
    srand(time(NULL));
    Game *g = newGame(BEGINNER);
    initscr();
    drawGame(g);
    getch();
    return 0;
}

Game *newGame(Difficulty d)
{
    uint16_t numMines;
    uint16_t boardSize;
    Game *g = malloc(sizeof(Game));

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
        for (uint16_t c = 0; c < g->boardSize; ++r) {
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
}

void quitWithError(const char *msg)
{
    fputs(msg, stderr);
    exit(EXIT_FAILURE);
}
