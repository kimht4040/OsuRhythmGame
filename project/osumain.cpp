#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <SFML/Audio.hpp>
#include <string>
#include <deque>
#include "bass.h"
#include "OsuElements.h"

using namespace std;

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

    vector<sf::RectangleShape> laneEffects;
    for (int i = 0; i < 4; ++i) {
        sf::RectangleShape laneEffect(sf::Vector2f({98.f, 5.f}));
        laneEffect.setFillColor(sf::Color(255, 255, 255, 50));
        laneEffect.setPosition(sf::Vector2f({200.f + i * 100.f, JGLINE}));
    }



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

                if (currentMusicTime - deadline > 150) {
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
                            for (auto it = laneNotes[i].begin(); it != laneNotes[i].end(); ++it) {
                                Note& target = *it;
                                if (target.isHolding) continue;

                                JudgeResult res = judgment.judge(currentMusicTime, target.startTime);
                                if (res != JudgeResult::None) {
                                    effects.emplace_back(texture, i);

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

                                    if (res == JudgeResult::Miss) {
                                        laneNotes[i].erase(it);
                                    } else if (target.isLongNote) {
                                        target.isHolding = true;
                                    } else {
                                        laneNotes[i].erase(it);
                                    }

                                    break;
                                }
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
                                
                                if (res == JudgeResult::None) {
                                    judgment.BREAK();
                                    judgeText.setString("MISS");
                                    judgeText.setFillColor(sf::Color::Magenta);
                                } else {
                                    effects.emplace_back(texture, i);
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
                                laneNotes[i].pop_front();
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
                    long long endDiff = note.endTime - currentMusicTime;
                    if (timeDiff <= 6000 && endDiff > -150) {
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