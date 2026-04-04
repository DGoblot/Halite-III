#pragma once
#include <string>

class ShipBT
{
protected:
	static void printLog(std::string message, Context& ctx)
	{
		hlt::log::log("[T" + std::to_string(ctx.game->turn_number) + "S" + std::to_string(ctx.ship->id) + "]\t" + message);
	}
};
