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


#ifndef STADEK_H
#define STADEK_H

#include <deque>
#include <mutex>
#include "zmbaq_common.h"

namespace ZMB {

/** A special purpose deque where part of the elements is stored on stack.
 *  A T typename, if it's class or C++ structure must have valid copy operator & constructor.
 *
 *  You may destruct elements on removal via the void(*element_deinit)(T*)
 *  callback. Better copy ~T() body there.
*/
template<typename T, size_t N_STACK = 32,
         bool WITH_LOCKS = false,
         typename T_CONTAINER = std::vector<T>>
class Stadek
{


public:

    Stadek(const Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>&
           other) = delete;

    Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>& operator=( const Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>& );

    /** Should be set to explicitly destruct an element
     *  at persistent stack memory location.*/
    void (*element_deinit)(T* elem_ptr);

    static const bool m_using_locks = WITH_LOCKS;
    static const int stacked_size = N_STACK;
    T st_buf[stacked_size];
    size_t st_size;

    T_CONTAINER list;
    std::mutex* mu;

    Stadek()
    {
        if (m_using_locks)
            mu = new std::mutex;
        st_size = 0;
        element_deinit = 0;
        memset(st_buf, 0x00, sizeof(st_buf));
    }
    virtual ~Stadek()
    {
        clear();
        if (m_using_locks)
        {
            delete mu;
            mu = 0;
        }
    }

    size_t size() const { return st_size; }
    void clear();
    bool erase(size_t idx);
    void pop(T& deq_value, bool& ok);
    void push_back(const T& value);
    T* spawn();
    T* get(size_t idx);
    T* last_item() { return 0 < size()? get(size() - 1) : 0;}


};

template<typename T, size_t N_STACK, bool WITH_LOCKS, typename T_CONTAINER>
Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>& Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>::operator=
    ( const Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>& other)
{
    list = other.list;
    memcpy(st_buf, other.st_buf, N_STACK);
    st_size = other.st_size;
    if (WITH_LOCKS)
    {
        mu = new std::mutex;
    }
    return *this;
}

template<typename T, size_t N_STACK, bool WITH_LOCKS, typename T_CONTAINER>
void Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>::clear()
{
    list.clear();
    if (nullptr != element_deinit)
    {
        for (u_int32_t c = 0; c < ZMB::min(N_STACK, st_size); ++c)
            element_deinit(st_buf + c);
    }
    memset(st_buf, 0x00, sizeof(st_buf));
    st_size = 0;
}

template<typename T, size_t N_STACK, bool WITH_LOCKS, typename T_CONTAINER>
T* Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>::get(size_t idx)
{
    if (idx >= st_size)
        return 0;

    if (idx < N_STACK)
        return st_buf + idx;

    if (idx >= N_STACK && (st_size - N_STACK) <= list.size())
        //form heap:
        return &list[idx - N_STACK];
    return 0;
}

template<typename T, size_t N_STACK, bool WITH_LOCKS, typename T_CONTAINER>
T* Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>::spawn()
{
    if (m_using_locks) mu->lock();
    ++st_size;
    if (st_size <= N_STACK)
    {
        if (m_using_locks) mu->unlock();
        return st_buf + st_size - 1;
    }
    list.emplace_back();
    auto li = list.end();
    --li;
    if (m_using_locks) mu->unlock();
    return &(*li);
}

template<typename T, size_t N_STACK, bool WITH_LOCKS, typename T_CONTAINER>
void Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>::push_back(const T& value)
{
    if (m_using_locks) mu->lock();

    ++st_size;
    if (st_size <= N_STACK)
    {
        st_buf[st_size - 1] = value;
    }
    else
    {
        list.push_back(value);
    }
    if (m_using_locks) mu->unlock();
}

template<typename T, size_t N_STACK, bool WITH_LOCKS, typename T_CONTAINER>
void Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>::pop(T& deq_value, bool& ok)
{
    if (m_using_locks) mu->lock();
    if (st_size == 0)
    {
        ok = false;
    }
    else
    {
        ok = true;
        --st_size;
        if (st_size < N_STACK)
        {
            if (nullptr != element_deinit)
                element_deinit(st_buf + st_size);

            deq_value = st_buf[st_size];
        }
        else if(st_size >= N_STACK)
        {
            deq_value = list[list.size() - 1];
            list.pop_back();
        }
    }
    if (m_using_locks) mu->unlock();
}

template<typename T, size_t N_STACK, bool WITH_LOCKS, typename T_CONTAINER>
bool Stadek<T, N_STACK, WITH_LOCKS, T_CONTAINER>::erase(size_t idx)
{
    if (m_using_locks) mu->lock();
    if (0 == st_size || idx >= st_size)
    {
        if (m_using_locks) mu->unlock();
        return false;
    }

    bool ok = false;
    T* elem = 0;
    --st_size;
    if (idx < N_STACK)
    {//on stack:
        elem = st_buf + idx;
        if (nullptr != element_deinit)
            element_deinit(elem);
        if (!list.empty())
        {//move item from heap:
            *elem = list[list.size() - 1];
            list.pop_back();
        }
        else if ( idx < st_size && st_size < N_STACK)
        {//move last on stack to replace:
            *elem = *(st_buf + st_size );
        }
        else
        {
            //do nothing if (idx == (st_size - 1))
        }
        ok = true;
    }
    else if (idx >= N_STACK
             && (st_size - N_STACK) < list.size())
    {//heap
        elem = &list[idx - N_STACK];
        //move last on heap to this element:
        if (list.size() > 1)
        {
            if (nullptr != element_deinit)
                element_deinit(elem);
            *elem = list[list.size() - 1];
        }

        list.pop_back();
        ok = true;
    }
    else
    {
        ok = false;
    }
    if (m_using_locks) mu->unlock();
    return ok;
}

}//ZMB

#endif // STADEK_H

