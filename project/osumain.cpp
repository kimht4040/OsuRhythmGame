#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <SFML/Audio.hpp>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <deque>
#include "bass.h"
#define JGLINE 500.f
#define GET_LANE(x, keyCount) ((x) * (keyCount) / 512)
#define NOTE_SPEED 0.5f



using namespace std;
class Effect {
private:
    sf::Sprite sprite;
    float lifetime;
    float maxLifetime;
    int lane;
public:
    Effect(const sf::Texture& texture, int lane): lifetime(20.f), maxLifetime(40.f), sprite(texture), lane(lane) {
        sprite.setPosition({100.f*static_cast<float>(lane)+165.f, JGLINE-95.f});
        sprite.setScale({0.7f, 0.7f});
    }

    void update(float dt) {
        lifetime -= dt;
        float alpha = (lifetime / maxLifetime) * 255;
        sprite.setColor(sf::Color(255, 255, 255, (unsigned char)alpha));
    }

    void draw(sf::RenderWindow& window) {
        window.draw(sprite);
    }

    bool isFinished() const { return lifetime <= 0.f; }
};

class Note {
public:
    int lane;
    long long startTime;
    long long endTime;
    bool isLongNote;
    bool isHolding = false;
    bool isProcessed=false;
    sf::RectangleShape shape;

    Note(long long startTime=0, long long endTime=0, bool isLongNote=0, int lane=-1)
        : startTime(startTime), endTime(endTime), isLongNote(isLongNote), lane(lane) {
        shape.setFillColor(sf::Color::Yellow);
        shape.setPosition({202.f + static_cast<float>(lane) * 100.f, 0.f});
        if (isLongNote) {
            // [FIX 4] 롱노트 길이를 NOTE_SPEED 기반으로 통일
            shape.setSize(sf::Vector2f({98.f, static_cast<float>(endTime - startTime) * NOTE_SPEED}));
        }
        else {
            shape.setSize(sf::Vector2f({98.f, 10.f}));
        }
    }

    void update(long long currentTime, float speed, float hitLineY) {
        auto timeGap = static_cast<float>(startTime - currentTime);
        float y = hitLineY - (timeGap * speed);
        shape.setPosition({shape.getPosition().x, y});
    }
};

enum class JudgeResult { Perfect, Good, None, Miss};
class Judgment {
    int score = 0;
    int combo = 0;
public:
    JudgeResult judge(long long key_time, long long map_time) {
        long long diff = std::abs(key_time - map_time);
        if (diff < 100) { score += 100; combo++; return JudgeResult::Perfect; }
        else if (diff < 200) { score += 50; combo++; return JudgeResult::Good; }
        // [FIX 1] Miss 판정 시 combo++ 제거 → BREAK() 호출로 교체
        else if (diff < 300) { BREAK(); return JudgeResult::Miss; }
        return JudgeResult::None;
    }
    void BREAK() { combo = 0; }
    int getScore() const { return score; }
    int getCombo() const { return combo; }
};


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

int main() {

    deque<Note> notes = parseOsuMania("osu/173612"
                                      " xi - FREEDOM DiVE/xi - FREEDOM DiVE (razlteh) [4K Normal].osu");

    if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
        std::cerr << "BASS 초기화 실패. 에러 코드: " << BASS_ErrorGetCode() << std::endl;
        return -1;
    }

    HSTREAM stream = BASS_StreamCreateFile(FALSE, "/Users/kimht4040/Desktop/code/miniproject/project/cmake-build-debug/osu/173612 xi - FREEDOM DiVE/Freedom Dive.mp3", 0, 0, 0);
    if (stream == 0) {
        std::cerr << "오디오 파일 로드 실패!" << std::endl;
        BASS_Free();
        return -1;
    }
    HSAMPLE hitSample = BASS_SampleLoad(FALSE, "output.wav", 0, 0, 10, BASS_SAMPLE_OVER_POS);
    if (hitSample == 0) {
        std::cerr << "타격음 로드 실패! 에러 코드: " << BASS_ErrorGetCode() << std::endl;
    }

    BASS_ChannelPlay(stream, FALSE);

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Rhythm Cpp");
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);

    Judgment judgment;

    vector<sf::Keyboard::Key> myKeys = {
        sf::Keyboard::Key::S, sf::Keyboard::Key::D, sf::Keyboard::Key::K, sf::Keyboard::Key::L
    };

    vector<sf::RectangleShape> lines;
    for (int i = 1; i <= 5; ++i) {
        sf::RectangleShape line(sf::Vector2f({2.f, 600.f}));
        line.setFillColor(sf::Color(100, 100, 100));
        line.setPosition(sf::Vector2f({200.f + (i - 1) * 100.f, 0.f}));
        lines.push_back(line);
    }

    sf::RectangleShape judgmentLine(sf::Vector2f({400.f, 5.f}));
    judgmentLine.setFillColor(sf::Color::Blue);
    judgmentLine.setPosition(sf::Vector2f({200.f, JGLINE}));

    sf::Font font;
    if (!font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
        std::cout << "폰트 로드 실패" << std::endl;
        return -1;
    }

    sf::Text comboText(font);
    comboText.setCharacterSize(40);
    comboText.setFillColor(sf::Color::Yellow);
    comboText.setPosition({400.f, 100.f});

    sf::Text judgeText(font);
    judgeText.setCharacterSize(50);
    judgeText.setFillColor(sf::Color::White);
    judgeText.setPosition({400.f, 250.f});

    sf::Text scoreText(font);
    scoreText.setCharacterSize(50);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition({400.f, 550.f});

    sf::Texture texture;
    if (!texture.loadFromFile("effect.png")) {
        return -1;
    }
    vector<Effect> effects;

    deque<Note> laneNotes[4];
    for (auto& n : notes) {
        laneNotes[n.lane].push_back(n);
    }

    float scrollSpeed = NOTE_SPEED;
    Note* holdingNotes[4] = { nullptr, nullptr, nullptr, nullptr };
    while (window.isOpen()) {

        QWORD bytePosition = BASS_ChannelGetPosition(stream, BASS_POS_BYTE);
        double timeInSeconds = BASS_ChannelBytes2Seconds(stream, bytePosition);
        long long currentMusicTime = static_cast<long long>(timeInSeconds * 1000.0);


        for (int i = 0; i < 4; ++i) {
            while (!laneNotes[i].empty()) {
                Note& front = laneNotes[i].front();

                long long deadline = (front.isLongNote && front.isHolding) ? front.endTime : front.startTime;

                if (currentMusicTime - deadline > 300) {
                    judgment.BREAK();
                    judgeText.setString("MISS");
                    judgeText.setFillColor(sf::Color::Magenta);
                    laneNotes[i].pop_front();
                } else {
                    break;
                }
            }
        }

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                for (int i = 0; i < 4; ++i) {
                    if (keyPressed->code == myKeys[i]) {
                        HCHANNEL hitChannel = BASS_SampleGetChannel(hitSample, FALSE);
                        BASS_ChannelPlay(hitChannel, FALSE);
                        if (!laneNotes[i].empty()) {
                            Note& target = laneNotes[i].front();
                            JudgeResult res = judgment.judge(currentMusicTime, target.startTime);
                            if (res != JudgeResult::None) {
                                if (target.isLongNote) {
                                    target.isHolding = true;
                                }
                                else {
                                    effects.emplace_back(texture, i);
                                    laneNotes[i].pop_front();
                                }
                            }
                            if (res == JudgeResult::Perfect) {
                                judgeText.setString("PERFECT");
                                judgeText.setFillColor(sf::Color::Cyan);
                            } else if (res == JudgeResult::Good) {
                                judgeText.setString("GOOD");
                                judgeText.setFillColor(sf::Color::Green);
                            } else if (res == JudgeResult::Miss) {
                                judgeText.setString("MISS");
                                judgeText.setFillColor(sf::Color::Magenta);
                            }
                        }
                    }
                }
            }
            if (const auto* KeyReleased = event->getIf<sf::Event::KeyReleased>()) {
                for (int i = 0; i < 4; ++i) {
                    if (KeyReleased->code == myKeys[i]) {
                        if (!laneNotes[i].empty()) {
                            Note& target = laneNotes[i].front();
                            if (target.isLongNote && target.isHolding) {
                                HCHANNEL hitChannel = BASS_SampleGetChannel(hitSample, FALSE);
                                BASS_ChannelPlay(hitChannel, FALSE);
                                JudgeResult res = judgment.judge(currentMusicTime, target.endTime);
                                if (res != JudgeResult::None) {
                                    laneNotes[i].pop_front();
                                    effects.emplace_back(texture, i);
                                }
                                if (res == JudgeResult::Perfect) {
                                    judgeText.setString("PERFECT");
                                    judgeText.setFillColor(sf::Color::Cyan);
                                } else if (res == JudgeResult::Good) {
                                    judgeText.setString("GOOD");
                                    judgeText.setFillColor(sf::Color::Green);
                                } else if (res == JudgeResult::Miss) {
                                    judgeText.setString("MISS");
                                    judgeText.setFillColor(sf::Color::Magenta);
                                }
                            }
                        }
                    }
                }
            }
        }

        for (auto it = effects.begin(); it != effects.end(); ) {
            it->update(0.5f);
            if (it->isFinished()) {
                it = effects.erase(it);
            } else {
                ++it;
            }
        }

        comboText.setString(std::to_string(judgment.getCombo()));
        scoreText.setString(std::to_string(judgment.getScore()));


        window.clear(sf::Color::Black);

        for (const auto& line : lines) {
            window.draw(line);
        }

        for (auto& lane : laneNotes) {
            for (auto& note : lane) {
                if (note.isProcessed) continue;
                long long timeDiff = note.startTime - currentMusicTime;
                note.update(currentMusicTime, NOTE_SPEED, JGLINE);
                if (note.isLongNote) {
                    if (timeDiff <= 6000 && timeDiff > -150) {
                        window.draw(note.shape);
                    }
                } else {
                    if (timeDiff <= 1500 && timeDiff > -150) {
                        window.draw(note.shape);
                    }
                }
            }
        }

        if (judgeText.getString() != "") {
            sf::FloatRect jRect = judgeText.getLocalBounds();
            judgeText.setOrigin(jRect.getCenter());
            window.draw(judgeText);
        }

        if (judgment.getCombo() > 0) {
            sf::FloatRect CRect = comboText.getLocalBounds();
            comboText.setOrigin(CRect.getCenter());
            window.draw(comboText);
        }

        // 점수 텍스트 출력
        {
            sf::FloatRect sRect = scoreText.getLocalBounds();
            scoreText.setOrigin({0.f, 0.f});
            window.draw(scoreText);
        }

        for (auto& effect : effects) {
            effect.draw(window);
        }

        window.draw(judgmentLine);
        window.display();

        if (BASS_ChannelIsActive(stream) == BASS_ACTIVE_STOPPED) break;
    }

    BASS_SampleFree(hitSample);
    BASS_StreamFree(stream);
    BASS_Free();
    return 0;
}