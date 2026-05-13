# 学习日记（个人能看懂）

## 目录

- [程序库总览和游戏框架搭建](#程序库总览和游戏框架搭建)
  - [2026.4.20之前 SDL相关库、JSON与CSV格式解析](#2026420之前-sdl相关库json与csv格式解析)
  - [2026.4.20 可继承单例模板类实践](#2026420-可继承单例模板类实践)
  - [2026.4.29 GameManager的主体实现框架](#2026429-gamemanager的主体实现框架)
    - [初始化与初始化断言](#初始化与初始化断言)
    - [游戏运行时](#游戏运行时)
    - [游戏退出逻辑](#游戏退出逻辑)
  - [2026.5.1 配置数据的设计思路](#202651-配置数据的设计思路)
  - [2026.5.2 瓦片地图数据功能实现](#202652-瓦片地图数据功能实现)
    - [tile类实现](#tile类实现)
    - [Map类数据加载](#map类数据加载)
    - [地图缓存生成](#地图缓存生成)
- [游戏主体内容开发](#游戏主体内容开发)
  - [2026.5.3 洋流图与预烘焙寻路实现](#202653-洋流图与预烘焙寻路实现)
  - [2026.5.9 ConfigManager游戏配置管理器实现](#202659-configmanager游戏配置管理器实现)
    - [Map类更多接口](#map类更多接口)
    - [配置器类](#配置器类)
  - [2026.5.13 关卡配置数据加载](#2026513-关卡配置数据加载)
  - [2026.5.13 游戏配置加载](#2026513-游戏配置加载)
  - [2026.5.14 游戏资源加载管理器](#2026514-游戏资源加载管理器)



## 程序库总览和游戏框架搭建

### 2026.4.20之前 SDL相关库、JSON与CSV格式解析



### 2026.4.20 可继承单例模板类实践

单例：比如管理器，程序中不允许创建第二个实例

```c++
class Manager{
public:
	static Manager* Instance(){ // 静态函数
        if(!instance){
            instance = new Manager();
        }
        return instance;
    }    
private:
    static Manager* instance = nullptr;
    
private:
    Manager() = default; // 如果单独定义了构造函数在private当中，那外部就不可能访问到构造函数
    // 同理对析构和拷贝函数
    
}
```

模板：（模板类完整实现、为何需要友元函数）

```c++
-----manager.h------

template<typename T>
class Manager{
public:
	static T* Instance(){ // 静态函数
        if(!instance){
            instance = new T(); // 注意这里的逻辑是基类函数实现中需要在派生类中去新建派生类，但是protected只能保证派生类能够访问基类函数，反过来不保证，所以需要在后续写友元
        }
        return instance;
    }    
private:
    static T* instance = nullptr;
    
protected: // 子类可以访问
    Manager() = default; 
    ~Manager() = default;
     
    Manager(const Manager&) = delete; // 删除拷贝函数
 	Manager& operator=(const Manager&) = delete; // 删除赋值函数   
}
```

那么在某一个类的具体实现中：

```c++
-----game_manager.h------

    
#include<manager.h>
class GameManager:public Manager<GameManager>{
    friend Manager<GameManager>; // 再次注意这里的友元，原因在上面有讲到
protected:
    GameManager(){
        
    }
    ...
}

```

### 2026.4.29 GameManager的主体实现框架

分为三个板块：初始化与初始化断言、游戏运行时、游戏结束逻辑

#### 初始化与初始化断言

这里的断言是指在运行时的断言（不是在编译时发生的错误）

所有代码都在 `game_manager.h` 中，我们的初始化就放在构造函数中

```c++
-----game_manager.h------
    


private:
	init_assert(bool is_ok, const char* err_msg){
        // @param bool is_ok 表示是否成功
		// @param const char* err_msg 字符串表示需要汇报的错误信息
		
        if(is_ok) return;
        
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, // FLAG表示类型：error, information, warning
				u8"初始化失败",
				err_msg,
				window); // window表示父窗口（生成的新Box会层级在这之上显示，并且子窗口关闭父程序才会继续），所以我们需要在游戏管理器中创建一个窗口
        
    }

private:
	SDL_Window* window = nullptr;
```

接下来是构造函数，一起将初始化工作做了：

```c++
-----game_manager.h------


protected:
	GameManager() { // 构造函数需要初始化库
	
		/*  初始化与初始化断言   */ 

		init_assert(!SDL_Init(SDL_INIT_EVERYTHING), u8"SDL库启动失败");			// SDL: 0 表示成功，负数表示失败
		init_assert(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG), u8"IMG库启动失败");	// IMG: 返回成功初始话的标志，若失败则返回0
		init_assert(Mix_Init(MIX_INIT_MP3), u8"mixer库启动失败");					// mixer: 返回成功初始话的标志，若失败则返回0
		init_assert(!TTF_Init(), u8"字体库ttf启动失败");

		Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);						// 打开音频设备，参数分别是：频率、格式、声道数、缓冲区大小

		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");  // 开启hint， 注意第二个参数是字符串
	
		// 创建游戏窗口
		window = SDL_CreateWindow(u8"塔防游戏", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1080, 720, SDL_WINDOW_SHOWN);
		init_assert(window, u8"游戏窗口创建失败");

		// 创建渲染器
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
		// 参数分别是：窗口、索引（-1表示使用第一个支持的渲染器）、标志（加速、垂直同步、支持渲染到纹理）
		init_assert(renderer, u8"渲染器创建失败");
		
	}
```

#### 游戏运行时

游戏运行的逻辑是：处理事件（获取新的输入）、更新数据（通常时间相关）、渲染

分别对应函数 `on_input()`，`on_update(double delta)`、`on_render()`

并且为了保证与主函数的参数保持一致性，我们的游戏管理器中的运行函数输入参数与 `main` 的输入一样也就是 `int run(int argc, char ** argv)`

==值得一提的是SDL的`int main()`不是真正的SDL运行入口，有`main`的地方需要加`#define SDL_MAIN_HANDLED`== 

```c++
-----game_manager.h------

int run(int argc, char ** argv) {
		Uint64 last_counter = SDL_GetPerformanceCounter(); // 高精度计数器
		Uint64 counter_freq = SDL_GetPerformanceFrequency(); // 计数器频率
		
		while (!is_quit) {
			// 事件处理
			while (SDL_PollEvent(&event)) {
				on_input();
			}

            
			Uint64 current_counter = SDL_GetPerformanceCounter();
			double delta = (double)(current_counter - last_counter) / counter_freq;  // 实际从上一帧结束过去的时间
			last_counter = current_counter;
			if (delta * 1000.0 < 1000.0 / 60.0) { // 目前实现动态帧率控制
				SDL_Delay(1000.0 / 60 - delta * 1000.0); // 以60帧为目标帧率，控制帧率
			}

            
            
			// 更新数据 （时间相关）
			on_update(delta);


            
			// 渲染绘图
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer); // 用上面的颜色清空渲染器
			
            // 渲染
			on_render();
			SDL_RenderPresent(renderer); // 将渲染器的内容呈现到屏幕上
		}

		return 0;
	}
```

#### 游戏退出逻辑

```c++
-----game_manager.h------

~GameManager() {	//析构函数需要关闭掉库
		// 申请从下往上销毁资源，销毁顺序与申请顺序相反（栈，否则可能会出现野指针）
		SDL_DestroyRenderer(renderer);	// 销毁渲染器
		SDL_DestroyWindow(window);		// 销毁窗口
		Mix_CloseAudio();				// 关闭音频设备
		TTF_Quit();						
		Mix_Quit();
		IMG_Quit();
		SDL_Quit();						// 推出各库，释放资源
	
	};
```

这样在 `main.cpp` 只需要这样写即可

```c++
#define SDL_MAIN_HANDLED // 否则SDL会定义main函数，导致链接错误
						 // 这样写之后才可能让这个程序中的main函数成为真正SDL的main函数
#include<iostream>
#include "game_manager.h"

int main(int argc, char ** argv) {
	return GameManager::Instance()->run(argc, argv);
}
```

### 2026.5.1 配置数据的设计思路

属性配置：游戏窗口属性、角色敌人的属性

关卡配置：波次、难度

**地图配置**：瓦片地图数据不仅可以存储不同层级上的**渲染**，还可以存储**逻辑**信息：比如怪物行进路线、怪物生成点之类的逻辑信息，在内存空间中划分好位置即可（也可以用斜线分开（读取有利））

### 2026.5.2 瓦片地图数据功能实现

小插曲：才知道这具体表示什么意思

```c++
#ifndef _TILE_H_  // if not define
#define _TILE_H_  // define
#endif   		  // end define
```

#### tile类实现

```c++
-------tile.h--------

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
```

#### Map类数据加载

这里主要是练习csv的数据加载流程

请一定要注意src是流式，不管是 `fstream` 还是 `sstream` 都要先变成流式

先**成功**打开文件，直接按照行切分`getline(src, dst)`，空行直接跳过。若不是空行则去掉开头结尾的空格等

接下来还是要继续使用 `getline()`函数，只不过新增字符串表示间隔字符（之前默认间隔字符为 `\n`）

先用 `,` 做分割，得到单个tile的可能数据，然后再用单独的函数处理，用 `\\` 做分割——注意这里细致一点，对各种错误情况做适当的处理

以上所有都先存到 `tile_map_temp` 中，只有等到都确定成功了才考虑放入真正的地图中

```c++
-------map.h--------
#include <tile.h>

private:
	TileMap tile_map;


public:
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
		return true;
	}


private: // 一些类内的辅助方法
	string trim_str(const string& str) {
		// function: 删除开头和结尾的空格等占位符
		
		
		size_t begin = str.find_first_not_of(" \t");
		// 找第一个不是" \t"的字符

		if (begin == string::npos) return " ";// npos = -1, meaning no position
	
		size_t end = str.find_last_not_of(" \t");
		size_t length = end - begin + 1;

		return str.substr(begin, length);
	}
```

#### 地图缓存生成

其实缓存的本质就是用一次遍历去换后续游戏的多次遍历。地图上所存储的家（塔防游戏怪物的目的地）如果每次都要遍历性能一定降低，所以说先遍历一次并且存储

```c++
-------map.h--------
    
    
private:
	SDL_Point idx_home = { 0 };

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
			}
		}
	}

public:
	size_t get_width() const{ // const表示对类内不会修改
		if (tile_map.empty()) // 如果不做这一步操作，程序运行中可能会出错
			return 0;
		return tile_map[0].size();
	}
	
	size_t get_height() const {
		return tile_map.size();
	}

```





## 游戏主体内容开发

### 2026.5.3 洋流图与预烘焙寻路实现

洋流图在这个项目中就是指敌人的行进路线（预先生成好的地图向量场设计），塔防游戏中一般不会用A*算法之类的算法而是用预先定义好的洋流图进行寻路（流场寻路）

本节是基于上一节的地图缓存生成的，具体来说新增一个缓存，是一个出生点int型index和对应路径的一个map（哈希表）

```c++
if (tile.special_tag < 0)
	continue;

if (tile.special_tag == 0) {
	idx_home.x = x;
	idx_home.y = y;
} // 之后还可以做其他缓存
else {
	spawn_route_pool[tile.special_tag] = Route(tile_map, {x, y});
}


public:
	typedef unordered_map<int, Route> SpawnRoutePool;
private:
	SpawnRoutePool spawn_route_pool;
```

接下来有关 `Route()` 具体实现在 `Route.h` 中

```c++
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
					...
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
	bool check_is_duplicate(const SDL_Point& idx_target){} // 遍历数组即可
		// 有重复则返回 true，否则返回false
};

#endif
```



### 2026.5.9 ConfigManager游戏配置管理器实现

#### Map类更多接口

`Map` 类需要有暴露给外界自己的私有成员的需求，并且放置防御塔后需要更新自己私有成员内的tile_map中对应idx的tile中的has_tower参数：

```cpp
const TileMap& get_tile_map() const;
const SDL_Point& get_idx_home() const;
const SpawnRoutePool& get_spawn_route_pool() const;

void place_tower(const SDL_Point& idx_tile) {
	tile_map[idx_tile.y][idx_tile.x].has_tower = true;
}
```

#### 配置器类

配置器类只需要根据配置文件的结构进行相应字段的添加与初始化即可（加载后面再进行）。跟游戏玩法和配置文件结构强相关

### 2026.5.13 关卡配置数据加载

使用 cJSON 读取配置文件的整体思路：**第一步检查**（文件→内存→JSON合法性）、**第二步解析**（逐字段解析，默认值兜底，空容器回退）。

第一步的核心：每一步都必须验证，验证失败立即清理资源并返回（`cJSON_Delete` 删指针）。

第二步的核心：有默认值的字段没扫到不用 `continue`，但容器类字段（如数组）如果为空则必须做回退处理（删除之前预留的空间）。

```cpp
-------config_manager.h--------

bool load_level_config(const string& path) {
    // ===== 第一步：检查 =====
    
    ifstream file(path);
    if (!file.good()) {  // 检查文件是否成功打开
        return false;
    }
     
    stringstream buffer;
    buffer << file.rdbuf();  // 读入内存（流式）
    file.close();            // 读到内存后就可以关闭文件了，养成好习惯

    cJSON* json_root = cJSON_Parse(buffer.str().c_str());  // strstream → c_str → cJSON
    
    if (!json_root) {  // 检查 json_root 是否存在
        cerr << "Failed to parse config file: " << path << endl;
        return false;
    } 
    if (json_root->type != cJSON_Array) {  // 检查是否为期待的数组类型
        cerr << "Config file is not an array: " << path << endl;
        cJSON_Delete(json_root);  // 不合法就直接删掉，防止内存泄漏
        return false;
    }

    
    
    
    
    
    // ===== 第二步：解析 =====
    
    cJSON* json_wave = nullptr;
    cJSON_ArrayForEach(json_wave, json_root) {
        if (json_wave->type != cJSON_Object) {
            cout << "Invalid wave config, expected an object." << endl;
            continue;  // 类型不对，跳过这个结点
        }

        wave_list.emplace_back();  // 检查通过后才创建 C++ 对象
        Wave& wave = wave_list.back();

        // 判断模式：if (json_item && json_item->type == 期待类型)，缺一不可
        // 有默认值的字段 —— 找不到或类型不对就以默认值为准，不需要 continue
        
        cJSON* json_wave_interval = cJSON_GetObjectItem(json_wave, "interval");
        if (json_wave_interval && json_wave_interval->type == cJSON_Number) {
            wave.interval = json_wave_interval->valuedouble;
        }

        cJSON* json_wave_rewards = cJSON_GetObjectItem(json_wave, "rewards");
        if (json_wave_rewards && json_wave_rewards->type == cJSON_Number) {
            wave.rewards = json_wave_rewards->valuedouble;
        }

        // 容器类字段 —— 必须检查是否为空，为空则回退
        cJSON* json_wave_spawn_events = cJSON_GetObjectItem(json_wave, "spawn_list");
        if (json_wave_spawn_events && json_wave_spawn_events->type == cJSON_Array) {
            cJSON* json_spawn_event = nullptr;
            cJSON_ArrayForEach(json_spawn_event, json_wave_spawn_events) {
                if (json_spawn_event->type != cJSON_Object) {
                    cout << "Invalid spawn event config, expected an object." << endl;
                    continue;
                }
                Wave::SpawnEvent spawn_event;  // 用默认值初始化，后续只覆盖找到的字段

                cJSON* json_spawn_interval = cJSON_GetObjectItem(json_spawn_event, "interval");
                if (json_spawn_interval && json_spawn_interval->type == cJSON_Number) {
                    spawn_event.interval = json_spawn_interval->valuedouble;
                }
				...

                // 字符串到枚举的映射，都不匹配则以默认值 Slim 为准
                cJSON* json_enemy_type = cJSON_GetObjectItem(json_spawn_event, "enemy_type");
                if (json_enemy_type && json_enemy_type->type == cJSON_String) {
                    string enemy_type_str = json_enemy_type->valuestring;
                    if (enemy_type_str == "Slim") {
                        spawn_event.enemy_type = EnemyType::Slim;
                    } else if ...
                }
                wave.spawn_event_list.push_back(spawn_event);
            }

            // ★ 核心：如果一个 spawn_event 都没有，这个 wave 就没意义了
            if (wave.spawn_event_list.empty())
                wave_list.pop_back();  // 删除之前预留的空间，interval 等也随之丢弃
        }
    }

    // ===== 收尾 =====
    cJSON_Delete(json_root);  // 解析完毕，释放 JSON 内存

    if (wave_list.empty()) {
        cout << "Warning: No valid wave config found in file: " << path << endl;
        return false;
    }
    return true;
}
```

关键点再强调一下：

- **默认值兜底**：结构体成员已有默认值（如 `interval = 0`, `spawn_point = 1`, `enemy_type = EnemyType::Slim`），JSON 中找不到或类型不对直接跳过即可，不需要 `continue`。
- **空容器必须回退**：`spawn_list` 数组如果一条 `spawn_event` 都没解析成功，必须 `pop_back()` 把前面 `emplace_back()` 预留的 wave 删掉，否则会留下一个没有生成事件的空壳 wave。
- **cJSON 指针手动管理**：`cJSON_Parse` 返回的指针不用时必须 `cJSON_Delete`，否则内存泄漏。类型不匹配时记得先删再 `return`。

### 2026.5.13 游戏配置加载

大致的思路与上一节保持一样，但现在enemy是一个模板完全可以写一个函数来进行加载

```cpp
public:
	bool load_game_config(const string& path) {
		
        ... // 各种检查
            
		// 同层级下一遍获取
		cJSON* json_basic = cJSON_GetObjectItem(json_root, "basic");
		cJSON* json_player = cJSON_GetObjectItem(json_root, "player");
		cJSON* json_tower = cJSON_GetObjectItem(json_root, "tower");
		cJSON* json_enemy = cJSON_GetObjectItem(json_root, "enemy");

		... // 获取之后一边检查即可

		parse_basic_template(basic_template, json_basic);
		
		parse_player_template(player_template, json_player);
	
		parse_tower_template(archer_template, cJSON_GetObjectItem(json_tower, "archer"));
		parse_tower_template(axeman_template, cJSON_GetObjectItem(json_tower, "axeman"));
		...

		parse_enemy_template(slim_template, cJSON_GetObjectItem(json_enemy, "Slim"));
		...

		cJSON_Delete(json_root);
		return true;
}
```

```cpp
private:
	void parse_number_array(double* arr, int max_len, cJSON* json_root){
		if(!json_root || json_root->type != cJSON_Array){
			return;
		}
		int idx = -1; // 数组索引，初始值为-1，因为数组从0开始索引
		cJSON* json_item = nullptr;
		cJSON_ArrayForEach(json_item, json_root){
			idx++;	
			if(json_item->type == cJSON_Number){
				if(idx < max_len){
					arr[idx] = json_item->valuedouble;
				}
				else{
					break; // 超出数组长度了，后面的就不处理了
				}
			}
		}
	}

	bool parse_tower_template(TowerTemplate& tower_template, cJSON* json_tower) {
		if(!json_tower || json_tower->type != cJSON_Object){
			cJSON_Delete(json_tower);
			return false;
		}

		cJSON* json_interval = cJSON_GetObjectItem(json_tower, "interval");
		cJSON* json_damage = cJSON_GetObjectItem(json_tower, "damage");
		cJSON* json_view_range = cJSON_GetObjectItem(json_tower, "view_range");
		cJSON* json_cost = cJSON_GetObjectItem(json_tower, "cost");
		cJSON* json_upgrade_cost = cJSON_GetObjectItem(json_tower, "upgrade_cost");

		parse_number_array(tower_template.interval, 10, json_interval);
		parse_number_array(tower_template.damage, 10, json_damage);
		parse_number_array(tower_template.view_range, 10, json_view_range);
		parse_number_array(tower_template.cost, 10, json_cost);
		parse_number_array(tower_template.upgrade_cost, 9, json_upgrade_cost);

		return true;
	}
```

其实就是将之前的加载更加模块化了

### 2026.5.14 游戏资源加载管理器

将所有的用到的游戏资源进行加载，并且进行存储（使用高效的哈希表的方式，这样可以更快的进行资源的查找）

请注意这个管理器的本质是一个资源池，存放所有需要的资源，为了方便首先对资源取别名进行列举

```cpp
enum class ResID
{	
	// Tex_开头的资源是纹理资源
	Tex_Tileset,
	...
	Tex_SlimeSketch, // Sketch剪影的作用是在敌人被攻击的时候显示一个被攻击的视觉效果，和原来的纹理资源是分开的
	...
	Tex_UIPlaceIdle,
	Tex_UIPlaceHoveredTop, // 这表示高亮上的状态，和Idle状态是分开的
	Tex_UIPlaceHoveredLeft,
	Tex_UIPlaceHoveredRight,
    
	// Sound_表示音效资源
	Sound_ArrowFire_1,
	...

	// Music_表示音乐资源（比如持续播放的背景音乐）
	Music_BGM,
	
	// Font_表示字体资源
	Font_Main
};
```

接下来在类中定义一些有意义的类，一遍存储不同类型的资源：

```cpp
public:
	typedef unordered_map<ResID, SDL_Texture*> TexturePool;
	typedef unordered_map<ResID, Mix_Chunk*> SoundPool;
	typedef unordered_map<ResID, Mix_Music*> MusicPool;
	typedef unordered_map<ResID, TTF_Font*> FontPool;

public:
	const TexturePool& get_texture_pool() const { return texture_pool; }
	const SoundPool& get_sound_pool() const { return sound_pool; }
	const MusicPool& get_music_pool() const { return music_pool; }
	const FontPool& get_font_pool() const { return font_pool; }
```

最后写上加载函数：

```cpp
public:
	bool load_resources(SDL_Renderer* renderer) {
		// texture:二合一函数（省了转换为SDL_Surface再转成SDL_Texture的步骤）
		// 直接加载纹理资源，路径是相对于可执行文件的路径
		texture_pool[ResID::Tex_Tileset] = IMG_LoadTexture(renderer, "resources/tileset.png");
		...
        // 哈希表的使用方式：类似于数组下标，只不过下标为去映射的类型
            
            
		for (auto pair : texture_pool) { // 检查资源是否加载失败
			if (!pair.second)
				return false;
		}

		// sound类音效使用Mix_LoadWAV函数加载，这样更便于混音
		sound_pool[ResID::Sound_ArrowFire_1] = Mix_LoadWAV("resources/sound_arrow_fire_1.mp3");
		...
		for (const auto& pair : sound_pool)
			if (!pair.second) return false;

		// music类用Mix_LoadMUS函数
		music_pool[ResID::Music_BGM] = Mix_LoadMUS("resources/music_bgm.mp3");

		for (const auto& pair : music_pool)
			if (!pair.second) return false;

		// ttf字体类用TTF_OpenFont函数，参数是字体文件路径和字体大小
		font_pool[ResID::Font_Main] = TTF_OpenFont("resources/ipix.ttf", 25);

		for (const auto& pair : font_pool)
			if (!pair.second) return false;

		return true;

	}
```