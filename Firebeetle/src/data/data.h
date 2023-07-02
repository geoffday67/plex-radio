#pragma once

#include <sqlite3.h>

#include "album.h"
#include "track.h"

class classData {
 private:
  sqlite3 *database;
  bool initDatabase();

 public:
  bool begin();
  void clearAll();
  int getAlbums(Album **ppresult);
  void storeAlbum(Album *);
  void storeAlbums(Album *palbums, int count);
  void storeTracks(Track* ptracks, int count);
  void dumpDatabase();
};

extern classData Data;