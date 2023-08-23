#include "data.h"

#include <SPIFFS.h>

classData Data;

bool classData::begin() {
  return initDatabase();
}

bool classData::initDatabase() {
  bool result = false;
  int rc;

  // SPIFFS.remove("/plex.db");

  rc = sqlite3_initialize();
  if (rc != SQLITE_OK) {
    Serial.printf("Can't initialise database, error code: %d\n", rc);
    goto exit;
  }

  rc = sqlite3_open("/spiffs/plex.db", &database);
  if (rc != SQLITE_OK) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS albums (id TEXT, title TEXT, artist TEXT)", NULL, NULL, NULL);
  // rc = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS albums (id TEXT PRIMARY KEY, title TEXT, artist TEXT)", NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Can't create albums table: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS tracks (id TEXT, album TEXT, title TEXT, resource TEXT)", NULL, NULL, NULL);
  // rc = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS tracks (id TEXT PRIMARY KEY, album TEXT, title TEXT, resource TEXT)", NULL, NULL, NULL);
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

void classData::storeTrack(Track *ptrack) {
  int rc;
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(database, "INSERT INTO tracks (id, album, title, resource) VALUES (?, ?, ?, ?)", -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during album preparation: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_bind_text(stmt, 1, ptrack->id, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during track id binding: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_bind_text(stmt, 2, ptrack->album, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during track album binding: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_bind_text(stmt, 3, ptrack->title, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during track title binding: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_bind_text(stmt, 4, ptrack->resource, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during track resource binding: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("Error during track step: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

exit:
  sqlite3_finalize(stmt);
}

int classData::getTracks(char *palbumid, Track **ppresult) {
  int count, index;
  sqlite3_stmt *stmt;
  Track *ptrack;

  count = 0;
  sqlite3_prepare_v2(database, "SELECT COUNT(*) FROM tracks WHERE album=?", -1, &stmt, NULL);
  sqlite3_bind_text(stmt, 1, palbumid, -1, SQLITE_STATIC);
  sqlite3_step(stmt);
  count = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);

  if (count == 0) {
    return 0;
  }

  *ppresult = new Track[count];
  sqlite3_prepare_v2(database, "SELECT id, title, resource FROM tracks WHERE album=? ORDER BY rowid", -1, &stmt, NULL);
  sqlite3_bind_text(stmt, 1, palbumid, -1, SQLITE_STATIC);
  index = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    ptrack = (*ppresult) + index++;
    strlcpy(ptrack->id, (char *)sqlite3_column_text(stmt, 0), ID_SIZE);
    strlcpy(ptrack->title, (char *)sqlite3_column_text(stmt, 1), TITLE_SIZE);
    strlcpy(ptrack->resource, (char *)sqlite3_column_text(stmt, 2), RESOURCE_SIZE);
  }
  sqlite3_finalize(stmt);

  return count;
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
  index = 0;
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

void classData::dumpDatabase() {
  int rc, count, n;
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(database, "SELECT id, title, artist FROM albums", -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during preparation: ", sqlite3_errmsg(database));
    goto exit;
  }
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    char *id = (char *)sqlite3_column_text(stmt, 0);
    char *title = (char *)sqlite3_column_text(stmt, 1);
    char *artist = (char *)sqlite3_column_text(stmt, 2);
    Serial.printf("%s by %s\n", title, artist);

    Track *ptracks;
    count = getTracks(id, &ptracks);
    if (count > 0) {
      for (n = 0; n < count; n++) {
        Serial.printf("  %s\n", (ptracks + n)->title);
      }
      delete[] ptracks;
      Serial.println();
    }
  }
  if (rc != SQLITE_DONE) {
    Serial.printf("Error during steps: ", sqlite3_errmsg(database));
  }

exit:
  sqlite3_finalize(stmt);
}

void classData::dumpTracks() {
  int rc, count, n;
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(database, "SELECT id, album, title, resource FROM tracks ORDER BY rowid", -1, &stmt, NULL);
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    char *id = (char *)sqlite3_column_text(stmt, 0);
    char *album = (char *)sqlite3_column_text(stmt, 1);
    char *title = (char *)sqlite3_column_text(stmt, 2);
    char *resource = (char *)sqlite3_column_text(stmt, 3);
    Serial.printf("Track id %s for album %s, %s at %s\n", id, album, title, resource);
  }
  sqlite3_finalize(stmt);
}