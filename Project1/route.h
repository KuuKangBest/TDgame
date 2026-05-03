#ifndef _ROUTE_H_
#define _ROUTE_H_

#include "tile.h"
#include <vector>
#include <SDL.h>

using namespace std;

class Route {

public:
	typedef vector<SDL_Point> IdxList;


public:
	Route() = default;
	~Route() = default;

	Route(const TileMap& tilemap, SDL_Point idx_start) {
		// 表示从 idx_start 的idx开始的路径生成
		size_t width = tilemap[0].size();
		size_t height = tilemap.size();

		SDL_Point idx_next = idx_start;

		while (true) {

			// 超出屏幕边界
			if (idx_next.x >= width || idx_next.y >= height)
				break;

			// 有无环路
			if (check_is_duplicate(idx_next))
				break;

			idx_list.push_back(idx_next);


			// 终止条件
			// 终止条件1：当前idx_next已经到达目的地
			const Tile& tile = tilemap[idx_next.y][idx_next.x];
			
			if (tile.special_tag == 0)
				break;

			
			// 终止条件2：当前idx_next没有方向

			// 不终止：更新idx_next

			bool has_direction = true;
			switch (tile.direction)
			{
				case Tile::Direction::Up:
					idx_next.y--;
					break;

				case Tile::Direction::Down:
					idx_next.y++;
					break;
				case Tile::Direction::Left:
					idx_next.x--;
					break;
				case Tile::Direction::Right:
					idx_next.x++;
					break;
				case Tile::Direction::None:
					has_direction = false;
					break;
			}

			if (!has_direction)
				break;

		}
	}

	const IdxList& get_idx_list() const{
		return idx_list;
	}
private:
	IdxList idx_list;

private:
	bool check_is_duplicate(const SDL_Point& idx_target) {

		// 有重复则返回 true，否则返回false

		for (const SDL_Point& idx_now : idx_list) {
			if (idx_target.x == idx_now.x && idx_target.y == idx_now.y)
				return true;
		}

		return false;
	}
};

#endif
