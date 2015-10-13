#include <tbb/cache_aligned_allocator.h>
#include <tbb/concurrent_queue.h>
#include <chrono>
#include <thread>

namespace  ChanPrivate{

template<typename T>
class PV
{
    PV();

public:



    tbb::cache_aligned_allocator<T> allocate_obj;
    std::shared_ptr<ChanCtrl> hnd;
    tbb::concurrent_bounded_queue<T> list;
    size_t n_max;
    bool is_opened;

    PV(std::shared_ptr<ChanCtrl> handle, size_t max_items )
        : hnd(handle), is_opened(true), n_max(max_items)
    {
    }
    ~PV()
    {

    }

    template<class ... TArgs>
    std::shared_ptr<T> make_shared(TArgs&& ... args)
    {
       return std::allocate_shared<T,tbb::cache_aligned_allocator<T>>(allocate_obj, args...);
    }

    bool is_overflow()
    {
        if (list.size() == n_max)
        {
            hnd->make_sig_overflow(n_max);
            return true;
        }
        return false;
    }
    //try hard:
    void push(const T& value)
    {
        bool checks_ok = true;
        bool good = false;
        while (!good && checks_ok && is_opened)
        {
            try {
                good = list.try_push(value);
            } catch(std::bad_alloc& ba)
            {
                hnd->make_sig_bad_alloc();
                checks_ok = false;
            }
        }
    }
    void push(const T& value, bool& ok)
    {
       ok = list.try_push(value);
       ok &= is_overflow();
    }
    //try hard:
    void push(std::shared_ptr<T>& value_ptr)
    {
       push(*value_ptr);
    }

    void push(std::shared_ptr<T>& value_ptr, bool& ok)
    {
       push(*value_ptr, ok);
    }

    //try hard:
    void pull(T& deq_value)
    {
        bool checks_ok = true;
        bool good = false;
        while (!good && checks_ok && is_opened)
        {
            try {
                good = list.try_pop(deq_value);
            } catch(std::bad_alloc& ba)
            {
                hnd->make_sig_bad_alloc();
                checks_ok = false;
            }
        }
    }

    void pull(T& deq_value, bool& ok)
    {
        ok = false;
        ok = list.try_pop(deq_value);
    }
};

}//namespace ChanPrivate

//===================================================
