#pragma once

#include "constants.h"

class Track {
 public:
  Track();
  char id[ID_SIZE];
  char title[TITLE_SIZE];
  char resource[RESOURCE_SIZE];
};