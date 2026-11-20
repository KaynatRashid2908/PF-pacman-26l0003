#include <SFML/Graphics.hpp>

int main()
{
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "My Game");
    window.setFramerateLimit(60);

    sf::RectangleShape player({ 50.f, 50.f });
    player.setFillColor(sf::Color::Cyan);
    player.setPosition({ 375.f, 275.f });

    while (window.isOpen())
    {
        // 1. INPUT
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        // 2. UPDATE
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            player.move({ -5.f, 0.f });
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            player.move({ 5.f, 0.f });
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            player.move({ 0.f, -5.f });
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            player.move({ 0.f, 5.f });

        // 3. DRAW
        window.clear();
        window.draw(player);
        window.display();
    }
}