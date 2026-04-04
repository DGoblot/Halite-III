#pragma once

#include "BT.hpp"
#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "Context.hpp"
#include "shipBT.hpp"

class HunterBT:ShipBT
{
public:
    HunterBT();
    void evaluate(hlt::Game* game,
        const std::shared_ptr<hlt::Ship> ship,
        hlt::Command* command);

private:
    NodePtr tree;
};
