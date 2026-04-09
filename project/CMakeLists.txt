cmake_minimum_required(VERSION 4.1)
project(project)

set(CMAKE_CXX_STANDARD 17)

# Homebrew SFML 경로 설정
set(SFML_INCLUDE_DIR "/opt/homebrew/include")
set(SFML_LIBRARY_DIR "/opt/homebrew/lib")

# BASS 라이브러리 경로 설정 (프로젝트 내 include / lib 폴더)
set(BASS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(BASS_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

# 헤더 파일 경로 포함 (SFML + BASS)
include_directories(${SFML_INCLUDE_DIR} ${BASS_INCLUDE_DIR})

# 라이브러리 검색 경로 포함 (SFML + BASS)
link_directories(${SFML_LIBRARY_DIR} ${BASS_LIBRARY_DIR})

add_executable(project main.cpp)
add_executable(bitmap_app makeBitMap.cpp)
add_executable(osuProject osumain.cpp)

# 라이브러리 연결
target_link_libraries(project sfml-graphics sfml-window sfml-system sfml-audio)
target_link_libraries(bitmap_app sfml-graphics sfml-window sfml-system sfml-audio)

# osuProject에 SFML과 함께 'bass' 라이브러리 추가
target_link_libraries(osuProject sfml-graphics sfml-window sfml-system sfml-audio bass)