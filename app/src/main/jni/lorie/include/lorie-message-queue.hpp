#pragma once
#include <functional>
#include <queue>
#include <pthread.h>

class LorieMessageQueue {
public:
	LorieMessageQueue();
	void write(std::function<void()> func);
	
	template<typename Ret, typename Class, typename ... Args>
	void call(Ret Class::* __pm, Args&& ... args) {
		write(std::bind(std::mem_fn(__pm), args...));
	};
	void run();
	int get_fd();
private:
	int fd;
	pthread_mutex_t write_mutex;
	pthread_mutex_t read_mutex;
	std::queue<std::function<void()>> queue;
};

template<class Class, class T, class... Args>
struct LorieFuncWrapper {
    using TFn = T (Class::*)(Args...);
	LorieFuncWrapper(TFn fn, Class* cl, LorieMessageQueue& q): cl(cl), ptr(fn), q(q) {};
	
	Class* cl;
    TFn ptr;
	LorieMessageQueue& q;
    T operator()(Args... params) {
        q.call(ptr, cl, params...);
        //return (cl->*ptr)(params...);
    }
};

template<class R, class... A>
struct LorieFuncWrapperTypeHelper;
template<class R, class C, class... A>
struct LorieFuncWrapperTypeHelper<R(C::*)(A...)> {
  using type = LorieFuncWrapper<C, R, A...>;
};

template <class F>
using LorieFuncWrapperType = typename LorieFuncWrapperTypeHelper<F>::type;

template<typename Ret, typename Class, typename ... Args>
LorieFuncWrapper<Class, Ret, Args...> LorieFuncWrap(Ret(Class::*F)(Args...), Class* c) {
	return LorieFuncWrapper<Class, Ret, Args...>(F, c);
};
