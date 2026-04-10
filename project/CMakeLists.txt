cmake_minimum_required(VERSION 4.1)
cmake_minimum_required(VERSION 3.20)
project(osuProject)

set(CMAKE_CXX_STANDARD 17)

# 1. 라이브러리 경로 설정
set(SFML_DIR "/opt/homebrew")
set(BASS_INC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(BASS_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libbass.dylib")

include_directories(${SFML_DIR}/include ${BASS_INC})

# 2. 개별 리소스 파일들 (중복 없는 루트 파일들만)
set(ROOT_ASSETS
        "effect.png"
        "output.wav"
)

# 3. 실행 파일 생성
add_executable(osuProject MACOSX_BUNDLE
        osumain.cpp
        PlayGame.cpp
        ShowMenu.cpp
        ${ROOT_ASSETS}
)

# 4. 라이브러리 링크
target_link_libraries(osuProject
        ${SFML_DIR}/lib/libsfml-graphics.dylib
        ${SFML_DIR}/lib/libsfml-window.dylib
        ${SFML_DIR}/lib/libsfml-system.dylib
        ${SFML_DIR}/lib/libsfml-audio.dylib
        ${BASS_LIB}
        "-framework CoreFoundation"
)

# 5. 번들 기본 속성
set_target_properties(osuProject PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.kimht.osu"
        MACOSX_BUNDLE_BUNDLE_NAME "RhythmGame"
)

# 6. [핵심] 폴더 구조를 유지하며 통째로 복사하기 ⭐
# 빌드가 끝난 후, 폴더 자체를 Contents/Resources 안으로 복사합니다.
add_custom_command(TARGET osuProject POST_BUILD
        # 1) 기존 Resources 폴더 안에 osu, font 폴더가 있다면 삭제(클린 빌드 효과)
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_BUNDLE_CONTENT_DIR:osuProject>/Resources/osu"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_BUNDLE_CONTENT_DIR:osuProject>/Resources/font"

        # 2) 폴더 통째로 복사 (이러면 light.png가 겹쳐도 폴더가 달라 상관없음)
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/osu" "$<TARGET_BUNDLE_CONTENT_DIR:osuProject>/Resources/osu"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/font" "$<TARGET_BUNDLE_CONTENT_DIR:osuProject>/Resources/font"

        # 3) 개별 루트 파일들 복사
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/effect.png" "$<TARGET_BUNDLE_CONTENT_DIR:osuProject>/Resources/"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/output.wav" "$<TARGET_BUNDLE_CONTENT_DIR:osuProject>/Resources/"

        # 4) BASS 라이브러리 복사 및 경로 수정 (이전과 동일)
        COMMAND ${CMAKE_COMMAND} -E copy "${BASS_LIB}" "$<TARGET_FILE_DIR:osuProject>/"
        COMMAND install_name_tool -change "libbass.dylib" "@executable_path/libbass.dylib" "$<TARGET_FILE:osuProject>"
)