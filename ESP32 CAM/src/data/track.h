#pragma once

#include "constants.h"

class Track {
 public:
  Track();
  Track(const Track&);
  char id[ID_SIZE];
  char album[ID_SIZE];
  char title[TITLE_SIZE];
  char resource[RESOURCE_SIZE];
};