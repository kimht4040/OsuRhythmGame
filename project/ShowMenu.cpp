#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <SFML/Audio.hpp>
#include <string>
#include "PlayGame.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include "ShowMenu.h"

#define MENU_X 400
#define MENU_Y 400
using namespace std;

namespace fs = std::filesystem;


string removeSpaces(std::string str) {
    str.erase(std::remove_if(str.begin(), str.end(), [](unsigned char c) {
        return std::isspace(c);
    }), str.end());
    return str;
}
bool check_osu(string filePath) {
    fstream file2;
    bool is_mania = false;
    bool is_4k = false;
    string index = "";
    string line;
    file2.open(filePath, ios::in);
    if (!file2.is_open()) {
        std::cerr << "파일을 열 수 없습니다: " << filePath << std::endl;
    }
    while (getline(file2, line)) {
        std::string cleanLine = removeSpaces(line);
        if (is_mania && is_4k) {
            break;
        }
        if (cleanLine=="[General]" || cleanLine == "[Difficulty]") {
            index = cleanLine;
        }
        if (index== "[General]" ) {
            if (cleanLine == "Mode:3") {
                is_mania = true;
                index = "";
            }

        }
        if (index == "[Difficulty]") {
            if (cleanLine == "CircleSize:4") {
                is_4k = true;
                index = "";
            }
        }

    }
    file2.close();
    return is_mania && is_4k;
}
vector<fs::path> get_4k_osus_from_mp3_dir(const fs::path& mp3_path) {
    vector<fs::path> matching_osus;

    // 1. mp3 파일이 위치한 "부모 폴더(디렉토리)" 경로 추출
    fs::path target_dir = mp3_path.parent_path();

    // 폴더가 실제로 존재하는지 안전 검사
    if (!fs::exists(target_dir) || !fs::is_directory(target_dir)) {
        cerr << "유효하지 않은 폴더 경로입니다: " << target_dir << '\n';
        return matching_osus;
    }

    // 2. 해당 폴더 내부만 탐색 (하위 폴더 탐색 안 함)
    for (const auto& entry : fs::directory_iterator(target_dir)) {
        // 일반 파일이고 확장자가 .osu 인 경우
        if (fs::is_regular_file(entry) && entry.path().extension() == ".osu") {

            // 3. 4K 매니아 맵인지 검사
            if (check_osu(entry.path().string())) {
                matching_osus.push_back(entry.path());
            }
        }
    }

    return matching_osus;
}
fs::path get_mp3_from_osu(const fs::path& osu_path) {
    // 1. .osu 파일이 들어있는 디렉토리 경로 추출
    // 예: "./osu/folder/beatmap.osu" -> "./osu/folder/"
    fs::path directory = osu_path.parent_path();

    // 2. 해당 디렉토리의 모든 파일을 순회
    for (const auto& entry : fs::directory_iterator(directory)) {
        // 3. 확장자가 .mp3인 파일을 찾으면 즉시 반환
        if (entry.is_regular_file() && entry.path().extension() == ".mp3") {
            return entry.path(); // 찾은 mp3의 전체 경로 반환
        }
    }

    // 파일을 못 찾았을 경우 빈 경로 반환
    return "";
}
Menu_MP3::Menu_MP3(float width, float height, sf::Font& font, const vector<fs::path>& mp3_path)
    : width(width), height(height), font(font) {

    mp3_menu_Bar.setSize({width + 400.f, RESOLUTION_Y});
    mp3_menu_Bar.setOrigin(mp3_menu_Bar.getSize() / 2.f);
    mp3_menu_Bar.setFillColor(sf::Color(50, 50, 50));
    mp3_menu_Bar.setPosition({static_cast<float>(RESOLUTION_X) / 2.f, static_cast<float>(RESOLUTION_Y) / 2.f});

    for (int i = 0; i < (int)mp3_path.size(); ++i) {
        sf::Text text(font, mp3_path[i].filename().string(), 30);

        // 중앙 정렬 기준점 설정
        sf::FloatRect bounds = text.getGlobalBounds();
        text.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});

        text.setFillColor(sf::Color::White);
        text.setPosition({
            static_cast<float>(RESOLUTION_X) / 2.f,
            ((static_cast<float>(RESOLUTION_Y) / 2.f) - height / 3.f) + ((height / 3.f) * static_cast<float>(i))
        });

        texts.push_back(text);
    }
    // Menu_MP3 생성자의 마지막 부분 (texts.push_back(text); 이후)
    if (!texts.empty()) {
        float lastItemBottom = texts.back().getPosition().y + 30.f;
        float menuBottom = (static_cast<float>(RESOLUTION_Y) / 2.f) + (height / 2.f);
        maxScroll = std::max(0.f, lastItemBottom - menuBottom + 50.f); // 50.f는 여백
    }
}

void Menu_MP3::draw(sf::RenderWindow& window) {
    window.draw(mp3_menu_Bar); // 배경은 스크롤되지 않음

    sf::RenderStates states;
    states.transform.translate({0.f, scrollY}); // Y축 이동 적용

    for (const auto& entry : texts) {
        window.draw(entry, states);
    }
}

void Menu_MP3::update(sf::RenderWindow& window) {
    sf::Vector2f mouseCoords = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    mouseCoords.y -= scrollY; // 스크롤된 만큼 마우스 좌표 보정

    for (auto& text : texts) {
        if (text.getGlobalBounds().contains(mouseCoords)) {
            text.setFillColor(sf::Color::Yellow);
        }
        else {
            text.setFillColor(sf::Color::White);
        }
    }
}

int Menu_MP3::GetIndex(sf::Vector2f mouseCoords) {
    mouseCoords.y -= scrollY; // 마우스 좌표 보정
    for (int i = 0; i < (int)texts.size(); ++i) {
        if (texts[i].getGlobalBounds().contains(mouseCoords)) {
            return i;
        }
    }
    return -1;
}


Menu_SONG::Menu_SONG(float width, float height, sf::Font& font, const fs::path& mp3_path)
    : font(font), width(width), height(height) {

    osu_paths = get_4k_osus_from_mp3_dir(mp3_path);

    song_menu_bar.setSize({width + 400.f, RESOLUTION_Y});
    song_menu_bar.setOrigin(song_menu_bar.getSize() / 2.f);
    song_menu_bar.setPosition({static_cast<float>(RESOLUTION_X) / 2.f, static_cast<float>(RESOLUTION_Y) / 2.f});
    song_menu_bar.setFillColor(sf::Color(60, 60, 60, 230));

    for (int i = 0; i < (int)osu_paths.size(); ++i) {
        sf::Text text(font, osu_paths[i].filename().string(), 30);

        sf::FloatRect bounds = text.getGlobalBounds();
        text.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});

        text.setPosition({
            static_cast<float>(RESOLUTION_X) / 2.f,
            ((static_cast<float>(RESOLUTION_Y) / 2.f) - height / 3.f) + ((height / 3.f) * static_cast<float>(i))
        });

        text.setFillColor(sf::Color::White);
        texts.push_back(text);
    }
    if (!texts.empty()) {
        float lastItemBottom = texts.back().getPosition().y + 60.f;
        float menuBottom = (static_cast<float>(RESOLUTION_Y) / 2.f) + (height / 2.f);
        maxScroll = std::max(0.f, lastItemBottom - menuBottom + 50.f);
    }
}

void Menu_SONG::draw(sf::RenderWindow& window) {
    window.draw(song_menu_bar);

    sf::RenderStates states;
    states.transform.translate({0.f, scrollY});

    for (const auto& t : texts) window.draw(t, states);
}

void Menu_SONG::update(const sf::RenderWindow& window) {
    sf::Vector2f mouseCoords = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    mouseCoords.y -= scrollY; // 스크롤된 만큼 마우스 좌표 보정 필수

    for (auto& text : texts) {
        if (text.getGlobalBounds().contains(mouseCoords)) {
            text.setFillColor(sf::Color::Yellow);
        }
        else {
            text.setFillColor(sf::Color::White);
        }
    }

}

fs::path Menu_SONG::GetIndex(sf::Vector2f mouseCoords) {
    mouseCoords.y -= scrollY; //여기서도 마우스 좌표 보정 필수
    for (int i = 0; i < (int)texts.size(); ++i) {
        if (texts[i].getGlobalBounds().contains(mouseCoords)) {
            return osu_paths[i];
        }
    }
    return "./no";
}

void Menu_MP3::onScroll(float delta) {
    scrollY += delta * 30.f;

    if (scrollY > 0.f) scrollY = 0.f; // 위로 더 못 올라가게 제한
    if (scrollY < -maxScroll) scrollY = -maxScroll;
}

void Menu_SONG::onScroll(float delta) {
    scrollY += delta * 30.f;

    if (scrollY > 0.f) scrollY = 0.f;
    if (scrollY < -maxScroll) scrollY = -maxScroll;
}



