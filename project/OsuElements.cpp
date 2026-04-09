#include "OsuElements.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

using namespace std;

Effect::Effect(const sf::Texture& texture, int lane) : lifetime(20.f), maxLifetime(40.f), sprite(texture), lane(lane) {
    sprite.setPosition({100.f * static_cast<float>(lane) + 165.f, JGLINE - 95.f});
    sprite.setScale({0.7f, 0.7f});
}

void Effect::update(float dt) {
    lifetime -= dt;
    float alpha = (lifetime / maxLifetime) * 255;
    sprite.setColor(sf::Color(255, 255, 255, (unsigned char)alpha));
}

void Effect::draw(sf::RenderWindow& window) {
    window.draw(sprite);
}

bool Effect::isFinished() const { return lifetime <= 0.f; }





Note::Note(long long startTime, long long endTime, bool isLongNote, int lane)
    : startTime(startTime), endTime(endTime), isLongNote(isLongNote), lane(lane), isHolding(false), isProcessed(false) {
    shape.setFillColor(sf::Color::Yellow);
    shape.setPosition({202.f + static_cast<float>(lane) * 100.f, 0.f});
    if (isLongNote) {
        // [FIX 4] 롱노트 길이를 NOTE_SPEED 기반으로 통일
        shape.setSize(sf::Vector2f({98.f, static_cast<float>(endTime - startTime) * NOTE_SPEED}));
    } else {
        shape.setSize(sf::Vector2f({98.f, 10.f}));
    }
}

void Note::update(long long currentTime, float speed, float hitLineY) {
    auto timeGap = static_cast<float>(startTime - currentTime);
    float y = hitLineY - (timeGap * speed);
    if (isLongNote) {
        shape.setPosition({shape.getPosition().x, y - shape.getSize().y});
    } else {
        shape.setPosition({shape.getPosition().x, y});
    }
}

JudgeResult Judgment::judge(long long key_time, long long map_time) {
    long long diff = std::abs(key_time - map_time);
    if (diff < 50) { score += 100; combo++; return JudgeResult::Perfect; }
    else if (diff < 100) { score += 50; combo++; return JudgeResult::Good; }
    // [FIX 1] Miss 판정 시 combo++ 제거 → BREAK() 호출로 교체
    else if (diff < 150) { BREAK(); return JudgeResult::Miss; }
    return JudgeResult::None;
}

void Judgment::BREAK() { combo = 0; }

int Judgment::getScore() const { return score; }

int Judgment::getCombo() const { return combo; }

std::deque<Note> parseOsuMania(std::string filePath) {
    std::deque<Note> notes;
    std::string line;
    bool isHitObjectSection = false;
    fstream file;
    file.open(filePath, ios::in);
    if (!file.is_open()) {
        std::cerr << "파일을 열 수 없습니다: " << filePath << std::endl;
        return notes;
    }
    while (getline(file, line)) {
        if (line.find("[HitObjects]") != string::npos) {
            isHitObjectSection = true;
            continue;
        }
        if (isHitObjectSection && !line.empty()) {
            stringstream ss(line);
            string s;
            vector<string> tokens;
            while (getline(ss, s, ',')) {
                tokens.push_back(s);
            }
            if (tokens.size() < 5) continue;

            int x = stoi(tokens[0]);
            long long st = stol(tokens[2]);
            bool isLong = (stoi(tokens[3]) & 128) != 0;
            long long et = 0;
            if (isLong) {
                size_t colonPos = tokens[5].find(':');
                if (colonPos != std::string::npos) {
                    et = std::stol(tokens[5].substr(0, colonPos));
                }
            }
            int laneIdx = GET_LANE(x, 4);

            Note note(st, et, isLong, laneIdx);
            notes.push_back(note);
        }
    }
    file.close();
    return notes;
}