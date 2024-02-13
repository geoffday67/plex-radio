#pragma once

#include <sqlite3.h>

#include "album.h"
#include "track.h"

class classData {
 private:
  sqlite3 *database;
  bool initDatabase();
  static int collateIgnoreThe (void *, int, const void *, int, const void *);
  sqlite3_stmt *trackStatement;

 public:
  bool begin();
  void clearAll();

  bool beginStoreTracks();
  bool storeTrack(Track *ptrack);
  bool endStoreTracks();
  int getTracks(char *, Track **ppresult);

  void storeAlbum(Album *);
  void storeAlbums(Album *palbums, int count);
  int getAlbums(Album **ppresult);
  int getAlbumCount();

  void dumpDatabase();
  void dumpTracks();

  void startTransaction();
  void endTransaction();
};

extern classData Data;