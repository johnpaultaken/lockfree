#include <functional>
#include <string>

#include "map.h"

using std::cout;

void assert_t(bool b, const std::string & what)
{
    cout << (b ? ("\nOK : " + what) : ("\nFAIL : " + what));
}

void fail(const std::string & what)
{
    assert_t(false, what);
}

void pass(const std::string & what)
{
    assert_t(true, what);
}

void testcase_map()
{
    lockfree::map<int, int> m;
    m[0] = 1;
    m[2] = 3;
    m[4] = 5;
    assert_t(m[2] == 3, __FUNCTION__);

    try {
        auto v = m.at(100);
        fail(__FUNCTION__);
    }
    catch (const std::out_of_range &)
    {
        pass(__FUNCTION__);
    }

    if (0 == m[101])
    {
        pass(__FUNCTION__);
    }
    else
    {
        fail(__FUNCTION__);
    }

    lockfree::map<int, int, std::greater<int>> m2;
    m2[0] = 1;
    m2[2] = 3;
    m2[4] = 5;
    assert_t(m2.at(2) == 3, __FUNCTION__);

    m2.find(2);
    m2.lower_bound(2);
    m2.upper_bound(2);
    m2.equal_range(2);
}

template <typename T>
struct identity
{
    T operator() (T p) const
    {
        return p;
    }
};

void testcase_unordMap()
{
    lockfree::unordered_map<int, int> m;
    m[0] = 1;
    m[2] = 3;
    m[4] = 5;
    assert_t(m.at(2) == 3, __FUNCTION__);

    try {
        auto v = m.at(100);
        fail(__FUNCTION__);
    }
    catch (const std::out_of_range &)
    {
        pass(__FUNCTION__);
    }

    if (0 == m[101])
    {
        pass(__FUNCTION__);
    }
    else
    {
        fail(__FUNCTION__);
    }

    lockfree::unordered_map<int, int, identity<int>, std::equal_to<int>> m2;
    m2[0] = 1;
    m2[2] = 3;
    m2[4] = 5;
    assert_t(m2[2] == 3, __FUNCTION__);
}

template <typename K, typename M>
class my_map: public std::map<K,M>
{
public:
    const std::string msg_move_constructor{"copy constructor invoked must be move constructor."};

    my_map() = default;

    my_map(const my_map &)
    {
        fail(msg_move_constructor);
    }

    my_map(my_map &&)
    {
        pass(msg_move_constructor);
    }
};

template <typename K, typename M>
using my_lockfree_map = lockfree::map_template <my_map <K, M>>;

void testcase_myMap()
{
    my_map<int,int> m;
    m.insert(std::make_pair(0, 1));
    m.insert(std::make_pair(2, 3));
    m.insert(std::make_pair(4, 5));

    // check move constructor invoke.
    my_lockfree_map<int, int> ml(std::move(m));
}

int main(int argc, char ** argv)
{
    testcase_map();

    testcase_unordMap();

    testcase_myMap();

    cout << "\ndone\n";
    getchar();
    return 0;
}
