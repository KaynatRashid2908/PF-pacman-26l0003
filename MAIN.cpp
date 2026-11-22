#include <SFML/Graphics.hpp>
#include <fstream>
#include <iostream>
#include <optional>
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

const int MENU = 0, PLAYING = 1, GAME_OVER = 2;

// Maze symbols (change here if you change maze.txt)
const char WALL = 'X', PELLET = '*', POWER = '@', EMPTY = '_';
const char PLAYER_START = 'S', GHOST_START = 'E';

// ---------- Globals ----------
char maze[ROWS][COLS];
int gameState = PLAYING;
int playerRow = 0;
int playerCol = 0;

int requestedDirection = 0;   // 0 none, 1 up, 2 down, 3 left, 4 right
int moveCounter = 0;

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
            if (key->code == sf::Keyboard::Key::Up ||
                key->code == sf::Keyboard::Key::W)
            {
                requestedDirection = UP;
            }
            else if (key->code == sf::Keyboard::Key::Down ||
                key->code == sf::Keyboard::Key::S)
            {
                requestedDirection = DOWN;
            }
            else if (key->code == sf::Keyboard::Key::Left ||
                key->code == sf::Keyboard::Key::A)
            {
                requestedDirection = LEFT;
            }
            else if (key->code == sf::Keyboard::Key::Right ||
                key->code == sf::Keyboard::Key::D)
            {
                requestedDirection = RIGHT;
            }
        }
    }
}
bool canMove(int row, int col)
{
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

void movePlayer()
{
    moveCounter++;

    if (moveCounter < PLAYER_SPEED_FRAMES)
    {
        return;
    }

    moveCounter = 0;

    if (requestedDirection == 0)
    {
        return;
    }

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
}

void updateGame()
{
    switch (gameState)
    {
    case MENU:      break;
    case PLAYING:   break;
    case GAME_OVER: break;
    }
}

void drawGame(sf::RenderWindow& window)
{
    window.clear(sf::Color::Black);
    switch (gameState)
    {
    case MENU:      break;
    case PLAYING:   break;
    case GAME_OVER: break;
    }
    window.display();
}
bool loadMaze()
{
    ifstream file("maze.txt");

    if (!file.is_open())
    {
        mazeError = "ERROR: Could not open maze.txt";
        mazeLoaded = false;
        return false;
    }

    int fileRows = 0;
    int fileCols = 0;

    if (!(file >> fileRows >> fileCols))
    {
        mazeError = "ERROR: First line must contain rows and columns";
        mazeLoaded = false;
        return false;
    }

    if (fileRows != ROWS || fileCols != COLS)
    {
        mazeError = "ERROR: Maze size must be 21 rows and 19 columns";
        mazeLoaded = false;
        return false;
    }

    bool playerFound = false;
    int ghostCount = 0;

    // Read every maze row safely.
    for (int row = 0; row < ROWS; row++)
    {
        string line;

        if (!getline(file, line))
        {
            mazeError = "ERROR: Maze does not have enough rows";
            mazeLoaded = false;
            return false;
        }

        for (int col = 0; col < COLS; col++)
        {
            char cell = EMPTY;

            if (col < (int)line.length())
            {
                cell = line[col];
            }

            if (cell == WALL)
            {
                maze[row][col] = WALL;
            }
            else if (cell == PELLET)
            {
                maze[row][col] = PELLET;
            }
            else if (cell == POWER)
            {
                maze[row][col] = POWER;
            }
            else if (cell == EMPTY)
            {
                maze[row][col] = EMPTY;
            }
            else if (cell == PLAYER_START)
            {
                maze[row][col] = EMPTY;
                playerRow = row;
                playerCol = col;
                playerFound = true;
            }
            else if (cell == GHOST_START)
            {
                maze[row][col] = EMPTY;
                ghostCount++;
            }
            else
            {
                mazeError = "ERROR: Invalid symbol in maze.txt";
                mazeLoaded = false;
                return false;
            }
        }
    }

    if (!playerFound)
    {
        mazeError = "ERROR: Maze must contain one S player start";
        mazeLoaded = false;
        return false;
    }

    if (ghostCount != 4)
    {
        mazeError = "ERROR: Maze must contain exactly 4 E ghost starts";
        mazeLoaded = false;
        return false;
    }

    mazeLoaded = true;
    return true;
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({ WIN_W, WIN_H }), "Pac-Man");
    window.setFramerateLimit(60);

    while (window.isOpen())
    {
        handleInput(window);
        updateGame();
        drawGame(window);
    }
    return 0;
}