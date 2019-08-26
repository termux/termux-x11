#include <lorie-compositor.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/eventfd.h>

LorieMessageQueue::LorieMessageQueue() {	
	pthread_mutex_init(&write_mutex, nullptr);
	pthread_mutex_init(&read_mutex, nullptr);
	
	fd = eventfd(0, EFD_CLOEXEC);
	if (fd == -1) {
		LOGE("Failed to create socketpair for message queue: %s", strerror(errno));
		return;
	}
}

void LorieMessageQueue::write(std::function<void()> func) {
	static uint64_t i = 1;
	pthread_mutex_lock(&write_mutex);
	queue.push(func);
	pthread_mutex_unlock(&write_mutex);
	::write(fd, &i, sizeof(uint64_t));
}

void LorieMessageQueue::run() {
	static uint64_t i = 0;
	::read(fd, &i, sizeof(uint64_t));
	while(!queue.empty()){
		queue.front()();

		pthread_mutex_lock(&read_mutex);
		queue.pop();
		pthread_mutex_unlock(&read_mutex);
	}
}

int LorieMessageQueue::get_fd() {
	return fd;
}
