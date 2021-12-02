#include "converter.h"
#include <cstdio>
#include <cstring>
#include <exception>
#include <mpg123.h>
#include <sstream>

static void cleanup(mpg123_handle *mh) {
  mpg123_close(mh);
  mpg123_delete(mh);
}

PCM_data from_mp3(const char *filename) {
  PCM_data result;

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
      || mpg123_getformat(mh, &result.rate, &result.channels,
                          &result.encoding) != MPG123_OK) {
    cleanup(mh);
    std::stringstream ss;
    ss << filename << " couldn't be read as a mp3 file.";
    throw new std::runtime_error(ss.str());
  }

  /* Ensure that this output format will not change
     (it might, when we allow it). */
  mpg123_format_none(mh);
  mpg123_format(mh, result.rate, result.channels, result.encoding);

  size_t buffer_size = mpg123_outblock(mh);
  std::vector<uint8_t> buffer(buffer_size, 0);

  size_t buffer_read;
  do {
    err = mpg123_read(mh, buffer.data(), buffer_size, &buffer_read);
    result.bytes.insert(result.bytes.end(), buffer.begin(),
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
  return result;
}
