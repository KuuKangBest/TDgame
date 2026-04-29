#ifndef _GAME_MANAGER_H_
#define _GAME_MANEGER_H_

#include "manager.h"
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
	
		// 创建游戏窗口
		window = SDL_CreateWindow(u8"塔防游戏", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1080, 720, SDL_WINDOW_SHOWN);
		init_assert(window, u8"游戏窗口创建失败");

		// 创建渲染器
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
		// 参数分别是：窗口、索引（-1表示使用第一个支持的渲染器）、标志（加速、垂直同步、支持渲染到纹理）
		init_assert(renderer, u8"渲染器创建失败");
		
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

	}

	void on_update(double delta) {
		// 数据更新逻辑，一半要将时间传进去以保证逻辑合理性

	}

	void on_render() {

	}
};

#endif
