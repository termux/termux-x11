#pragma once
#include <functional>
#include <queue>
#include <mutex>

class lorie_message_queue {
public:
	lorie_message_queue();
	void write(std::function<void()> func);

	void run();
	int get_fd();
private:
	int fd;
	std::mutex mutex;
	std::queue<std::function<void()>> queue;
};
