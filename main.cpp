#include "converter.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "tinyfiledialogs.h"
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_opengl.h>
#include <cassert>
#include <exception>
#include <fmt123.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <stdio.h>

const int MP3_FREQ = 44100;
const int TARGET_FPS = 60;

static SDL_Window *window;
static SDL_GLContext gl_context;
static ImGuiIO *io;

static const char *audio_types[1] = {"*.mp3"};
static char *audio_name = nullptr;
static bool audio_played = false;
static bool done = false;

std::unique_ptr<PCM_data> audio_data;

// TODO: make sure that visualization matches audio

void SDL_error_exit() {
  printf("Error: %s\n", SDL_GetError());
  exit(1);
}

void audio_callback(void *udata, Uint8 *stream, int len) {
  SDL_memset(stream, 0, len);

  size_t left_in_buffer =
      audio_data->bytes.size() - audio_data->processed_bytes;
  if (left_in_buffer == 0) {
    audio_played = false;
    audio_data->processed_bytes = 0;
    return;
  }

  int copied = len < left_in_buffer ? len : left_in_buffer;
  SDL_MixAudio(stream, audio_data->bytes.data() + audio_data->processed_bytes,
               copied, SDL_MIX_MAXVOLUME);
  audio_data->processed_bytes += copied;

  // TODO: fill buffer and generate visualization data (not on every callback).
}

void start_audio() {
  if (audio_played || audio_data.get() == nullptr) {
    return;
  }

  uint8_t sample_byte_size = (audio_data->format & SDL_AUDIO_MASK_BITSIZE) / 8;

  if (sample_byte_size > 1) {
    audio_data->processed_bytes -=
        audio_data->processed_bytes % sample_byte_size;
  }

  SDL_AudioSpec wanted_spec;
  wanted_spec.freq = MP3_FREQ;
  wanted_spec.format = audio_data->format;
  wanted_spec.channels = audio_data->channels;
  wanted_spec.silence = 0;
  wanted_spec.size = audio_data->bytes.size() - audio_data->processed_bytes;
  wanted_spec.samples = MP3_FREQ / (TARGET_FPS - 1);
  wanted_spec.callback = audio_callback;
  wanted_spec.userdata = nullptr;

  if (SDL_OpenAudio(&wanted_spec, nullptr) != 0) {
    SDL_error_exit();
  }
  SDL_PauseAudio(0);
  audio_played = true;
}

void stop_audio() {
  if (!audio_played) {
    return;
  }

  SDL_LockAudio();
  SDL_PauseAudio(1);
  SDL_UnlockAudio();
  SDL_CloseAudio();

  audio_played = false;
}

void select_file() {
  stop_audio();
  audio_name = nullptr;
  char *new_audio_name = tinyfd_openFileDialog(
      "Pick file", nullptr, 1, audio_types, "All supported files", false);

  if (new_audio_name == nullptr) {
    return;
  }

  try {
    audio_data = from_mp3(new_audio_name);
    audio_name = new_audio_name;
  } catch (...) {
    std::cout << "Error reading or opening file " << new_audio_name
              << std::endl;
  }
}

void toggle_playback() {
  if (audio_played) {
    stop_audio();
  } else {
    start_audio();
  }
}

void set_up() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
    SDL_error_exit();
  }

  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  // TODO: replace SDL_WINDOW_RESIZABLE with SDL_WINDOW_FULLSCREEN or
  // SDL_WINDOW_FULLSCREEN_DESKTOP
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);
  window = SDL_CreateWindow("Audio Visualizer", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
  gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  // Setup Dear ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  io = &ImGui::GetIO();
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);
}

void clean_up() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void imgui_frame() {
  // Start the Dear ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  {
    ImGui::Begin("Player");

    if (ImGui::Button("Select file")) {
      select_file();
    }
    ImGui::SameLine();
    ImGui::Text("Currently played file = %s", audio_name);

    if (ImGui::Button("Play/Pause")) {
      toggle_playback();
    }
    ImGui::SameLine();
    ImGui::Text("Playback status: %s", audio_played ? "PLAY" : "PAUSE");

    ImGui::Text("Average FPS: %.1f", ImGui::GetIO().Framerate);

    ImGui::End();
  }
  // Rendering
  ImGui::Render();
}

int main() {
  set_up();
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        done = true;
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window))
        done = true;

      if (!io->WantCaptureMouse || !io->WantCaptureKeyboard) {
        // TODO: handle mouse or keyboard event if it's of appropriate type
      }
    }

    imgui_frame();

    glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
    // TODO: visualization
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }
  clean_up();

  return 0;
}
