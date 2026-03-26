#include "farmer.hpp"
#include "hlt/log.hpp"

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
					leaf([](Context& ctx) { return fleeingPossible(ctx); }),
					leaf([](Context& ctx) { return goingBackHome(ctx); })
				}),
				leaf([](Context& ctx) { return createDropoff(ctx); })
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

    hlt::log::log("Ship " + std::to_string(ship->id));

    tree->evaluate(ctx);
}

BT_NODE::State FarmerBT::enemyTooClose(Context& ctx)
{
    hlt::log::log("I have no enemy");
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::shipFull(Context& ctx)
{
    if (ctx.ship->halite >= 900)
    {
        hlt::log::log("Ship full");
        return BT_NODE::State::SUCCESS;
    }

    hlt::log::log("Ship not full");
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::timeIsUp(Context& ctx)
{
    if (ctx.game->game_map->calculate_distance(ctx.ship->position, ctx.game->me->shipyard->position) >= hlt::constants::MAX_TURNS-ctx.game->turn_number)
    {
		hlt::log::log("Time's up !");
        return BT_NODE::State::SUCCESS;
    }
    hlt::log::log("There is time left");
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::fleeingPossible(Context& ctx)
{
    hlt::log::log("I believe I can flee");
    return BT_NODE::State::SUCCESS;
}

BT_NODE::State FarmerBT::notOnBestCell(Context& ctx)
{
    const auto surrounding =
        ctx.ship->position.get_surrounding_cardinals();

    bool betterCell = false;

    for (auto pos : surrounding)
    {
        if (ctx.game->game_map->at(pos)->halite >
            ctx.game->game_map->at(ctx.ship->position)->halite &&
            ctx.game->game_map->at(pos)->is_empty())
        {
            ctx.bestNearbyCell = pos;
            betterCell = true;
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
        hlt::log::log("Better cell nearby");
        return BT_NODE::State::SUCCESS;
    }

    hlt::log::log("Cell is good");
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
        hlt::log::log("Worth moving");
        return BT_NODE::State::SUCCESS;
    }

    hlt::log::log("Not worth moving");
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::goingBackHome(Context& ctx)
{
    *ctx.command =
        ctx.ship->move(
            ctx.game->game_map->naive_navigate(
                ctx.ship,
                ctx.game->me->shipyard->position));

    hlt::log::log("Home run");
    return BT_NODE::State::SUCCESS;
}

BT_NODE::State FarmerBT::createDropoff(Context& ctx)
{
    *ctx.command = ctx.ship->make_dropoff();

    hlt::log::log("Making dropoff");
    return BT_NODE::State::SUCCESS;
}

BT_NODE::State FarmerBT::farm(Context& ctx)
{
    *ctx.command = ctx.ship->stay_still();

    hlt::log::log("Farming");
    return BT_NODE::State::SUCCESS;
}

BT_NODE::State FarmerBT::goingToHaliteSpot(Context& ctx)
{
    *ctx.command =
        ctx.ship->move(
            ctx.game->game_map->naive_navigate(
                ctx.ship,
                ctx.bestNearbyCell));

    hlt::log::log("Going to better halite spot");
    return BT_NODE::State::SUCCESS;
}

BT_NODE::State FarmerBT::nearbyTarget(Context& ctx)
{
    int circle_radius = 3;
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
						if (ctx.game->game_map->calculate_distance(target->position, pos) < smallestDropoffDist)
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
    if (bestTargetCargo>0 && bestHunter(ctx) == ctx.ship)
    {
    	hlt::log::log("Target Acquired");
        return BT_NODE::State::SUCCESS;
    }
    hlt::log::log("No Target");
    return  BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::loadAdvantage(Context& ctx)
{
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
        hlt::log::log("Big fish");
        return BT_NODE::State::SUCCESS;
    }
    hlt::log::log("Not big enough target");
    return BT_NODE::State::FAILURE;
}

BT_NODE::State FarmerBT::chaseTarget(Context& ctx)
{
    hlt::Direction next_move = ctx.game->game_map->naive_navigate(ctx.ship, ctx.target->position);
    if (ctx.game->game_map->calculate_distance(ctx.ship->position, ctx.target->position) == 1)
    {
        next_move = ctx.game->game_map->get_unsafe_moves(ctx.ship->position, ctx.target->position)[0];
    }
    *ctx.command = ctx.ship->move(next_move);
    hlt::log::log("Pursuit predation");
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
