#include "farmer.hpp"
#include "hlt/log.hpp"

FarmerBT::FarmerBT()
{
    tree = selector({
		sequencer({
			selector({
				leaf([](Context& ctx) { return enemyTooClose(ctx); }),
				leaf([](Context& ctx) { return shipFull(ctx); })
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
    std::shared_ptr<hlt::Ship> ship,
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
        ctx.game->game_map->at(ctx.ship->position)->has_structure())
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

    if (ctx.game->game_map->at(ctx.ship->position)->has_structure() ||
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