#include "track.h"

#include <string.h>

Track::Track() {
  id[0] = 0;
  album[0] = 0;
  title[0] = 0;
  resource[0] = 0;
}

Track::Track(const Track &source) {
  strcpy(id, source.id);
  strcpy(album, source.album);
  strcpy(title, source.title);
  strcpy(resource, source.resource);
}