#ifndef NONCOPYABLE_HPP
#define NONCOPYABLE_HPP

namespace WebGrep {

class noncopyable
{
public:
    noncopyable() = default;
    ~noncopyable() = default;
    noncopyable( const noncopyable& ) = delete;
    noncopyable& operator=( const noncopyable& ) = delete;
};

}//WebGrep
#endif // NONCOPYABLE_HPP
