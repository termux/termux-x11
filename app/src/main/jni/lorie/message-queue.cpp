#include <lorie-compositor.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

LorieMessageQueue::LorieMessageQueue() {	
	pthread_mutex_init(&write_mutex, nullptr);
	pthread_mutex_init(&read_mutex, nullptr);
	
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fds)) {
		LOGE("Failed to create socketpair for message queue");
		return;
	}
	
	int flags = fcntl(fds[1], F_GETFL, 0);
	fcntl(fds[1], F_SETFL, flags | O_NONBLOCK);
}

void LorieMessageQueue::write(std::function<void()> func) {
	static uint8_t i = 0;
	pthread_mutex_lock(&write_mutex);
	queue.push(func);
	::write(fds[0], &i, sizeof(uint8_t));
	pthread_mutex_unlock(&write_mutex);
}

void LorieMessageQueue::run() {
	pthread_mutex_lock(&read_mutex);
	static uint8_t i = 0;
	while(::read(fds[1], &i, sizeof(uint8_t))>=1);
	while(!queue.empty()){
		queue.front()();
		queue.pop();
	}
	pthread_mutex_unlock(&read_mutex);
}

int LorieMessageQueue::get_fd() {
	return fds[1];
}
