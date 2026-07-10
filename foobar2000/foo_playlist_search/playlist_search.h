#pragma once

#include <SDK/cfg_var.h>

extern cfg_string cfg_rowFormat, cfg_infoFormat, cfg_titleFormat;
extern const char default_rowFormat[], default_infoFormat[], default_titleFormat[];

// Marks the cached playlist index stale (rebuilt lazily) and refreshes the search window if open.
void playlistSearchInvalidateIndex();
