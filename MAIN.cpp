#include <SFML/Graphics.hpp>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include<cstdlib>
#include<ctime>

using namespace std;

// ---------- Constants ----------
const int TILE = 30;
const int ROWS = 21;
const int COLS = 19;
const int HUD = 50;
const int WIN_W = COLS * TILE;          // 570
const int WIN_H = ROWS * TILE + HUD;    // 680
const int UP = 1;
const int DOWN = 2;
const int LEFT = 3;
const int RIGHT = 4;
const int PLAYER_SPEED_FRAMES = 8;
const int GHOST_MOVE_FRAMES = 10;
constexpr int GHOST_COUNT = 4;
const int BLUE_GHOST_MOVE_FRAMES = 16;
const int GHOST_EAT_SCORE = 200;
const int START_LIVES = 3;   // placeholder until Feature 6

const sf::Color GHOST_COLORS[GHOST_COUNT] =
{
    sf::Color::Red,                // ghost 0 (chaser)
    sf::Color(255, 184, 255),      // ghost 1 (wanderer)
    sf::Color::Cyan,               // ghost 2 (chaser)
    sf::Color(255, 184, 82)        // ghost 3 (wanderer)
};
const sf::Color BLUE_GHOST_COLOR(33, 33, 255);


const int MENU = 0, PLAYING = 1, GAME_OVER = 2, WIN = 3;

// Maze symbols (must match maze.txt exactly)
const char WALL = 'X', PELLET = '.', POWER = '0', EMPTY = '_';
const char PLAYER_START = 'S', GHOST_START = 'E';
const int PELLET_SCORE = 10;
const int POWER_PELLET_SCORE = 50;      // spec: 50 points
const int POWER_MODE_FRAMES = 300;      // 5 seconds at 60 FPS

// ---------- Globals ----------
char maze[ROWS][COLS];
int gameState = MENU;
int playerRow = 0;
int playerCol = 0;
int ghostRow[GHOST_COUNT];
int ghostCol[GHOST_COUNT];
int ghostDir[GHOST_COUNT];          // 0 = no direction yet
int ghostStartRow[GHOST_COUNT];
int ghostStartCol[GHOST_COUNT];
int ghostPrevRow[GHOST_COUNT];
int ghostPrevCol[GHOST_COUNT];
int ghostMoveCounter[GHOST_COUNT];
bool ghostBlue[GHOST_COUNT];

int playerStartRow = 0, playerStartCol = 0;
int playerPrevRow = 0, playerPrevCol = 0;
int lives = START_LIVES;

int requestedDirection = 0;   // 0 none, 1 up, 2 down, 3 left, 4 right
int moveCounter = 0;

int score = 0;
int pelletsRemaining = 0;
bool powerModeActive = false;
int powerModeTimer = 0;

bool mazeLoaded = false;
string mazeError = "";

// ---------- Functions ----------

void handleInput(sf::RenderWindow& window)
{
    while (const optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
        }
        else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
        {
            if (key->code == sf::Keyboard::Key::Up || key->code == sf::Keyboard::Key::W)
                requestedDirection = UP;
            else if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::S)
                requestedDirection = DOWN;
            else if (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::A)
                requestedDirection = LEFT;
            else if (key->code == sf::Keyboard::Key::Right || key->code == sf::Keyboard::Key::D)
                requestedDirection = RIGHT;
        }
    }
}

bool canMove(int row, int col)
{
    if (!mazeLoaded)
    {
        return false;
    }
    if (row < 0 || row >= ROWS)
    {
        return false;
    }
    if (col < 0 || col >= COLS)
    {
        return false;
    }
    if (maze[row][col] == WALL)
    {
        return false;
    }
    return true;
}
int oppositeDirection(int dir)
{
    if (dir == UP) return DOWN;
    if (dir == DOWN) return UP;
    if (dir == LEFT) return RIGHT;
    if (dir == RIGHT) return LEFT;
    return 0;
}

void getNextCell(int row, int col, int dir, int& newRow, int& newCol)
{
    newRow = row;
    newCol = col;
    if (dir == UP) newRow--;
    else if (dir == DOWN) newRow++;
    else if (dir == LEFT) newCol--;
    else if (dir == RIGHT) newCol++;
}

int distanceToPlayer(int row, int col)
{
    return abs(row - playerRow) + abs(col - playerCol);
}

void sendGhostHome(int i)
{
    ghostRow[i] = ghostStartRow[i];
    ghostCol[i] = ghostStartCol[i];
    ghostPrevRow[i] = ghostRow[i];
    ghostPrevCol[i] = ghostCol[i];
    ghostDir[i] = 0;
    ghostMoveCounter[i] = 0;
    ghostBlue[i] = false;
}

void resetGhosts()
{
    for (int i = 0; i < GHOST_COUNT; i++)
        sendGhostHome(i);
}

void turnGhostsBlue()
{
    for (int i = 0; i < GHOST_COUNT; i++)
        ghostBlue[i] = true;
}

void turnGhostsNormal()
{
    for (int i = 0; i < GHOST_COUNT; i++)
        ghostBlue[i] = false;
}

// Returns the direction ghost i should take, or 0 if it cannot move.
int chooseGhostDirection(int i)
{
    const int order[4] = { UP, LEFT, DOWN, RIGHT };   // tie-break order
    int options[4];
    int count = 0;
    int back = oppositeDirection(ghostDir[i]);

    for (int k = 0; k < 4; k++)
    {
        int d = order[k];
        if (d == back)
            continue;                                  // never turn back...
        int r, c;
        getNextCell(ghostRow[i], ghostCol[i], d, r, c);
        if (canMove(r, c))
            options[count++] = d;
    }

    if (count == 0)                                    // ...unless it is the only way
    {
        if (back == 0)
            return 0;
        int r, c;
        getNextCell(ghostRow[i], ghostCol[i], back, r, c);
        return canMove(r, c) ? back : 0;
    }

    bool chaser = (i == 0 || i == 2) && !ghostBlue[i];
    if (!chaser)
        return options[rand() % count];                // wanderers and blue ghosts

    int best = options[0];
    int r, c;
    getNextCell(ghostRow[i], ghostCol[i], best, r, c);
    int bestDist = distanceToPlayer(r, c);

    for (int k = 1; k < count; k++)
    {
        getNextCell(ghostRow[i], ghostCol[i], options[k], r, c);
        int dist = distanceToPlayer(r, c);
        if (dist < bestDist)                           // strict < keeps up, left, down, right order on ties
        {
            bestDist = dist;
            best = options[k];
        }
    }
    return best;
}

void moveGhosts()
{
    for (int i = 0; i < GHOST_COUNT; i++)
    {
        ghostMoveCounter[i]++;
        int needed = ghostBlue[i] ? BLUE_GHOST_MOVE_FRAMES : GHOST_MOVE_FRAMES;
        if (ghostMoveCounter[i] < needed)
            continue;
        ghostMoveCounter[i] = 0;

        int dir = chooseGhostDirection(i);
        if (dir == 0)
            continue;

        int r, c;
        getNextCell(ghostRow[i], ghostCol[i], dir, r, c);
        ghostRow[i] = r;
        ghostCol[i] = c;
        ghostDir[i] = dir;
    }
}

void savePreviousPositions()
{
    playerPrevRow = playerRow;
    playerPrevCol = playerCol;
    for (int i = 0; i < GHOST_COUNT; i++)
    {
        ghostPrevRow[i] = ghostRow[i];
        ghostPrevCol[i] = ghostCol[i];
    }
}

// Placeholder: adjust when you build Feature 6.
void playerCaught()
{
    lives--;
    if (lives <= 0)
    {
        gameState = GAME_OVER;
        return;
    }
    playerRow = playerStartRow;
    playerCol = playerStartCol;
    requestedDirection = 0;
    moveCounter = 0;
    powerModeActive = false;
    powerModeTimer = 0;
    resetGhosts();
}

// Returns true if the player was caught.
bool checkGhostCollisions()
{
    for (int i = 0; i < GHOST_COUNT; i++)
    {
        // Case 1: same cell after moving
        bool sameCell = (playerRow == ghostRow[i] && playerCol == ghostCol[i]);

        // Case 2: they swapped cells in the same step
        bool swapped = (playerRow == ghostPrevRow[i] && playerCol == ghostPrevCol[i] &&
            ghostRow[i] == playerPrevRow && ghostCol[i] == playerPrevCol);

        if (!sameCell && !swapped)
            continue;

        if (ghostBlue[i])
        {
            score += GHOST_EAT_SCORE;
            sendGhostHome(i);
        }
        else
        {
            playerCaught();
            return true;
        }
    }
    return false;
}

// Eats whatever is under the player. Called after every successful player move.
void checkPelletCollection()
{
    char currentTile = maze[playerRow][playerCol];

    if (currentTile == PELLET)
    {
        score += PELLET_SCORE;
        maze[playerRow][playerCol] = EMPTY;
        pelletsRemaining--;
    }
    else if (currentTile == POWER)
    {
        score += POWER_PELLET_SCORE;
        maze[playerRow][playerCol] = EMPTY;
        pelletsRemaining--;
        powerModeActive = true;
        powerModeTimer = POWER_MODE_FRAMES;
        turnGhostsBlue();                

        // Starts blue mode, or restarts the 5 seconds if it is already running.
        powerModeActive = true;
        powerModeTimer = POWER_MODE_FRAMES;
    }

    if (pelletsRemaining <= 0)
    {
        gameState = WIN;
    }
}

void updatePowerMode()
{
    if (!powerModeActive)
    {
        return;
    }

    powerModeTimer--;

    if (powerModeTimer <= 0)
    {
        powerModeTimer = 0;
        powerModeActive = false;
        turnGhostsNormal();
    }
}

void movePlayer()
{
    if (requestedDirection == 0)
    {
        return;
    }

    moveCounter++;

    if (moveCounter < PLAYER_SPEED_FRAMES)
    {
        return;
    }

    moveCounter = 0;

    int newRow = playerRow;
    int newCol = playerCol;

    if (requestedDirection == UP)
    {
        newRow--;
    }
    else if (requestedDirection == DOWN)
    {
        newRow++;
    }
    else if (requestedDirection == LEFT)
    {
        newCol--;
    }
    else if (requestedDirection == RIGHT)
    {
        newCol++;
    }

    // A direction towards a wall is ignored.
    if (!canMove(newRow, newCol))
    {
        return;
    }

    playerRow = newRow;
    playerCol = newCol;

    checkPelletCollection();
}

void updateGame()
{
    switch (gameState)
    {
    case MENU:
        break;
    case PLAYING:
        updatePowerMode();
        savePreviousPositions();
        movePlayer();
        if (gameState != PLAYING)
            break;      // player just won
        if (checkGhostCollisions())
            break;    // caught right after the player moved
        moveGhosts();
        checkGhostCollisions();               // caught after the ghosts moved
        break;
    
       
    case GAME_OVER:
        break;
    case WIN:
        break;
    }
}

void drawGame(sf::RenderWindow& window)
{
    window.clear(sf::Color::Black);
    switch (gameState)
    {
    case MENU:
        break;
    case PLAYING:
        break;
    case GAME_OVER:
        break;
    case WIN:
        break;
    }
    window.display();
}

bool loadMaze()
{
    // Reset all game data so loading is also safe for a restart.
    score = 0;
    pelletsRemaining = 0;
    powerModeActive = false;
    powerModeTimer = 0;
    requestedDirection = 0;
    moveCounter = 0;
    mazeLoaded = false;
    lives = START_LIVES;

    ifstream file("maze.txt");

    if (!file.is_open())
    {
        mazeError = "ERROR: Could not open maze.txt";
        return false;
    }

    int fileRows = 0;
    int fileCols = 0;

    if (!(file >> fileRows >> fileCols))
    {
        mazeError = "ERROR: First line must contain rows and columns";
        return false;
    }
    file.ignore(numeric_limits<streamsize>::max(), '\n');

    if (fileRows != ROWS || fileCols != COLS)
    {
        mazeError = "ERROR: Maze size must be 21 rows and 19 columns";
        return false;
    }

    int playerCount = 0;
    int ghostCount = 0;

    for (int row = 0; row < ROWS; row++)
    {
        string line;

        if (!getline(file, line))
        {
            mazeError = "ERROR: Maze does not have enough rows";
            return false;
        }

        for (int col = 0; col < COLS; col++)
        {
            char cell = EMPTY;

            if (col < (int)line.length())
            {
                cell = line[col];
            }

            if (cell == WALL || cell == EMPTY)
            {
                maze[row][col] = cell;
            }
            else if (cell == PELLET || cell == POWER)
            {
                maze[row][col] = cell;
                pelletsRemaining++;          // count both pellet types while loading
            }
            else if (cell == PLAYER_START)
            {
                maze[row][col] = EMPTY;
                playerRow = row;
                playerCol = col;
                playerStartRow = row;
                playerStartCol = col;
                playerCount++;
            }
            else if (cell == GHOST_START)
            {
                maze[row][col] = EMPTY;
                if (ghostCount < GHOST_COUNT)
                {
                    ghostStartRow[ghostCount] = row;
                    ghostStartCol[ghostCount] = col;

                }
                ghostCount++;
            }
            else
            {
                mazeError = "ERROR: Invalid symbol in maze.txt";
                return false;
            }
        }
    }

    if (playerCount != 1)
    {
        mazeError = "ERROR: Maze must contain exactly one S player start";
        return false;
    }

    if (ghostCount != GHOST_COUNT)
    {
        
            mazeError = "ERROR: Maze must contain exactly 4 E ghost starts";
        return false;
    }

    if (pelletsRemaining == 0)
    {
        mazeError = "ERROR: Maze has no pellets";
        return false;
    }
    resetGhosts();

    mazeLoaded = true;
    return true;
}

int main()
{
    if (!loadMaze())
    {
        cout << mazeError << "\n";
        return 1;
    }

    sf::RenderWindow window(sf::VideoMode({ WIN_W, WIN_H }), "Pac-Man");
    window.setFramerateLimit(60);
    srand((unsigned)time(0));

    gameState = PLAYING;   // TEMPORARY: remove once your menu starts the game


    while (window.isOpen())
    {
        handleInput(window);
        updateGame();
        drawGame(window);
    }
    return 0;
}