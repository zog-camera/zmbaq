#include "thread_pool.h"
#include <iostream>
#include <list>
#include <chrono>

namespace ZMBCommon {
struct Maker
{
  Maker(const TPool_ThreadDataPtr& data) : pos(0), dataPtr(data)
  {
    localArray.reserve(32);
  }
  ~Maker()
  {
    //
    try {
      //case we have to export abandoned tasks:
      if (nullptr != dataPtr->exportTaskFn)
        {
          //move and export tasks that are left there:
          std::unique_lock<std::mutex> lk(dataPtr->mu);
          dataPtr->exportTaskFn(&localArray[pos], localArray.size() - pos);
          localArray.clear();
          dataPtr->exportTaskFn(&(dataPtr->workQ[0]), dataPtr->workQ.size());

        } else if (!dataPtr->terminateFlag)
        {//finish the jobs left there:
          exec(dataPtr->terminateFlag);
          std::unique_lock<std::mutex> lk(dataPtr->mu);
          pull(dataPtr);
          exec(dataPtr->terminateFlag);
        }
    } catch(std::exception& ex)
    {
      std::cerr << ex.what() << std::endl;
    }

  }
  void pull(const TPool_ThreadDataPtr& td)
  {
    //move task queue to local array
    for(CallableDoubleFunc& f : td->workQ)
      {
//        if(f.tag[0] != '\0')
//          std::cerr << "pulled for making: " << f.tag.data() << "\n";
        localArray.push_back(f);
      }
    td->workQ.clear();
  }
  void exec(volatile bool& term_flag)
  {
    for(; pos < localArray.size() && !term_flag; ++pos)
      {
        CallableDoubleFunc& pair(localArray[pos]);
        try {
          if (nullptr != pair.functor)
            pair.functor();
        }
        catch(const std::exception& ex)
        {
          if (pair.cbOnException)
            pair.cbOnException(ex);
        }
      }//for
    //clear if was not interrupted:
    if (pos == localArray.size())
      {
        localArray.clear();
        pos = 0;
      }
  }
  TPool_ThreadDataPtr dataPtr;
  size_t pos;//position
  std::vector<CallableDoubleFunc> localArray;

};
size_t TPool_ThreadData::enqueue(std::unique_lock<std::mutex>& lk,
                                 WebGrep::CallableFunc_t* array, size_t len, IteratorFunc_t iterFn)
{
  (void)lk;
  WebGrep::CallableFunc_t* ptr = array;

  size_t cnt = 0;
  WebGrep::CallableDoubleFunc dfunc;
  bool ok = true;
  for(; ok; ok = iterFn(&ptr, &cnt, len))
    {
      dfunc.functor = *ptr;
      workQ.push_back(dfunc);
    }
  return cnt;
}

size_t TPool_ThreadData::enqueue(std::unique_lock<std::mutex>& lk,
                                 CallableDoubleFunc* array, size_t len, IteratorFunc2_t iterFn)
{
  (void)lk;
  CallableDoubleFunc* ptr = array;

  bool ok = true;
  size_t cnt = 0;
  for(; ok; ok = iterFn(&ptr, &cnt, len))
    {
      workQ.push_back(*ptr);
    }
  return cnt;
}

void ThreadsPool_processingLoop(const TPool_ThreadDataPtr& td)
{
  Maker taskM(td);

  while(!td->stopFlag)
    {
      std::unique_lock<std::mutex> lk(td->mu);
      if (taskM.pos >= taskM.localArray.size() && td->workQ.empty())
        {
          td->cond.wait(lk);
        }
      taskM.pull(td);
      lk.unlock();

      taskM.exec(td->terminateFlag);
    }//while

  //the dtor() will either execute or export unfinished jobs
}

bool PtrForwardIterationDbl(WebGrep::CallableDoubleFunc** arrayPPtr, size_t* counter, size_t maxValue)
{
  ++(*arrayPPtr);
  return ++(*counter) < maxValue;
}
bool PtrForwardIteration(WebGrep::CallableFunc_t** arrayPPtr, size_t* counter, size_t maxValue)
{
  ++(*arrayPPtr);
  return ++(*counter) < maxValue;
}


ThreadsPool::ThreadsPool(uint32_t nthreads)
  : d_closed(false)
{
  threadsVec.resize(nthreads);
  mcVec.resize(nthreads);

  size_t idx = 0;

  for(std::thread& t : threadsVec)
    {
      mcVec[idx] = std::make_shared<TPool_ThreadData>();
      t = std::thread(ThreadsPool_processingLoop, mcVec[idx]);
      idx++;
    }
}

size_t ThreadsPool::threadsCount() const
{
    return threadsVec.size();
}

bool ThreadsPool::closed() const
{
    return d_closed;
}
bool ThreadsPool::submit(CallableDoubleFunc& ftor)
{
  if (closed())
    return false;
  return submit(&ftor, 1);
}
bool ThreadsPool::submit(const WebGrep::CallableFunc_t& ftor)
{
  if (closed())
    return false;
  CallableDoubleFunc pair;
  pair.functor = ftor;
  pair.cbOnException = [](const std::exception& ex)
  {
      std::cerr << "Exception suppressed: " << ex.what() << std::endl;
  };
  return submit(&pair, 1);
}

bool ThreadsPool::submit(CallableDoubleFunc* ftorArray, size_t len,
                         IteratorFunc2_t iterFn, bool spray)
{
  if (closed())
    return false;

  size_t inc = std::max((size_t)1, len);
  d_current.fetch_add(inc, std::memory_order_acquire);
  unsigned idx = d_current.load(std::memory_order_relaxed) % threadsVec.size();

  try {
    if (!spray)
      {//case we serialize tasks just to 1 thread
        TPool_ThreadData* td = mcVec[idx].get();
        {
          std::unique_lock<std::mutex>lk(td->mu); (void)lk;
          td->enqueue(lk, ftorArray, len, iterFn);
        }
        td->notify();
        return true;
      }

    //case we serialize tasks to all threads (spraying them):
    CallableDoubleFunc* ptr = ftorArray;
    bool ok = true;
    for(size_t cnt = 0; ok; ok = iterFn(&ptr, &cnt, len))
      {
        TPool_ThreadData* td = mcVec[idx].get();
        {
          std::unique_lock<std::mutex>lk(td->mu);
          (void)lk;
          td->workQ.push_back(*ptr);
        }
        td->notify();

        d_current.fetch_add(inc, std::memory_order_acquire);
        idx = d_current.load(std::memory_order_relaxed) % threadsVec.size();
      }
  } catch(...)
  {
    return false;//on exception like bad_alloc
  }

  return true;
}
//-----------------------------------------------------------------------------
bool ThreadsPool::submit(WebGrep::CallableFunc_t* ftorArray, size_t len,
                         IteratorFunc_t iterFn, bool spray)
{
  if (closed())
    return false;

  size_t inc = std::max((size_t)1, len);
  d_current.fetch_add(inc, std::memory_order_acquire);
  unsigned idx = d_current.load(std::memory_order_relaxed) % threadsVec.size();

  try {
    if (!spray)
      {//case we serialize tasks just to 1 thread
        TPool_ThreadData* td = mcVec[idx].get();
        size_t n = 0;
        {
          std::unique_lock<std::mutex>lk(td->mu);
          n = td->enqueue(lk, ftorArray, len, iterFn);
        }
        td->notify();
        return n == len;
      }

    //case we serialize tasks to all threads (spraying them):
    CallableFunc_t* ptr = ftorArray;
    WebGrep::CallableDoubleFunc dfunc;

    bool ok = true;
    for(size_t cnt = 0; ok; ok = iterFn(&ptr, &cnt, len))
      {
        TPool_ThreadData* td = mcVec[idx].get();
        {
          std::unique_lock<std::mutex>lk(td->mu);
          td->enqueue(lk, &dfunc, 1);
        }
        td->notify();

        d_current.fetch_add(inc, std::memory_order_acquire);
        idx = d_current.load(std::memory_order_relaxed) % threadsVec.size();
      }
  } catch(...)
  {
    return false;//on exception like bad_alloc
  }

  return true;
}

TPool_ThreadDataPtr ThreadsPool::getDataHandle()
{
  if (closed())
    return nullptr;

  d_current.fetch_add(1, std::memory_order_acquire);
  unsigned idx = d_current.load(std::memory_order_relaxed) % threadsVec.size();

  return mcVec[idx];
}

void ThreadsPool::close()
{
    d_closed = true;
}

bool ThreadsPool::joined()
{
  std::lock_guard<std::mutex> lk(joinMutex);
  return threadsVec.empty();
}

void ThreadsPool::joinAll(bool terminateCurrentTasks)
{
  std::lock_guard<std::mutex> lk(joinMutex);

  close();
  for(TPool_ThreadDataPtr& dt : mcVec)
    {
      dt->terminateFlag = terminateCurrentTasks;
      dt->stopFlag = true;
      dt->cond.notify_all();
    }

  for(std::thread& t : threadsVec)
    {
      t.join();
    }
  threadsVec.clear();
  mcVec.clear();
}

void ThreadsPool::terminateDetach()
{
  std::lock_guard<std::mutex> lk(joinMutex);
  close();
  for(std::thread& t : threadsVec)
    { t.detach(); }

  for(TPool_ThreadDataPtr& dt : mcVec)
    {
      dt->terminateFlag = true;
      dt->stopFlag = true;
      dt->cond.notify_all();
    }
  threadsVec.clear();
  mcVec.clear();
}

void ThreadsPool::joinExportAll(const std::function<void(CallableDoubleFunc*, size_t)>& exportFunctor)
{
  {//set the exporing callback
    std::lock_guard<std::mutex> lk(joinMutex);
    if (threadsVec.empty())
      return;

    close();
    for(TPool_ThreadDataPtr& dt : mcVec)
      {
        dt->exportTaskFn = exportFunctor;
        dt->stopFlag = true;
      }
  }
  //terminate tasks, they'll export abandoned exec. functor
  joinAll(true);
}

}//ZMBCommon
