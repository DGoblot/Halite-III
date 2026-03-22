#pragma once
#include "BT.hpp"
#include "hlt/game.hpp"
#include "hlt/constants.hpp"

class FarmerBT
{
	struct Context
	{
		hlt::Game* game;
		std::shared_ptr<hlt::Ship> ship;
		hlt::Command* command;
	};

public:
	FarmerBT();
	void evaluate(hlt::Game* game,  std::shared_ptr<hlt::Ship> ship, hlt::Command* command);
private:
	static BT_SELECTOR *btRoot, *motivation, *flee;
	static BT_SEQUENCER *retreat, *goToHaliteSpot, *home;
	// Input leafs
	static BT_LEAF *enemyTooClose, *shipFull, *fleeingPossible, *notOnBestCell, *worthMoving;
	static BT_NODE::State evEnemyTooClose(void* data), evShipFull(void* data), evFleeingPossible(void* data), evNotOnBestCell(void* data), evWorthMoving(void* data);
	// Action leafs
	static BT_LEAF *goingBackHome, *createDropoff, *mine, *goingToHaliteSpot;
	static BT_NODE::State evGoingBackHome(void* data), evCreateDropoff(void* data), evMine(void* data), evGoingToHaliteSpot(void* data);
};
