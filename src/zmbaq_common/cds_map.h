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


#ifndef CDS_MAP_H
#define CDS_MAP_H

#include <string>
#include <mutex>
#include <map>
#include <assert.h>
#include <memory>

#include "zmbaq_common.h"
#include "char_allocator_singleton.h"

namespace LCDS
{

/** Generic comparator for a set/map.
 * @return -1 if A < B, 1 case A > B, 0 if A == B.
*/
template<typename T>
struct generic_cmp
{
    bool operator ()(const T& v1, const T& v2 ) const
    {
        return v1 < v2;
    }
};
//-------------------------------------------------------------------
/** Compasioin of std::string with const char*,
 *  avoids creation of temporary object.*/
template<typename TStr>
struct generic_string_cmp
{
    int operator()(TStr const& s1, TStr const& s2) const { return s1.compare( s2 ); }

    int operator()(TStr const& s1, char const* s2) const { return s1.compare( s2 ); }

    int operator()(char const* s1, TStr const& s2) const {return -s2.compare(s1);}
};
//-------------------------------------------------------------------
typedef generic_string_cmp<std::string> string_cmp;
//-------------------------------------------------------------------
template<typename TKey, typename TValue,
         typename Compare = generic_cmp<TKey>, typename ... TArgs>
class MapHnd
{

public:
    typedef std::map<TKey, TValue, Compare, TArgs... > map_type;
    typedef std::shared_ptr<map_type> map_type_ptr;

    struct MapNMutex
    {
        std::mutex mutex;
        map_type map;
    };
    typedef MapNMutex MapNMutex_type;
    typedef std::shared_ptr<MapNMutex> MapNMutexPtr;

    //---
    class map_guard : public ZMB::GenericLocker
    {

    public:
        static void lock_fn(void* side)
        { MapNMutex* pmm = (MapNMutex*)side; pmm->mutex.lock(); }

        static void unlock_fn(void* side)
        { MapNMutex* pmm = (MapNMutex*)side; pmm->mutex.unlock(); }


        map_guard(MapNMutex* map_p) :
            ZMB::GenericLocker
            ( /*sidedata*/(void*)map_p, &lock_fn, &unlock_fn )
        {

        }

        bool empty() const
        {
            return map().empty();
        }
        size_t size() const
        {
            return map().size();
        }

        void enlist(std::list<TValue>& foreign_list)
        {
            for (auto it = map().begin(); it != map().end(); ++it)
                foreign_list.push_back(it.value());
        }

        //prepend a new element before the current(modif iterators value)
        bool emplace(const TKey&& key, const TValue&& value)
        {
            auto it_pair = map().emplace_hint(std::make_pair<TKey, TValue>(key, value));
            return it_pair.second;
        }

        bool emplace(const TKey& key, const TValue&& value)
        {
            ptr()->map.operator [](key) = TValue(value);
            return true;
        }

        bool emplace(const TKey& key, const TValue& value)
        {
            ptr()->map.operator [](key) = value;
            return true;
        }

        void clear()
        {
            ptr()->map.clear();
        }
        map_type& map() const { return ptr()->map;}
        MapNMutex* ptr() const {return (MapNMutex*)sidedata;}


    private:
        map_guard();

    };
    typedef std::shared_ptr<ZMB::GenericLocker> map_guard_ptr;
    //---
    class iter
    {
    public:
        iter(map_guard_ptr a_guard, typename map_type::iterator an_iter)
            : _guard_ptr(a_guard), pm_iterator(an_iter)
        {
            guard = (map_guard*)_guard_ptr.get();
        }

        bool map_empty() const { return guard->map().empty(); }
        size_t map_size() const { return guard->map().size(); }
        void map_clear() {guard->map().clear(); to_end();}


        bool is_end() const
        {
            return pm_iterator == guard->map().end();
        }
        bool is_begin() const
        {
            return pm_iterator == guard->map().begin();
        }

        TValue& value()
        {
            return (*pm_iterator).second;
        }
        const TKey& key() const
        {
            return (*pm_iterator).first;
        }

        void to_begin()
        {
            pm_iterator = guard->map().begin();
        }
        void to_end()
        {
            pm_iterator = guard->map().end();
        }
        /** @return false if last is end().*/
        bool to_last()
        {
            pm_iterator = guard->map().end();
            --pm_iterator;
            return guard->map().end() != pm_iterator;
        }
        bool to_first()
        {
            pm_iterator = guard->map().begin();
            return guard->map().end() != pm_iterator;
        }

        iter& operator++()
        {
            ++pm_iterator;
            return *this;
        }

        iter& operator--()
        {
            --pm_iterator;
            return *this;
        }

        //prepend a new element before the current(modif iterators value)
        bool prepend(const TKey&& key, const TValue&& value)
        {
            bool ok = guard->emplace(key, value);
            to_begin();;
            return ok;
        }

        bool prepend(const TKey& key, const TValue& value)
        {
            bool ok = guard->emplace(key, value);
            to_begin();;
            return ok;
        }

        //erase current item and go to begin();
        void erase(bool then_go_begin = true)
        {
            if (!is_end())
            {
                guard->map().erase(pm_iterator);
            }
            if (then_go_begin)
                to_begin();
            else
                to_end();
        }

        map_guard_ptr _guard_ptr;
        map_guard* guard;
        typename map_type::iterator pm_iterator;

    private:
        iter();
    };

    //-------
    MapHnd()
    {
        pv = std::make_shared<MapNMutex>();
    }
    ~MapHnd()
    {

    }

    //------------------------------------------------
    /** Get the ref. counted map locker (can be casted to accessor (map_guard*)).
     * Use std::move(ptr) to unref the pointer (and unblock the map)*/
    std::shared_ptr<ZMB::GenericLocker> m() const
    {
        return ZMB::GenericLocker::make_item(pv.get(), &map_guard::lock_fn, &map_guard::unlock_fn);
    }

    /** Get the ref. counted map locker AND casted (map_guard*) pointer. */
    std::pair<std::shared_ptr<ZMB::GenericLocker>, map_guard*> mg() const
    {
        auto shp = m();
        return std::pair<std::shared_ptr<ZMB::GenericLocker>, map_guard*> (shp, (map_guard*)shp.get());
    }
    //-------------------------------------------------

    //insert using iter type that provides mutex lock:
    bool emplace(iter& it, const TKey& key, const TValue& val)
    {
        assert(it.guard->ptr() == pv.get() );
        return it.prepend(key, val);
    }

    bool emplace(iter& it, const TKey&& key, const TValue&& val)
    {
        assert(it.guard->pmap == pv );
        return it.prepend(key, val);
    }

    //may return end() if failed:
    iter emplace(const TKey& key, const TValue& val, map_guard_ptr guard)
    {
        assert(nullptr != guard);
        map_guard* g = (map_guard*)guard.get();
        //asssert if using wrong guard (from foreign object)
        assert(g->ptr() == pv.get() );
        auto it = iter(guard, g->map().end());
        it.prepend(key, val);
        return it;
    }

    void erase(iter& it)
    {
        if (!it.is_end())
        {
            pv->map.erase(it.pm_iterator);
        }
    }

    iter begin()
    {
        return iter (m(), pv->map.begin());
    }
    iter end()
    {
        return iter (m(), pv->map.end());
    }

    iter find(const TKey& key) const
    {
        return iter (m(), pv->map.find(key));
    }

private:
    std::shared_ptr<MapNMutex> pv;
};
//--------------------------------------------------------------------

}

#endif // CDS_MAP_H
