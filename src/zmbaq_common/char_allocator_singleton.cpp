/*A video surveillance software with support of H264 video sources.
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
along with this program.  If not, see <http://www.gnu.org/licenses/>.*/


#include "char_allocator_singleton.h"
#include <atomic>
#include <mutex>
#include "mbytearray.h"

namespace ZMB {

GenericLocker::GenericLocker(void* data,
                             lk_pfn_t lock_pfn,
                             lk_pfn_t unlock_pfn)
    :
      sidedata(data), p_lock(lock_pfn), p_unlock(unlock_pfn)
{
   if (0 != p_lock)
       p_lock(sidedata);
}

GenericLocker::~GenericLocker()
{
    if (0 != p_unlock)
       p_unlock(sidedata);
}

std::shared_ptr<GenericLocker> GenericLocker::make_item
    (void* data, lk_pfn_t lock_pfn, lk_pfn_t unlock_pfn)
{
    return std::allocate_shared
            <
            ZMB::GenericLocker,
            tbb::cache_aligned_allocator<ZMB::GenericLocker>,
            void*, ZMB::GenericLocker::lk_pfn_t, ZMB::GenericLocker::lk_pfn_t
            >
            (CharAllocatorSingleton::instance()->c_genericlock_allocator(),
                 std::forward<void*>(data), std::forward<lk_pfn_t>(lock_pfn), std::forward<lk_pfn_t>(unlock_pfn)
             );

}
}//namespace ZMB

std::atomic<CharAllocatorSingleton*> CharAllocatorSingleton::m_instance;
std::mutex CharAllocatorSingleton::m_mutex;

CharAllocatorSingleton* CharAllocatorSingleton::instance()
{
    CharAllocatorSingleton* tmp = m_instance.load();
    if (tmp == nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        tmp = m_instance.load();


        if (tmp == nullptr) {
            tbb::scalable_allocator<char> char_allocator;
            void* mem = char_allocator.allocate(sizeof(CharAllocatorSingleton));

            //call constructor on existing memory:
            tmp = new (mem) CharAllocatorSingleton;
            m_instance.store(tmp);
        }
    }
    return tmp;
}

CharAllocatorSingleton::CharAllocatorSingleton()
{

}

CharAllocatorSingleton::~CharAllocatorSingleton()
{

}

const std::new_handler& CharAllocatorSingleton::get_new_handler()
{
    return d_new_handler;
}

tbb::cache_aligned_allocator<ZMB::GenericLocker>& CharAllocatorSingleton::c_genericlock_allocator()
{
   return d_gl_allocator;
}

tbb::cache_aligned_allocator<char>& CharAllocatorSingleton::c_allocator()
{
   return d_cached_allocator;
}
tbb::scalable_allocator<char>& CharAllocatorSingleton::s_allocator()
{
   return d_scalable_allocator;
}

tbb::scalable_allocator<ZMBCommon::MByteArray>& CharAllocatorSingleton::s_mbytes_allocator()
{
   return d_scalable_mbytes_allocator;
}
