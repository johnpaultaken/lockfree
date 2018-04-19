//----------------------------------------------------------------------------
// year   : 2018
// author : John Paul
// email  : johnpaultaken@gmail.com
// source : https://github.com/johnpaultaken
// description :
//      Lock-free wait-free implementation of map and unordered_map in C++11.
//----------------------------------------------------------------------------

#pragma once

#include <map>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <iostream>

/*
Notes:
1.  No members are provided that return iterator because such a functionality
    will require modifiability of the atomic container using the returned
    iterator.
    However members that return const_iterator can be provided if required.
    This can be done by holding a reference to the Implementation inside the
    iterator to guarantee lifetime of the Implementation object.
2.  Do not use -pthread compiler option in gcc because
    using that option seems to trigger lock based implementation.
3.  To avoid locks in memory allocators you must use one of the per-thread
    allocators like a not too old version of libc malloc
    (or tcmalloc / jemalloc etc). If you have multi-threaded code, you are most
    likely already using one.
*/

namespace lockfree
{

using std::shared_ptr;

template <
    typename Implementation
>
class map_template
{
public:
    using implementation_type = Implementation;
    using this_type = map_template <implementation_type>;
    using key_type = typename implementation_type::key_type;
    using mapped_type = typename implementation_type::mapped_type;
    using value_type = typename implementation_type::value_type;
    using size_type = typename implementation_type::size_type;

    map_template ()
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
    // No copy constructor.
    //
    map_template (const this_type &) = delete;

    //
    // No move constructor.
    //
    map_template (this_type &&) = delete;

    //
    // Move constructor from implementation type.
    // If lockfree::map will be initialized by single-threaded code,
    // it is recommended to use this constructor for efficiency.
    // Using set_value for initialization is required only for
    // multi-threaded initialization.
    //
    map_template (implementation_type && imp)
    {
        if (! atomic_is_lock_free (& implementation_))
        {
            std::cerr << "\nlockfree::map not supported by this platform."
                      << " Falling back to lock based implementation.";
        }

        // This call will invoke the move constructor.
        auto implementation = std::make_shared <implementation_type> (
            std::forward <implementation_type> (imp)
        );

        atomic_store (& implementation_, implementation);
    }

    //
    // No copy assignment.
    //
    this_type & operator = (const this_type &) = delete;

    //
    // No move assignment.
    //
    this_type & operator = (this_type &&) = delete;

    //
    // map a key to a given value.
    // If key already exists in map, its value is updated if need be.
    //
    void set_value (const key_type & key, const mapped_type & val)
    {
        // has_value check is for efficiency only.
        // So it doesn't have to be atomic with setting value.
        if (! has_value(key, val))
        {
            auto expected = atomic_load (& implementation_);
            shared_ptr <implementation_type> desired;
            do
            {
                // clone implementation_type by copy construction.
                desired = std::make_shared <implementation_type> (* expected);

                (* desired) [key] = val;
            } while (
                atomic_compare_exchange_weak (
                    & implementation_, & expected, desired
                )
            );
        }
    }

    //
    // Similar to std::map empty().
    //
    bool empty () const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        return implementation->empty ();
    }

    //
    // Similar to std::map size().
    //
    size_type size () const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        return implementation->size ();
    }

    //
    // check whether a key is present in the map.
    //
    bool has_key (const key_type & key) const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        auto itr = implementation->find (key);
        if (itr != implementation->end ())
        {
            return true;
        }

        return false;
    }

    //
    // check whether a key is mapped to a given value in the map.
    //
    bool has_value (
        const key_type & key,
        const mapped_type & val
    ) const noexcept
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
    // Similar to std::map at(key).
    // Difference: Unlike std::map this function cannot return a reference
    // because the lifetime of the implementation is only guaranteed until
    // return of this function.
    //
    mapped_type at (const key_type & key) const
    {
        auto implementation = atomic_load (&implementation_);

        return implementation->at (key);
    }

    //
    // Similar to std::map erase(key).
    //
    size_type erase (const key_type & key)
    {
        size_type count = 0;

        // has_key check is for efficiency only.
        // So it doesn't have to be atomic with erase itself.
        if (has_key (key))
        {
            auto expected = atomic_load (& implementation_);
            shared_ptr <implementation_type> desired;
            do
            {
                // clone implementation_type by copy construction.
                desired = std::make_shared <implementation_type> (* expected);

                count = desired->erase (key);
            } while (
                atomic_compare_exchange_weak (
                    & implementation_, & expected, desired
                )
            );
        }

        return count;
    }

    //
    // Similar to std::map clear().
    // Difference: can throw bad_alloc.
    //
    void clear ()
    {
        // empty check is for efficiency only.
        // So it doesn't have to be atomic with clear itself.
        if (! empty ())
        {
            auto expected = atomic_load (& implementation_);
            shared_ptr <implementation_type> desired;
            do
            {
                // clone implementation_type by copy construction.
                desired = std::make_shared <implementation_type> (* expected);

                desired->clear ();
            } while (
                atomic_compare_exchange_weak (
                    & implementation_, & expected, desired
                )
            );
        }
    }

private:
    shared_ptr <implementation_type> implementation_;
};

template <
    typename Key,
    typename Mapped,
    typename Predicate = std::less <Key>,
    typename Allocator = std::allocator <std::pair <const Key, Mapped>>
>
using map = map_template <std::map <Key, Mapped, Predicate, Allocator>>;


template <
    typename Key,
    typename Mapped,
    typename Hash = std::hash<Key>,
    typename Predicate = std::equal_to <Key>,
    typename Allocator = std::allocator <std::pair <const Key, Mapped>>
>
using unordered_map = map_template <
    std::unordered_map <Key, Mapped, Hash, Predicate, Allocator>
>;

}
