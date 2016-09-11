#ifndef NONCOPYABLE_HPP
#define NONCOPYABLE_HPP

namespace ZMBCommon {

class noncopyable
{
public:
    noncopyable() = default;
    ~noncopyable() = default;
    noncopyable( const noncopyable& ) = delete;
    noncopyable& operator=( const noncopyable& ) = delete;
};

}//ZMBCommon
#endif // NONCOPYABLE_HPP
