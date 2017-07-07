#pragma once
#include "boost/thread/thread_pool.hpp"
#include "rethinkdb.h"


namespace ZMB
{
  typedef boost::executors::basic_thread_pool TPool;
  typedef std::shared_ptr<TPool> TPoolSharedPtr;
  typedef std::unique_ptr<RethinkDB::Connection> ConnectionUPtr;

  struct DBAuthParams
  {
    std::string host = "localhost";
    int port = 28015;
    std::string auth_key; //empty by default

    DBAuthParams() { }
    DBAuthParams(const std::string& aHost, int aPort, std::string aAuthKey)
      : host(aHost), port(aPort), auth_key(aAuthKey)
    {  }
  };

  //tries to connect
  struct DBConnectionIgnitor
  {
    typedef std::function<void(DBAuthParams, std::string whatMsg)> FnFailFunc;

    FnFailFunc onFailFunc; //< on connection failure

    struct SensitiveData
    {
      ConnectionUPtr conn;
      TPoolSharedPtr threadsPool;

      DBAuthParams params;   //< connection params
      FnFailFunc onFailFunc; //< on connection failure
    };
    
    template<class TActionOnConnected /*void operator()(SensitiveData&&)*/>
    struct OpConnect
    {
      SensitiveData data;
      TActionOnConnected fnPostFactum;

      void operator ()()
      {
        try {
          //establish connection
          DBAuthParams& p(data.params);
          ConnectionUPtr context(std::move(RethinkDB::connect(p.host, p.port, p.auth_key)));
          fnPostFactum.operator()(std::move(data));
        } catch(const std::exception& ex)
        {
	  //call functor on exception
          onFailFunc.operator()(p, ex.what());
        }
      }
    };

    DBConnectionIgnitor(FnFailFunc onConnectionFail = [](DBAuthParams, std::string whatMsg) { })
    : onFailFunc(onConnectionFail)
    {    }

    template<class TActionOnConnected>
    void operator()(DBAuthParams params, TPoolSharedPtr threadsPool, TActionOnConnected&&  postFactumActions)
    {
      OpConnect<TActionOnConnected> op;
      op.fnPostFactum = std::move(postFactumActions);
      op.data.threadsPool = threadsPool;
      op.data.onFailFunc = this->onFailFunc;
      op.data.params = params;
      threadsPool->submit(op);
    }
  };

//  /** @brief Keeps connection information about the database, sends the requests.*/
//  class DBContext
//  {
//  public:

//    //the constructor must acquire RethingDB connection context.
//  DBContext(ConnectionUPtr&& activeConnection) : conn(std::move(activeConnection))
//      { }

//  private:
//    ConnectionUPtr&& conn;
//  };


}//namespace ZMB

