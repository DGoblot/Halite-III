#include "farmer.hpp"
#include "hlt/log.hpp"
#include <unordered_map>

static std::unordered_map<hlt::EntityId, bool> s_goingHomeStates;

FarmerBT::FarmerBT()
{
    tree = selector({
        sequencer({
            leaf([](Context& ctx) { return nearbyTarget(ctx); }),
            leaf([](Context& ctx) { return loadAdvantage(ctx); }),
            leaf([](Context& ctx) { return chaseTarget(ctx); })
        }),
		sequencer({
			selector({
				leaf([](Context& ctx) { return enemyTooClose(ctx); }),
				leaf([](Context& ctx) { return shipFull(ctx); }),
				leaf([](Context& ctx) { return timeIsUp(ctx); })
			}),
			selector({
                sequencer({
					leaf([](Context& ctx) { return EnoughHaliteForDropoff(ctx); }),
                    leaf([](Context& ctx) { return NoDropoffNearby(ctx); }),
                    leaf([](Context& ctx) { return createDropoff(ctx); })
                }),
				sequencer({
					leaf([](Context& ctx) { return fleeingPossible(ctx); }),
					leaf([](Context& ctx) { return goingBackHome(ctx); })
				}),
				
			})
		}),
		sequencer({
			leaf([](Context& ctx) { return notOnBestCell(ctx); }),
			leaf([](Context& ctx) { return worthMoving(ctx); }),
			leaf([](Context& ctx) { return goingToHaliteSpot(ctx); })
		}),
		leaf([](Context& ctx) { return farm(ctx); })
	});
}

void FarmerBT::evaluate(hlt::Game* game,
    const std::shared_ptr<hlt::Ship> ship,
    hlt::Command* command)
{
    Context ctx{ game, ship, command };

    hlt::log::log("--- Ship " + std::to_string(ship->id) + " ---");
    bool goingHomeFlag = false;
    auto it = s_goingHomeStates.find(ship->id);
    if (it != s_goingHomeStates.end()) {
        goingHomeFlag = it->second;
    }

    if (goingHomeFlag)
    {
        goingBackHome(ctx);
        hlt::log::log("just going home bruh");
    }
    else { tree->evaluate(ctx); }
}

BT_NODE::State FarmerBT::enemyTooClose(Context& ctx)
{
    printLog("I have no enemy", ctx);
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::shipFull(Context& ctx)
{
    if (ctx.ship->halite >= 700 + 20 * ctx.game->game_map->calculate_distance(ctx.ship->position, ctx.game->me->shipyard->position) || ctx.ship->halite >= 900)
    {
		printLog("Ship full", ctx);
        return BT_NODE::State::SUCCESS;
    }
	printLog("Ship not full", ctx);
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::timeIsUp(Context& ctx)
{
    if (ctx.game->game_map->calculate_distance(ctx.ship->position, ctx.game->me->shipyard->position) >= hlt::constants::MAX_TURNS-ctx.game->turn_number - 10)
    {
		printLog("Time's up !", ctx);
        return BT_NODE::State::SUCCESS;
    }
    printLog("There is time left", ctx);
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::fleeingPossible(Context& ctx)
{
    printLog("I believe I can flee", ctx);
    return BT_NODE::State::SUCCESS;
}

BT_NODE::State FarmerBT::EnoughHaliteForDropoff(Context& ctx)
{
    if (ctx.ship->halite + ctx.game->me->halite + ctx.game->game_map->at(ctx.ship->position)->halite >= 4000)
    {
        hlt::log::log("Enough halite for dropoff");
        return BT_NODE::State::SUCCESS;
	}
    hlt::log::log("Not enough halite for dropoff");
	return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::NoDropoffNearby(Context& ctx)
{
    int min_distance = ctx.game->game_map->calculate_distance(ctx.ship->position, ctx.game->me->shipyard->position);
    for (auto& dropoff : ctx.game->me->dropoffs) {
        min_distance = std::min(min_distance, ctx.game->game_map->calculate_distance(ctx.ship->position, dropoff.second->position));
    }
    
    if (min_distance > 5)
    {
        hlt::log::log("No structures nearby");
        return BT_NODE::State::SUCCESS;
	}

    hlt::log::log("Structure nearby");
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::notOnBestCell(Context& ctx)
{
    const auto surrounding =
        ctx.ship->position.get_circle_zone_positions(5);

    bool betterCell = false;
    double bestCellValue = 0;

    for (auto pos : surrounding)
    {
        if (ctx.game->game_map->at(pos)->halite >
            ctx.game->game_map->at(ctx.ship->position)->halite &&
            ctx.game->game_map->at(pos)->is_empty())
        {
            auto newCellValue = ctx.game->game_map->at(pos)->halite - 0.1 * ctx.game->game_map->at(pos)->halite * ctx.game->game_map->calculate_distance(ctx.ship->position, pos);
            if (newCellValue > bestCellValue)
            {
                bestCellValue = newCellValue;
				ctx.bestNearbyCell = pos;
				betterCell = true;
            }
        }
    }

    if (!betterCell &&
        ctx.game->game_map->at(ctx.ship->position)->halite <= 10)
    {
        int i = 0;
        while (i < 3 &&
            ctx.game->game_map->at(surrounding[i])->is_occupied())
        {
            i++;
        }

        ctx.bestNearbyCell = surrounding[i];
        betterCell = true;
    }

    if (betterCell)
    {
        printLog("Better cell nearby: " + ctx.bestNearbyCell.to_string(), ctx);
        return BT_NODE::State::SUCCESS;
    }

    printLog("Cell is good", ctx);
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::worthMoving(Context& ctx)
{
    auto current =
        ctx.game->game_map->at(ctx.ship->position)->halite;

    auto best =
        ctx.game->game_map->at(ctx.bestNearbyCell)->halite;

    if (current <= 10 ||
        (current <= 100 && best - current > current))
    {
        printLog("Worth moving", ctx);
        return BT_NODE::State::SUCCESS;
    }

    printLog("Not worth moving", ctx);
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::goingBackHome(Context& ctx)
{
    s_goingHomeStates[ctx.ship->id] = true;

    hlt::Direction nextDir;
    if(ctx.game->game_map->calculate_distance(ctx.ship->position,  ctx.game->me->shipyard->position) <= 1)
    {
        nextDir = ctx.game->game_map->naive_navigate(ctx.ship, ctx.game->me->shipyard->position);
    } else
    {
		nextDir = ctx.game->game_map->naive_navigate(
			ctx.ship,
			*(ctx.game->game_map->find_path(ctx.ship->position, ctx.game->me->shipyard->position).end()-2));
    }
    *ctx.command =
        ctx.ship->move(nextDir);

    printLog("Home run : " + std::to_string(ctx.ship->position.directional_offset(nextDir).x) + ", " + std::to_string(ctx.ship->position.directional_offset(nextDir).y), ctx);
    if (ctx.game->game_map->calculate_distance(ctx.ship->position, ctx.game->me->shipyard->position) == 0) {
        s_goingHomeStates[ctx.ship->id] = false;
    }
    
    return BT_NODE::State::SUCCESS;
}

BT_NODE::State FarmerBT::createDropoff(Context& ctx)
{
    *ctx.command = ctx.ship->make_dropoff();

    printLog("Making dropoff", ctx);
    return BT_NODE::State::SUCCESS;
}

BT_NODE::State FarmerBT::farm(Context& ctx)
{
    *ctx.command = ctx.ship->stay_still();

    printLog("Farming", ctx);
    return BT_NODE::State::SUCCESS;
}

BT_NODE::State FarmerBT::goingToHaliteSpot(Context& ctx)
{
    hlt::Direction nextDir;
    bool exitingStructure = ctx.game->game_map->at(ctx.ship->position)->has_structure();
    if(ctx.game->game_map->calculate_distance(ctx.ship->position,  ctx.bestNearbyCell) <= 1)
    {
        nextDir = ctx.game->game_map->naive_navigate(ctx.ship, ctx.bestNearbyCell);
    } else
    {
		nextDir = ctx.game->game_map->naive_navigate(
			ctx.ship,
			*(ctx.game->game_map->find_path(ctx.ship->position, ctx.bestNearbyCell, exitingStructure).end()-2));    
    }
    *ctx.command =
        ctx.ship->move(nextDir);

    printLog("Going to better halite spot : " + ctx.ship->position.directional_offset(nextDir).to_string(), ctx);
    return BT_NODE::State::SUCCESS;
}

BT_NODE::State FarmerBT::nearbyTarget(Context& ctx)
{
    int circle_radius = 3;
    if (ctx.ship->halite <= 50 && hlt::constants::MAX_TURNS - ctx.game->turn_number <= 10)
    {
        circle_radius = ctx.game->game_map->width;
        printLog("Last minute hunter mode", ctx);
    }
    hlt::Position pos = ctx.ship->position;
    int bestTargetCargo = 0;
    for (int i = -circle_radius; i <= circle_radius; i++) {
        for (int j = -circle_radius; j <= circle_radius; j++) {

            if (abs(i) + abs(j) > circle_radius) continue;
            if (ctx.game->game_map->at(ctx.game->game_map->normalize({ pos.x + i,pos.y + j }))->is_occupied() && ctx.game->game_map->at(ctx.game->game_map->normalize({ pos.x + i,pos.y + j }))->ship->owner != ctx.game->me->id)
            {
                std::shared_ptr<hlt::Ship> target = ctx.game->game_map->at(ctx.game->game_map->normalize({ pos.x + i,pos.y + j }))->ship;
                for (std::shared_ptr<hlt::Player> player : ctx.game->players)
                {
                    if (player->id != ctx.game->me->id)
                    {
                        int smallestDropoffDist = ctx.game->game_map->calculate_distance(target->position, player->shipyard->position);
                        for (auto dropoff : player->dropoffs)
                        {
                            smallestDropoffDist = std::min(smallestDropoffDist, ctx.game->game_map->calculate_distance(target->position, dropoff.second->position));
                        }
                        if (ctx.game->game_map->calculate_distance(target->position, pos) < smallestDropoffDist ||  hlt::constants::MAX_TURNS - ctx.game->turn_number <= 10)
                        {
                            if (bestTargetCargo < target->halite)
                            {
                                ctx.target = target;
                                bestTargetCargo = target->halite;
                            }
                        }
                    }
                }
            }
        }
    }
    if (bestTargetCargo > 0 && (bestHunter(ctx) == ctx.ship || hlt::constants::MAX_TURNS - ctx.game->turn_number <= 10))
    {
    	printLog("Target Acquired: " + ctx.target->position.to_string(), ctx);
        return BT_NODE::State::SUCCESS;
    }
    printLog("No Target", ctx);
    return  BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::loadAdvantage(Context& ctx)
{
    if (hlt::constants::MAX_TURNS-ctx.game->turn_number <= 10)
    {
        return BT_NODE::State::SUCCESS;
    }
    std::shared_ptr<hlt::Player> opponent;
    for (std::shared_ptr<hlt::Player> player : ctx.game->players)
    {
        if (player->id != ctx.game->me->id)
        {
            opponent = player;
        }
    }
    if (ctx.target->halite - ctx.ship->halite >= 300 && (ctx.game->me->halite >= 1000 || ctx.game->me->ships.size() > opponent->ships.size()))
    {
        printLog("Big fish", ctx);
        return BT_NODE::State::SUCCESS;
    }
    printLog("Not big enough target", ctx);
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::chaseTarget(Context& ctx)
{
    hlt::Direction next_move = ctx.game->game_map->naive_navigate(ctx.ship, ctx.target->position);
    if (ctx.game->game_map->calculate_distance(ctx.ship->position, ctx.target->position) == 1)
    {
        next_move = ctx.game->game_map->get_unsafe_moves(ctx.ship->position, ctx.target->position)[0];
    } else
    {
		next_move = ctx.game->game_map->naive_navigate(
			ctx.ship,
			*(ctx.game->game_map->find_path(ctx.ship->position, ctx.target->position, true).end()-2));
    }
    *ctx.command = ctx.ship->move(next_move);
    printLog("Pursuit predation", ctx);
    return BT_NODE::State::SUCCESS;
}

std::shared_ptr<hlt::Ship> FarmerBT::bestHunter(Context& ctx)
{
    int circle_radius = 3;
    hlt::Position pos = ctx.target->position;
    int bestDist = 4;
    int bestCargo = 1000;
    std::shared_ptr<hlt::Ship> hunter;
	for (int i = -circle_radius; i <= circle_radius; i++) {
		for (int j = -circle_radius; j <= circle_radius; j++) {

			if (abs(i) + abs(j) > circle_radius) continue; 
            if (ctx.game->game_map->at(ctx.game->game_map->normalize({ pos.x + i,pos.y + j }))->is_occupied() && ctx.game->game_map->at(ctx.game->game_map->normalize({ pos.x + i,pos.y + j }))->ship->owner == ctx.game->me->id)
            {
                std::shared_ptr<hlt::Ship> newHunter = ctx.game->game_map->at(ctx.game->game_map->normalize({ pos.x + i,pos.y + j }))->ship;
                if (ctx.game->game_map->calculate_distance(newHunter->position, pos) <= bestDist && newHunter->halite <= bestCargo)
                {
                    bestDist = ctx.game->game_map->calculate_distance(newHunter->position, pos);
                    bestCargo = newHunter->halite;
                    hunter = newHunter;
                }
            }
		}
	}
    return hunter;
}

void FarmerBT::printLog(std::string message, Context& ctx)
{
	hlt::log::log("[T" + std::to_string(ctx.game->turn_number) + "S" + std::to_string(ctx.ship->id) + "]\t" + message);
}
