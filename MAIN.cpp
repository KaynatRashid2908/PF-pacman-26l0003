#include <SFML/Graphics.hpp>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <string>

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

const int MENU = 0, PLAYING = 1, GAME_OVER = 2, WIN = 3;

// Maze symbols (must match maze.txt exactly)
const char WALL = 'X', PELLET = '.', POWER = '0', EMPTY = '_';
const char PLAYER_START = 'S', GHOST_START = 'E';
constexpr int GHOST_COUNT = 4;
const int PELLET_SCORE = 10;
const int POWER_PELLET_SCORE = 50;      // spec: 50 points
const int POWER_MODE_FRAMES = 300;      // 5 seconds at 60 FPS

// ---------- Globals ----------
char maze[ROWS][COLS];
int gameState = MENU;
int playerRow = 0;
int playerCol = 0;
int ghostRows[GHOST_COUNT];
int ghostCols[GHOST_COUNT];
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
        movePlayer();
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
                playerCount++;
            }
            else if (cell == GHOST_START)
            {
                maze[row][col] = EMPTY;
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

    gameState = PLAYING;   // TEMPORARY: remove once your menu starts the game

    while (window.isOpen())
    {
        handleInput(window);
        updateGame();
        drawGame(window);
    }
    return 0;
}