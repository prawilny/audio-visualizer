#ifndef _AUDIO_VISUALIZER_GLOBAL_H_
#define _AUDIO_VISUALIZER_GLOBAL_H_

#include <mutex>
#include <SDL_stdinc.h>

extern std::mutex big_lock;
extern const Uint8 * keyboard_state;

#endif