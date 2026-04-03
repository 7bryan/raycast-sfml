#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#define _USE_MATH_DEFINES
#include <cmath>
#include <optional>

// sfml version 3.0.2

const int MAP_WIDTH = 16;
const int MAP_HEIGHT = 10;
float TILE_SIZE = 100.f;

const float FOV = 60.f * (M_PI / 180.f); // convert to radian (60 degree)
const int NUM_RAYS = 120;                // number of rays to cast

class Map {
public:
  int map[MAP_HEIGHT][MAP_WIDTH] = {
      {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1}};

  void draw(sf::RenderWindow &window) {
    float minimapScale = 0.2f; // 20% of its original size
    sf::RectangleShape rect(sf::Vector2f{(TILE_SIZE * minimapScale) - 1.0f,
                                         (TILE_SIZE * minimapScale) - 1.0f});
    rect.setFillColor(sf::Color(100, 100, 100, 200));

    for (int i = 0; i < MAP_HEIGHT; i++) {
      for (int j = 0; j < MAP_WIDTH; j++) {
        if (map[i][j] == 1) {
          rect.setPosition(sf::Vector2f{j * TILE_SIZE * minimapScale,
                                        i * TILE_SIZE * minimapScale});
          window.draw(rect);
        }
      }
    }
  }

  bool isWall(float x, float y) { // determine if a specific coordinate is a
                                  // wall
    int mapX = static_cast<int>(x / TILE_SIZE);
    int mapY = static_cast<int>(y / TILE_SIZE);

    // stay inside the array bounds
    if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT)
      return true; // treat out of bounds as wall
    return map[mapY][mapX] == 1;
  }
};

class Player {
public:
  sf::Vector2f pos;
  float angle; // using radian
  float moveSpeed = 6.0f;
  float rotSpeed = 0.06f;

  Player(float x, float y) : pos(x, y), angle(0) {}

  void update() {
    // Rotation
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
      angle -= rotSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
      angle += rotSpeed;

    // movement
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
      pos.x += std::cos(angle) * moveSpeed;
      pos.y += std::sin(angle) * moveSpeed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
      pos.x -= std::cos(angle) * moveSpeed;
      pos.y -= std::sin(angle) * moveSpeed;
    }
  }

  void draw(sf::RenderWindow &window) {
    float minimapScale = 0.2f;
    sf::Vector2f minimapPos = pos * minimapScale;

    sf::CircleShape circle(5.f);
    circle.setFillColor(sf::Color::Red);
    circle.setOrigin({5.f, 5.f});
    circle.setPosition(minimapPos);
    window.draw(circle);

    // draw the direction line (nose)
    sf::Vertex line[] = {{minimapPos, sf::Color::Yellow},
                         {minimapPos + sf::Vector2f(std::cos(angle) * 10.f,
                                                    std::sin(angle) * 10.f),
                          sf::Color::Yellow}};
    window.draw(line, 2, sf::PrimitiveType::Lines);
  }

  void castRays(sf::RenderWindow &window, Map &map) {
    // calcualting the starting angle of the first ray
    float startAngle = angle - (FOV / 2.0f);

    // calculate the angle increment between each ray
    float angleStep = FOV / static_cast<float>(NUM_RAYS);

    for (int i = 0; i < NUM_RAYS; i++) {
      float currentRayAngle = startAngle + (i * angleStep);

      float rayX = pos.x;
      float rayY = pos.y;

      // direction for current ray
      float stepX = std::cos(currentRayAngle);
      float stepY = std::sin(currentRayAngle);

      // keep moveing until hit a wall
      while (!map.isWall(rayX, rayY)) {
        rayX += stepX;
        rayY += stepY;
      }

      // 3D Projection
      // calculate the raw distance
      float dx = rayX - pos.x;
      float dy = rayY - pos.y;
      float distance = std::sqrt(dx * dx + dy * dy);

      // fixing the fish eye effect
      distance *= std::cos(currentRayAngle - angle);

      if (distance < 1.0f)
        distance = 1.0f;

      // calculate the height of wall
      float wallHeight = (TILE_SIZE * 800.f) / distance;
      if (wallHeight > 1000.f)
        wallHeight =
            1000.f; // cap the wall height to prevent it from being too tall

      // drawing the 3D vertical line
      sf::RectangleShape wallStrip(
          sf::Vector2f{(1600.f / NUM_RAYS) + 1.0f, wallHeight});

      // making the further walls darker (lighting)
      int colorVal = static_cast<int>(200 - (distance / 5.f));
      if (colorVal < 50)
        colorVal = 50;
      wallStrip.setFillColor(sf::Color(colorVal, colorVal, colorVal));

      // center the wall strip vertically
      wallStrip.setPosition(
          {i * (1600.f / NUM_RAYS), (1000.f - wallHeight) / 2.f});

      window.draw(wallStrip);

      // draw the ray on the minimap
      float minimapScale = 0.2f;
      sf::Vertex rayLine[] = {
          {pos * minimapScale, sf::Color(255, 0, 0, 80)},
          {sf::Vector2f{rayX, rayY} * minimapScale, sf::Color(255, 0, 0, 80)}};
      window.draw(rayLine, 2, sf::PrimitiveType::Lines);
    }
  }
};

int main() {
  sf::RenderWindow window(sf::VideoMode({1600, 1000}), "Raycasting Demo");
  window.setFramerateLimit(60);

  Map map;
  Player player(300.f, 300.f);

  while (window.isOpen()) {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
      window.close();
    }

    window.clear(sf::Color::Black);
    map.draw(window);

    player.update();
    player.castRays(window, map);
    player.draw(window);

    window.display();
  }

  return 0;
}
