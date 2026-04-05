#include "hunter.hpp"

//the hunter ship goal is to block the ennemy shipyard to prevent storing halite

HunterBT::HunterBT()
{
}

void HunterBT::evaluate(hlt::Game* game,
    const std::shared_ptr<hlt::Ship> ship,
    hlt::Command* command)
{
    Context ctx{ game, ship, command };

    printLog("Ship " + std::to_string(ship->id), ctx);

    std::shared_ptr<hlt::Player> opponent = getOpponent(ctx);

    hlt::Direction nextDir = getNextDirectionTowards(opponent->shipyard->position, ctx);
    *ctx.command = ctx.ship->move(nextDir);
    printLog("Going to shipyard: " + opponent->shipyard->position.to_string(), ctx);
}
