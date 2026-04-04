#include "shipBT.hpp"

void ShipBT::printLog(const std::string& message, const Context& ctx)
{
	hlt::log::log("[T" + std::to_string(ctx.game->turn_number) + "S" + std::to_string(ctx.ship->id) + "]\t" + message);
}

hlt::Direction ShipBT::getNextDirectionTowards(hlt::Position destination, Context& ctx, bool unsafeLastMove)
{
    if (ctx.ship->halite < ctx.game->game_map->at(ctx.ship->position)->halite / 10)
    {
        printLog("Not enough halite to move", ctx);
        return hlt::Direction::STILL;
    }
    hlt::Position destination_n = ctx.game->game_map->normalize(destination);
    hlt::Direction nextDir;
    bool exitingStructure = ctx.game->game_map->at(ctx.ship->position)->has_structure();
    if(ctx.game->game_map->calculate_distance(ctx.ship->position,  destination_n) <= 1)
    {
        if (unsafeLastMove)
        {
			nextDir = ctx.game->game_map->get_unsafe_moves(ctx.ship->position, destination_n)[0];
        }
        else
        {
			nextDir = ctx.game->game_map->naive_navigate(ctx.ship, destination_n);
        }
    } else
    {
		nextDir = ctx.game->game_map->naive_navigate(
			ctx.ship,
			*(ctx.game->game_map->find_path(ctx.ship->position, destination_n, exitingStructure).end()-2));
    }
    return nextDir;
}

std::shared_ptr<hlt::Player> ShipBT::getOpponent(Context& ctx)
{
    for (std::shared_ptr<hlt::Player> player : ctx.game->players)
    {
        if (player->id != ctx.game->me->id)
        {
            return player;
        }
    }
    return ctx.game->players.at(1);
}
