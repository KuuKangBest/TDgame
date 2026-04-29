#ifndef _MANAGER_H_
#define _MANAGER_H_

template
<typename T>
class Manager {

public:
	static T* Instance() { // instance是静态方法
		if (!instance)
			instance = new T(); // 注意根本逻辑是要去子类的构造函数创建实例，所以这里需要用到友元技术，否则父类将无法访问到子类的protected构造函数

		return instance;
	}


private:
	static T* instance;

protected: // 保护的含义是子类可以访问但是外部不能访问
	       // 外部访问这些函数将会失效
	Manager() = default;
	~Manager() = default;

	Manager(const Manager&) = delete; // 拷贝函数定义为delete，禁止拷贝，删除掉
	Manager& operator=(const Manager&) = delete;
};

template <typename T>
T* Manager<T>::instance = nullptr;

#endif
