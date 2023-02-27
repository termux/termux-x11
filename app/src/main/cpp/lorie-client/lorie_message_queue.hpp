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

static always_inline timespec operator+(const timespec& lhs, const timespec& rhs);
static always_inline timespec operator-(const timespec& lhs, const timespec& rhs);
static always_inline bool operator<(const timespec& lhs, const timespec& rhs);
static always_inline timespec now();
static always_inline timespec ms_to_timespec(long ms);
static always_inline long timespec_to_ms(timespec& spec);
static always_inline void timespec_to_human_readable(const timespec& ts, const char* comment);

class lorie_message_queue {
public:
	ALooper* looper{};

    int efd;

	lorie_message_queue() {
        efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (efd == -1) {
            ALOGE("Failed to create socketpair for message queue: %s", strerror(errno));
            return;
        }

		timer = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
		if (timer < 0) {
			ALOGE("Failed to create timerfd: %s", strerror(errno));
			return;
		}
	}

	void post(std::function<void()> func, long ms_delay = 0) {
		std::unique_lock<std::mutex> lock(mutex);
		if (ms_delay <= 0) {
			queue.push(std::move(func));
            ::write(efd, (const uint64_t[]) {1}, sizeof(uint64_t));
		} else {
			delayed_queue.emplace(task_t{ now() + ms_to_timespec(ms_delay), std::move(func) });
			reset_timer();
		}
	};

	void run() {
		std::function<void()> fn;
		std::unique_lock<std::mutex> lock(mutex);
        ::read(efd, (void*) (const uint64_t[]) {1}, sizeof(uint64_t));
		while(!queue.empty()){
			fn = queue.front();
			queue.pop();

			lock.unlock();
			fn();
			lock.lock();
		}
	};
	void dispatch_delayed() {
		std::unique_lock<std::mutex> lock(mutex);
		while (!delayed_queue.empty() && delayed_queue.top().expiration < now()) {
			auto fn = delayed_queue.top().f;
			delayed_queue.pop();

			lock.unlock();
			fn();
			lock.lock();
		}

		reset_timer();
	}

	int timer;
private:

	struct task_t {
		timespec expiration;
		std::function<void()> f;

		bool operator<(const task_t& rhs) const {
			return rhs.expiration < expiration;
		}
	};

	void reset_timer() {
		if (delayed_queue.empty()) {
			timerfd_settime(timer, 0, &zero, nullptr);
			return;
		}

		itimerspec t = { {}, delayed_queue.top().expiration };
		if (timerfd_settime(timer, TFD_TIMER_ABSTIME, &t, nullptr) < 0)
			ALOGE("Failed to set timerfd: %s", strerror(errno));
	}

	std::mutex mutex;
	std::queue<std::function<void()>> queue;
	std::priority_queue<task_t> delayed_queue;

	static constexpr itimerspec zero = {{0, 0}, {0, 0}};
};

static always_inline timespec operator+(const timespec& lhs, const timespec& rhs) {
	timespec result{};
	result.tv_sec = lhs.tv_sec + rhs.tv_sec;
	result.tv_nsec = lhs.tv_nsec + rhs.tv_nsec;
	if (result.tv_nsec >= 1000000000L) {
		++result.tv_sec;
		result.tv_nsec -= 1000000000L;
	}
	return result;
}

static always_inline timespec operator-(const timespec& lhs, const timespec& rhs) {
	timespec result{};
	result.tv_sec = lhs.tv_sec - rhs.tv_sec;
	result.tv_nsec = lhs.tv_nsec - rhs.tv_nsec;
	if (result.tv_nsec < 0) {
		--result.tv_sec;
		result.tv_nsec += 1000000000L;
	}
	return result;
}

static always_inline bool operator<(const timespec& lhs, const timespec& rhs) {
	if (lhs.tv_sec < rhs.tv_sec) {
		return true;
	}
	if (lhs.tv_sec > rhs.tv_sec) {
		return false;
	}
	return lhs.tv_nsec < rhs.tv_nsec;
}

static always_inline timespec now() {
	timespec now; // NOLINT(cppcoreguidelines-pro-type-member-init)
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now;
}

static always_inline timespec ms_to_timespec(long ms) {
	return timespec { ms / 1000, (ms % 1000) * 1000000 };
}

static always_inline long timespec_to_ms(timespec spec) {
	return spec.tv_sec * 1000 + spec.tv_nsec / 1000000 ;
}

static always_inline void timespec_to_human_readable(const timespec& ts, const char* comment) {
	time_t sec = ts.tv_sec;
	tm timeinfo{};
	gmtime_r(&sec, &timeinfo);
	ALOGE("%-10s %02d:%02d:%02d.%03ld", comment, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, ts.tv_nsec / 1000000);
}
