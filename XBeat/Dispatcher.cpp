#include "Dispatcher.h"


Dispatcher::Dispatcher()
:sync()
{
}


Dispatcher::~Dispatcher()
{
}


void Dispatcher::Initialize()
{
	if (run)
		return;

	run = true;
	auto threadCount = std::thread::hardware_concurrency();
	threadPool.resize(threadCount);

	for (size_t i = 0; i < threadPool.size(); i++) {
		threadPool[i] = std::thread(std::bind(&Dispatcher::consumer, this));
	}
}

void Dispatcher::Shutdown()
{
	if (run) {
		run = false;

		condition.notify_all();

		for (auto &thread : threadPool)
			thread.join();
	}
}

void Dispatcher::AddTask(std::function<void(void)> task)
{
	std::lock_guard<std::mutex> lock(this->sync);

	tasks.push_back(task);

	condition.notify_one();
}

void Dispatcher::consumer()
{
	while (run) {
		std::unique_lock<std::mutex> lock(this->sync);
		while (tasks.empty() && run) condition.wait(lock);

		if (!run) break;

		auto task = tasks.front();
		tasks.pop_front();
		lock.unlock();

		task();
	}
}