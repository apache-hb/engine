#include "game/game.h"

using namespace game;

void GuiSink::send(logging::Category &category, logging::Level level, const char *pzMessage) {
    categories.insert(category.getName());
    entries.push_back({level, category.getName(), pzMessage});
}
