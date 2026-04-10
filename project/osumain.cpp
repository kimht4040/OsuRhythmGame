#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <string>
#include <deque>
#include "bass.h"
#include "PlayGame.h"
#include "ShowMenu.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <SFML/Audio/Music.hpp>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>

void setMacOSResourcePath() {
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX)) {
        chdir(path);
    }
    CFRelease(resourcesURL);
}
#endif

using namespace std;
namespace fs = std::filesystem;
enum class GameState {
    Login,
    Menu,
    PlayGame,
    Result,
    Exit
};

struct MenuResult {
    GameState nextState = GameState::Menu;// 다음으로 갈 상태
    string id;
    fs::path selectedOsuPath = "";
    fs::path selectedMp3Path = "";
    int score = 0;
};
MenuResult result;

GameState ShowMenu(sf::RenderWindow& window, sf::Font& font) {
    fs::path project_path = "osu";
    vector<fs::path> mp3_path;
    vector<fs::path> osus_path;
    string target_mp3 = ".mp3";
    try {
        if (fs::is_directory(project_path)&& fs::exists(project_path)) {
            for (const auto& entry : fs::recursive_directory_iterator(project_path)) {
                if (fs::is_regular_file(entry)) {
                    if (entry.path().extension() == target_mp3) {
                        mp3_path.push_back(entry.path());
                    }
                }
            }

        }else {
            std::cerr << "경로가 존재하지 않거나 폴더가 아닙니다: " << project_path << '\n';
        }

    }catch (const fs::filesystem_error& e) {
        std::cerr << "파일 시스템 에러 발생: " << e.what() << '\n';
    }

    // 메뉴바 객체
    Menu_MP3 menu_mp3(MENU_X, MENU_Y, font, mp3_path);
    unique_ptr<Menu_SONG> menu_song = nullptr;
    // 곡선택 상태인지 난이도 선택 상태인지 체크
    bool is_song = false;



    // 프리뷰 노래 재생
    sf::Music previewMusic;
    // 메인루프
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (event->is<sf::Event::KeyPressed>()) {
                if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                    // ESC 키가 눌렸을 때
                    if (keyEvent->code == sf::Keyboard::Key::Escape) {
                        if (menu_song) {
                            is_song = false;
                            menu_song.reset(); //메모리 해제하여 접근 제어
                            previewMusic.stop();
                        }
                    }
                    if (keyEvent->code == sf::Keyboard::Key::Escape) {
                        return GameState::Exit;
                    }
                }
            }
            if (const auto* scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (scrollEvent->wheel == sf::Mouse::Wheel::Vertical) {
                    if (is_song && menu_song) {
                        menu_song->onScroll(scrollEvent->delta);
                    }
                    else {
                        menu_mp3.onScroll(scrollEvent->delta);
                    }
                }
            }
            if (event->is<sf::Event::MouseButtonPressed>()) {
                if (const auto* mouseEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (mouseEvent->button == sf::Mouse::Button::Left) {
                        // 마우스 좌표 변환 후 전달
                        sf::Vector2f mouseCoords = window.mapPixelToCoords(mouseEvent->position);
                        if (is_song && menu_song) {
                            result.selectedOsuPath = menu_song->GetIndex(mouseCoords);
                            if (!(result.selectedOsuPath == "./no")) {
                                result.selectedMp3Path = get_mp3_from_osu(result.selectedOsuPath);
                                result.nextState = GameState::PlayGame;
                                previewMusic.stop();
                                return GameState::PlayGame;
                            }



                        }
                        else {
                            //SONG 메뉴가 꺼져 있을 때만 MP3 메뉴 클릭 감지
                            int selected_mp3 = menu_mp3.GetIndex(mouseCoords);
                            if (selected_mp3 != -1) {
                                menu_song = std::make_unique<Menu_SONG>(MENU_X, MENU_Y, font, mp3_path[selected_mp3]);
                                is_song = true;
                                if (previewMusic.openFromFile(mp3_path[selected_mp3].string())) {
                                    previewMusic.setVolume(20.f);
                                    previewMusic.setLooping(true);
                                    previewMusic.setPlayingOffset(sf::seconds(10.f));
                                    previewMusic.play();
                                } else {
                                    std::cout << "오디오 파일을 불러오지 못했습니다: " << mp3_path[selected_mp3] << std::endl;
                                }
                            }
                        }
                    }
                }
            }

        }
        // 송 메뉴 메모리가 해제되면 is_song이 트루
        if (is_song) {
            menu_song->update(window);
        }
        else {
            menu_mp3.update(window);
        }
        window.clear();
        if (is_song) {
            menu_song->draw(window);
        }
        else {
            menu_mp3.draw(window);
        }
        window.display();
    }

    return GameState::PlayGame;
}
GameState PlayGame(sf::RenderWindow& window, const string &osuFile, const string mp3File, sf::Font &font, sf::Texture &texture, HSAMPLE hitSample) {
    // 파싱
    deque<Note> notes = parseOsuMania(osuFile);

    // 오디오 초기화
    HSTREAM stream = BASS_StreamCreateFile(FALSE, mp3File.c_str(), 0, 0, 0);
    if (stream == 0) {
        std::cerr << "오디오 파일 로드 실패!" << std::endl;
        BASS_Free();
    }



    Judgment judgment;

    vector<sf::Keyboard::Key> myKeys = {
        sf::Keyboard::Key::D, sf::Keyboard::Key::F, sf::Keyboard::Key::J, sf::Keyboard::Key::K
    };


    vector<sf::RectangleShape> lines;
    for (int i = 1; i <= 5; ++i) {
        sf::RectangleShape line(sf::Vector2f({2.f * SCALE_X, 1080.f * SCALE_Y}));
        line.setFillColor(sf::Color(100, 100, 100));
        line.setPosition(sf::Vector2f({(768.f + (i - 1) * 100.f) * SCALE_X-LINE_POSITION_X, 0.f}));
        lines.push_back(line);
    }

    sf::RectangleShape judgmentLine(sf::Vector2f({400.f * SCALE_X, 7.f * SCALE_Y}));
    judgmentLine.setFillColor(sf::Color(155,155,155,100));
    judgmentLine.setPosition(sf::Vector2f({768.f * SCALE_X-LINE_POSITION_X, JGLINE * SCALE_Y}));



    sf::Text comboText(font);
    comboText.setCharacterSize(static_cast<unsigned int>(50 * SCALE_Y));
    comboText.setFillColor(sf::Color::Yellow);
    comboText.setPosition({968.f * SCALE_X-LINE_POSITION_X, 250.f * SCALE_Y});

    sf::Text judgeText(font);
    judgeText.setCharacterSize(static_cast<unsigned int>(50 * SCALE_Y));
    judgeText.setFillColor(sf::Color::White);
    judgeText.setPosition({968.f * SCALE_X-LINE_POSITION_X, 450.f * SCALE_Y});

    sf::Text scoreText(font);
    scoreText.setCharacterSize(static_cast<unsigned int>(30 * SCALE_Y));
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition({968.f * SCALE_X-LINE_POSITION_X, 950.f * SCALE_Y});

    //노트 처리시 이펙트
    vector<Effect> effects;
    // 키입력시 누름 효과
    vector<Gear> gears;
    for (int i = 0; i < 4; ++i) {
        gears.emplace_back(i);
    }

    //라인별 노트 저장 큐
    deque<Note> laneNotes[4];
    for (auto& n : notes) {
        laneNotes[n.lane].push_back(n);
    }



    sf::Clock readyClock;
    bool isMusicPlaying = false;
    float delaySeconds = 2.0f;

    while (window.isOpen()) {
        float elapsed = readyClock.getElapsedTime().asSeconds();
        long long currentMusicTime = 0;

        if (!isMusicPlaying) {
            if (elapsed >= delaySeconds) {
                BASS_ChannelPlay(stream, FALSE);
                isMusicPlaying = true;
            }
            currentMusicTime = static_cast<long long>((elapsed - delaySeconds) * 1000.0);
        } else {

            QWORD bytePosition = BASS_ChannelGetPosition(stream, BASS_POS_BYTE);
            double seconds = BASS_ChannelBytes2Seconds(stream, bytePosition);
            currentMusicTime = static_cast<long long>(seconds * 1000.0);
        }
        for (int i = 0; i < 4; ++i) {
            while (!laneNotes[i].empty()) {
                Note& front = laneNotes[i].front();

                long long deadline = (front.isLongNote && front.isHolding) ? front.endTime : front.startTime;

                if (currentMusicTime - deadline > 200) {
                    judgment.BREAK();
                    judgeText.setString("MISS");
                    judgeText.setFillColor(sf::Color::Magenta);
                    sf::FloatRect bounds = judgeText.getLocalBounds();
                    judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                    judgeText.setScale({1.5f, 1.5f});
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
                if (keyPressed->code == sf::Keyboard::Key::Escape ) {
                    BASS_StreamFree(stream);
                    result.nextState = GameState::Menu;
                    return GameState::Menu;
                }
                for (int i = 0; i < 4; ++i) {
                    if (keyPressed->code == myKeys[i]) {
                        HCHANNEL hitChannel = BASS_SampleGetChannel(hitSample, FALSE);
                        //BASS_ATTRIB_VOL 히트사운드 볼륨 조절임 뒤에 벨류 값으로
                        BASS_ChannelSetAttribute(hitChannel, BASS_ATTRIB_VOL, 1.f);
                        BASS_ChannelPlay(hitChannel, FALSE);
                        if (!laneNotes[i].empty()) {
                            for (auto it = laneNotes[i].begin(); it != laneNotes[i].end(); ++it) {
                                Note& target = *it;
                                if (target.isHolding) {
                                    continue;
                                }

                                JudgeResult res = judgment.judge(currentMusicTime, target.startTime);
                                if (res != JudgeResult::None) {
                                    effects.emplace_back(texture, i);

                                    if (res == JudgeResult::Perfect) {
                                        judgeText.setString("PERFECT");
                                        judgeText.setFillColor(sf::Color::Cyan);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    }else if (res == JudgeResult::Great) {
                                        judgeText.setString("GREAT");
                                        judgeText.setFillColor(sf::Color::Yellow);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    }else if (res == JudgeResult::Good) {
                                        judgeText.setString("GOOD");
                                        judgeText.setFillColor(sf::Color::Green);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    } else if (res == JudgeResult::Miss) {
                                        judgeText.setString("MISS");
                                        judgeText.setFillColor(sf::Color::Magenta);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
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
                                    sf::FloatRect bounds = judgeText.getLocalBounds();
                                    judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                    judgeText.setScale({1.5f, 1.5f});
                                } else {
                                    effects.emplace_back(texture, i);
                                    if (res == JudgeResult::Perfect) {
                                        judgeText.setString("PERFECT");
                                        judgeText.setFillColor(sf::Color::Cyan);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    }else if (res == JudgeResult::Great) {
                                        judgeText.setString("GREAT");
                                        judgeText.setFillColor(sf::Color::Yellow);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    }else if (res == JudgeResult::Good) {
                                        judgeText.setString("GOOD");
                                        judgeText.setFillColor(sf::Color::Green);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    } else if (res == JudgeResult::Miss) {
                                        judgeText.setString("MISS");
                                        judgeText.setFillColor(sf::Color::Magenta);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    }
                                }
                                laneNotes[i].pop_front();
                            }
                        }
                    }
                }
            }
        }
        // 기어효과 적용
        for (int i = 0; i < 4; ++i) {
            bool isPressed = sf::Keyboard::isKeyPressed(myKeys[i]);
            gears[i].update(isPressed);
        }
        //노트 이펙트 사라지는 속도 설정dt
        for (auto it = effects.begin(); it != effects.end(); ) {
            it->update(.03f);
            judgment.update(1.f,judgeText);
            if (it->isFinished()) {
                it = effects.erase(it);
            } else {
                ++it;
            }
        }
        if (judgeText.getScale().x > 1.0f) {
            // 현재 크기에 0.95를 곱해서 점점 작아지게 만듭니다. (0.95를 조절해 속도 변경 가능)
            judgeText.setScale(judgeText.getScale() * 0.95f);

            // 크기가 1.0배 이하로 떨어지면 딱 1.0으로 고정합니다.
            if (judgeText.getScale().x < 1.0f) {
                judgeText.setScale({1.0f, 1.0f});
            }
        }

        comboText.setString(std::to_string(judgment.getCombo()));
        scoreText.setString(std::to_string(judgment.getScore()));


        window.clear(sf::Color::Black);

        for (int i = 0; i < 4; ++i) {
            gears[i].draw_beam(window);
        }

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
                        window.draw(note.headshape);
                        window.draw(note.tailshape);
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
        //기어 배경 그리기
        gears[0].draw_gear_bg(window);
        if (judgment.getCombo() > 0) {
            sf::FloatRect CRect = comboText.getLocalBounds();
            comboText.setOrigin(CRect.getCenter());
            window.draw(comboText);
        }
        // 기어 그리기
        for (int i = 0; i < 4; ++i) {
            gears[i].draw_gear(window);
        }

        // 점수 텍스트 출력
        sf::FloatRect sRect = scoreText.getLocalBounds();
        scoreText.setOrigin(sRect.getCenter());
        window.draw(scoreText);
        // 이펙트 효과 그리기
        for (auto& effect : effects) {
            effect.draw(window);
        }

        window.draw(judgmentLine);
        window.display();

        if (isMusicPlaying && BASS_ChannelIsActive(stream) == BASS_ACTIVE_STOPPED) {
            break;
        }
    }

    BASS_SampleFree(hitSample);
    BASS_StreamFree(stream);

    result.score = judgment.getScore();
    return GameState::Result;

};
GameState ShowResult(sf::RenderWindow& window, int score) {
    return GameState::Menu;
} // 점수 매개변수 추가 필요
int main() {
#ifdef __APPLE__
    setMacOSResourcePath(); // 맥일 때만 실행
#endif

    //윈도우 설정
    sf::RenderWindow window(sf::VideoMode({RESOLUTION_X, RESOLUTION_Y}), "Rhythm Cpp");
    window.setFramerateLimit(120);
    window.setKeyRepeatEnabled(false);
    // 베이스 초기화
    if (!BASS_Init(-1, 44100, 0, 0, nullptr)) {
        std::cerr << "BASS 초기화 실패 " << BASS_ErrorGetCode() << std::endl;
        return 1;
    }
    // 폰트 로드
    sf::Font font;
    if (!font.openFromFile("font/Anton.ttf")) {
        std::cout << "폰트 로드 실패" << std::endl;
    }
    //타격음 로드
    HSAMPLE hitSample = BASS_SampleLoad(FALSE,
        "output.wav",
        0, 0, 3, BASS_SAMPLE_OVER_POS );
    if (hitSample == 0) {
        std::cerr << "타격음 로드 실패 " << BASS_ErrorGetCode() << std::endl;
        return 1;
    }
    // 히트 이펙트 로드
    sf::Texture texture;
    if (!texture.loadFromFile("effect.png")) {
        cout << "이펙트 로드 실패" << endl;
        return 1;
    }

    GameState currentState = GameState::Menu;

    while (window.isOpen() && currentState != GameState::Exit) {
        if (currentState == GameState::Menu) {
            currentState = ShowMenu(window, font);
        }
        else if (currentState == GameState::PlayGame) {
            currentState = PlayGame(window,
                result.selectedOsuPath.string(),
                result.selectedMp3Path.string(),
                font, texture, hitSample);
        }
        else if (currentState == GameState::Result) {
            currentState = ShowResult(window, result.score);
        }
    }


    BASS_Free();
    return 0;
}

