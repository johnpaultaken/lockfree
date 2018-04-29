#include <functional>
#include <algorithm>
#include <string>
#include <cstring>
#include <sstream>
#include <list>

#include "map.h"

using std::cout;

void assert_m(
    bool cond,
    const std::string & what,
    const std::string & func,
    int line
)
{
    auto filepath = __FILE__;
    auto filename = std::max(
        filepath,
        std::max(strrchr(filepath, '\\'), strrchr(filepath, '/')) + 1
    );

    std::ostringstream msg;
    msg << (cond ? "\nOK : " : "\nFAIL : ")
            << func 
            << " at "<< filename << ":" << line << " "
            << what;

    cout << msg.str();
}

#define ASSERT_M(cond, what) assert_m(cond, what, __func__, __LINE__ );
#define FAIL_M(what) assert_m(false, what, __func__, __LINE__ );
#define PASS_M(what) assert_m(true, what, __func__, __LINE__ );

#define ASSERT(cond) assert_m(cond, "", __func__, __LINE__ );
#define FAIL() assert_m(false, "", __func__, __LINE__ );
#define PASS() assert_m(true, "", __func__, __LINE__ );



template<class Map>
void test_construct_assign(Map &)
{
    // default constructor
    Map m1;
    ASSERT_M(m1.size() == 0, "default constructor");

    // move constructor from imp 
    typename Map::implementation_type imp2{ { 1,2 },{ 3,4 },{ 5,6 },{ 7,8 } };
    Map m2{ std::move(imp2) };
    ASSERT_M(imp2.size() == 0, "move constructor from imp");
    ASSERT_M(m2.size() == 4, "move constructor from imp");

    // copy constructor
    Map m3{ m2 };
    ASSERT_M(m2.size() == 4, "copy constructor");
    ASSERT_M(m3.size() == 4, "copy constructor");

    // move constructor
    Map m4{ std::move(m3) };
    ASSERT_M(m3.size() == 0, "move constructor");
    ASSERT_M(m4.size() == 4, "move constructor");

    // copy assignment
    typename Map::implementation_type imp5{ { 0,1 },{ 2,3 } };
    Map m5{ std::move(imp5) };
    m5 = m4;
    ASSERT_M(m4.size() == 4, "copy assignment");
    ASSERT_M(m5.size() == 4, "copy assignment");

    // move assignment
    typename Map::implementation_type imp6{ { 0,1 },{ 2,3 } };
    Map m6{ std::move(imp6) };
    m6 = std::move(m5);
    ASSERT_M(m5.size() == 0, "move assignment");
    ASSERT_M(m6.size() == 4, "move assignment");

    // move assignment from imp
    typename Map::implementation_type imp6b{ { 0,1 },{ 2,3 } };
    m6 = std::move(imp6b);
    ASSERT_M(m6.size() == 2, "move assignment from imp");
}

template<class Map>
void test_read(Map &)
{
    Map m1{
        typename Map::implementation_type{ { 1,2 },{ 3,4 },{ 5,6 },{ 7,8 } }
    };

    // at() api
    ASSERT_M(m1.at(5) == 6, "at");
    try
    {
        m1.at(9);
        FAIL_M("at");
    }
    catch (const std::out_of_range &)
    {
        PASS_M("at");
    }

    // indexing
    ASSERT_M(m1[5] == 6, "indexing");
    ASSERT_M(m1[9] == 0, "indexing");

    // iteration
    std::list<int> expected{ 1,3,5,7,9 };
    std::list<int> actual;
    for (auto item : m1)
    {
        actual.push_back(item.first);
    }
    actual.sort();  // needed for unordered_map
    ASSERT_M(actual == expected, "iteration");

    // find
    ASSERT_M(m1.find(5)->second == 6, "find");
    ASSERT_M(m1.find(11) == m1.end(), "find end");

    // equal_range
    ASSERT_M(m1.equal_range(5).first->second == 6, "equal_range");
    ASSERT_M(m1.equal_range(5).second->second == 8, "equal_range");

    //count
    ASSERT_M(m1.count(5) == 1, "count");
    ASSERT_M(m1.count(11) == 0, "count");

    // max_size
    ASSERT_M(m1.max_size() > 1, "max_size");
}

template<class Map>
void test_write(Map &)
{
    Map m1{
        typename Map::implementation_type{ { 1,2 },{ 3,4 },{ 5,6 },{ 7,8 } }
    };

    // indexing
    m1[5] = 99;
    ASSERT_M(m1.at(5) == 99, "indexing");
    m1[17] = 100;
    ASSERT_M(m1.at(17) == 100, "indexing");

    // insert
    std::vector<std::pair<int, int>> v{ { 21,22 },{ 23,24 },{ 25,26 } };
    m1.insert(v.begin(), v.end());
    std::list<int> expected{ 1,3,5,7,17,21,23,25 };
    std::list<int> actual;
    for (auto item : m1)
    {
        actual.push_back(item.first);
    }
    actual.sort();  // needed for unordered_map
    ASSERT_M(actual == expected, "insert");

    m1.erase(17);
    ASSERT_M(m1.find(17) == m1.end(), "erase");

    ASSERT_M(!m1.empty(), "empty");
    m1.clear();
    ASSERT_M(m1.size() == 0, "clear");
    ASSERT_M(m1.empty(), "empty");
}

template<class Map>
void test_interface(Map & m)
{
    test_construct_assign(m);
    test_read(m);
    test_write(m);
}

template <typename K, typename M>
class my_map: public std::map<K,M>
{
public:
    const std::string msg_move_constructor{
        "copy constructor invoked must be move constructor."
    };

    my_map() = default;

    my_map(const my_map &)
    {
        FAIL_M(msg_move_constructor);
    }

    my_map(my_map &&)
    {
        PASS_M(msg_move_constructor);
    }
};

template <typename K, typename M>
using my_lockfree_map = lockfree::map_template <my_map <K, M>>;

void test_myMap()
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
    lockfree::map<int, int> m1;
    test_interface(m1);

    lockfree::unordered_map<int, int> m2;
    test_interface(m2);

    test_myMap();

    cout << "\ndone\n";
    getchar();
    return 0;
}
