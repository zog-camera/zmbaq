#include "linked_task.h"
#include <cassert>
#include <iostream>

namespace ZMBCommon {

void TraverseFunc(LinkedTask* head, void* additional,
                  void(*func)(LinkedTask*, void*))
{
  if (nullptr == head || nullptr == func) return;
  LinkedTask* next = ItemLoadAcquire(head->next);
  LinkedTask* child = ItemLoadAcquire(head->child);
  TraverseFunc(next, additional, func);
  TraverseFunc(child, additional, func);
  func(head, additional);
}

// Recursively traverse the list and call functor on each item
void TraverseFunctor(LinkedTask* head, void* additional,
                     std::function<void(LinkedTask*, void* additional/*nullptr*/)> func)
{
  if (nullptr == head || nullptr == func) return;
  LinkedTask* next = ItemLoadAcquire(head->next);
  LinkedTask* child = ItemLoadAcquire(head->child);
  TraverseFunctor(next, additional, func);
  TraverseFunctor(child, additional, func);
  func(head, additional);
}

static void DeleteCall(LinkedTask* item, void* data)
{
  (void)data;
  LinkedTask* root = ZMBCommon::ItemLoadAcquire(item->root);
  if (root == item)
    {
      delete item;
      return;
    }
  root->utilPtr->deleteNode(root, item);
}

// Free memory recursively.
void DeleteList(LinkedTask* head)
{
  TraverseFunc(head, nullptr, &DeleteCall);
}

LinkedTask* FuncNeu(LinkedTask* RootNodePtr)
{
  LinkedTask* p = nullptr;
  try {
    p = new LinkedTask(RootNodePtr);
    RootNodePtr->utilPtr->counter.fetch_add(1);
  }catch(std::bad_alloc& ba)
  {
    std::cerr << ba.what() << std::endl;
  }
  return p;
}
static inline void FuncDeleteNode(LinkedTask* RootNodePtr, LinkedTask* node_ptr)
{
    delete node_ptr;
    RootNodePtr->utilPtr->counter.fetch_sub(1);
}
//---------------------------------------------------------------
LinkedTask::LinkedTask() : level(0)
{
  order = 0;
  next.store(0);
  child.store(0);
  root.store(0);
  parent.store(0);
}

LinkedTask::LinkedTask(LinkedTask* rootNode) : LinkedTask()
{
  shallowCopy(*rootNode);
  root.store((std::uintptr_t)rootNode);
}

std::shared_ptr<LinkedTask> LinkedTask::createRootNode()
{
  std::shared_ptr<LinkedTask> rootNode;
  auto ptr = new LinkedTask;
  rootNode.reset(ptr,
             [](LinkedTask* ptr){ZMBCommon::DeleteList(ptr);});

  //allocate LTSpawnDeleteFuncs only for root node:
  rootNode->utilPtr = std::make_shared<LTSpawnDeleteFuncs>();

  LTSpawnDeleteFuncs& u (*(rootNode->utilPtr));
  u.counter.fetch_add(1);
  u.makeNewNode = &FuncNeu;
  u.deleteNode = &FuncDeleteNode;

  rootNode->root.store((std::uintptr_t)ptr);
  return rootNode;
}

//---------------------------------------------------------------
void LinkedTask::shallowCopy(const LinkedTask& other)
{
  level = other.level;
  root.store(other.root.load());
  parent.store(other.parent.load());
  utilPtr = other.utilPtr;
}

LinkedTask* LinkedTask::getLastOnLevel()
{
  std::atomic_uintptr_t* freeslot = &next;
  LinkedTask* item = ItemLoadAcquire(*freeslot);
  LinkedTask* last_item = item;
  for(; nullptr != item; item = ItemLoadAcquire(*freeslot))
    {
      freeslot = &(item->next);
      last_item = item;
    }
  if (nullptr == last_item)
    last_item = this;
  return last_item;
}

size_t LinkedTask::spawnNextNodes(size_t nodesCount)
{
  if (0 == nodesCount)
    return 0;

  LinkedTask* root = ItemLoadAcquire(this->root);

  //go to the end elemet:
  LinkedTask* last_item = getLastOnLevel();

  //spawn items on same level (access by .next)
  size_t c = 0;
  try {

    //put 1 item into last zero holder:
    LinkedTask* item = nullptr;
    for(; c < nodesCount; ++c, last_item = item)
      {
        item = root->utilPtr->makeNewNode(root);
        StoreAcquire(last_item->next, item);
        if (nullptr == item)
          break;
        item->shallowCopy(*this);
        StoreAcquire(item->parent, ItemLoadAcquire(this->parent));
        item->order = childNodesCount.load(std::memory_order_acquire);
        this->childNodesCount.fetch_add(1);
      };
  }catch(std::exception& ex)
  {
    std::cerr << ex.what() << "\n";
    std::cerr << __FUNCTION__ << "\n";
  }
  return c;
}

LinkedTask* LinkedTask::spawnChildNode(LinkedTask*& expelledChild)
{
  LinkedTask* rootNode = ItemLoadAcquire(this->root);
  expelledChild = ItemLoadAcquire(child);
  try {
    auto item = rootNode->utilPtr->makeNewNode(rootNode);
    if(nullptr == item)
      return nullptr;
    StoreAcquire(child, item);
    item->shallowCopy(*this);
    item->parent.store((std::uintptr_t)this);
    item->level = 1u + this->level;
    item->order = childNodesCount.load(std::memory_order_acquire);
    this->childNodesCount.fetch_add(1);
    return item;
  } catch(std::exception& ex)
  {
    std::cerr << __FUNCTION__ << std::endl;
    std::cerr << ex.what() << std::endl;
  }
  return nullptr;
}

LinkedTask* LinkedTask::spawnChildNode()
{
  LinkedTask* ptr = nullptr;
  auto neu = spawnChildeNode(ptr);
  if (nullptr != ptr)
    {
      ptr->utilPtr->deleteNode(ptr);
    }
  return neu;
}

size_t ForEachOnBranch(LinkedTask* head, std::function<void(LinkedTask*)> functor,
                       uint32_t skipCount)
{
  size_t sk_cnt = 0;
  LinkedTask* item = head;
  for(; nullptr != item && sk_cnt < skipCount; ++sk_cnt)
    {//traverse to skip first N elements as required in (skipCount)
      item = ItemLoadAcquire(item->next);
    }

  //process the elements with the functor:
  size_t cnt = 0;
  try {
  for(; nullptr != item; ++cnt, item = ItemLoadAcquire(item->next))
    {
      functor(item);
    }
  } catch(std::exception& ex)
  {
    std::cerr << __FUNCTION__ << " exception: " << ex.what() << std::endl;
  }
  return cnt;
}

//---------------------------------------------------------------
std::string ExtractHostPortHttp(const std::string& targetUrl)
{
  std::string url(targetUrl.data());
  size_t hpos = url.find_first_of("://");
  if (std::string::npos != hpos)
    {
      url = targetUrl.substr(hpos + sizeof("://")-1);
    }
  auto slash_pos = url.find_first_of('/');
  if (std::string::npos != slash_pos)
    {
      url.resize(slash_pos);
    }
  return url;
}

size_t FindURLAddressBegin(const char* str, size_t nmax)
{
  if (str[0] == '/')
    return 0;

  size_t pos = 1;
  const char* ptr = str;
  static const char semSlash [] = "://";
  static const size_t szSemSlash = 3;
  size_t _bound = std::min(ZMBCommon::MaxURLlen,nmax);
  for(; pos < _bound && ( 0 != ::memcmp(semSlash, ptr, szSemSlash)); ++pos)
    {
      ptr = str + pos;
    }
  if (pos < _bound)
    {
      ptr += (':'== ptr[0]? 3 : 0);
      return ptr - str;
    }
  return nmax;
}

size_t FindURLPathBegin(const char* str, size_t nmax)
{
  size_t upos = FindURLAddressBegin(str,nmax);
  if (upos <= nmax)
    upos = 0;
  const char* ptr = str;
  //look for first of '/'
  for(; ptr[0] != '/' && ptr < str + std::min(nmax,ZMBCommon::MaxURLlen); ++ptr)
    {  }
  return ptr - str;
}

size_t FindClosingQuote(const char* strPtr, const char* end)
{
  //inc pointer until we get on of stop charactres
  auto old = strPtr;
  static const char stopChars[] = "\"'\n> <\0";
  for(char c = strPtr[0]; strPtr < end; c = *(++strPtr))
    {
      for(uint32_t z = 0; z < sizeof(stopChars) - 1; ++z)
        {
          if (c == stopChars[z])
            return strPtr - old;
        }
    }
  return strPtr - old;
}

std::string MakeFullPath(ZMBCommon::ZConstString uri,
                         ZMBCommon::ZConstString host_and_port,
                         ZMBCommon::ZConstString scheme,
                         ZMBCommon::ZConstString siteAddress)
{
  size_t upos = ZMBCommon::FindURLAddressBegin(uri.begin(), uri.size());
  std::string path;
  if('/' != uri.begin()[0] && uri.size() <= upos) {
      //case it's a subdirectory without leading '/'
      path.reserve(siteAddress.size() + 2 + uri.size());
      path = siteAddress.begin();
      path += '/';
    }
  else if (0 == upos)
    {//case starts with '/': "/some/path"
      path.reserve(4 + scheme.size()
                   + host_and_port.size() + uri.size());
      path = scheme.begin(); //"http" or "https"
      path += "://";
      // append site.com:443
      path += host_and_port.begin();
    }
  // append local resource URI
  size_t appendPos = path.size();
  path.resize(path.size() + uri.size());
  ::memcpy(&path[appendPos], uri.begin(), uri.size());
  return path;
}
//-----------------------------------------------------------------

}//ZMBCommon
