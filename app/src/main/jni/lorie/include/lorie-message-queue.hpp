#pragma once
#include <functional>
#include <queue>
#include <mutex>

class LorieMessageQueue {
public:
	LorieMessageQueue();
	void write(std::function<void()> func);

	void run();
	int get_fd();
private:
	int fd;
	std::mutex mutex;
	std::queue<std::function<void()>> queue;
};
