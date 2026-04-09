#pragma once

#include <SFML/Graphics.hpp>
#include <deque>
#include <string>
#include <vector>

#define JGLINE 500.f
#define GET_LANE(x, keyCount) ((x) * (keyCount) / 512)
#define NOTE_SPEED 0.6f
#define RESOLUTION_X 1920
#define RESOLUTION_Y 1080
class Effect {
private:
    sf::Sprite sprite;
    float lifetime;
    float maxLifetime;
    int lane;
public:
    Effect(const sf::Texture& texture, int lane);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    bool isFinished() const;
};


class Note {
public:
    int lane;
    long long startTime;
    long long endTime;
    bool isLongNote;
    bool isHolding;
    bool isProcessed;
    sf::RectangleShape shape;
    sf::RectangleShape headshape;
    sf::RectangleShape tailshape;

    Note(long long startTime = 0, long long endTime = 0, bool isLongNote = false, int lane = -1);
    void update(long long currentTime, float speed, float hitLineY);
};

enum class JudgeResult { Perfect,Great, Good, None, Miss };

class Judgment {
    int score = 0;
    int combo = 0;
public:
    JudgeResult judge(long long key_time, long long map_time);
    void BREAK();
    int getScore() const;
    int getCombo() const;
};

std::deque<Note> parseOsuMania(std::string filePath);