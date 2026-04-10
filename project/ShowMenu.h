#ifndef PROJECT_SHOWMENU_H
#define PROJECT_SHOWMENU_H

#include <iostream>
#include <SFML/Graphics.hpp>

#include <vector>
#include <string>
#include <deque>
#include <filesystem>
#include "ShowMenu.h"

#define MENU_X 400
#define MENU_Y 400
using namespace std;
namespace fs = std::filesystem;
class Menu_MP3 {
    sf::Font& font;
    float width, height;
    sf::RectangleShape mp3_menu_Bar;
    std::vector<sf::Text> texts;

    // 추가된 멤버 변수
    float scrollY = 0.f;
    float maxScroll = 0.f;
public:
    Menu_MP3(float width, float height, sf::Font& font, const std::vector<std::filesystem::path>& mp3_path);
    void draw(sf::RenderWindow& window);
    void update(sf::RenderWindow& window);
    int GetIndex(sf::Vector2f mouseCoords);
    void onScroll(float delta);
};

class Menu_SONG {
    sf::Font& font;
    sf::RectangleShape song_menu_bar;
    vector<sf::Text> texts;
    float width, height;
    vector<fs::path> osu_paths;

    // 추가된 멤버 변수
    float scrollY = 0.f;
    float maxScroll = 0.f;
public:
    Menu_SONG(float width, float height, sf::Font& font, const std::filesystem::path& mp3_path);
    void draw(sf::RenderWindow& window);
    void update(const sf::RenderWindow& window);
    fs::path GetIndex(sf::Vector2f mouseCoords);
    void onScroll(float delta);
};
string removeSpaces(std::string str);
vector<fs::path> get_4k_osus_from_mp3_dir(const fs::path& mp3_path);
bool check_osu(string filePath);
fs::path get_mp3_from_osu(const fs::path& osu_path);
#endif //PROJECT_SHOWMENU_H
