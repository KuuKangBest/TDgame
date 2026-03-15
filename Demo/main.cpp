#define SDL_MAIN_HANDLED

#include <iostream>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL2_gfxPrimitives.h>


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <cJSON.h>

void test_json() {
	std::ifstream file("test.json"); // input file stream
	
	if (!file.good()){
		std::cout << "Failed to open file: test.json" << std::endl;
		return;
	}

	std::stringstream str_stream;
	str_stream << file.rdbuf(); // read the file into a stringstream  // 注意时一口气读入整个文件，而不是一行一行地读入
	file.close();

	cJSON* json_root = cJSON_Parse(str_stream.str().c_str());
	// parse the JSON string

	cJSON* json_name = cJSON_GetObjectItem(json_root, "name");
	cJSON* json_age = cJSON_GetObjectItem(json_root, "Age");
	cJSON* json_pets = cJSON_GetObjectItem(json_root, "pets");

	std::cout << json_name->string << ":" << json_name->valuestring << std::endl;
	std::cout << json_age->string << ":" << json_age->valueint << std::endl;
	// string 表示键的名称， valuestring, valueint表示具体的实例

	std::cout << json_pets->string << ":" << std::endl;
	cJSON* json_item = nullptr;
	cJSON_ArrayForEach(json_item, json_pets) {
		std::cout << "\t" << json_item->valuestring << std::endl;
	}

}

void test_csv() {
	std::ifstream file("test.csv");
	if (!file.good()) {
		std::cout << "Failed to open file: test.csv" << std::endl;
		return;
	}

	std::string str_line;
	while (std::getline(file, str_line)) {
		std::string str_grid;
		std::stringstream str_stream(str_line);
		while (std::getline(str_stream, str_grid, ',')) {
			std::cout << str_grid << " ";
		}
		std::cout << std::endl;
	}

	file.close();
}

int main() {

	test_json();
	test_csv();

	// Initialize SDL, SDL_image, SDL_mixer, and SDL_ttf
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
	Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3);
	TTF_Init();

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048); // 44100 is the frequency, MIX_DEFAULT_FORMAT is the format, 2 is the number of channels (stereo), and 2048 is the chunk size.
	
	// Create a window and renderer
	SDL_Window* window = SDL_CreateWindow(u8"你好世界", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);


	SDL_Surface* suf_image = IMG_Load("avatar.jpg");
	SDL_Texture* tex_image = SDL_CreateTextureFromSurface(renderer, suf_image);
	// 读取图片上传到GPU，生成纹理对象
	
	// 加载字体，规定颜色，渲染文本，生成纹理对象
	TTF_Font* font = TTF_OpenFont("ipix.ttf", 32);
	SDL_Color color = { 255, 255, 255, 255 };
	SDL_Surface* suf_text = TTF_RenderUTF8_Blended(font, u8"你好世界", color);
	SDL_Texture* tex_text = SDL_CreateTextureFromSurface(renderer, suf_text);

	// 加载音乐
	Mix_Music* music = Mix_LoadMUS("music.mp3");
	Mix_FadeInMusic(music, -1, 1500); // -1表示循环播放，1500表示淡入时间为1500毫秒


	SDL_Event event;
	SDL_Point pos_cursor = { 0, 0 };
	SDL_Rect rect_image, rect_text;

	rect_image.w = suf_image->w;
	rect_image.h = suf_image->h;

	rect_text.w = suf_text->w;
	rect_text.h = suf_text->h;


	int fps = 60;
	Uint64 last_counter = SDL_GetPerformanceCounter();
	Uint64 counter_freq = SDL_GetPerformanceFrequency();

	bool is_quit = false;
	while (!is_quit) {
		while (SDL_PollEvent(&event)) {
			
			if (event.type == SDL_QUIT) {
				is_quit = true;
			}
			else if (event.type == SDL_MOUSEMOTION) {
				pos_cursor.x = event.motion.x;
				pos_cursor.y = event.motion.y;
			}

		}

		// 时间计算（目前实现帧率控制）		
		Uint64 current_counter = SDL_GetPerformanceCounter();
		double delta = (double)(current_counter - last_counter) / counter_freq;
		last_counter = current_counter;

		if (delta * 1000 < 1000.0 / 60)
			SDL_Delay((Uint32)(1000.0 / 60 - delta * 1000));
		
		// 处理数据
		rect_image.x = pos_cursor.x;
		rect_image.y = pos_cursor.y;

		rect_text.x = pos_cursor.x;
		rect_text.y = pos_cursor.y;


		// 渲染绘图
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		SDL_RenderCopy(renderer, tex_image, nullptr, &rect_image);// srcrect为nullptr，不用截取
		filledCircleRGBA(renderer, pos_cursor.x, pos_cursor.y, 50, 255, 0, 0, 125);
		SDL_RenderCopy(renderer, tex_text, nullptr, &rect_text);
		
		// 将渲染器的内容呈现到屏幕上
		SDL_RenderPresent(renderer);
	}

	return 0;
}