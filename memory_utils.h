#pragma once

#include <memory>

template <class T, class U>
class unique_ptr_ptr
{
public:
    typedef std::unique_ptr<T, U> unique_ptr;

    unique_ptr_ptr(unique_ptr& up)
        : up(up), p(nullptr)
    {
    }

    ~unique_ptr_ptr()
    {
        up.reset(p);
    }

    operator typename unique_ptr::pointer* ()
    {
        return &p;
    }

private:
    unique_ptr& up;
    typename unique_ptr::pointer p;
};

template<class T, class U>
typename unique_ptr_ptr<T, U> operator&(std::unique_ptr<T, U>& up)
{
    return unique_ptr_ptr<T, U>(up);
}

template<class T, class U>
typename std::unique_ptr<T, U>::pointer operator-(std::unique_ptr<T, U>& up)
{
    return up.get();
}
