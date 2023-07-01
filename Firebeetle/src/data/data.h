#pragma once

#include <sqlite3.h>

#include "album.h"

class classData {
 private:
  sqlite3 *database;
  bool initDatabase();

 public:
  bool begin();
  Album *getAlbums();
  void storeAlbums(Album *palbums, int count);
  void dumpDatabase();
};

extern classData Data;