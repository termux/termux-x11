#pragma once
#include <iostream>
#include <functional>
#include <cstring>
#include <cerrno>
#include <queue>
#include <mutex>
#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/poll.h>
#include <sys/epoll.h>

static inline timespec operator+(const timespec& lhs, const timespec& rhs) {
	timespec result{};
	result.tv_sec = lhs.tv_sec + rhs.tv_sec;
	result.tv_nsec = lhs.tv_nsec + rhs.tv_nsec;
	if (result.tv_nsec >= 1000000000L) {
		++result.tv_sec;
		result.tv_nsec -= 1000000000L;
	}
	return result;
}

static inline timespec operator-(const timespec& lhs, const timespec& rhs) {
	timespec result{};
	result.tv_sec = lhs.tv_sec - rhs.tv_sec;
	result.tv_nsec = lhs.tv_nsec - rhs.tv_nsec;
	if (result.tv_nsec < 0) {
		--result.tv_sec;
		result.tv_nsec += 1000000000L;
	}
	return result;
}

static inline bool operator<(const timespec& lhs, const timespec& rhs) {
	if (lhs.tv_sec < rhs.tv_sec) {
		return true;
	}
	if (lhs.tv_sec > rhs.tv_sec) {
		return false;
	}
	return lhs.tv_nsec < rhs.tv_nsec;
}

static inline timespec now() {
	timespec now; // NOLINT(cppcoreguidelines-pro-type-member-init)
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now;
}

static inline timespec ms_to_timespec(long ms) {
	return timespec { ms / 1000, (ms % 1000) * 1000000 };
}

void timespec_to_human_readable(const timespec& ts, const char* comment) {
	time_t sec = ts.tv_sec;
	tm timeinfo{};
	gmtime_r(&sec, &timeinfo);
	ALOGE("%-10s %02d:%02d:%02d.%03ld", comment, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, ts.tv_nsec / 1000000);
}

class lorie_message_queue {
public:
	lorie_message_queue() {
		timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
		if (timer_fd < 0) {
			ALOGE("Failed to create timerfd: %s", strerror(errno));
		}

		fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
		if (fd < 0) {
			ALOGE("Failed to create eventfd: %s", strerror(errno));
		}
		// For some reason epoll_fd does not dispatch timerfd's events...
		std::thread([=] {
			struct pollfd pfd = { .fd = timer_fd, .events = POLLIN };
			while (poll(&pfd, 1, 1000) >= 0) {
				if (pfd.revents & POLLIN) {
					post([=] { dispatch_delayed(); });
				}
			};
		}).detach();
	}
	~lorie_message_queue() { close(timer_fd); close(fd); }

	void post(std::function<void()> func, long ms_delay = 0) {
		if (!ms_delay) {
			static uint64_t i = 1;
			std::unique_lock<std::mutex> lock(mutex);
			queue.push(std::move(func));
			::write(fd, &i, sizeof(uint64_t));
		} else {
			delayed_queue.emplace(task_t{ now() + ms_to_timespec(ms_delay), std::move(func) });
			reset_timer();
		}
	}

	void run() {
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

	void dispatch_delayed() {
		while (!delayed_queue.empty() && delayed_queue.top().expiration < now()) {
			auto f = delayed_queue.top().f;
			delayed_queue.pop();
			f();
		}

		reset_timer();
	}

	int get_fd() {
		return fd;
	}
private:
	int fd;
	std::mutex mutex;
	std::queue<std::function<void()>> queue;
private:
	void reset_timer() {
		if (delayed_queue.empty())
			return;

		timespec planned = (delayed_queue.top().expiration - now() < ms_to_timespec(0))
						   ? ms_to_timespec(20)
						   : delayed_queue.top().expiration - now();
		itimerspec t = { {}, planned };
		if (timerfd_settime(timer_fd, 0, &t, nullptr) < 0)
			ALOGE("Failed to set timerfd: %s", strerror(errno));
	}

	struct task_t {
		timespec expiration;
		std::function<void()> f;

		bool operator<(const task_t& rhs) const {
			return rhs.expiration < expiration;
		}
	};

	int timer_fd;
	std::priority_queue<task_t> delayed_queue;
};
