#pragma once
#include <string>
#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "BT.hpp"
#include "Context.hpp"

class ShipBT
{
protected:
    static void printLog(const std::string& message, const Context& ctx);
    static hlt::Direction getNextDirectionTowards(hlt::Position destination, Context& ctx, bool unsafeLastMove = false);
    static std::shared_ptr<hlt::Player> getOpponent(Context& ctx);
};
