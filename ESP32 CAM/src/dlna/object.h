#pragma once

#include "constants.h"

class Object {
 public:
  Object();

  char id[ID_SIZE];
  char name[NAME_SIZE];
  char resource[RESOURCE_SIZE];
};