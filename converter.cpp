#include "converter.h"
#include <SDL_audio.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fmt123.h>
#include <memory>
#include <mpg123.h>
#include <sstream>

static void cleanup(mpg123_handle *mh) {
  mpg123_close(mh);
  mpg123_delete(mh);
}

static SDL_AudioFormat format_from_mpg123(int encoding) {
  switch (encoding) {
  case MPG123_ENC_SIGNED_16:
    return AUDIO_S16;
  case MPG123_ENC_UNSIGNED_16:
    return AUDIO_U16;
  case MPG123_ENC_UNSIGNED_8:
    return AUDIO_U8;
  case MPG123_ENC_SIGNED_8:
    return AUDIO_S8;
  case MPG123_ENC_SIGNED_32:
    return AUDIO_S32;
  case MPG123_ENC_FLOAT_32:

  /* Not supported */
  case MPG123_ENC_SIGNED_24:
  case MPG123_ENC_UNSIGNED_24:
  case MPG123_ENC_FLOAT_64:
  case MPG123_ENC_UNSIGNED_32:

  /* Compounding */
  case MPG123_ENC_ULAW_8:
  case MPG123_ENC_ALAW_8:

  /* Not specified bitness */
  case MPG123_ENC_8:
  case MPG123_ENC_16:
  case MPG123_ENC_24:
  case MPG123_ENC_32:
  case MPG123_ENC_SIGNED:
  case MPG123_ENC_FLOAT:

  default:
    std::stringstream ss;
    ss << "mpg123 format not supported by SDL: " << encoding;
    throw new std::runtime_error(ss.str());
  }
}

std::unique_ptr<PCM_data> from_mp3(const char *filename) {
  std::unique_ptr<PCM_data> result = std::make_unique<PCM_data>();
  int encoding;

  int err = MPG123_OK;
  mpg123_handle *mh = NULL;

#if MPG123_API_VERSION < 46
  // Newer versions of the library don't need that anymore, but it is safe
  // to have the no-op call present for compatibility with old versions.
  err = mpg123_init();
#endif

  if ((mh = mpg123_new(NULL, &err)) == NULL) {
    std::stringstream ss;
    ss << "mpg123 failed to open a new handle with error code: " << err;
    throw new std::runtime_error(ss.str());
  }

  if (mpg123_open(mh, filename) != MPG123_OK
      /* Peek into track and get first output format. */
      || mpg123_getformat(mh, &result->rate, &result->channels, &encoding) !=
             MPG123_OK) {
    cleanup(mh);
    std::stringstream ss;
    ss << filename << " couldn't be read as a mp3 file.";
    throw new std::runtime_error(ss.str());
  }

  /* Ensure that this output format will not change
     (it might, when we allow it). */
  mpg123_format_none(mh);
  mpg123_format(mh, result->rate, result->channels, encoding);

  size_t buffer_size = mpg123_outblock(mh);
  std::vector<uint8_t> buffer(buffer_size, 0);

  size_t buffer_read;
  do {
    err = mpg123_read(mh, buffer.data(), buffer_size, &buffer_read);
    result->bytes.insert(result->bytes.end(), buffer.begin(),
                         buffer.begin() + buffer_read);
  } while (buffer_read && err == MPG123_OK);

  if (err != MPG123_DONE) {
    cleanup(mh);
    std::stringstream ss;
    ss << "Decoding ended prematurely because: "
       << (err == MPG123_ERR ? mpg123_strerror(mh)
                             : mpg123_plain_strerror(err));
    throw new std::runtime_error(ss.str());
  }

  cleanup(mh);

  result->format = format_from_mpg123(encoding);
  result->processed_bytes = 0;

  assert(result->channels == 1 || result->channels == 2);
  assert(result->rate > 0);
  return result;
}
