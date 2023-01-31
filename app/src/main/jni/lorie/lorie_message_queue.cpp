#include <lorie_compositor.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <sys/eventfd.h>
#include <mutex>

lorie_message_queue::lorie_message_queue() {
	std::unique_lock<std::mutex> lock(mutex);

	fd = eventfd(0, EFD_CLOEXEC);
	if (fd == -1) {
		LOGE("Failed to create socketpair for message queue: %s", strerror(errno));
		return;
	}
}

void lorie_message_queue::write(std::function<void()> func) {
	static uint64_t i = 1;
	std::unique_lock<std::mutex> lock(mutex);
	queue.push(func);
	::write(fd, &i, sizeof(uint64_t));
}

void lorie_message_queue::run() {
	static uint64_t i = 0;
	std::unique_lock<std::mutex> lock(mutex);
	std::function<void()> fn;
	::read(fd, &i, sizeof(uint64_t));
	while(!queue.empty()){
		fn = queue.front();
		queue.pop();

		lock.unlock();
		fn();
		lock.lock();
	}
}

int lorie_message_queue::get_fd() {
	return fd;
}
