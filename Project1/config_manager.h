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
		string window_title = u8"村庄保卫战";
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

	SDL_Rect rect_tile_map = { 0 }; // 这个表示地图实际渲染在什么举行当中

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

	bool load_game_config(const string& path = "config.json") {
		ifstream file(path);
        if(!file.good()){
			cerr << "Failed to open config file: " << path << endl;
			return false;
		}

		stringstream buffer;
		buffer << file.rdbuf();
		file.close();

		cJSON* json_root = cJSON_Parse(buffer.str().c_str());
		if (!json_root) {
			cerr << "Failed to parse config file: " << path << endl;
			return false;
		}
		if(json_root->type != cJSON_Object){
			cerr << "Config file is not an object: " << path << endl;
			cJSON_Delete(json_root);
			return false;
		}


		// 同层级下一遍获取
		cJSON* json_basic = cJSON_GetObjectItem(json_root, "basic");
		cJSON* json_player = cJSON_GetObjectItem(json_root, "player");
		cJSON* json_tower = cJSON_GetObjectItem(json_root, "tower");
		cJSON* json_enemy = cJSON_GetObjectItem(json_root, "enemy");

		if(!json_basic || !json_player || !json_tower || !json_enemy
			|| json_basic->type != cJSON_Object || json_player->type != cJSON_Object
			|| json_tower->type != cJSON_Object || json_enemy->type != cJSON_Object){
			cerr << "Config file is missing required fields: " << path << endl;
			cJSON_Delete(json_root);
			return false;
		} // 同层级一遍检查是否正确

		parse_basic_template(basic_template, json_basic);
		
		parse_player_template(player_template, json_player);
		
		parse_tower_template(archer_template, cJSON_GetObjectItem(json_tower, "archer"));
		parse_tower_template(axeman_template, cJSON_GetObjectItem(json_tower, "axeman"));
		parse_tower_template(gunner_template, cJSON_GetObjectItem(json_tower, "gunner"));

		parse_enemy_template(slim_template, cJSON_GetObjectItem(json_enemy, "Slim"));
		parse_enemy_template(king_slim_template, cJSON_GetObjectItem(json_enemy, "KingSlim"));
		parse_enemy_template(skeleton_template, cJSON_GetObjectItem(json_enemy, "Skeleton"));	
		parse_enemy_template(goblin_template, cJSON_GetObjectItem(json_enemy, "Goblin"));
		parse_enemy_template(goblin_priest_template, cJSON_GetObjectItem(json_enemy, "GoblinPriest"));
	
		cJSON_Delete(json_root);
		return true;
	
	}


	bool load_level_config(const string& path = "level.json") {
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

protected:
	ConfigManager() = default;
	~ConfigManager() = default;
private:
	bool parse_basic_template(BasicTemplate& basic_template, cJSON* json_basic) {
		if(!json_basic || json_basic->type != cJSON_Object){
			cJSON_Delete(json_basic);
			return false;
		}

		cJSON* json_window_title = cJSON_GetObjectItem(json_basic, "window_title");
		cJSON* json_window_width = cJSON_GetObjectItem(json_basic, "window_width");
		cJSON* json_window_height = cJSON_GetObjectItem(json_basic, "window_height");
		
		if (json_window_title && json_window_title->type == cJSON_String) {
			basic_template.window_title = json_window_title->valuestring;
		}
		
		if (json_window_width && json_window_width->type == cJSON_Number) {
			basic_template.window_width = json_window_width->valueint;
		}
		
		if (json_window_height && json_window_height->type == cJSON_Number) {
			basic_template.window_height = json_window_height->valueint;
		}
	}

	bool parse_player_template(PlayerTemplate& player_template, cJSON* json_player) {
		if(!json_player || json_player->type != cJSON_Object){
			cJSON_Delete(json_player);
			return false;
		}

		cJSON* json_speed = cJSON_GetObjectItem(json_player, "speed");
		cJSON* json_normal_attack_interval = cJSON_GetObjectItem(json_player, "normal_attack_interval");
		cJSON* json_normal_attack_damage = cJSON_GetObjectItem(json_player, "normal_attack_damage");
		cJSON* json_skill_interval = cJSON_GetObjectItem(json_player, "skill_interval");
		cJSON* json_skill_damage = cJSON_GetObjectItem(json_player, "skill_damage");

		if (json_speed && json_speed->type == cJSON_Number) {
			player_template.speed = json_speed->valuedouble;
		}
		
		if (json_normal_attack_interval && json_normal_attack_interval->type == cJSON_Number) {
			player_template.normal_attack_interval = json_normal_attack_interval->valuedouble;
		}
		
		if (json_normal_attack_damage && json_normal_attack_damage->type == cJSON_Number) {
			player_template.normal_attack_damage = json_normal_attack_damage->valuedouble;
		}
		
		if (json_skill_interval && json_skill_interval->type == cJSON_Number) {
			player_template.skill_interval = json_skill_interval->valuedouble;
		}
		
		if (json_skill_damage && json_skill_damage->type == cJSON_Number) {
			player_template.skill_damage = json_skill_damage->valuedouble;
		}

		return true;
	}

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

	bool parse_enemy_template(EnemyTemplate& enemy_template, cJSON* json_enemy) {
		if(!json_enemy || json_enemy->type != cJSON_Object){
			cJSON_Delete(json_enemy);
			return false;
		}

		double recover_range = 0;
		double recover_intensity = 25;
		cJSON* json_hp = cJSON_GetObjectItem(json_enemy, "hp");
		cJSON* json_speed = cJSON_GetObjectItem(json_enemy, "speed");
		cJSON* json_damage = cJSON_GetObjectItem(json_enemy, "damage");
		cJSON* json_reward_ratio = cJSON_GetObjectItem(json_enemy, "reward_ratio");
		cJSON* json_recover_interval = cJSON_GetObjectItem(json_enemy, "recover_interval");
		cJSON* json_recover_range = cJSON_GetObjectItem(json_enemy, "recover_range");
		cJSON* json_recover_intensity = cJSON_GetObjectItem(json_enemy, "recover_intensity");
		if (json_hp && json_hp->type == cJSON_Number) {
			enemy_template.hp = json_hp->valuedouble;
		}
		if (json_speed && json_speed->type == cJSON_Number) {
			enemy_template.speed = json_speed->valuedouble;
		}
		if (json_damage && json_damage->type == cJSON_Number) {
			enemy_template.damage = json_damage->valuedouble;
		}
		if (json_reward_ratio && json_reward_ratio->type == cJSON_Number) {
			enemy_template.reward_ratio = json_reward_ratio->valuedouble;
		}
		if (json_recover_interval && json_recover_interval->type == cJSON_Number) {
			enemy_template.recover_interval = json_recover_interval->valuedouble;
		}
		if (json_recover_range && json_recover_range->type == cJSON_Number) {
			enemy_template.recover_range = json_recover_range->valuedouble;
		}
		if (json_recover_intensity && json_recover_intensity->type == cJSON_Number) {
			enemy_template.recover_intensity = json_recover_intensity->valuedouble;
		}

	}


};

#endif