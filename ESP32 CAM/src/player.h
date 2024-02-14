#pragma once

#include "playlist.h"

namespace Player {
extern bool begin();
extern void setVolume(int volume);
extern void playUrl(char *purl);

extern void togglePause();
extern void skipToNext();
extern void playTracks(Track *, int);
extern void playTrack(Track *);

extern Playlist *pPlaylist;
}  // namespace Player