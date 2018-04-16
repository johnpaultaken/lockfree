#include <functional>

#include "map.h"

using std::cout;

void testcase_map()
{
    lockfree::map<int, int> m;
    m.set_value(2, 3);
    cout << m.at(2);

    lockfree::map<int, int, std::greater<int>> m2;
    m2.set_value(2, 3);
    cout << m2.at(2);
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
    m.set_value(2, 3);
    cout << m.at(2);

    lockfree::unordered_map<int, int, identity<int>, std::equal_to<int>> m2;
    m2.set_value(2, 3);
    cout << m2.at(2);
}


int main(int argc, char ** argv)
{
    testcase_map();

    testcase_unordMap();

    cout << "\ndone\n";
    getchar();
    return 0;
}
