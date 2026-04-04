#include "hunter.hpp"
#include "hlt/log.hpp"

HunterBT::HunterBT()
{
}

void HunterBT::evaluate(hlt::Game* game,
    const std::shared_ptr<hlt::Ship> ship,
    hlt::Command* command)
{
    Context ctx{ game, ship, command };

    hlt::log::log("Ship " + std::to_string(ship->id));

    std::shared_ptr<hlt::Player> opponent;
    for (std::shared_ptr<hlt::Player> player : ctx.game->players)
    {
        if (player->id != ctx.game->me->id)
        {
            opponent = player;
        }
    }

    hlt::Direction nextDir;
    if(ctx.game->game_map->calculate_distance(ctx.ship->position,  opponent->shipyard->position) <= 1)
    {
        nextDir = ctx.game->game_map->naive_navigate(ctx.ship, opponent->shipyard->position);
    } else
    {
		nextDir = ctx.game->game_map->naive_navigate(
			ctx.ship,
			*(ctx.game->game_map->find_path(ctx.ship->position, opponent->shipyard->position).end()-2));
    }
    *ctx.command =
        ctx.ship->move(nextDir);
    hlt::log::log("Going to shipyard: " + std::to_string(opponent->shipyard->position.x) + ", " + std::to_string(opponent->shipyard->position.y));
}
