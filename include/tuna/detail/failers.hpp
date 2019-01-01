#ifndef TUNA_DETAIL_INCLUDED_DETAIL_FAILERS
#define TUNA_DETAIL_INCLUDED_DETAIL_FAILERS

#include <boost/predef/os.h>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#if BOOST_OS_WINDOWS
    #include <boost/winapi/get_last_error.hpp>
#else
    #include <cerrno>
#endif

namespace tuna::detail {
///////////////////////////////////////////////////////////////////////////////

template <typename Self>
struct basic_failer
{
    void operator()()
    {
        static_cast<Self&>(*this)(
            #if BOOST_OS_WINDOWS
                boost::winapi::GetLastError()
            #else
                errno
            #endif
            ,
            boost::system::system_category());
    }
};

struct error_code_setter
: basic_failer<error_code_setter>
{
    explicit
    error_code_setter(boost::system::error_code& ec)
    : ec_(ec)
    { ec_.clear(); }

    template <typename... Args>
    void operator()(Args&&... args)
    { ec_ = boost::system::error_code(std::forward<Args>(args)...); }

    using basic_failer::operator();

private:
    boost::system::error_code& ec_;
};


struct system_error_thrower
: basic_failer<system_error_thrower>
{
    template <typename... Args>
    void operator()(Args&&... args)
    { throw boost::system::system_error(std::forward<Args>(args)...); }

    using basic_failer::operator();
};

///////////////////////////////////////////////////////////////////////////////
}

#endif
