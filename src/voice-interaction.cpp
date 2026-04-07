#include "aipfg/sdl3-context.hpp"
#include "aipfg/sdl3-sprite-sheet.hpp"
#include "aipfg/sdl3-sprite.hpp"
#include "aipfg/sdl3-typedefs.hpp"
#include "sdl3-audio-recorder.hpp"
#include "whisper-transcriber.hpp"
#include "sdl3-audio-recorder.hpp"
#include "whisper-transcriber.hpp"
#include "aipfg/imgui-context.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <exception>
#include <utility>
#include <vector>
#include <SDL3/SDL.h>
#ifdef _WIN32
#include <windows.h> // SetConsoleOutputCP/SetConsoleCP for unicode
#endif

class Game
{
public:
  explicit Game(SDLContext&, int w, int h)
    : window_{SDL_CreateWindow("Voice Interaction", w, h,
                               SDL_WINDOW_RESIZABLE)},
      renderer_{SDL_CreateRenderer(window_.get(), nullptr)},
      knight_sheet_{renderer_.get(),
                    "../resources/time_fantasy/knights_3x.json"},
      reaper_sheet_{renderer_.get(),
                    "../resources/time_fantasy/reaper_blade_3.json"},
      pickup_sheet_{renderer_.get(),
                    "../resources/time_fantasy/hallowicons_2.json"},
      player_prefix_{"knight2"},
      player_{knight_sheet_, player_prefix_ + "-down", 100.0f, 100.0f},
      last_tick_{SDL_GetTicks()},
      imgui_ctx_{ nullptr }

  {
    if (!window_ || !renderer_)
    {
      throw std::runtime_error(SDL_GetError());
    }

    SDL_SetRenderVSync(renderer_.get(), 1);

    float font_size = h / 25.0f;
    imgui_ctx_ =
        std::make_unique<ScopedImGui>(window_.get(), renderer_.get(), font_size);

    npcs_.emplace_back(reaper_sheet_, "idle", 330.0f, 240.0f);
    npcs_.emplace_back(knight_sheet_, "knight1-idle", 500.0f, 320.0f);

    pickups_.emplace_back(pickup_sheet_, "sweet-yellow", 300.0f, 150.0f);
    pickups_.emplace_back(pickup_sheet_, "sweet-red",    420.0f, 200.0f);
    pickups_.emplace_back(pickup_sheet_, "sweet-blue",   180.0f, 280.0f);
  }

  void run()
  {
    bool running = true;

    while (running)
    {
      process_events(running);
      update();
      render();
    }
  }

private:
  void process_events(bool& running)
  {
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
      if (e.type == SDL_EVENT_QUIT ||
          (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)) 
      {
        running = false;
      }
      else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_SPACE
          && !e.key.repeat)
      {
          space_held_ = true;
          recorder_.clear();
          recorder_.resume();
      }
      else if (e.type == SDL_EVENT_KEY_UP && e.key.key == SDLK_SPACE)
      {
          space_held_ = false;
          recorder_.pause();
          transcript = transcriber.transcribe(recorder_.buffer()); 

      }
    }
  }
  
  bool player_collides() const
  {
    for (const auto& npc : npcs_)
      if (player_.overlaps(npc))
        return true;
    return false;
  }

  void update()
  {
    Uint64 current_tick = SDL_GetTicks();
    float  dt_ms = static_cast<float>(current_tick - last_tick_);
    last_tick_ = current_tick;

    std::erase_if(pickups_, [this](const Sprite& p) {
      return player_.overlaps(p);
    });

    const bool* keys = SDL_GetKeyboardState(nullptr);

    float vx = 0.0f, vy = 0.0f;
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])
      vx -= 1.0f;
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
      vx += 1.0f;
    if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W])
      vy -= 1.0f;
    if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) 
      vy += 1.0f; 

    if (vx == 0.0f && vy == 0.0f)
      return;

    

    // Horizontal direction preferred for diagonals
    const std::string new_facing =   (vx > 0.0f) ? player_prefix_ + "-right"
                                   : (vx < 0.0f) ? player_prefix_ + "-left"
                                   : (vy < 0.0f) ? player_prefix_ + "-up"
                                                 : player_prefix_ + "-down";
    if (new_facing != player_.tag_name())
    {
      player_.play(new_facing);
    }

    // vx_/vy_ on player Sprite are always 0; update() only advances animation
    player_.update(dt_ms);

    // Normalise diagonal so speed is consistent across all directions
    if (vx != 0.0f && vy != 0.0f)
    {
      constexpr float INV_SQRT2 = 0.70711f;
      vx *= INV_SQRT2;
      vy *= INV_SQRT2;
    }

    const float ds = speed_ * dt_ms / 1000.0f;

    // Try each axis independently so the player slides along obstacles
    player_.x_ += vx * ds;
    if (player_collides())
      player_.x_ -= vx * ds;

    player_.y_ += vy * ds;
    if (player_collides())
      player_.y_ -= vy * ds;

    // Wrap around window edges
    int w, h;
    SDL_GetWindowSize(window_.get(), &w, &h);
    const AABB pa = player_.aabb();
    if (vx > 0.0f && player_.x_ > static_cast<float>(w))
      player_.x_ = -pa.w;
    if (vx < 0.0f && player_.x_ < -pa.w)
      player_.x_ = static_cast<float>(w);
    if (vy > 0.0f && player_.y_ > static_cast<float>(h))
      player_.y_ = -pa.h;
    if (vy < 0.0f && player_.y_ < -pa.h)
      player_.y_ = static_cast<float>(h);
  }

  void render()
  {
    SDL_Renderer* r = renderer_.get();

    SDL_SetRenderDrawColorFloat(r, 0.2f, 0.2f, 0.3f, 1.0f);
    SDL_RenderClear(r);



    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove 
                                    |ImGuiWindowFlags_NoDecoration| ImGuiWindowFlags_NoBackground;

	

    ImGui::Begin("Overlay", nullptr, window_flags);

    ImGui::SetWindowPos(ImVec2(10, 10));
    ImGui::Text("Sweets collected:");
    ImGui::Text("Golden Fleeces Collected:");


    if (space_held_)
        ImGui::PushStyleColor(ImGuiCol_Button,
        ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);

    ImGui::Button("Record (Space)");

   if (space_held_)
        ImGui::PopStyleColor();

    ImGui::Text("Captured samples: %zu", recorder_.buffer().size());

    ImGui::Separator();

    ImGui::TextWrapped("%s", transcript.c_str()); 

    ImGui::End();

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(),
        renderer_.get());

    std::vector<std::pair<float, Sprite*>> order;
    order.reserve(1 + npcs_.size() + pickups_.size());
    const AABB pa = player_.aabb();
    order.push_back({pa.y + pa.h, &player_});
    for (auto& npc : npcs_)
      order.push_back({npc.aabb().y + npc.aabb().h, &npc});
    for (auto& pickup : pickups_)
      order.push_back({pickup.aabb().y + pickup.aabb().h, &pickup});
    std::sort(order.begin(), order.end());
    for (auto& [feet_y, sprite] : order)
      sprite->render(r);

    SDL_RenderPresent(r);


    

   
    
  }

  const float           speed_ = 130.0f;
  WindowPtr             window_;
  RendererPtr           renderer_;
  SpriteSheet           knight_sheet_;
  SpriteSheet           reaper_sheet_;
  SpriteSheet           pickup_sheet_;
  std::string           player_prefix_;
  Sprite                player_;
  std::vector<Sprite>   npcs_;
  std::vector<Sprite>   pickups_;
  Uint64                last_tick_;
  std::unique_ptr<ScopedImGui> imgui_ctx_;
  SDL3_AudioRecorder           recorder_;
  WhisperTranscriber transcriber;
  std::string       transcript;

  bool        space_held_ = false;
  
};

int main()
{
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8); // Unicode for cmd.exe
  SetConsoleCP(CP_UTF8);       // for unicode/emojis
#endif

  try
  {
      SDLContext sdl{ SDL_INIT_VIDEO | SDL_INIT_AUDIO };
    Game       game{sdl, 800, 600};
    game.run();
  }
  catch (const std::exception& e)
  {
    SDL_Log("Fatal error: %s", e.what());
    return -1;
  }

  return 0;
}
