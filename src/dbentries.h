#pragma once

namespace ZMB
{
  /** @brief Keeps connection information about the database, sends the requests.*/
  class DBContext
  {
  public:
    DBContext();
    void connect(const std::string& host, u_int16_t port = 4321)
    {
      (void)host; (void)port;
    };
  };

}//namespace ZMB
