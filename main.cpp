#include "converter.h"
#include "fft.h"
#include "gl.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "implot.h"
#include "spectrogram.h"
#include "tinyfiledialogs.h"
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_opengl.h>
#include <cassert>
#include <exception>
#include <fmt123.h>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <stdio.h>
#include <vector>

const int TARGET_FPS = 25;

static SDL_Window *window;
static SDL_GLContext gl_context;
static ImGuiIO *io;

static const char *audio_types[1] = {"*.mp3"};
static char *audio_name = nullptr;
static bool audio_played = false;
static bool done = false;

// Needed for debugging.
template class std::unique_ptr<PCM_data>;
std::unique_ptr<PCM_data> audio_data;

std::unique_ptr<std::vector<double>> plot_data;
std::unique_ptr<std::vector<double>> plot_fft_input;

// TODO: make sure that visualization matches audio

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

  size_t sample_byte_size = (SDL_AUDIO_MASK_BITSIZE & audio_data->format) / 8;
  size_t num_samples = num_bytes / (sample_byte_size * audio_data->channels);

  std::vector<double> result(num_samples, 0);

  size_t processed = 0;
  for (int i = 0; i < num_samples; i++) {
    for (int ch = 0; ch < audio_data->channels; ch++) {
      result[i] += (double)(fromBytes(
          audio_data->bytes.data() + audio_data->processed_bytes + processed,
          audio_data->format));
      processed += sample_byte_size;
    }
  }
  assert(num_bytes == processed);

  return result;
}

void audio_callback(void *udata, Uint8 *stream, int len) {
  SDL_memset(stream, 0, len);

  size_t left_in_buffer =
      audio_data->bytes.size() - audio_data->processed_bytes;
  if (left_in_buffer == 0) {
    audio_played = false;
    return;
  }

  int bytes_to_be_copied = len < left_in_buffer ? len : left_in_buffer;

  std::vector<double> fft_input = fft_samples(bytes_to_be_copied);
  std::vector<double> *copied_input = new std::vector<double>();
  *copied_input = fft_input;
  plot_fft_input = std::unique_ptr<std::vector<double>>(copied_input);

  plot_data = amplitudes_of_harmonics(fft_input);

  SDL_MixAudio(stream, audio_data->bytes.data() + audio_data->processed_bytes,
               bytes_to_be_copied, SDL_MIX_MAXVOLUME);
  audio_data->processed_bytes += bytes_to_be_copied;

  // TODO: fill buffer and generate visualization data. How often should it be?
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
  wanted_spec.freq = audio_data->rate;
  wanted_spec.format = audio_data->format;
  wanted_spec.channels = audio_data->channels;
  wanted_spec.silence = 0;
  wanted_spec.size = audio_data->bytes.size() - audio_data->processed_bytes;
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

// TODO: not called after running file to end!
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
  ImPlot::CreateContext();
  io = &ImGui::GetIO();
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  spectrogramInit();
}

void clean_up() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImPlot::DestroyContext();
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

void draw_visualization() {
  if (plot_data.get() != nullptr) {
    size_t fftN = plot_data.get()->size();
    std::vector<double> fftLabels(fftN);
    for (size_t i = 0; i < fftN; i++) {
      fftLabels[i] = i * TARGET_FPS;
    }

    size_t waveN = plot_fft_input.get()->size();
    std::vector<double> waveLabels(waveN);
    std::iota(waveLabels.begin(), waveLabels.end(), 0);

    spectrogramDisplay(fftLabels.data(), plot_data.get()->data(), fftN,
                       waveLabels.data(), plot_fft_input->data(), waveN);
  }
}

int main() {
  try {
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
