//----------------------------------------------------------------------------
// year   : 2018
// author : John Paul
// email  : johnpaultaken@gmail.com
//----------------------------------------------------------------------------

#pragma once

#include <map>
#include <memory>
#include <atomic>
#include <iostream>

namespace lockfree
{

using std::shared_ptr;

template <
    typename Key,
    typename Mapped,
    template <
        typename K,
        typename M,
        typename P,
        typename A
    >
    typename Implementation = std::map,
    typename Predicate = std::less <Key>,
    typename Allocator = std::allocator <std::pair <const Key, Mapped>>
>
class map
{
public:
    using implementation_type = Implementation<Key, Mapped, Predicate, Allocator>;
    using key_type = typename implementation_type::key_type;
    using mapped_type = typename implementation_type::mapped_type;
    using value_type = typename implementation_type::value_type;

    map ()
    {
        if (! atomic_is_lock_free (& implementation_))
        {
            std::cerr << "\nlockfree::map not supported by this platform."
                            << " Falling back to lock based implementation.";
        }

        auto implementation = std::make_shared <implementation_type> ();

        atomic_store (& implementation_, implementation);
    }

    //
    // check whether a key is mapped to a given value in the map.
    //
    bool has_value (const key_type & key, const mapped_type & val) const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        auto itr = implementation->find (key);
        if (itr != implementation->end ())
        {
            return itr->second == val ? true : false;
        }

        return false;
    }

    //
    // map a key to a given value.
    // If key already exists in map, its value is updated if need be.
    //
    void set_value (const key_type & key, const mapped_type & val)
    {
        if (! has_value(key, val))
        {
            auto expected = atomic_load (& implementation_);
            shared_ptr <implementation_type> desired;
            do
            {
                // clone implementation_type by copy construction.
                desired = std::make_shared <implementation_type> (* expected);

                (* desired) [key] = val;
            } while (atomic_compare_exchange_weak (& implementation_, & expected, desired));
        }
    }

    //
    // Note: Unlike std::map this function cannot return a reference
    // because this function is read only.
    // It cannot also return a const reference because the lifetime of
    // implementation is only guaranteed until return.
    //
    mapped_type at(const key_type & key) const
    {
        auto implementation = atomic_load (&implementation_);

        return implementation->at (key);
    }
private:
    shared_ptr <implementation_type> implementation_;
};

}
