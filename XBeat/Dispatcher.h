#pragma once

#include <thread>
#include <vector>
#include <deque>
#include <functional>
#include <mutex>
#include <condition_variable>

class Dispatcher
{
public:
	Dispatcher();
	~Dispatcher();

	void Initialize();
	void Shutdown();

	void AddTask(std::function<void(void)> task);

private:
	std::vector<std::thread> threadPool;
	bool run;

	std::deque<std::function<void(void)>> tasks;
	std::mutex sync;
	std::condition_variable condition;
	
	void consumer();
};

