#ifndef THREAD_BOOL_H
#define THREAD_BOOL_H

#include <thread>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <atomic>
#include <vector>
#include "noncopyable.hpp"

namespace ZMBCommon {

typedef std::function<void()> CallableFunc_t;

/** encapsulates a callable and an exception callback.*/
struct CallableDoubleFunc
{
  CallableDoubleFunc() { tag.fill(0x00); }
  std::array<char, 16> tag;
  WebGrep::CallableFunc_t functor;
  std::function<void(const std::exception&)> cbOnException;
};

//iterator function type to iterate over array of functors for submission
typedef std::function<bool(CallableFunc_t**, size_t*, size_t)> IteratorFunc_t;
typedef std::function<bool(CallableDoubleFunc**, size_t*, size_t)> IteratorFunc2_t;

//simple(default) array iteration function: increment a pointer and a counter
//returns TRUE while last item is not reached
bool PtrForwardIterationDbl(WebGrep::CallableDoubleFunc** arrayPPtr, size_t* counter, size_t maxValue);
//same as above, but for function<void()> type
bool PtrForwardIteration(WebGrep::CallableFunc_t** arrayPPtr, size_t* counter, size_t maxValue);


//-------------------------------------------------------------------------
/** A structure that can be used directly to enqueue tasks to a threads pool.*/
struct TPool_ThreadData
{
  TPool_ThreadData() : stopFlag(false), terminateFlag(false)
  {
    workQ.reserve(32);
  }

  /** Serialize functors to this thread. Call notify() later to take effect.
   * Possible exceptions: bad_alloc.
   * @return count of items serialized */
  size_t enqueue(std::unique_lock<std::mutex>& lk,
                 WebGrep::CallableFunc_t* ftorArray, size_t len,
                 IteratorFunc_t iterFn = PtrForwardIteration);

  /** Serialize functors to this thread. Call notify() later to take effect.
   * Possible exceptions: bad_alloc.
   * @return count of items serialized*/
  size_t enqueue(std::unique_lock<std::mutex>& lk,
                 CallableDoubleFunc* array, size_t len,
                 IteratorFunc2_t iterFn = PtrForwardIterationDbl);

  /** Must be called when the mutex is unlocked.*/
  void notify() { cond.notify_all();}

  std::mutex mu;
  std::condition_variable cond;
  volatile bool stopFlag;     //< tells to stop after finishing current tasks
  volatile bool terminateFlag;//< tells to quite the loop ASAP
  std::vector<CallableDoubleFunc> workQ;

  //used to export abandoned tasks
  std::function<void(CallableDoubleFunc*, size_t/*n_items*/)> exportTaskFn;
};
typedef std::shared_ptr<std::thread> ThreadPtr;
typedef std::shared_ptr<TPool_ThreadData> TPool_ThreadDataPtr;
//-------------------------------------------------------------------------


/** A thread pool that schedules tasks represented as functors,
 *  similar to boost::basic_thread_pool.
 *  It never throws except the constructor where only bad_alloc can occur.
 *
 *  The destructor will join the threads,
 *  if you want it to happen earlier or in separate thread -- call joinAll() manually.
 *  There are 2 possible ways of thread joining: joinAll() will wait for the tasks to finish,
 *  joinAll(true) will terminate ASAP and make the pool to abandon tasks left in the queue;
 *  joinExportAll(functor) will terminate ASAP but with invocation of a functor that can
 *  pass abandoned tasks from the queue to where ever you'll pass it, the functor must be thread-safe!
 *
 *  To control exceptions raised from execution of the functor
 *  the user must set CallableDoubleFunc.cbOnException callbacks for each task during submission
 *  via the submit(const CallableDoublefunc* array ...) method.
*/
class ThreadsPool : public WebGrep::noncopyable
{
public:

  //can throw std::bad_alloc on when system has got no bytes for spare
  explicit ThreadsPool(uint32_t nthreads = 1);
  virtual ~ThreadsPool() { close(); joinAll(); }
  size_t threadsCount() const;

  bool closed() const;

  //submit 1 task that has no cbOnException callback.
  bool submit(const WebGrep::CallableFunc_t& ftor);

  //submit 1 task with cbOnException callback
  bool submit(CallableDoubleFunc& ftor);

  /** Submit(len) tasks from array of data. A generalized interface
   *  to work with raw pointers or containers by providing iteration functor.
   *  The iteration functor is called after each element access by pointer dereference.
   *  Example for the linked list:
   *
   *  @verbatim
   *  struct LList { CallableDoubleFunc ftor; LList* next;};
   *  LList* list_head = new LList;//fill the linked list with N elements
   *  ThreadsPool thp(10);
   *
   *  auto iterFunc = [](const CallableDoubleFunc** list, size_t*,size_t) -> bool
   *  {//iterate over linked list
   *    (void)dcount;//unused
   *    *list = list->next;
   *    return nullptr != *list;
   *  };
   *  thp.submit(list_head, 0, iterFunc, true);
   *
   *  @endverbatim
   *  @param ftorArray: plain array or linked list's head pointer.
   *  @param len: count of elements
   *  @param iterFn: functor to be called after each pointer dereference
   *  @param spray: case TRUE -- spread the tasks between all pool's threads,
   *  case FALSE -- it will push the tasks to just one thread's task queue.
   *  FALSE option is useful for consequent/dependent tasks.
*/
  bool submit(CallableDoubleFunc* ftorArray, size_t len,
              IteratorFunc2_t iterFn = PtrForwardIterationDbl, bool spray = true);

  /** same as submit(CallableDoubleFunc* ftorArray, size_t ..)
   *  but for functors without exception control (they're catched and ignored)*/
  bool submit(WebGrep::CallableFunc_t* ftorArray, size_t len,
              IteratorFunc_t iterFn = PtrForwardIteration, bool spray = true);

  /** Get a one thread handle to serialize things in your own manner,
   *  be careful with the locks! I hope you known what you're doing.
   *  @return thread data pointer or NULL if closed(). */
  TPool_ThreadDataPtr getDataHandle();


  void close();  //< close the submission of tasks

  /** Method is thread-safe, synchronized by this->joinMutex.
   * @param terminateCurrentTasks: when FALSE it'll wait for current tasks to be procesed,
   * if TRUE a termination flag will be set and it'll leave the scope ASAP. */
  void joinAll(bool terminateCurrentTasks = false);

  /** Notify all threads to stop, but do not join(), detach() instead.*/
  void terminateDetach();

  /** Terminates execution of tasks ASAP, exports abandoned task functors by given functor.
   * Method is thread-safe.*/
  void joinExportAll(const std::function<void(CallableDoubleFunc*, size_t)>& exportFunctor);

  bool joined(); //< synced by joinMutex.

protected:


  std::vector<std::thread> threadsVec;
  std::vector<TPool_ThreadDataPtr> mcVec;
  std::atomic_uint d_current;
  volatile bool d_closed;

  std::mutex joinMutex;
};

}//namespace ZMBCommon

#endif // THREAD_BOOL_H
