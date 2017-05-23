#ifndef LOCKABLE_HPP
#define LOCKABLE_HPP
#include <mutex>
#include "boost/noncopyable.hpp"

namespace ZMB
{
  typedef std::mutex Mutex_t;
  
  struct None
  {
    None(const None&) { }
    None(None&&) { }
  };
  
  template<typename TMutexStub = ZMB::None,
           typename TGuard = ZMB::None>
  struct NonLockableObject : public boost::noncopyable
  {
    typedef TGuard Lock_t;
    typedef TMutexStub Mutex_t;
    
    Lock_t&& operator()()
    {
      return std::move(Lock_t(d_mutex));
    }
    TMutexStub d_mutex;
  };
  
  struct LockableObject : public NonLockableObject<std::mutex, std::unique_lock<Mutex_t>>
  {
    //it has it all
  };


}//namespace ZMB

#endif //LOCKABLE_HPPff
