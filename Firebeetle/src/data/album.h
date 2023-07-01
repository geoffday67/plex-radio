#pragma once

#include "constants.h"

class Album {
 public:
  Album();
  char id[ID_SIZE];
  char title[TITLE_SIZE];
  char artist[ARTIST_SIZE];
};