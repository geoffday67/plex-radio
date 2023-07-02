#include "data.h"

#include <LittleFS.h>

classData Data;

bool classData::begin() {
  return initDatabase();
}

bool classData::initDatabase() {
  bool result = false;
  int rc;

  // LittleFS.remove("/plex.db");

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

  rc = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS tracks (id TEXT PRIMARY KEY, title TEXT, resource TEXT)", NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Can't create tracks table: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  result = true;

exit:
  return result;
}

void classData::clearAll() {
  int rc;

  rc = sqlite3_exec(database, "DELETE FROM albums", NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Can't clear tracks table: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_exec(database, "DELETE FROM tracks", NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Can't clear tracks table: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

exit:
  return;
}

int classData::getAlbums(Album **ppresult) {
  int rc, count, index;
  sqlite3_stmt *stmt;
  Album *palbum;

  // Get count of albums.
  count = 0;
  rc = sqlite3_prepare_v2(database, "SELECT COUNT(*) FROM albums", -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during preparation: ", sqlite3_errmsg(database));
    goto exit_count;
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW) {
    Serial.printf("Error during steps: ", sqlite3_errmsg(database));
    goto exit_count;
  }
  count = sqlite3_column_int(stmt, 0);

exit_count:
  sqlite3_finalize(stmt);

  if (count == 0) {
    return 0;
  }

  // Allocate space and get the albums themselves.
  *ppresult = new Album[count];
  rc = sqlite3_prepare_v2(database, "SELECT id, title, artist FROM albums", -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during preparation: ", sqlite3_errmsg(database));
    goto exit_data;
  }
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    palbum = (*ppresult) + index++;
    strlcpy(palbum->id, (char *)sqlite3_column_text(stmt, 0), ID_SIZE);
    strlcpy(palbum->title, (char *)sqlite3_column_text(stmt, 1), TITLE_SIZE);
    strlcpy(palbum->artist, (char *)sqlite3_column_text(stmt, 2), ARTIST_SIZE);
  }
  if (rc != SQLITE_DONE) {
    Serial.printf("Error during steps: ", sqlite3_errmsg(database));
    goto exit_data;
  }

exit_data:
  sqlite3_finalize(stmt);

  return count;
}

void classData::storeAlbum(Album *palbum) {
  int rc;
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(database, "INSERT INTO albums (id, title, artist) VALUES (?, ?, ?)", -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during album preparation: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_bind_text(stmt, 1, palbum->id, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during album id binding: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_bind_text(stmt, 2, palbum->title, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during album title binding: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_bind_text(stmt, 3, palbum->artist, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during album artist binding: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("Error during album step: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

exit:
  sqlite3_finalize(stmt);
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

void classData::storeTracks(Track *ptracks, int count) {
  int n, rc;
  Track *ptrack = ptracks;

  String query = "INSERT INTO tracks (id, title, resource) VALUES ";

  for (n = 0; n < count; n++) {
    query += "('";
    query += ptrack->id;
    query += "', '";
    query += ptrack->title;
    query += "', '";
    query += ptrack->resource;
    query += "')";
    if (n < count - 1) {
      query += ", ";
    }

    ptrack++;
  }

  rc = sqlite3_exec(database, query.c_str(), NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Error inserting tracks: %s\n", sqlite3_errmsg(database));
  }
}

void classData::dumpDatabase() {
  int rc;
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(database, "SELECT id, title, artist FROM albums", -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during preparation: ", sqlite3_errmsg(database));
    goto exit;
  }
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    char *id = (char*)sqlite3_column_text(stmt, 0);
    char *title = (char*)sqlite3_column_text(stmt, 1);
    char *artist = (char*)sqlite3_column_text(stmt, 2);
    Serial.printf("ID %s: %s, %s\n", id, title, artist);
  }
  if (rc != SQLITE_DONE) {
    Serial.printf("Error during steps: ", sqlite3_errmsg(database));
  }

exit:
  sqlite3_finalize(stmt);
}
