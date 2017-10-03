#pragma once

namespace ThreadPoolTests {

  /** Test submission of ThreadPool tasks from data structure with custom iterator functor.*/
  bool test1();

  /** Test submission of tasks from continuous array and of single functor.*/
  bool test2();

  /** Test joibExportAll() that lets you to get abandoned tasks.*/
  bool test3();

  //accumulative test:
  bool Test();
}

