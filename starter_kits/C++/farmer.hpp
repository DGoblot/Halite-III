#pragma once

#include "BT.hpp"
#include "shipBT.hpp"
#include <unordered_map>

class FarmerBT:ShipBT
{
public:
    FarmerBT();
    void evaluate(hlt::Game* game,
        const std::shared_ptr<hlt::Ship> ship,
        hlt::Command* command);
    static const int MAX_DROPOFF_NB;

private:
    NodePtr tree;
	static std::unordered_map<hlt::EntityId, bool> s_goingHomeStates;

    // Input
    static BT_NODE::State enemyTooClose(Context& ctx);
    static BT_NODE::State shipFull(Context& ctx);
    static BT_NODE::State timeIsUp(Context& ctx);
    static BT_NODE::State fleeingPossible(Context& ctx);
    static BT_NODE::State EnoughHaliteForDropoff(Context& ctx);
	static BT_NODE::State NoDropoffNearby(Context& ctx);
	static BT_NODE::State IsDropoffWorthIt(Context& ctx);
    static BT_NODE::State notOnBestCell(Context& ctx);
    static BT_NODE::State worthMoving(Context& ctx);
    static BT_NODE::State nearbyTarget(Context& ctx);
    static BT_NODE::State loadAdvantage(Context& ctx);
    static BT_NODE::State alreadyGoing(Context& ctx);

    // Actions
    static BT_NODE::State goingBackHome(Context& ctx);
    static BT_NODE::State createDropoff(Context& ctx);
    static BT_NODE::State farm(Context& ctx);
    static BT_NODE::State goingToHaliteSpot(Context& ctx);
    static BT_NODE::State chaseTarget(Context& ctx);

    static std::shared_ptr<hlt::Ship> bestHunter(Context& ctx);
};
