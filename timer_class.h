#ifndef  __TIMER_CLASS_H 
#define __TIMER_CLASS_H 


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
 

extern int __syscall_errno; 

template <typename _Tp, class _InternalObj> 
    __TIMER_DECL class Timer { 
        __BEGIN_TIMER_DECL 
        public: 
            CHECK_THREAD_AVAILABILITY(hc);
            _CONSTEXPR20 Timer() = default; 
            _CONSTEXPR20 Timer(int &&seed) noexcept; 

            _Abstract ~Timer();
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

   struct _Timer_InterObj { 
        int value_type; 
    }; 

}
 
#include "timer_class.tpp"
#endif 
