#include "synchronizer.hpp"

synqueen::Synchronizer::Synchronizer(uv_loop_t *loop) : patchExchange(loop) {}

void synqueen::Synchronizer::loadSettings(const Settings &settings) {
  for (const auto &folderSettings : settings.folders) {
    FolderStatePtr folderState =
        std::make_shared<FolderState>(folderSettings.path);
    folderState->initialize();
    folderStates.push_back(folderState);
  }
}
