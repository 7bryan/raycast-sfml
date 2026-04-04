#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Window.hpp>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
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
    rect.setFillColor(sf::Color(100, 200, 100));

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

  void drawCeilingFloor(sf::RenderWindow &window) {
    sf::RectangleShape ceiling(sf::Vector2f{1600.f, 500.f});
    ceiling.setFillColor(sf::Color(100, 120, 160));
    window.draw(ceiling);

    sf::RectangleShape floor(sf::Vector2f{1600.f, 500.f});
    floor.setPosition({0.f, 500.f});
    floor.setFillColor(sf::Color(70, 60, 50));
    window.draw(floor);
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
  float rotSpeed = 0.04f;

  Player(float x, float y) : pos(x, y), angle(0) {}

  void update(Map &map) {
    // Rotation
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
      angle -= rotSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
      angle += rotSpeed;

    float dx = std::cos(angle) * moveSpeed;
    float dy = std::sin(angle) * moveSpeed;

    // movement
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
      if (!map.isWall(pos.x + dx * 3.0f, pos.y))
        pos.x += dx;
      if (!map.isWall(pos.x, pos.y + dy * 3.0f))
        pos.y += dy;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
      if (!map.isWall(pos.x - dx * 3.0f, pos.y))
        pos.x -= dx;
      if (!map.isWall(pos.x, pos.y - dy * 3.0f))
        pos.y -= dy;
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

  void castRays(sf::RenderWindow &window, Map &map, sf::Texture &tex) {
    float startAngle = angle - (FOV / 2.0f);
    float angleStep = FOV / static_cast<float>(NUM_RAYS);

    for (int i = 0; i < NUM_RAYS; i++) {
      float currentRayAngle = startAngle + (i * angleStep);
      float rayDirX = std::cos(currentRayAngle);
      float rayDirY = std::sin(currentRayAngle);

      // Which box of the map we're in
      int mapX = static_cast<int>(pos.x / TILE_SIZE);
      int mapY = static_cast<int>(pos.y / TILE_SIZE);

      // Length of ray from one x or y-side to next x or y-side
      // We use 1e30 to avoid division by zero
      float deltaDistX = (rayDirX == 0) ? 1e30f : std::abs(1.0f / rayDirX);
      float deltaDistY = (rayDirY == 0) ? 1e30f : std::abs(1.0f / rayDirY);

      float sideDistX, sideDistY;
      int stepX, stepY;

      // Calculate step and initial sideDist
      if (rayDirX < 0) {
        stepX = -1;
        sideDistX = (pos.x / TILE_SIZE - mapX) * deltaDistX;
      } else {
        stepX = 1;
        sideDistX = (mapX + 1.0 - pos.x / TILE_SIZE) * deltaDistX;
      }
      if (rayDirY < 0) {
        stepY = -1;
        sideDistY = (pos.y / TILE_SIZE - mapY) * deltaDistY;
      } else {
        stepY = 1;
        sideDistY = (mapY + 1.0 - pos.y / TILE_SIZE) * deltaDistY;
      }

      // Perform DDA
      int side; // 0 for X-side, 1 for Y-side
      while (true) {
        // Jump to next map square in X or Y direction
        if (sideDistX < sideDistY) {
          sideDistX += deltaDistX;
          mapX += stepX;
          side = 0;
        } else {
          sideDistY += deltaDistY;
          mapY += stepY;
          side = 1;
        }
        // booundary check
        if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT)
          break; // treat out of bounds as wall

        if (map.map[mapY][mapX])
          break;
      }

      // Calculate distance projected on camera direction (prevents fish-eye)
      float perpWallDist;
      if (side == 0)
        perpWallDist = (sideDistX - deltaDistX);
      else
        perpWallDist = (sideDistY - deltaDistY);

      if (perpWallDist < 0.1f)
        perpWallDist = 0.1f;

      // Calculate height of line to draw on screen
      float wallHeight =
          (1.0f / perpWallDist) * 800.f; // 800 is a scaling factor

      // --- TEXTURE MAPPING ---
      float wallX; // exact hit point on the wall (0.0 to 1.0)
      if (side == 0)
        wallX = (pos.y / TILE_SIZE) + perpWallDist * rayDirY;
      else
        wallX = (pos.x / TILE_SIZE) + perpWallDist * rayDirX;
      wallX -= std::floor(wallX);

      int texX = static_cast<int>(wallX * static_cast<float>(tex.getSize().x));

      // Flip texture if we're looking "backwards" to maintain consistency
      if ((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0))
        texX = tex.getSize().x - texX - 1;

      // Draw the vertical strip
      sf::RectangleShape wallStrip(
          sf::Vector2f{(1600.f / NUM_RAYS) + 0.5f, wallHeight});
      wallStrip.setTexture(&tex);
      wallStrip.setTextureRect(
          sf::IntRect({texX, 0}, {1, (int)tex.getSize().y}));

      // Apply Shading
      float distanceFade =
          std::min(1.0f, 15.0f / (perpWallDist * 5.0f)); // Simple darkness fade
      sf::Color wallColor =
          (side == 1) ? sf::Color(160, 160, 160) : sf::Color::White;

      float hitX = pos.x + rayDirX * perpWallDist * TILE_SIZE;
      float hitY = pos.y + rayDirY * perpWallDist * TILE_SIZE;

      float minimapScale = 0.2f;

      sf::Vertex rayline[] = {
          {pos * minimapScale, sf::Color(255, 0, 0)},
          {sf::Vector2f{hitX, hitY} * minimapScale, sf::Color(255, 0, 0)}};
      window.draw(rayline, 2, sf::PrimitiveType::Lines);

      // Combine distance fade with side shading
      wallStrip.setFillColor(sf::Color(wallColor.r * distanceFade,
                                       wallColor.g * distanceFade,
                                       wallColor.b * distanceFade));

      wallStrip.setPosition(
          {i * (1600.f / NUM_RAYS), (1000.f - wallHeight) / 2.f});
      window.draw(wallStrip);
    }
  }
};

int main() {
  sf::RenderWindow window(sf::VideoMode({1600, 1000}), "Raycasting Demo");
  window.setFramerateLimit(60);

  sf::Texture wallTexture;
  if (!wallTexture.loadFromFile("brick.jpg")) {
    // create a colored texture if the file is missing
    sf::Image placeholder;
    placeholder.resize({2, 2}, sf::Color::Red);
    placeholder.setPixel({0, 0}, sf::Color::Magenta);
    placeholder.setPixel({1, 1}, sf::Color::Magenta);
    wallTexture.loadFromImage(placeholder);
  }

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

    map.drawCeilingFloor(window);
    player.update(map);
    player.castRays(window, map, wallTexture);
    player.draw(window);
    map.draw(window);

    window.display();
  }

  return 0;
}
