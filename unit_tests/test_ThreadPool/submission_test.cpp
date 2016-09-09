#include "webgrep/thread_pool.h"
#include "webgrep/linked_task.h"
#include <list>
#include <chrono>
#include "submission_test.h"

int main(int argc, char** argv)
{
  bool result = ThreadPoolTests::Test();
  return (int)!result;
}

namespace ThreadPoolTests {
//=============================================================================

using namespace WebGrep;

//helper class for test1() : linked list with functors
class LList : public std::enable_shared_from_this<LList>
{
public:
  explicit LList(std::atomic_uint& ref, uint32_t i = 0)
    : pref(&ref), idx(i)
  {
    //set up a sinle task functor
    dfunc.functor = [this](){ ;
        pref->fetch_add(1);
        throw std::logic_error("just checking reaction for fake error...");
      };
    dfunc.cbOnException = [this](const std::exception& ex)
    {
      };
  }
  uint32_t idx;
  WebGrep::CallableDoubleFunc dfunc;
  std::shared_ptr<LList> next;
  std::atomic_uint* pref;
};


//helper class for test1(), starts a task and checks the result
class TestItems
{
public:
  static const unsigned _N = 50;
  static const unsigned _N_dispatch_tests = 10;

  TestItems() : result(false)
  {
    g_cnt.store(0);
    ltask = LinkedTask::createRootNode();
    spawned = ltask->spawnNextNodes(_N);
    head = std::make_shared<LList>(g_cnt, 0);
    list_cur_ptr = head;

    uint32_t cnt = 0;
    WebGrep::ForEachOnBranch(ltask.get(),
                             [&cnt, this](LinkedTask*)
    {
        list_cur_ptr->next = std::make_shared<LList>(g_cnt, 1000 + (++cnt));
        list_cur_ptr = list_cur_ptr->next;
      }, 0);

  }
  bool success()
  {
    auto value = g_cnt.load();
    return _N_dispatch_tests * (2 + _N) == value;
  }
  void dispatch(ThreadsPool& pool)
  {
    list_cur_ptr = head;

    //the iterator functor:
    ifunc = [this](WebGrep::CallableDoubleFunc** pptr, size_t*, size_t) -> bool
    {//a functor that is iteration interface on LList class items
      if (nullptr != list_cur_ptr->next)
        {
          *pptr = &(list_cur_ptr->dfunc);
          list_cur_ptr = list_cur_ptr->next;
          return true;
        }
      return false;
    };
    //submit array of tasks with custom iteration functor, no spray (all to same thread)
    pool.submit(&(head->dfunc), 0, ifunc, false);
  }

  bool result;
  std::atomic_uint g_cnt;
  std::shared_ptr<WebGrep::LinkedTask> ltask;
  size_t spawned;
  //functor that works as iterator on head->dfunc tasks within the linked list
  WebGrep::IteratorFunc2_t ifunc;
  std::shared_ptr<LList> head, list_cur_ptr;
};

/** TODO: fix this test, it's body is complicated.
 * Test submission of ThreadPool tasks from data structure with custom iterator functor.*/
bool test1()
{

  ThreadsPool pool(6);

  //array of test items, each will dispatch own tasks
  std::vector<std::shared_ptr<TestItems>> testArray;
  testArray.resize(10);
  for(std::shared_ptr<TestItems>& test : testArray)
    {
      test = std::make_shared<TestItems>();
    }


  //threaded dispatch : one thread for each test
  std::vector<std::thread> threadsArray;
  threadsArray.resize(testArray.size());

  unsigned idx = 0;
  for(std::thread& thr: threadsArray)
    {
      std::shared_ptr<TestItems> testp = testArray[idx++];
      thr = std::thread
      (
         [testp, &pool](){
         //submit each item (TestItems::_N_dispatch_tests) times
         for(uint32_t z = 0; z < TestItems::_N_dispatch_tests; ++z)
           {
             testp->dispatch(pool);
             //make time shift:
             std::this_thread::sleep_for(std::chrono::milliseconds(TestItems::_N_dispatch_tests - z));
           }
        }
      );
    }
  //join submission threads:
  for(std::thread& thr: threadsArray)
    { thr.join(); }

  //join pool threads in separate thread (just for test)
  std::thread t([&pool](){pool.joinAll();});
  t.join();
  //check success:
  bool ok = true;
  for(std::shared_ptr<TestItems>& test : testArray)
    {
      ok = ok && test->success();
    }
  return ok;
}
//--------------------------------------------------------------
/** Test submission of tasks from continuous array and of single functor.*/
bool test2()
{
  std::atomic_uint g_cnt2;
  g_cnt2.store(0);
  ThreadsPool pool2(5);
  std::array<WebGrep::CallableDoubleFunc, 128> funcArray;
  for(WebGrep::CallableDoubleFunc& dfunc : funcArray)
    {
      dfunc.functor = [&g_cnt2]()
      {
        g_cnt2.fetch_add(1, std::memory_order_acquire);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        throw std::logic_error("fake error");
      };
      dfunc.cbOnException = [](const std::exception&){};
    }
  pool2.submit(funcArray.data(), funcArray.size());
  for(WebGrep::CallableDoubleFunc& dfunc : funcArray)
    {
      pool2.submit(dfunc);
    }
  pool2.close();
  pool2.joinAll();
  auto val = g_cnt2.load();
  std::cerr << "val = " << val << "\n";
  return 2 * funcArray.size() == g_cnt2.load();
}
//--------------------------------------------------------------
/** Test joibExportAll() that lets you to get abandoned tasks.*/
bool test3()
{
  std::vector<WebGrep::CallableDoubleFunc> tasksLeft;
  ThreadsPool pool(3);

  //threaded dispatch : one thread for each test
  std::vector<std::thread> threadsArray;
  threadsArray.resize(5);

  std::atomic_uint counter, dispatchCnt;
  counter.store(0);
  dispatchCnt.store(0);

  volatile bool stopFlag = false;

  for(std::thread& thr: threadsArray)
    {
      thr = std::thread
      (
         [ &pool, &dispatchCnt, &counter, &stopFlag](){
         for(uint32_t z = 0; z < 1000 && !stopFlag; ++z)
           {
             pool.submit([&counter](){counter.fetch_add(1);});
             dispatchCnt.fetch_add(1);
           }
        }
      );
    }
  {//forse to stop the submission threads
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stopFlag = true;
    for(std::thread& thr: threadsArray)
      { thr.join(); }
  }

  //task export functor:
  std::function<void(CallableDoubleFunc*, size_t)> taskSavior = [&tasksLeft](CallableDoubleFunc* abandonedTasksArray, size_t len)
  {
    static std::mutex vectorLock;
    std::lock_guard<std::mutex> lk(vectorLock); (void)lk;
    for(size_t c = 0; c < len; ++c)
      {
        tasksLeft.push_back(abandonedTasksArray[c]);
      }
  };
  pool.joinExportAll(taskSavior);

  //execute the abandoned tasks here:
  for(WebGrep::CallableDoubleFunc& lonelyTask : tasksLeft)
    {
      lonelyTask.functor();
    }
  //check the sum
  auto value = counter.load();
  return value == dispatchCnt.load();
}
//--------------------------------------------------------------
bool Test()
{
  typedef std::pair<std::string, std::function<bool()>> NamedTask;
  std::list<NamedTask> testsList;
  testsList.push_back
      ( NamedTask("test ThreadsPool.submit for sparse arrays of tasks with custom iterators: ",
                  []()->bool {return test1();}) );
  testsList.push_back
      ( NamedTask("test ThreadsPool.submit for continuos array of func.tasks: ",
                  []()->bool {return test2();}) );
  testsList.push_back
      ( NamedTask("test ThreadsPool.joinExportAll for abandoned tasks export: ",
                  []()->bool {return test3();}) );

  bool ok = true;

  try {
    for(NamedTask& t : testsList)
      {
        bool res = t.second();
        std::string msg = res? "PASSED." : "FAILED.";
        std::cerr << t.first << msg << std::endl;
        ok = ok && res;
      }

  } catch(std::exception& ex)
  {
    std::cerr << __FUNCTION__ << " test failed: " << ex.what() << std::endl;
    return false;
  }
  return ok;
}
//=============================================================================


}//WebGrepTests
