#pragma once

#include <memory>
#include "hlt/game.hpp"

//everything the beahvior tree needs to process inputs

struct Context
{
	hlt::Game* game;
	std::shared_ptr<hlt::Ship> ship;
	hlt::Command* command;
	hlt::Position bestNearbyCell;
	std::shared_ptr<hlt::Ship> target;
	bool timeIsUp = false;
};
