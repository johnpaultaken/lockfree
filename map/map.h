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
1.  Usage of this map is recommended only if the number of expected reads are
    much larger than the number of expected writes, during its steady state of
    operation, ie after initialization.
2.  No members are provided that return iterator because such a functionality
    will require modifiability of the atomic container using the returned
    iterator.
    However members that return const_iterator are provided.
3.  To avoid locks in memory allocators you must use one of the per-thread
    allocators like a not too old version of libc malloc
    (or tcmalloc / jemalloc etc). If you have multi-threaded code, you are most
    likely already using one.
4.  Do not use -pthread compiler option in gcc because
    using that option seems to trigger lock based implementation.
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
    using container_type = this_type;
    using key_type = typename implementation_type::key_type;
    using mapped_type = typename implementation_type::mapped_type;
    using value_type = typename implementation_type::value_type;
    using size_type = typename implementation_type::size_type;
    using allocator_type = typename implementation_type::allocator_type;

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
    // Copy constructor.
    //
    map_template (const this_type & other)
    {
        auto other_implementation = atomic_load (& (other.implementation_));

        // copy construct implementation
        auto implementation = (
            std::make_shared <implementation_type> (* other_implementation)
        );

        atomic_store (& implementation_, implementation);
    }

    //
    // Move constructor.
    //
    map_template (this_type && other)
    {
        auto implementation = std::make_shared <implementation_type> ();

        auto other_implementation = (
            atomic_exchange (& (other.implementation_), implementation)
        );

        atomic_store (& implementation_, other_implementation);
    }

    //
    // Move constructor from implementation type.
    // For initialization of lockfree::map it is recommended to use this
    // constructor (or the following move assignment from implementation type)
    // for the sake of efficiency.
    //
    map_template (implementation_type && imp)
    {
        if (! atomic_is_lock_free (& implementation_))
        {
            std::cerr << "\nlockfree::map not supported by this platform."
                      << " Falling back to lock based implementation.";
        }

        // This call will invoke the move constructor of implementation_type.
        auto implementation = std::make_shared <implementation_type> (
            std::forward <implementation_type> (imp)
        );

        atomic_store (& implementation_, implementation);
    }

    //
    // Copy assignment.
    //
    this_type & operator = (const this_type & other)
    {
        if (this != & other)
        {
            auto other_implementation = atomic_load (& (other.implementation_));

            // clone other_implementation by copy construction.
            auto implementation = (
                std::make_shared <implementation_type> (* other_implementation)
            );

            atomic_store (& implementation_, implementation);
        }

        return * this;
    }

    //
    // Move assignment.
    //
    this_type & operator = (this_type && other)
    {
        if (this != & other)
        {
            auto implementation = std::make_shared <implementation_type> ();

            auto other_implementation = (
                atomic_exchange (& (other.implementation_), implementation)
            );

            atomic_store (& implementation_, other_implementation);
        }

        return * this;
    }

    //
    // Move assignment from implementation type.
    // For initialization of lockfree::map it is recommended to use this
    // function (or the above move constructor from implementation type)
    // for the sake of efficiency.
    //
    void operator = (implementation_type && imp)
    {
        // This call will invoke the move constructor of implementation_type.
        auto implementation = std::make_shared <implementation_type> (
            std::forward <implementation_type> (imp)
        );

        atomic_store (& implementation_, implementation);
    }

    //
    // Cannot swap two atomics atomically.
    //
    void swap (this_type & other) = delete;

    //
    // Similar to std::map at(key).
    // Difference: Unlike std::map this function does not return a const
    // reference but instead returns by value.
    // Cannot return mapped by reference because the lifetime of the
    // implementation is only guaranteed until the return of this function,
    // whereas the returned reference to mapped may be held beyond that.
    //
    mapped_type at (const key_type & key) const
    {
        auto implementation = atomic_load (&implementation_);

        return implementation->at (key);
    }

    //
    // The following class is to support indexing operation of lockfree::map.
    // It provides a wrapper for a non-const reference to mapped_type.
    //
    class reference_to_mapped
    {
    public:
        operator mapped_type ()
        {
            return pContainer_->get_mapped (key_);
        }

        reference_to_mapped & operator = (const mapped_type & mapped)
        {
            pContainer_->set_mapped (key_, mapped);
            return * this;
        }

    private:
        reference_to_mapped (
            container_type * pContainer,
            const key_type & key
        ) : pContainer_(pContainer), key_(key)
        {
        }

        friend container_type;

    private:
        container_type * pContainer_;
        key_type key_;
    };

    reference_to_mapped operator [] (const key_type & key)
    {
        return reference_to_mapped {this, key};
    }

    bool empty () const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        return implementation->empty ();
    }

    size_type size () const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        return implementation->size ();
    }

    template <class InputIterator>
    void insert (InputIterator first, InputIterator last)
    {
        auto expected = atomic_load (& implementation_);
        shared_ptr <implementation_type> desired;
        do
        {
            // clone implementation_type by copy construction.
            desired = std::make_shared <implementation_type> (* expected);

            desired->insert (first, last);
        } while ( !
            atomic_compare_exchange_weak (
                & implementation_, & expected, desired
            )
        );
    }

    size_type erase (const key_type & key)
    {
        size_type count = 0;

        // has_key check is for efficiency only.
        // It doesn't have to be atomic with erase itself.
        if (has_key (key))
        {
            auto expected = atomic_load (& implementation_);
            shared_ptr <implementation_type> desired;
            do
            {
                // clone implementation_type by copy construction.
                desired = std::make_shared <implementation_type> (* expected);

                count = desired->erase (key);
            } while ( !
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
        auto implementation = std::make_shared <implementation_type> ();

        atomic_store (& implementation_, implementation);
    }

    //
    // const_iterator holds a reference to the Implementation inside the
    // iterator to guarantee lifetime of the Implementation object.
    //
    class const_iterator : public implementation_type::const_iterator
    {
    private:
        const_iterator (
            typename implementation_type::const_iterator && base,
            const shared_ptr <const implementation_type> & implementation
        ) :
            implementation_type::const_iterator {std::move (base)},
            implementation_ {implementation}
        {
        }

        shared_ptr <const implementation_type> implementation_;

        friend container_type;
    };

    const_iterator begin () const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        return const_iterator {implementation->cbegin (), implementation};
    }

    const_iterator end () const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        return const_iterator {implementation->cend (), implementation};
    }

    const_iterator cbegin () const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        return const_iterator {implementation->cbegin (), implementation};
    }

    const_iterator cend () const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        return const_iterator {implementation->cend (), implementation};
    }

    const_iterator find (const key_type & key) const
    {
        // note: specify type as const explicitly here to make sure
        // the const version of find() gets called next.
        shared_ptr <const implementation_type> implementation = (
            atomic_load (& implementation_)
        );

        return const_iterator {implementation->find (key), implementation};
    }

    std::pair<const_iterator,const_iterator>
    equal_range (const key_type & key) const
    {
        // note: specify type as const explicitly here to make sure
        // the const version of equal_range() gets called next.
        shared_ptr <const implementation_type> implementation = (
            atomic_load (& implementation_)
        );

        auto itr_pair = implementation->equal_range (key);

        return std::make_pair (
            const_iterator {std::move (itr_pair.first), implementation},
            const_iterator {std::move (itr_pair.second), implementation}
        );
    }

    allocator_type get_allocator () const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        return implementation->get_allocator ();
    }

    size_type count (const key_type & key) const
    {
        auto implementation = atomic_load (& implementation_);

        return implementation->count (key);
    }

    size_type max_size () const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        return implementation->max_size ();
    }

protected:
    //
    // Unexposed member functions.
    // These are APIs unavailable in std::map.
    // If clients start using these, switching between lockfree::map & std::map
    // is no longer easy.
    //

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
    // check whether a value is present in the map ie
    // check whether a key is mapped to a given mapped.
    //
    bool has_value (
        const key_type & key,
        const mapped_type & mapped
    ) const noexcept
    {
        auto implementation = atomic_load (& implementation_);

        auto itr = implementation->find (key);
        if (itr != implementation->end ())
        {
            return itr->second == mapped ? true : false;
        }

        return false;
    }

    //
    // fetch the mapped for a key.
    // If the key does not exist in the map, it is added to the map with a
    // value initialized mapped.
    // This is equivalent to the following std::map statement
    // map[key];
    //
    mapped_type get_mapped (const key_type & key)
    {
        mapped_type mapped;

        // at() invocation is for efficiency only.
        try
        {
            mapped = at (key);
        }
        catch (const std::out_of_range &)
        {
            auto expected = atomic_load (& implementation_);
            shared_ptr <implementation_type> desired;
            do
            {
                // clone implementation_type by copy construction.
                desired = std::make_shared <implementation_type> (* expected);

                mapped = (* desired) [key];
            } while ( !
                atomic_compare_exchange_weak (
                    & implementation_, & expected, desired
                )
            );
        }

        return mapped;
    }

    //
    // map a key to a given mapped.
    // If the key already exists in the map, its mapped is updated only if it
    // is different from the one specified.
    // This is equivalent to the following std::map statement
    // map[key] = mapped;
    //
    void set_mapped (const key_type & key, const mapped_type & mapped)
    {
        // has_value check is for efficiency only.
        // It doesn't have to be atomic with setting value.
        if (! has_value(key, mapped))
        {
            auto expected = atomic_load (& implementation_);
            shared_ptr <implementation_type> desired;
            do
            {
                // clone implementation_type by copy construction.
                desired = std::make_shared <implementation_type> (* expected);

                (* desired) [key] = mapped;
            } while ( !
                atomic_compare_exchange_weak (
                    & implementation_, & expected, desired
                )
            );
        }
    }

private:
    //
    // private data members.
    //

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
