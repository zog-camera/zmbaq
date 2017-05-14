#include <mutex>

namespace ZMB
{
  typedef std::mutex Mutex_t;
  
  struct None
  {
    None(const None&) { }
    None(None&&) { }
  };
  
  template<typename TMutexStub = None,
	   typename TGuard = None>
  struct NonLockableObject
  {
    typedef typename TGuard Lock_t;
    typedef typename TMutexStub Mutex_t;
    
    Lock_t&& operator()()
    {
      return std::move(Lock_t(d_mutex));
    }
    typename TMutexStub d_mutex;
  };
  
  struct LockableObject : public NonLockableObject<std::mutex, std::unique_lock<Mutex_t>>
  {
    //it has it all
  };


}//namespace ZMB
