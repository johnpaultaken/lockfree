//----------------------------------------------------------------------------
// year   : 2018
// author : John Paul
// email  : johnpaultaken@gmail.com
//----------------------------------------------------------------------------

#pragma once

#include <map>
#include <memory>
#include <atomic>

namespace lockfree
{

using std::shared_ptr;

template <typename K, typename M>
class map
{
public:
    using implementation_type = std::map <K,M>;
    using value_type = typename implementation_type::value_type;
    using iterator = typename implementation_type::iterator;

    //std::pair <value_type, bool> insert (value_type &&);

    std::pair <value_type, bool> insert (const value_type & item)
    {
        auto implementation = atomic_load (& implementation_);
        implementation->insert (item);
    }
private:
    shared_ptr <implementation_type> implementation_;
};

}
