/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dlfcn.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "plugin_handler.h"
#include "base/logging.h"
namespace art {
static bool LoadUpPlugin(const char* plugin) {
  // Open the file.
  void *user_handle = dlopen(plugin, RTLD_NOW);
  bool failure = false;

  if (user_handle != 0) {
    // Open now the init function.
    void *tmp = dlsym(user_handle, "ArtPluginInit");

    if (tmp == 0) {
      LOG(INFO) << "PLUGIN: Problem with " << plugin << " cannot find ArtPluginInit function" << std::endl;

      // Set failure flag.
      failure = true;
    } else {
      // The plugin will fill in the plugin structure.

      // Transform it into a function pointer.
      bool (*plugin_initialization)() = (bool (*)()) (tmp);

      // Call it.
      failure = (plugin_initialization() == false);
    }
  } else {
    LOG(INFO) << "PLUGIN: Problem opening user plugin file: " << plugin;
    LOG(INFO) << "PLUGIN: dlerror() reports: " << dlerror();

    // Set failure flag.
    failure = true;
  }

  // If the failure flag is on.
  if (failure == true) {
    LOG(INFO) << "PLUGIN: Initialization function in " << plugin << " failed";
  }

  return failure == false;
}

void LoadUpPlugins(const char* directory) {
  // We want to walk the directory, so first open it.
  DIR* dir = opendir(directory);
  char name[1024];

  if (dir == nullptr) {
    LOG(INFO) << "Error opening the directory: " << directory << std::endl;
    return;
  }

  while (true) {
    // Get next entry.
    struct dirent* current = readdir(dir);

    if (current == nullptr) {
      break;
    }

    // Create the file path.
    snprintf(name, sizeof(name), "%s/%s", directory, current->d_name);
    struct stat file_stat;

    // If we cannot stat the file, skip it.
    if (stat(name, &file_stat) == -1) {
      continue;
    }

    // If it is a directory, skip it as well.
    if (S_ISDIR(file_stat.st_mode) != 0) {
      continue;
    }

    // If it is a symbolic link, skip it as well.
    if (S_ISLNK(file_stat.st_mode) != 0) {
      continue;
    }

    // Now differientiate depending on debug mode.
#ifdef NDEBUG
    // Non debug mode requires libart- prefix.
    if (strstr(name, "libart-") == nullptr) {
      continue;
    }
#else
    // Debug mode requires libartd- prefix.
    if (strstr(name, "libartd-") == nullptr) {
      continue;
    }
#endif

    bool success = LoadUpPlugin(name);

    // If succeeded, install it.
    if (success == false) {
      LOG(INFO) << "Error loading up " << name << std::endl;
    }
  }

  // Close the directory before leaving here.
  closedir(dir), dir = nullptr;
}
}  // namespace art
