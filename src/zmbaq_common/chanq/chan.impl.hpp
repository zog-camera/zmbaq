#ifndef CHAN_IMPL_HPP
#define CHAN_IMPL_HPP


template<typename T, int N_MAX>
Chan<T, N_MAX>::Chan(size_t max_items) : hnd(std::make_shared<ChanCtrl>()),
    pv(std::make_shared<ChanPrivate::PV<T>>(hnd, max_items))
{

}
template<typename T, int N_MAX>
Chan<T,N_MAX>::~Chan()
{

}
template<typename T, int N_MAX>
void Chan<T,N_MAX>::set_limit(size_t max_items)
{
    pv = std::make_shared<ChanPrivate::PV<T>>(hnd, max_items);
}

template<typename T, int N_MAX>
Chan<T,N_MAX>& Chan<T,N_MAX>::operator << (T value)
{
    if (pv->is_overflow())
        return *this;

    pv->push(value);
    return *this;
}

template<typename T, int N_MAX>
Chan<T,N_MAX>& Chan<T,N_MAX>::operator << (std::shared_ptr<T> value_ptr)
{
    assert(nullptr != value_ptr);
    if (pv->is_overflow())
        return *this;

    pv->push(value_ptr);
    return *this;
}

template<typename T, int N_MAX>
Chan<T,N_MAX>& Chan<T,N_MAX>::operator >> (std::shared_ptr<T>& value_ptr)
{
    assert(nullptr != value_ptr);
    pv->pull(*value_ptr);
    return *this;
}

template<typename T, int N_MAX>
Chan<T,N_MAX>& Chan<T,N_MAX>::operator >> (T& value)
{
    pv->pull(value);
    return *this;
}

template<typename T, int N_MAX>
void Chan<T,N_MAX>::push(const T& value, bool& ok)
{
   pv->push(value, ok);
}

template<typename T, int N_MAX>
void Chan<T,N_MAX>::push(std::shared_ptr<T> value, bool& ok)
{
   pv->push(value, ok);
}

template<typename T, int N_MAX>
void Chan<T,N_MAX>::pop(T& value, bool& ok)
{
    pv->pull(value, ok);
}

template<typename T, int N_MAX>
T Chan<T,N_MAX>::pop(bool& ok)
{
    T val;
    pv->pull(val, ok);
    return val;
}

template<typename T, int N_MAX>
std::shared_ptr<T> Chan<T,N_MAX>::pop_ptr(bool& ok)
{
    auto ptr = this->make_shared();
    pv->pull(*ptr, ok);
    return ptr;
}
template<typename T, int N_MAX>
std::shared_ptr<ChanCtrl> Chan<T,N_MAX>::handle()
{
    return pv->hnd;
}

//deny any further I/O
template<typename T, int N_MAX>
void Chan<T,N_MAX>::close()
{
    pv->is_opened = false;
    pv->hnd->make_sig_closed();
}

//reopen for I/O
template<typename T, int N_MAX>
void Chan<T,N_MAX>::reopen()
{
    pv->is_opened = true;
    pv->hnd->make_sig_opened();
}


#endif // CHAN_IMPL_HPP

