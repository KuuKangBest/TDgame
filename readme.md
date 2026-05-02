# 学习日记（个人能看懂）

## 目录

- [程序库总览和游戏框架搭建](#程序库总览和游戏框架搭建)
  - [2026.4.20 之前 SDL相关库、JSON与CSV格式解析](#2026420之前-sdl相关库json与csv格式解析)
  - [2026.4.20 可继承单例模板类实践](#2026420-可继承单例模板类实践)
  - [2026.4.29 GameManager的主体实现框架](#2026429-gamemanager的主体实现框架)
    - [初始化与初始化断言](#初始化与初始化断言)
    - [游戏运行时](#游戏运行时)
    - [游戏退出逻辑](#游戏退出逻辑)
  - [2026.5.1 配置数据的设计思路](#202651-配置数据的设计思路)
  - [2026.5.2 瓦片地图数据功能实现](#202652-瓦片地图数据功能实现)





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