#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <ctime>

class Bird {
public:
    sf::CircleShape shape;
    float velocity;
    float gravity;
    bool isAlive;
    
    Bird() {
        shape.setRadius(20);
        shape.setFillColor(sf::Color::Yellow);
        shape.setOutlineColor(sf::Color::Black);
        shape.setOutlineThickness(2);
        shape.setPosition(100, 300);
        velocity = 0;
        gravity = 0.5f;
        isAlive = true;
    }
    
    void update() {
        if (isAlive) {
            velocity += gravity;
            shape.move(0, velocity);
        }
    }
    
    void jump() {
        if (isAlive) {
            velocity = -8;
        }
    }
    
    void reset() {
        shape.setPosition(100, 300);
        velocity = 0;
        isAlive = true;
    }
};

class Pipe {
public:
    sf::RectangleShape topPipe;
    sf::RectangleShape bottomPipe;
    float speed;
    bool passed;
    
    Pipe(float x, float gapY, float gapHeight) {
        speed = -2;
        passed = false;
        
        // 创建上管道
        topPipe.setSize(sf::Vector2f(60, gapY));
        topPipe.setFillColor(sf::Color::Green);
        topPipe.setOutlineColor(sf::Color::Black);
        topPipe.setOutlineThickness(2);
        topPipe.setPosition(x, 0);
        
        // 创建下管道
        bottomPipe.setSize(sf::Vector2f(60, 600 - gapY - gapHeight));
        bottomPipe.setFillColor(sf::Color::Green);
        bottomPipe.setOutlineColor(sf::Color::Black);
        bottomPipe.setOutlineThickness(2);
        bottomPipe.setPosition(x, gapY + gapHeight);
    }
    
    void update() {
        topPipe.move(speed, 0);
        bottomPipe.move(speed, 0);
    }
    
    bool isOffScreen() {
        return topPipe.getPosition().x + 60 < 0;
    }
    
    bool checkCollision(const Bird& bird) {
        sf::FloatRect birdBounds = bird.shape.getGlobalBounds();
        sf::FloatRect topBounds = topPipe.getGlobalBounds();
        sf::FloatRect bottomBounds = bottomPipe.getGlobalBounds();
        
        return birdBounds.intersects(topBounds) || birdBounds.intersects(bottomBounds);
    }
    
    bool checkScore(const Bird& bird) {
        if (!passed && bird.shape.getPosition().x > topPipe.getPosition().x + 60) {
            passed = true;
            return true;
        }
        return false;
    }
};

class Game {
private:
    sf::RenderWindow window;
    Bird bird;
    std::vector<Pipe> pipes;
    int score;
    bool gameOver;
    sf::Font font;
    sf::Text scoreText;
    sf::Text gameOverText;
    sf::Text restartText;
    sf::Clock pipeClock;
    sf::Clock gameClock;
    
    std::mt19937 rng;
    std::uniform_real_distribution<float> gapDistribution;
    
public:
    Game() : window(sf::VideoMode(800, 600), "Flappy Bird"), score(0), gameOver(false) {
        rng.seed(std::time(nullptr));
        gapDistribution = std::uniform_real_distribution<float>(100, 400);
        
        // 加载字体
        if (!font.loadFromFile("arial.ttf")) {
            // 如果无法加载字体，使用默认字体
            std::cout << "Warning: Could not load font file. Using default font." << std::endl;
        }
        
        // 设置文本
        scoreText.setFont(font);
        scoreText.setCharacterSize(30);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(10, 10);
        
        gameOverText.setFont(font);
        gameOverText.setCharacterSize(50);
        gameOverText.setFillColor(sf::Color::Red);
        gameOverText.setString("GAME OVER");
        gameOverText.setPosition(250, 200);
        
        restartText.setFont(font);
        restartText.setCharacterSize(20);
        restartText.setFillColor(sf::Color::White);
        restartText.setString("Press SPACE to restart");
        restartText.setPosition(280, 300);
    }
    
    void handleInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    if (gameOver) {
                        restart();
                    } else {
                        bird.jump();
                    }
                }
            }
        }
    }
    
    void update() {
        if (!gameOver) {
            bird.update();
            
            // 检查小鸟是否撞到地面或天花板
            if (bird.shape.getPosition().y > 560 || bird.shape.getPosition().y < 0) {
                gameOver = true;
            }
            
            // 生成新管道
            if (pipeClock.getElapsedTime().asSeconds() > 2.0f) {
                float gapY = gapDistribution(rng);
                pipes.emplace_back(800, gapY, 150);
                pipeClock.restart();
            }
            
            // 更新管道
            for (auto& pipe : pipes) {
                pipe.update();
                
                // 检查碰撞
                if (pipe.checkCollision(bird)) {
                    gameOver = true;
                }
                
                // 检查得分
                if (pipe.checkScore(bird)) {
                    score++;
                }
            }
            
            // 移除屏幕外的管道
            pipes.erase(std::remove_if(pipes.begin(), pipes.end(),
                [](const Pipe& pipe) { return pipe.isOffScreen(); }), pipes.end());
        }
    }
    
    void render() {
        window.clear(sf::Color(135, 206, 235)); // 天蓝色背景
        
        // 绘制管道
        for (const auto& pipe : pipes) {
            window.draw(pipe.topPipe);
            window.draw(pipe.bottomPipe);
        }
        
        // 绘制小鸟
        window.draw(bird.shape);
        
        // 绘制分数
        scoreText.setString("Score: " + std::to_string(score));
        window.draw(scoreText);
        
        // 绘制游戏结束界面
        if (gameOver) {
            window.draw(gameOverText);
            window.draw(restartText);
        }
        
        window.display();
    }
    
    void restart() {
        bird.reset();
        pipes.clear();
        score = 0;
        gameOver = false;
        pipeClock.restart();
    }
    
    void run() {
        while (window.isOpen()) {
            handleInput();
            update();
            render();
            
            // 控制帧率
            sf::sleep(sf::milliseconds(16)); // 约60 FPS
        }
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}
