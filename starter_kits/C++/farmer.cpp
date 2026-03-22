#include "farmer.hpp"

#include <iso646.h>

#include "hlt/log.hpp"

BT_SELECTOR* FarmerBT::btRoot = nullptr;
BT_SELECTOR* FarmerBT::motivation = nullptr;
BT_SELECTOR* FarmerBT::flee = nullptr;

BT_SEQUENCER* FarmerBT::retreat = nullptr;
BT_SEQUENCER* FarmerBT::goToHaliteSpot = nullptr;
BT_SEQUENCER* FarmerBT::home = nullptr;

BT_LEAF* FarmerBT::enemyTooClose = nullptr;
BT_LEAF* FarmerBT::shipFull = nullptr;
BT_LEAF* FarmerBT::fleeingPossible = nullptr;
BT_LEAF* FarmerBT::notOnBestCell = nullptr;
BT_LEAF* FarmerBT::worthMoving = nullptr;

BT_LEAF* FarmerBT::goingBackHome = nullptr;
BT_LEAF* FarmerBT::createDropoff = nullptr;
BT_LEAF* FarmerBT::mine = nullptr;
BT_LEAF* FarmerBT::goingToHaliteSpot = nullptr;

hlt::Position bestNearbyCell;

FarmerBT::FarmerBT()
{
	btRoot = new BT_SELECTOR(3);
		retreat = new BT_SEQUENCER(btRoot, 2);
			motivation = new BT_SELECTOR(retreat, 2);
				enemyTooClose = new BT_LEAF(motivation, evEnemyTooClose);
				shipFull = new BT_LEAF(motivation, evShipFull);
			flee = new BT_SELECTOR(retreat, 2);
				home = new BT_SEQUENCER(flee,  2);
					fleeingPossible = new BT_LEAF(home, evFleeingPossible);
					goingBackHome = new  BT_LEAF(home, evGoingBackHome);
				createDropoff = new BT_LEAF(flee, evCreateDropoff);
		goToHaliteSpot = new BT_SEQUENCER(btRoot, 3);
			notOnBestCell = new BT_LEAF(goToHaliteSpot, evNotOnBestCell);
			worthMoving = new  BT_LEAF(goToHaliteSpot, evWorthMoving);
			goingToHaliteSpot = new BT_LEAF(goToHaliteSpot, evGoingToHaliteSpot);
		mine = new BT_LEAF(btRoot, evMine);
}

void FarmerBT::evaluate(hlt::Game* game, std::shared_ptr<hlt::Ship> ship, hlt::Command* command)
{
	Context ctx{ game, ship, command};
	hlt::log::log("Ship " + std::to_string(ship->id));
	btRoot->Evaluate(&ctx);
}

BT_NODE::State FarmerBT::evEnemyTooClose(void* data)
{
	// TODO: Check if enemy is close enough to catch ship before it reaches nearest dropoff
	hlt::log::log("I have no enemy");
	return BT_NODE::FAILURE;
}

BT_NODE::State FarmerBT::evShipFull(void* data)
{
	const auto ctx = static_cast<Context*>(data);
	if (ctx->ship->halite >=  900)
	{
		hlt::log::log("Ship full");
		return BT_NODE::SUCCESS;
	}
	hlt::log::log("Ship not full");
	return BT_NODE::FAILURE;
}

BT_NODE::State FarmerBT::evFleeingPossible(void* data)
{
	// TODO: Check if it is possible to reach dropoff before getting caught
	hlt::log::log("I believe I can flee");
	return BT_NODE::SUCCESS;
}

BT_NODE::State FarmerBT::evNotOnBestCell(void* data)
{
	const auto ctx = static_cast<Context*>(data);
	const std::array<hlt::Position, 4> surrounding_cells = ctx->ship->position.get_surrounding_cardinals();
	auto  betterCell = false;
	for (hlt::Position pos: surrounding_cells)
	{
		if (ctx->game->game_map->at(pos)->halite > ctx->game->game_map->at(ctx->ship->position)->halite && ctx->game->game_map->at(pos)->is_empty())
		{
			bestNearbyCell = pos;
			betterCell = true;
		}
	}
	if (!betterCell && ctx->game->game_map->at(ctx->ship->position)->has_structure())
	{
		int i = 0;
		while (ctx->game->game_map->at(surrounding_cells[i])->is_occupied() && i < 3)
		{
			i++;
		}
		bestNearbyCell = surrounding_cells[i];
		betterCell = true;
	}
	if (betterCell)
	{
		hlt::log::log("Better cell nearby");
		return BT_NODE::SUCCESS;
	}

	hlt::log::log("Cell is good");
	return BT_NODE::FAILURE;
}

BT_NODE::State FarmerBT::evWorthMoving(void* data)
{
	const auto ctx = static_cast<Context*>(data);
	auto  currentCellHalite = ctx->game->game_map->at(ctx->ship->position)->halite;
	auto  bestNearbyCellHalite = ctx->game->game_map->at(bestNearbyCell)->halite;
	if(ctx->game->game_map->at(ctx->ship->position)->has_structure() || (currentCellHalite <= 100  && bestNearbyCellHalite-currentCellHalite > currentCellHalite))
	{
		hlt::log::log("Worth moving");
		return BT_NODE::SUCCESS;
	}
	hlt::log::log("Not worth moving: " + std::to_string(ctx->ship->position.x) + ", " + std::to_string(ctx->ship->position.y) + " : " + std::to_string(currentCellHalite) + " ; " + std::to_string(bestNearbyCellHalite));
	return  BT_NODE::FAILURE;
}

BT_NODE::State FarmerBT::evGoingBackHome(void* data)
{
	// TODO: Pathfind to nearest dropoff through empty cells
	const auto ctx = static_cast<Context*>(data);
	*ctx->command = ctx->ship->move(ctx->game->game_map->naive_navigate(ctx->ship, ctx->game->me->shipyard->position));
	hlt::log::log("Home run");
	return BT_NODE::SUCCESS;
}

BT_NODE::State FarmerBT::evCreateDropoff(void* data)
{
	const auto ctx = static_cast<Context*>(data);
	*ctx->command = ctx->ship->make_dropoff();
	hlt::log::log("Making dropoff");
	return BT_NODE::SUCCESS;
}

BT_NODE::State FarmerBT::evMine(void* data)
{
	const auto ctx = static_cast<Context*>(data);
	*ctx->command = ctx->ship->stay_still();
	hlt::log::log("Farming");
	return BT_NODE::SUCCESS;
}

BT_NODE::State FarmerBT::evGoingToHaliteSpot(void* data)
{
	const auto ctx = static_cast<Context*>(data);
	*ctx->command = ctx->ship->move(ctx->game->game_map->naive_navigate(ctx->ship, bestNearbyCell));
	hlt::log::log("Going to better halite spot");
	return BT_NODE::SUCCESS;
}
