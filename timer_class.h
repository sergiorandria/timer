#include <bits/stdc++.h>
#include <ios>
#include <thread> 

using namespace std;

namespace Time
{

#if __cplusplus <= 2020803L 
#define _CONSTEXPR20 constexpr 
#else 
#define _CONSTEXPR20 
#endif

#define __TIMER_DECL 
#define __BEGIN_TIMER_DECL 
#define __END_TIMER_DECL 

#define __TIMER_DEF_BEGIN 
#define __TIMER_DEF_END 

#define _Abstract 
#define _Abstract_class 
#define _Abstract_method 
#define _Abstract_ph _Abstract 

#define ALMOST_PUBLIC
#define PRIVATE_PUBLIC
#define PUBLIC 

#ifndef SYS_nanosleep 
#define SYS_nanosleep 35
#endif 

#define CHECK_THREAD_AVAILABILITY(x) \
    int (x) = std::thread::hardware_concurrency()

template <typename _Tp, class _InternalObj> 
    __TIMER_DECL class Timer { 
        __BEGIN_TIMER_DECL 
        public: 
            CHECK_THREAD_AVAILABILITY(hc);
            _CONSTEXPR20 Timer() = default; 
            _CONSTEXPR20 Timer(int &&seed) noexcept; 

            _Abstract _CONSTEXPR20 ~Timer();
            _Abstract int run(unsigned int seconds); 
            _Abstract int __nanosleep(const struct timespec *req, struct timespec *rem); 
            _Abstract _Tp _MyTimer_currentValue() const; 
            _Abstract _Tp _MyTimer_firstValue() const; 

        private: 
            _Tp _MyTimer_counter = -INT_MAX; 

            _InternalObj _obj1LifeTime; 
            _InternalObj _obj1LifeTimeSec; 
            _InternalObj _obj1LifeTimeMin; 
            _InternalObj _obj1LifeTimeHour; 

            __END_TIMER_DECL 
    }; 

__TIMER_DEF_BEGIN 
    template <typename _Tp, class _InternalObj> 
    _CONSTEXPR20 Timer<_Tp, _InternalObj>::Timer(int &&seed) noexcept { 


    }

template <typename _Tp, class _InternalObj> 
    _CONSTEXPR20 Timer<_Tp, _InternalObj>::~Timer() {


    }

template <typename _Tp, class _InternalObj> 
    _Tp Timer<_Tp, _InternalObj>::_MyTimer_currentValue() const { 

    } 

template <typename _Tp, class _InternalObj> 
    _Tp Timer<_Tp, _InternalObj>::_MyTimer_firstValue() const { 

    }
// Fixed syscall implementation
static inline long syscall3(long n, long a1, long a2, long a3) {
    register long rax asm("rax") = n;
    register long rdi asm("rdi") = a1;
    register long rsi asm("rsi") = a2;
    register long rdx asm("rdx") = a3;

    __asm__ volatile (
            "syscall"
            : "+r"(rax)
            : "r"(rdi), "r"(rsi), "r"(rdx)
            : "rcx", "r11", "memory"
            );

    return rax;
}

static int __syscall_errno = 0;

static inline int syscall3_errno(long n, long a1, long a2, long a3) {
    long result = syscall3(n, a1, a2, a3);
    if (result < 0 && result >= -4095) {
        __syscall_errno = (int)(-result);
        return -1;
    }
    __syscall_errno = 0;
    return (int)result;
}

template <typename _Tp, class _InternalObj> 
    int Timer<_Tp, _InternalObj>::__nanosleep(const struct timespec *req, struct timespec *rem) 
    {
        if (!req) return -1;

        long result = syscall3(SYS_nanosleep, (long)req, (long)rem, 0);

        if (result < 0) {
            errno = (int)(-result);  // Set errno for compatibility
            return -1;
        }
        return 0;
    }

template <typename _Tp, class _InternalObj> 
    int Timer<_Tp, _InternalObj>::run(unsigned int seconds) { 
        const unsigned int max_step = (unsigned int)((1UL << (sizeof(time_t)*8 - 1)) - 1);
        struct timespec ts = {0, 0}; 

        do { 
            unsigned int step;

            if (sizeof(ts.tv_sec) <= sizeof(seconds)) {
                step = (seconds > max_step) ? max_step : seconds;
                ts.tv_sec = step;
                seconds -= step;
            } else {
                ts.tv_sec = seconds;
                seconds = 0;
            }

            ts.tv_nsec = 0; 

            struct timespec rem = {0, 0};
            int ret = __nanosleep(&ts, &rem);

            if (ret < 0) {
                return seconds + rem.tv_sec + (rem.tv_nsec > 0 ? 1 : 0);
            }

        } while (seconds > 0); 

        return 0; 
    }


__TIMER_DEF_END 

    struct _Timer_InterObj { 
        int value_type; 
    }; 

}
