//----------------------------------------------------------------------------
// year   : 2018
// author : John Paul
// email  : johnpaultaken@gmail.com
// source : https://github.com/johnpaultaken
// description :
//      Lock-free wait-free implementation of singleton in C++11.
//----------------------------------------------------------------------------

#pragma once

#include <memory>
#include <mutex>

namespace lockfree
{

using std::shared_ptr;
using std::weak_ptr;
using std::bad_weak_ptr;
using std::make_shared;
using std::mutex;
using std::unique_lock;

/*
Notes:
1.  Lock-free once the single instance is constructed.
    Uses a lock while constructing the single instance.
2.  Avoids double checked locking, which is incorrect, to achieve lock-free.
3.  Releases the singleton object once the last client holding a reference
    releases it.
4.  Please see test_singleton.cpp for usage example.
*/

template<typename T>
class singleton
{
public:
    singleton<T>() = delete;
    singleton<T>(const singleton<T> &) = delete;
    singleton<T>(singleton<T> &&) = delete;
    singleton<T> & operator= (const singleton<T> &) = delete;
    singleton<T> & operator= (singleton<T> &&) = delete;

    template<typename... P>
    static shared_ptr<T> instance(P... params)
    {
        try
        {
            return shared_ptr<T> {instance_};
        }
        catch (const bad_weak_ptr &)
        {
            unique_lock<mutex> l(mtx_);
            auto instance = instance_.lock();
            if (!instance)
            {
                instance = make_shared<T>(params...);
                instance_ = instance;
            }
            return instance;
        }
    }
private:
    static weak_ptr<T> instance_;
    static mutex mtx_;
};

template<typename T>
weak_ptr<T> singleton<T>::instance_;

template<typename T>
mutex singleton<T>::mtx_;

}
