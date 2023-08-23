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

  void storeTrack(Track *ptrack);
  int getTracks(char *, Track **ppresult);

  void storeAlbum(Album *);
  void storeAlbums(Album *palbums, int count);
  int getAlbums(Album **ppresult);

  void dumpDatabase();
  void dumpTracks();
};

extern classData Data;