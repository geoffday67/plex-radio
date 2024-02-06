#pragma once

#include "data/track.h"

namespace Player {
extern bool begin();
extern void setVolume(int volume);
extern void playUrl(char *purl);

extern void addToPlaylist(Track *ptrack);
extern void clearPlaylist();
extern void resetPlaylist(Track *ptrack);
extern void togglePause();
extern void stopPlay();
}