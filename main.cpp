#include "SDL_events.h"
#include "SDL_scancode.h"
#include "converter.h"
#include "fft.h"
#include "gl.h"
#include "global.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "plot3d.h"
#include "spectrogram.h"
#include "tinyfiledialogs.h"
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_opengl.h>
#include <cassert>
#include <deque>
#include <exception>
#include <fmt123.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <stdio.h>
#include <vector>

#define V2D 0
#define V3D 1

const int TARGET_FPS = 50;
const int HISTORY_SIZE = 5 * TARGET_FPS;

static SDL_Window *window;
static SDL_GLContext gl_context;
static ImGuiIO *io;

static int selected_visualization = V2D;

static const char *audio_types[1] = {"*.mp3"};
static char *audio_name = nullptr;
static bool audio_played = false;
static bool done = false;
static bool audio_finished = false;

std::optional<PCM_data> audio_data;
std::deque<std::vector<double>> plot_data;
std::deque<std::vector<double>> plot_fft_input;

std::mutex big_lock;
const Uint8 *keyboard_state;

void SDL_error_exit() {
  printf("Error: %s\n", SDL_GetError());
  exit(1);
}

double fromBytes(uint8_t *bytes, SDL_AudioFormat format) {
  switch (format) {
  case AUDIO_S16:
    return *(reinterpret_cast<int16_t *>(bytes));
  case AUDIO_U16:
    return *(reinterpret_cast<uint16_t *>(bytes));
  case AUDIO_U8:
    return *bytes;
  case AUDIO_S8:
    return *(reinterpret_cast<int8_t *>(bytes));
  case AUDIO_S32:
    return *(reinterpret_cast<int32_t *>(bytes));
  default:
    assert(false);
  }
}

std::vector<double> fft_samples(int num_bytes) {
  // We may have to merge samples of two channels and cast results to double.

  size_t sample_byte_size =
      (SDL_AUDIO_MASK_BITSIZE & audio_data.value().format) / 8;
  size_t num_samples =
      num_bytes / (sample_byte_size * audio_data.value().channels);

  std::vector<double> result(num_samples, 0);

  size_t processed = 0;
  for (int i = 0; i < num_samples; i++) {
    for (int ch = 0; ch < audio_data.value().channels; ch++) {
      result[i] +=
          (double)(fromBytes(audio_data.value().bytes.data() +
                                 audio_data.value().processed_bytes + processed,
                             audio_data.value().format));
      processed += sample_byte_size;
    }
  }
  assert(num_bytes == processed);

  return result;
}

void audio_callback(void *udata, Uint8 *stream, int len) {
  SDL_memset(stream, 0, len);

  size_t left_in_buffer =
      audio_data.value().bytes.size() - audio_data.value().processed_bytes;
  if (left_in_buffer == 0) {
    audio_played = false;
    return;
  }

  int bytes_to_be_copied = len < left_in_buffer ? len : left_in_buffer;

  big_lock.lock();
  plot_fft_input.push_front(fft_samples(bytes_to_be_copied));
  big_lock.unlock();
  if (plot_fft_input.size() > HISTORY_SIZE) {
    big_lock.lock();
    plot_fft_input.resize(HISTORY_SIZE);
    big_lock.unlock();
  }
  big_lock.lock();
  plot_data.push_front(amplitudes_of_harmonics(plot_fft_input.front()));
  big_lock.unlock();
  if (plot_data.size() > HISTORY_SIZE) {
    big_lock.lock();
    plot_data.resize(HISTORY_SIZE);
    big_lock.unlock();
  }

  SDL_MixAudio(stream,
               audio_data.value().bytes.data() +
                   audio_data.value().processed_bytes,
               bytes_to_be_copied, SDL_MIX_MAXVOLUME);
  audio_data.value().processed_bytes += bytes_to_be_copied;

  if (audio_data.value().processed_bytes == audio_data.value().bytes.size()) {
    audio_finished = true;
  }
}

void start_audio() {
  if (audio_played || !audio_data.has_value() || audio_name == nullptr) {
    return;
  }

  uint8_t sample_byte_size =
      (audio_data.value().format & SDL_AUDIO_MASK_BITSIZE) / 8;

  if (sample_byte_size > 1) {
    audio_data.value().processed_bytes -=
        audio_data.value().processed_bytes % sample_byte_size;
  }

  SDL_AudioSpec wanted_spec;
  wanted_spec.freq = audio_data.value().rate;
  wanted_spec.format = audio_data.value().format;
  wanted_spec.channels = audio_data.value().channels;
  wanted_spec.silence = 0;
  wanted_spec.size =
      audio_data.value().bytes.size() - audio_data.value().processed_bytes;
  wanted_spec.samples = wanted_spec.freq / TARGET_FPS;
  wanted_spec.samples -=
      wanted_spec.samples % wanted_spec.channels; // Aligning to 0 % channels

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

  plot_data.clear();
  plot_fft_input.clear();

  try {
    audio_data = std::optional(from_mp3(new_audio_name));
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

  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL);
  window = SDL_CreateWindow("Audio Visualizer", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
  gl_context = SDL_GL_CreateContext(window);
  gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
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

  keyboard_state = SDL_GetKeyboardState(nullptr);
  spectrogramInit();
  plot3dInit();
}

void clean_up() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void set_audio_position(int seconds) {
  bool audio_played_at_start = audio_played;

  if (audio_played_at_start) {
    stop_audio();
  }
  audio_data->processed_bytes =
      seconds * audio_data->rate * audio_data->channels * 2;

  if (audio_played_at_start) {
    start_audio();
  }
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

    ImGui::RadioButton("2D", &selected_visualization, V2D);
    ImGui::SameLine();
    ImGui::RadioButton("3D", &selected_visualization, V3D);

    if (audio_data.has_value()) {
      char label[12];
      int sample_byte_size =
          ((SDL_AUDIO_MASK_BITSIZE & audio_data.value().format) / 8);

      int seconds_now = audio_data->processed_bytes /
                        (audio_data->rate * audio_data->channels) /
                        sample_byte_size;
      int seconds_all = audio_data->bytes.size() /
                        (audio_data->rate * audio_data->channels) /
                        sample_byte_size;
      sprintf(label, "%02d:%02d/%02d:%02d", seconds_now / 60, seconds_now % 60,
              seconds_all / 60, seconds_all % 60);
      int seconds_slider = seconds_now;
      if (ImGui::SliderInt(label, &seconds_slider, 0, seconds_all, "")) {
        set_audio_position(seconds_slider);
      }
    }
    ImGui::Text("Average FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
  }
  ImGui::Render();
}

void draw_visualization() {
  if (plot_data.size() != 0) {
    size_t fftN = 0;
    big_lock.lock();
    for (auto &data : plot_data) {
      fftN = std::max(fftN, data.size());
    }
    big_lock.unlock();
    std::vector<double> fftLabels(fftN);
    for (size_t i = 0; i < fftN; i++) {
      fftLabels[i] = i * TARGET_FPS;
    }

    if (selected_visualization == V2D) {
      big_lock.lock();
      size_t waveN = plot_fft_input.front().size();
      std::vector<double> waveLabels(waveN);
      std::iota(waveLabels.begin(), waveLabels.end(), 0);
      spectrogramDisplay(fftLabels.data(), plot_data.front().data(), fftN,
                         waveLabels.data(), plot_fft_input.front().data(),
                         waveN, audio_data.value().format);
    } else if (selected_visualization == V3D) {
      big_lock.lock();
      plot3dDisplay(fftLabels, plot_data, audio_data.value().format);
    }
  }
}

int main() {
  try {
    set_up();
    selected_visualization = V3D;

    while (!done) {
      if (audio_finished) {
        stop_audio();
        audio_name = nullptr;
        audio_finished = false;
        plot_data.clear();
        plot_fft_input.clear();
      }

      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
          done = true;
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window))
          done = true;

        if (event.type == SDL_KEYDOWN && !io->WantCaptureKeyboard) {
          if (keyboard_state[SDL_SCANCODE_SPACE]) {
            toggle_playback();
          }
          if (keyboard_state[SDL_SCANCODE_LCTRL] &&
              keyboard_state[SDL_SCANCODE_O]) {
                select_file();
          }
          if (selected_visualization == V3D) {
            plot3dHandleKeyEvent();
          }
        }
      }

      imgui_frame();

      glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
      glClearColor(0, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT);

      draw_visualization();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      SDL_GL_SwapWindow(window);
    }
    clean_up();
  } catch (std::exception &e) {
    std::cout << "error: " << e.what();
    exit(1);
  }

  return 0;
}
