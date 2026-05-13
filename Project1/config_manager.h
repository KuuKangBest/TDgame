#ifndef _CONFIG_MANAGER_H_
#define _CONFIG_MANAGER_H_
 // 一个全局的配置中心
#include"manager.h"

#include"wave.h"

#include"map.h"
#include<SDL.h>
#include<string>
#include<fstream>
#include<sstream>
#include<cJSON.h> // 从json文件中读取配置
#include<iostream>

using namespace std;

class ConfigManager :public Manager<ConfigManager> {
	friend class Manager<ConfigManager>;


public:
	struct BasicTemplate { // 默认值是为了在解析失败的时候能有默认值
		string widow_title = u8"村庄保卫战";
		int window_width = 1280;
		int window_height = 720;
	};

	struct PlayerTemplate { // 这些设置与游戏玩法高度相关，可以随时修改
		double speed = 3;
		double normal_attack_interval = 0.5;
		double normal_attack_damage = 0;
		double skill_interval = 10;
		double skill_damage = 1;
	};

	struct TowerTemplate {
		double interval[10] = { 1 };
		double damage[10] = { 25 };
		double view_range[10] = { 5 };
		double cost[10] = { 50 };
		double upgrade_cost[9] = { 75 };
	};

	struct EnemyTemplate {
		double hp = 100;
		double speed = 1;
		double damage = 1;
		double reward_ratio = 0.5;
		double recover_interval = 10;
		double recover_range = 0;
		double recover_intensity = 25;
	};
public:
	Map map;
	vector<Wave> wave_list;

	int level_archer = 0;
	int level_axeman = 0;
	int level_gunner = 0;

	bool is_game_win = true;
	bool is_game_over = false;

	SDL_Rect rect_tile_map = { 0 };

	BasicTemplate basic_template;
	PlayerTemplate player_template;
	TowerTemplate archer_template;
	TowerTemplate axeman_template;
	TowerTemplate gunner_template;

	EnemyTemplate slim_template;
	EnemyTemplate king_slim_template;
	EnemyTemplate skeleton_template;
	EnemyTemplate goblin_template;
	EnemyTemplate goblin_priest_template;


	const double num_initial_hp = 10;
	const double num_initial_coin = 100;
	const double num_coin_per_prop = 10;

public:

	bool load_game_config(const string& path) {

	}

	bool load_level_config(const string& path) {
		ifstream file(path);


		// 作必要的检查，检查cJSON文件是否正常
		if (!file.good()) {
			cerr << "Failed to open config file: " << path << endl;
			return false;
		}
			
		stringstream buffer;
		buffer << file.rdbuf(); // read buffer
		file.close();			// 读到内存中就后就可以关闭文件了，养成好习惯

		cJSON* json_root = cJSON_Parse(buffer.str().c_str()); // 解析json
        
		if (!json_root) {
			cerr << "Failed to parse config file: " << path << endl;
			return false;
		} 
		if (json_root->type != cJSON_Array)
		{
			cerr << "Config file is not an array: " << path << endl;
			cJSON_Delete(json_root); // 不合法就直接删掉
			return false;
		}


		// 开始解析每一个数组中的每一个结点，每一个结点都是一个wave

		cJSON* json_wave = nullptr;
		cJSON_ArrayForEach(json_wave, json_root) {
			// json_wave 的类型应该是一个Object
			if (json_wave->type != cJSON_Object) {
				cout << "Invalid wave config, expected an object." << endl;
				continue; // 跳过这个结点
			}

			wave_list.emplace_back();
			Wave& wave = wave_list.back();

			cJSON* json_wave_interval = cJSON_GetObjectItem(json_wave, "interval");
			if (json_wave_interval && json_wave_interval->type == cJSON_Number) {
				wave.interval = json_wave_interval->valuedouble;
			}

			cJSON* json_wave_rewards = cJSON_GetObjectItem(json_wave, "rewards");
			if (json_wave_rewards && json_wave_rewards->type == cJSON_Number) {
				wave.rewards = json_wave_rewards->valuedouble;
			}

			cJSON* json_wave_spawn_events = cJSON_GetObjectItem(json_wave, "spawn_list");
			if (json_wave_spawn_events && json_wave_spawn_events->type == cJSON_Array) {
				cJSON* json_spawn_event = nullptr;
				cJSON_ArrayForEach(json_spawn_event, json_wave_spawn_events) {
					if (json_spawn_event->type != cJSON_Object) {
						cout << "Invalid spawn event config, expected an object." << endl;
						continue; // 跳过这个结点
					}
					Wave::SpawnEvent spawn_event;
					cJSON* json_spawn_interval = cJSON_GetObjectItem(json_spawn_event, "interval");
					if (json_spawn_interval && json_spawn_interval->type == cJSON_Number) {
						spawn_event.interval = json_spawn_interval->valuedouble;
					}// 假设没找到或者类型不对，就以默认值为准，不需要特别处理


					cJSON* json_spawn_point = cJSON_GetObjectItem(json_spawn_event, "spawn_point");
					if (json_spawn_point && json_spawn_point->type == cJSON_Number) {
						spawn_event.spawn_point = json_spawn_point->valueint;
					}// 同上，没找到或者类型不对就以默认值为准，不需要特别处理


					cJSON* json_enemy_type = cJSON_GetObjectItem(json_spawn_event, "enemy_type");
					if (json_enemy_type && json_enemy_type->type == cJSON_String) {
						string enemy_type_str = json_enemy_type->valuestring;
						if (enemy_type_str == "Slim") {
							spawn_event.enemy_type = EnemyType::Slim;
						}
						else if (enemy_type_str == "KingSlim") {
							spawn_event.enemy_type = EnemyType::KingSlim;
						}
						else if (enemy_type_str == "Skeleton") {
							spawn_event.enemy_type = EnemyType::Skeleton;
						}
						else if (enemy_type_str == "Goblin") {
							spawn_event.enemy_type = EnemyType::Goblin;
						}
						else if (enemy_type_str == "GoblinPriest") {
							spawn_event.enemy_type = EnemyType::GoblinPriest;
						}
						// 都不是则以默认值Slim为准，不需要特别处理
					}
					wave.spawn_event_list.push_back(spawn_event);
				} // for each spawn event

				if(wave.spawn_event_list.empty())
					wave_list.pop_back(); // 如果这个wave没有生成事件了，那就没什么意义了，直接删掉
				// 之前读取到的interval之类的设置就没什么意义了，删掉也不心疼
			}

			cJSON_Delete(json_root);

			if (wave_list.empty())
			{
				cout << "Warning: No valid wave config found in file: " << path << endl;
				return false;
			}
			return true;
		};
		


	}

public:
	ConfigManager() = default;
	~ConfigManager() = default;


};

#endif