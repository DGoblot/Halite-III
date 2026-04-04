#pragma once

#include "types.hpp"
#include "map_cell.hpp"

#include <vector>
#include <map>
#include <algorithm>

namespace hlt {
    struct GameMap {
        int width;
        int height;
        std::vector<std::vector<MapCell>> cells;
        int circle_radius = 3;

        MapCell* at(const Position& position) {
            Position normalized = normalize(position);
            return &cells[normalized.y][normalized.x];
        }

        MapCell* at(const Entity& entity) {
            return at(entity.position);
        }

        MapCell* at(const Entity* entity) {
            return at(entity->position);
        }

        MapCell* at(const std::shared_ptr<Entity>& entity) {
            return at(entity->position);
        }

        int calculate_distance(const Position& source, const Position& target) {
            const auto& normalized_source = normalize(source);
            const auto& normalized_target = normalize(target);

            const int dx = std::abs(normalized_source.x - normalized_target.x);
            const int dy = std::abs(normalized_source.y - normalized_target.y);

            const int toroidal_dx = std::min(dx, width - dx);
            const int toroidal_dy = std::min(dy, height - dy);

            return toroidal_dx + toroidal_dy;
        }

        Position normalize(const Position& position) {
            const int x = ((position.x % width) + width) % width;
            const int y = ((position.y % height) + height) % height;
            return { x, y };
        }

        std::vector<Direction> get_unsafe_moves(const Position& source, const Position& destination) {
            const auto& normalized_source = normalize(source);
            const auto& normalized_destination = normalize(destination);

            const int dx = std::abs(normalized_source.x - normalized_destination.x);
            const int dy = std::abs(normalized_source.y - normalized_destination.y);
            const int wrapped_dx = width - dx;
            const int wrapped_dy = height - dy;

            std::vector<Direction> possible_moves;

            if (normalized_source.x < normalized_destination.x) {
                possible_moves.push_back(dx > wrapped_dx ? Direction::WEST : Direction::EAST);
            } else if (normalized_source.x > normalized_destination.x) {
                possible_moves.push_back(dx < wrapped_dx ? Direction::WEST : Direction::EAST);
            }

            if (normalized_source.y < normalized_destination.y) {
                possible_moves.push_back(dy > wrapped_dy ? Direction::NORTH : Direction::SOUTH);
            } else if (normalized_source.y > normalized_destination.y) {
                possible_moves.push_back(dy < wrapped_dy ? Direction::NORTH : Direction::SOUTH);
            }

            return possible_moves;
        }

        Direction naive_navigate(std::shared_ptr<Ship> ship, const Position& destination) {
            // get_unsafe_moves normalizes for us
            for (auto direction : get_unsafe_moves(ship->position, destination)) {
                Position target_pos = ship->position.directional_offset(direction);
                if (!at(target_pos)->is_occupied()) {
                    at(target_pos)->mark_unsafe(ship);
                    return direction;
                }
            }

            return Direction::STILL;
        }

        std::vector<Position> reconstruct_path(std::map<Position, Position> cameFrom, Position current) {
            std::vector<Position> total_path = { current };
            auto search = cameFrom.find(current);
            while ( search != cameFrom.end())
            {
                current = cameFrom.at(current);
                total_path.push_back(current);
                search = cameFrom.find(current);
            }
            log::log("Pathfinding Result = ");
            for (auto pos : total_path)
            {
                log::log(pos.to_string());
            }
        	return total_path;
        }

        std::vector<Position> find_path(Position start_n, Position goal_n, bool exiting_structure = false)
        {
            Position start = normalize(start_n);
            Position goal = normalize(goal_n);
            if (calculate_distance(start, goal) <= 1)
            {
                return { goal, start };
            }
            // The greater this factor is the more the path will prioritize shortness over cost of moves
            int speedOverHalite = 100;
            std::vector<Position> openSet = {start};
            std::vector<Position> closedSet;

            std::map<Position, Position> cameFrom;

            std::map<Position, int> gScore;
            for (auto cellList : cells)
            {
                for (auto cell: cellList)
                {
                    gScore.insert({cell.position, INT_MAX});
                }
            }
            gScore.at(start) = 0;

            std::map<Position, int> fScore;
            for (auto cellList : cells)
            {
                for (auto cell: cellList)
                {
                    fScore.insert({cell.position, INT_MAX});
                }
            }
            fScore.at(start) = calculate_distance(start, goal) * speedOverHalite;

            while (!openSet.empty())
            {
                Position current = openSet.at(0);
                for (Position pos: openSet)
                {
	                if (fScore.at(pos) < fScore.at(current))
	                {
                        current = pos;
	                }
                }
                //log::log("Pathfinding testing position :" + std::to_string(current.x) + ", " + std::to_string(current.y));
                if (current == goal)
                {
                    return reconstruct_path(cameFrom, current);
                }
                closedSet.push_back(current);
                openSet.erase(std::remove(openSet.begin(), openSet.end(), current), openSet.end());
            	for (auto neighbor : current.get_surrounding_cardinals())
				{
                    neighbor = normalize(neighbor);
                    if (std::find(closedSet.begin(), closedSet.end(), neighbor) != closedSet.end())
                    {
                        continue;
                    }
                    int tentativeGScore = gScore.at(current) + at(current)->halite + speedOverHalite;
                    if(at(neighbor)->is_occupied() && neighbor!=goal ||
                        !exiting_structure && at(neighbor)->structureExit)
                    {
                        tentativeGScore = INT_MAX;
                    }
					if (tentativeGScore < gScore[neighbor])
					{
                        cameFrom[neighbor] = current;
                        gScore[neighbor] = tentativeGScore;
                        fScore.at(neighbor) = tentativeGScore + calculate_distance(neighbor, goal) * speedOverHalite;
                        if (std::find(openSet.begin(),  openSet.end(), neighbor) == openSet.end())
                        {
                            openSet.push_back(neighbor);
                        }
					}
				}
            }
			// Open set is empty but goal was never reached
            return {start, start};
        }

        // Check zone with lot of halite
        Position best_zone() {
            int max = 0;
            Position max_pos;
            for each(auto row in cells) {
                for each(MapCell cell in row) {
                    if (halite_around(cell.position) > max) {
                        max = halite_around(cell.position);
                        max_pos = cell.position;
                    }
                }
            }
            return max_pos;
        }

        int halite_around(Position pos) {
            int res = 0;
            for (auto position : pos.get_circle_zone_positions(3))
            {
				res += at(position)->halite;
            }
            return res;
        }

        void _update();
        static std::unique_ptr<GameMap> _generate();
    };
}
