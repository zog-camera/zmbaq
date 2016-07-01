/*
A video surveillance software with support of H264 video sources.
Copyright (C) 2015 Bogdan Maslowsky, Alexander Sorvilov. 

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef CHAR_ALLOCATOR_SINGLETON_H
#define CHAR_ALLOCATOR_SINGLETON_H

#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include <tbb/scalable_allocator.h>
#include <tbb/cache_aligned_allocator.h>

namespace ZMBCommon { class MByteArray; }

namespace ZMB {

class GenericLocker
{
public:
    typedef std::function<void(void*)> lk_pfn_t;

    /** Does lock if (lock_pfn) != nullptr.*/
    explicit GenericLocker(void* data = nullptr,
                           lk_pfn_t lock_pfn = nullptr,
                           lk_pfn_t unlock_pfn = nullptr);

    /** Does unlock if p_unlock != nullptr.*/
    virtual ~GenericLocker();

    lk_pfn_t p_lock, p_unlock;
    void* sidedata; //< a target of p_lock() and p_unlock();

    /** Allocate item with use of the allocator.*/
    static std::shared_ptr<GenericLocker> make_item
        (void* data = nullptr, lk_pfn_t lock_pfn = nullptr, lk_pfn_t unlock_pfn = nullptr);
};

}//ZMB

class CharAllocatorSingleton
{
public:
    CharAllocatorSingleton();
    ~CharAllocatorSingleton();

    tbb::cache_aligned_allocator<char>& c_allocator();
    tbb::scalable_allocator<char>& s_allocator();
    tbb::scalable_allocator<ZMBCommon::MByteArray>& s_mbytes_allocator();
    tbb::cache_aligned_allocator<ZMB::GenericLocker>& c_genericlock_allocator();

    const std::new_handler& get_new_handler();

    static CharAllocatorSingleton* instance();

private:
    static std::atomic<CharAllocatorSingleton*> m_instance;
    static std::mutex m_mutex;
    std::new_handler d_new_handler;

    tbb::cache_aligned_allocator<char> d_cached_allocator;
    tbb::scalable_allocator<char> d_scalable_allocator;

    tbb::scalable_allocator<ZMBCommon::MByteArray> d_scalable_mbytes_allocator;
    tbb::cache_aligned_allocator<ZMB::GenericLocker> d_gl_allocator;
};

//creates a shard pointer using the allocator
template<class ... Args>
std::shared_ptr<ZMBCommon::MByteArray> make_mbytearray(Args ... arg)
{
    return std::allocate_shared<ZMBCommon::MByteArray>(CharAllocatorSingleton::instance()->s_mbytes_allocator(), arg...);
}

#endif // CHAR_ALLOCATOR_SINGLETON_H
