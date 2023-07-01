#include "data.h"

#include <LittleFS.h>

classData Data;

bool classData::begin() {
  return initDatabase();
}

bool classData::initDatabase() {
  bool result = false;
  int rc;

  LittleFS.remove("/plex.db");  // For now remove the database at each run.

  rc = sqlite3_initialize();
  if (rc != SQLITE_OK) {
    Serial.printf("Can't initialise database, error code: %d\n", rc);
    goto exit;
  }

  rc = sqlite3_open("/littlefs/plex.db", &database);
  if (rc != SQLITE_OK) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS albums (id TEXT PRIMARY KEY, title TEXT, artist TEXT)", NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Can't create albums table: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  result = true;

exit:
  return result;
}

Album *classData::getAlbums() {
  // Fetch from database and convert to Albums.
}

void classData::storeAlbums(Album *palbums, int count) {
  int n, rc;
  Album *palbum = palbums;

  String query = "INSERT INTO albums (id, title, artist) VALUES ";

  for (n = 0; n < count; n++) {
    query += "('";
    query += palbum->id;
    query += "', '";
    query += palbum->title;
    query += "', '";
    query += palbum->artist;
    query += "')";
    if (n < count - 1) {
      query += ", ";
    }

    palbum++;
  }

  rc = sqlite3_exec(database, query.c_str(), NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Error inserting albums: %s\n", sqlite3_errmsg(database));
  }
}

void classData::dumpDatabase() {
  int rc;
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(database, "SELECT * FROM albums", -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during preparation: ", sqlite3_errmsg(database));
    goto exit;
  }
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    const unsigned char *id = sqlite3_column_text(stmt, 0);
    const unsigned char *title = sqlite3_column_text(stmt, 1);
    const unsigned char *artist = sqlite3_column_text(stmt, 2);
    Serial.printf("ID %s: %s, %s\n", id, title, artist);
  }
  if (rc != SQLITE_DONE) {
    Serial.printf("Error during steps: ", sqlite3_errmsg(database));
  }

exit:
  sqlite3_finalize(stmt);
}

