#ifndef _MAP_H_
#define _MAP_H_

#include "tile.h"
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <SDL.h>
#include <unordered_map>
#include "route.h"

using namespace std;

class Map {

public:
	typedef unordered_map<int, Route> SpawnRoutePool;


public:
	Map() = default;
	~Map() = default;

	bool load(const string& path) {
		fstream file(path);

		if (!file.good()) return false;


		TileMap tile_map_temp;
		int idx_x = -1, idx_y = -1;

		string str_line;
		while (getline(file, str_line)) {
			str_line = trim_str(str_line);

			if (str_line.empty()) continue; // 这一行是空的

			// 运行到这说明这一行是有数据的，则需要在TileMap中新增一行
			idx_x = -1, idx_y++;
			tile_map_temp.emplace_back();

			string str_file;
			stringstream str_stream(str_line);
			while(getline(str_stream, str_file, ',')) 
			{
				idx_x++;
				tile_map_temp[idx_y].emplace_back();

				Tile& tile = tile_map_temp[idx_y].back();
				load_tile_from_string(tile, str_file);
			}


		}

		file.close();


		// 如果一开始地图是空的，也会有问题，会越界
		if (tile_map_temp.empty() || tile_map_temp[0].empty())
			return false;

		tile_map = tile_map_temp;
		generate_map_cache();
		return true;
	}

	size_t get_width() const{ // const表示对类内不会修改
		if (tile_map.empty()) // 如果不做这一步操作，程序运行中可能会出错
			return 0;
		return tile_map[0].size();
	}
	
	size_t get_height() const {
		return tile_map.size();
	}

private:
	TileMap tile_map;

	// 缓存相关（不是每次都去遍历，提前遍历一次）
	// 塔防游戏中的家在哪应该只需要遍历一次
	SDL_Point idx_home = { 0 };

	SpawnRoutePool spawn_route_pool;

private:
	string trim_str(const string& str) {
		// function: 删除开头和结尾的空格等占位符
		
		
		size_t begin = str.find_first_not_of(" \t");
		// 找第一个不是" \t"的字符

		if (begin == string::npos) return " ";// npos = -1, meaning no position
	
		size_t end = str.find_last_not_of(" \t");
		size_t length = end - begin + 1;

		return str.substr(begin, length);
	}	

	void load_tile_from_string(Tile& tile, const string& str) {
		string str_tidy = trim_str(str); // 去掉空格


		stringstream str_stream(str_tidy);
		string str_value;
		vector<int> values;
		while (getline(str_stream, str_value, '\\')) {
			int value;
			try {
				value = stoi(str_value); // 尝试将字符串转换为int数
			}
			catch (const std::invalid_argument&) { // catch需要写异常信息，这里需要处理万一写入非数字的情况，而我们的Tile瓦片所有的-1都是没有特别含义的，所以直接为-1即可
				value = -1;
			}
			values.push_back(value);
		}

		// 接下来需要小心对Tile进行赋值，完全有可能漏数据或是超范围
		tile.terrian = (values.size() < 1 || values[0] < 0) ? 0 : values[0];
		tile.decoration = (values.size() < 2) ? -1 : values[1];
		tile.direction = (Tile::Direction)((values.size() < 3 || values[2] < 0) ? 0 : values[2]);
		tile.special_tag = (values.size() < 4) ? -1 : values[3];
	}


	void generate_map_cache() {
		// 其实就是一次遍历，只不过先存起来
		for (int y = 0; y < get_height(); y++) {
			for (int x = 0; x < get_width(); x++) {
				const Tile& tile = tile_map[y][x];
				if (tile.special_tag < 0)
					continue;

				if (tile.special_tag == 0) {
					idx_home.x = x;
					idx_home.y = y;
				} // 之后还可以做其他缓存
				else {
					spawn_route_pool[tile.special_tag] = Route(tile_map, {x, y});
				}
			}
		}
	}
};
#endif 
