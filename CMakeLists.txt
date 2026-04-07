cmake_minimum_required(VERSION 3.25)

set(n voice-interaction)
project(${n} CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(GGML_CUDA OFF CACHE BOOL "Enable CUDA acceleration via GGML")

# ---------- Fetch whisper.cpp from GitHub --------------------------------
include(FetchContent)
FetchContent_Declare(
  whisper
  GIT_REPOSITORY https://github.com/ggml-org/whisper.cpp.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)
# We only need the core library; skip building the bundled examples.
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
FetchContent_GetProperties(whisper)
if(NOT whisper_POPULATED)
  message(STATUS "[DOWNLOADING] Whisper speech recognition library from GitHub ...")
endif()
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(whisper)
set(CMAKE_WARN_DEPRECATED  ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.92.6
)

# ---------- Fetch ImGui from GitHub --------------------------------
message(STATUS "[DOWNLOADING] Dear ImGui library from GitHub ...")
FetchContent_MakeAvailable(imgui)

# Build ImGui as a static library
add_library(imgui_lib STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(imgui_lib PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui_lib PUBLIC SDL3::SDL3)

add_library(aipfg_lib STATIC
  src/aipfg/chat-client.cpp
  src/aipfg/http-client.cpp
  src/aipfg/elevenlabs.cpp
  src/aipfg/audio-player.cpp
  src/aipfg/http-streamer.cpp
)
target_include_directories(aipfg_lib PUBLIC src)
target_link_libraries(aipfg_lib PUBLIC
  SDL3::SDL3
  CURL::libcurl
  nlohmann_json::nlohmann_json
)

add_executable(${n} src/${n}.cpp)
target_compile_definitions(${n} PRIVATE SRC_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(${n} PRIVATE ${whisper_SOURCE_DIR}/include)
target_link_libraries(${n} PRIVATE
  whisper
  imgui_lib
  aipfg_lib
  SDL3::SDL3
  CURL::libcurl
  nlohmann_json::nlohmann_json
)
target_compile_definitions(${n} PRIVATE
  IMGUI_FONT_DIR="${imgui_SOURCE_DIR}/misc/fonts/"
)

if (WIN32)    # DLL search path
  set_target_properties(${n} PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "PATH=$<TARGET_FILE_DIR:whisper>;%PATH%"
  )
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${n})

# ---------- Download ggml-base.en.bin model at configure time ------------
# Uses CMake's built-in file(DOWNLOAD) so no wget/curl needed on any platform.
set(MODEL_URL  "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin")
set(MODEL_FILE "${CMAKE_SOURCE_DIR}/models/ggml-base.en.bin")
if(NOT EXISTS "${MODEL_FILE}")
  message(STATUS "[DOWNLOADING] Whisper speech model ggml-base.en.bin (~142 MB) ...")
  file(DOWNLOAD "${MODEL_URL}" "${MODEL_FILE}")
endif()
