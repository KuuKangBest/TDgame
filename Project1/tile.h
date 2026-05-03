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

	int terrian = 0; // 地图纹理的类别
	int decoration = -1; // 地图上有无装饰
	Direction direction = Direction::None; // 方向
	int special_tag; // 0表示家，正整数表示生成点
					 // -1表示没有特殊标记


	// 运行时
	bool has_tower = false;
};


typedef std::vector<std::vector<Tile>> TileMap;


#endif