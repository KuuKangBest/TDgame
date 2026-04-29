#define SDL_MAIN_HANDLED // 否则SDL会定义main函数，导致链接错误
						 // 这样写之后才可能让这个程序中的main函数成为真正SDL的main函数
#include<iostream>
#include "game_manager.h"

int main(int argc, char ** argv) {

	return GameManager::Instance()->run(argc, argv);
}