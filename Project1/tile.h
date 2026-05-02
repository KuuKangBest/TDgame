#ifndef _TILE_H_   // if not define
#define _TILE_H_   // define

#include <vector>

#define SIZE_TILE 48

struct Tile {
	enum class Direction {
		None = 0,
		Left,
		Right,
		Up,
		Down
	};

	int terrian = 0; // 地图
	int decoration = -1; // 地图上有无装饰
	Direction direction = Direction::None; // 方向
	int special_tag;


	// 运行时
	bool has_tower = false;
};


typedef std::vector<std::vector<Tile>> TileMap;


#endif