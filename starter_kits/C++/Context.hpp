#pragma once

#include <memory>
#include "hlt/game.hpp"

struct Context
{
	hlt::Game* game;
	std::shared_ptr<hlt::Ship> ship;
	hlt::Command* command;
	hlt::Position bestNearbyCell;
};
