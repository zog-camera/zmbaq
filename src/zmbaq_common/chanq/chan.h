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


#ifndef CHAN_H
#define CHAN_H

#include <list>
#include <memory>
#include "zmbaq_common.h"
#include <assert.h>
#include "mbytearray.h"
#include <functional>


class ChanCtrl : public std::enable_shared_from_this<ChanCtrl>
{
public:
    void* sidedata;

    ChanCtrl();

    void make_sig_closed();

    void make_sig_opened();

    //case cant alloc:
    void make_sig_bad_alloc();

    //when pushed above the limit:
    void make_sig_overflow(size_t n_items_count);

    std::function<void(SHP(ChanCtrl))> sig_closed;
    std::function<void(SHP(ChanCtrl))> sig_opened;

    //case cant alloc:
    std::function<void(SHP(ChanCtrl))> sig_bad_alloc;

    //when pushed above the limit:
    std::function<void(SHP(ChanCtrl), size_t /*n_items_count*/)>sig_overflow;

    std::function<void(SHP(ChanCtrl), ZMBCommon::MByteArray msg)> sig_error;
};

#include "chan.c.hpp"
/** The channel is opened when created.*/
template<typename T, int N_MAX = 256 * 1024>
class Chan
{
public:
    explicit Chan(size_t max_items = N_MAX);
    virtual ~Chan();

    void set_limit(size_t max_items);

    SHP(ChanCtrl) handle();

    //deny any further I/O
    void close();

    //reopen for I/O
    void reopen();

    void push(const T& value, bool& ok);
    void push(std::shared_ptr<T> value, bool& ok);

    void pop(T& value, bool& ok);
    T pop(bool& ok);
    std::shared_ptr<T> pop_ptr(bool& ok);

    // these are blocking operators(they try hard in a loop):
    Chan<T,N_MAX>& operator << (T value);
    Chan<T,N_MAX>& operator << (std::shared_ptr<T> value_ptr);

    Chan<T,N_MAX>& operator >> (std::shared_ptr<T>& value_ptr);
    Chan<T,N_MAX>& operator >> (T& value);

    /** Create std::shared_ptr<T>(..args) via allocator.*/
    template<class ... TArgs>
    std::shared_ptr<T> make_shared(TArgs&& ... args)
    {
        return pv->make_shared<TArgs ...>(args...);
    }


private:
    std::shared_ptr<ChanCtrl> hnd;
    std::shared_ptr<ChanPrivate::PV<T>> pv;
};

#include "chan.impl.hpp"

#endif // CHAN_H
