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

const int MENU = 0, PLAYING = 1, GAME_OVER = 2;

// Maze symbols (change here if you change maze.txt)
const char WALL = 'X', PELLET = '*', POWER = '@', EMPTY = '_';
const char PLAYER_START = 'S', GHOST_START = 'E';

// ---------- Globals ----------
char maze[ROWS][COLS];
int gameState = PLAYING;   // switch to MENU on Day 6

// ---------- Functions ----------
void handleInput(sf::RenderWindow& window)
{
    while (const optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
            window.close();
    }
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