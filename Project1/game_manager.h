#ifndef _GAME_MANAGER_H_
#define _GAME_MANEGER_H_

#include "manager.h"
#include "config_manager.h"
#include "resources_manager.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
class GameManager : public Manager<GameManager> {
	friend Manager<GameManager>;
public:

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
			if (delta * 1000.0 < 1000.0 / 60.0) {
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
protected:

	GameManager() { // 构造函数需要初始化库
	
		/*  初始化与初始化断言   */ 

		init_assert(!SDL_Init(SDL_INIT_EVERYTHING), u8"SDL库启动失败");			// SDL: 0 表示成功，负数表示失败
		init_assert(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG), u8"IMG库启动失败");	// IMG: 返回成功初始话的标志，若失败则返回0
		init_assert(Mix_Init(MIX_INIT_MP3), u8"mixer库启动失败");					// mixer: 返回成功初始话的标志，若失败则返回0
		init_assert(!TTF_Init(), u8"字体库ttf启动失败");

		Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);						// 打开音频设备，参数分别是：频率、格式、声道数、缓冲区大小

		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");  // 开启hint， 注意第二个参数是字符串
	

		ConfigManager* instance = ConfigManager::Instance();

		init_assert(instance->map.load(), u8"地图加载失败");
		init_assert(instance->load_game_config(), u8"游戏配置加载失败");
		init_assert(instance->load_level_config(), u8"关卡配置加载失败");


		// 创建游戏窗口
		window = SDL_CreateWindow(ConfigManager::Instance()->basic_template.window_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
			ConfigManager::Instance()->basic_template.window_width, ConfigManager::Instance()->basic_template.window_height, SDL_WINDOW_SHOWN);
		init_assert(window, u8"游戏窗口创建失败");

		// 创建渲染器
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
		// 参数分别是：窗口、索引（-1表示使用第一个支持的渲染器）、标志（加速、垂直同步、支持渲染到纹理）
		init_assert(renderer, u8"渲染器创建失败");
		

		ResourcesManager* resources_manager = ResourcesManager::Instance();
		init_assert(resources_manager->load_resources(renderer), u8"游戏资源加载失败");

		init_assert(generate_tile_map_texture(), u8"地图纹理生成失败");
	}
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

private:
	SDL_Event event; // 事件对象，用于处理用户输入和其他事件
	bool is_quit = false;


	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	SDL_Texture* texture_tile_map = nullptr; // 地图纹理，生成一次后就可以一直用，除非地图发生改变了
private:
	void init_assert(bool is_ok, const char* err_msg) {
		// 初始化断言，这个断言与编译时断言不同，是一个运行时断言
		// @param bool is_ok 表示是否成功
		// @param const char* err_msg 字符串表示需要汇报的错误信息
		// void

		if (is_ok) return; // 正常创建，无需创建窗口

		else {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, // FLAG表示类型：error, information, warning
				u8"初始化失败",
				err_msg,
				window);				  // 父窗口，表示生成的messagebox应该层级在这个窗口之上，子窗口关闭父窗口才会运行
													
		}
	}

	void on_input() { // 不用传参数，可以直接获得manager内的 event参数
		// 获取事件等的输入处理

		if (event.type == SDL_QUIT) {
			is_quit = true;
		}

	}

	void on_update(double delta) {
		// 数据更新逻辑，一半要将时间传进去以保证逻辑合理性

	}

	void on_render() {
		const SDL_Rect& rect_tile_map = ConfigManager::Instance()->rect_tile_map;
		SDL_RenderCopy(renderer, texture_tile_map, nullptr, &rect_tile_map);
		
	}

	bool generate_tile_map_texture() {	// renderer是一个画笔，为了不影响主游戏界面的渲染，我们先用这个画笔在一个纹理执行一些复杂逻辑画出地图并保存在纹理中
										// 注意这一步只画出地图的纹理，后续还需要按照layer关系画出塔、敌人等元素
										// 之后地图渲染就不用复杂逻辑了，直接渲染这一张纹理即可
		const TileMap& tile_map = ConfigManager::Instance()->map.get_tile_map();
		
		SDL_Rect& map_rect = ConfigManager::Instance()->rect_tile_map;	// 实际要渲染到画布上时的渲染矩形
																		// 待计算
		SDL_Texture* tex_tile_map_set = ResourcesManager::Instance()->get_texture_pool().find(ResID::Tex_Tileset)->second;
		// 注意这里的逻辑，为何不用ResourcesManager::Instance()->get_texture_pool()[ResID::Tex_Tileset]
		// 因为上面注释掉的这个式子不是静态的，也就是如果说找不到键值为ResID::Tex_Tileset的对会创建一个键值对，所以用find函数找到了指向所对应的键值对的指针
		int width_map_set, height_map_set;
		SDL_QueryTexture(tex_tile_map_set, nullptr, nullptr, &width_map_set, &height_map_set);//用于加载纹理并且获得其宽高
		int set_one_line_number = (int)ceil(double(width_map_set)/SIZE_TILE); // 纹理图片中一行有多少个样式的纹理

		int width_map_tex, height_map_tex;
		width_map_tex = SIZE_TILE *  ConfigManager::Instance()->map.get_width();
		height_map_tex = SIZE_TILE * ConfigManager::Instance()->map.get_height();

		int x_map_tex = (ConfigManager::Instance()->basic_template.window_width - width_map_tex) / 2;
		int y_map_tex = (ConfigManager::Instance()->basic_template.window_height - height_map_tex) / 2;
		

		map_rect.x = x_map_tex;
		map_rect.y = y_map_tex;
		map_rect.w = width_map_tex;
		map_rect.h = height_map_tex; // 实际地图在画布上的渲染矩形更新完毕

		cout << "SIZE_TILE" << SIZE_TILE;
		cout << "width:" << ConfigManager::Instance()->map.get_width();
		cout << map_rect.x << ' ' << map_rect.y << ' ' << map_rect.w << ' ' << map_rect.h;

		// 接下来就专心生成地图纹理到texture_tile_map中

		// 首先创建这样一个SDL_Texture，并且将模式设为混合（方便进行不同层级之间的叠加）
		texture_tile_map = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
			SDL_TEXTUREACCESS_TARGET, width_map_tex, height_map_tex);
			// SDL_PIXELFORMAT_ARGB8888 ARGB:透明度红绿蓝，8888：每个各8byte（4字节ARGB型颜色）
		SDL_SetTextureBlendMode(texture_tile_map, SDL_BLENDMODE_BLEND); // 这一步是将模式设置为混合：alpha混合模式
		
		// 将画笔renderer的渲染对象替换为地图纹理（记得最后还原）
		SDL_SetRenderTarget(renderer, texture_tile_map);
		
		for (int y = 0; y < ConfigManager::Instance()->map.get_height(); y++) {
			  
			for (int x = 0; x < ConfigManager::Instance()->map.get_width();x++) {
				
				const Tile& tile_now = tile_map[y][x];

				int terrian = tile_now.terrian;

				const SDL_Rect& rect_src = {
					(terrian % set_one_line_number) * SIZE_TILE,
					(terrian / set_one_line_number) * SIZE_TILE,
					SIZE_TILE,
					SIZE_TILE
				}; // 对应纹理在纹理图中的位置，并进行截取

				const SDL_Rect& rect_dst = {
					x * SIZE_TILE,
					y * SIZE_TILE,
					SIZE_TILE,
					SIZE_TILE
				}; // 生成纹理在地图中的位置

				// SDL_SetRenderTarget(renderer, texture_tile_map); 已经指明了画笔着色的目标
				SDL_RenderCopy(renderer, tex_tile_map_set, &rect_src, &rect_dst); // 这里是决定将哪个地方的纹理进行复制


				if (tile_now.decoration >= 0) {
					
					int decoration = tile_now.decoration;
					const SDL_Rect& rect_src = {
						(decoration % set_one_line_number) * SIZE_TILE,
						(decoration / set_one_line_number) * SIZE_TILE,
						SIZE_TILE,
						SIZE_TILE
					}; // 对应纹理在纹理图中的位置，并进行截取
					// SDL_SetRenderTarget(renderer, texture_tile_map); 已经指明了画笔着色的目标
					SDL_RenderCopy(renderer, tex_tile_map_set, &rect_src, &rect_dst); // 这里是决定将哪个地方的纹理进行复制
				}
			}	// end internal for
		} // end double for

		
		const SDL_Point& home_pos = ConfigManager::Instance()->map.get_idx_home();
		const SDL_Rect& rect_dst = {
			home_pos.x * SIZE_TILE,
			home_pos.y * SIZE_TILE,
			SIZE_TILE,
			SIZE_TILE
		}; // 生成纹理在地图中的位置
		SDL_RenderCopy(renderer, ResourcesManager::Instance()->get_texture_pool().find(ResID::Tex_Home)->second, nullptr, &rect_dst); // 这里是决定将哪个地方的纹理进行复制
		// nullptr 表示不用裁剪

		SDL_SetRenderTarget(renderer, nullptr); // null 表示现在在window上进行着色

		return true;
	}

};

#endif
