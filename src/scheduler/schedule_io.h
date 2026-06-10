// Schedule (.lsc) JSON load/save and file dialog handling.
#pragma once

#include <memory>

#include "portable-file-dialogs.h"

extern std::shared_ptr<pfd::open_file> open_file;
extern std::shared_ptr<pfd::save_file> save_file;

bool loadSchedule(const char* sFilename);
bool saveSchedule(const char* sFilename);

void DoFileDialog_Open();
void DoFileDialog_Save();
