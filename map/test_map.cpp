#include <functional>
#include <algorithm>
#include <string>
#include <cstring>
#include <sstream>
#include <list>
#include <vector>
#include <future>
#include <random>

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
    auto filename = std::max<const char *>(
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
        "constructor invoked must be move constructor."
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


template<class Map>
void test_concurrent_writes(Map &)
{
    Map m1{
        typename Map::implementation_type{ { 1,2 },{ 3,4 },{ 5,6 },{ 7,8 } }
    };

    std::atomic<bool> wait{ true };
    std::atomic<unsigned int> count{ 0 };
    auto t1 = std::async(
        std::launch::async,
        [&m1,&count,&wait]() { count++;  while (wait){}; m1[2] = 3; }
    );
    auto t2 = std::async(
        std::launch::async,
        [&m1, &count, &wait]() { count++;  while (wait){}; m1[4] = 5; }
    );
    auto t3 = std::async(
        std::launch::async,
        [&m1, &count, &wait]() { count++;  while (wait){}; m1[6] = 7; }
    );
    auto t4 = std::async(
        std::launch::async,
        [&m1, &count, &wait]() { count++;  while (wait){}; m1[8] = 9; }
    );
    while (count < 4);
    wait = false;
    t1.wait();
    t2.wait();
    t3.wait();
    t4.wait();

    // verify
    std::list<int> expected{ 1,2,3,4,5,6,7,8 };
    std::list<int> actual;
    for (auto item : m1)
    {
        actual.push_back(item.first);
    }
    actual.sort();  // needed for unordered_map
    ASSERT_M(actual == expected, "concurrent writes");
}

//
// Test concurrency with 4 threads,
// each thread reading, writing and modifying
// the same map concurrently.
//
template<class Map>
void test_concurrent4x_read_write_modify(Map &)
{
    int range_begin{ 0x0000000F };
    int range_end{ 0x000004F0 };

    std::atomic<bool> wait{ true };
    std::atomic<unsigned int> concurrency{ 0 };

    Map m1{
        typename Map::implementation_type{
            { range_begin - 2, range_begin - 2 },
            { range_begin - 1, range_begin - 1 },
            { range_end, range_end },
            { range_end + 1, range_end + 1 }
        }
    };

    cout << "\nPlease wait a min ...";

    auto threadfunc = (
        [&m1, &concurrency, &wait, range_begin, range_end]() {
            concurrency++;
            while (wait) {};
            std::mt19937 mt(std::random_device{}());
            std::uniform_int_distribution<int> ud(range_begin, range_end-1);
            auto r = std::bind(ud, mt);
            auto count = 2 * (range_end - range_begin);
            for (int i = 0; i < count; ++i)
            {
                auto key = r();
                auto dowhat = r() % 4;
                switch (dowhat)
                {
                case 0:
                    m1[key] = key;
                    break;
                case 1:
                    {
                        std::vector<std::pair<int, int>> v{ { key, key } };
                        m1.insert(v.begin(), v.end());
                        break;
                    }
                case 2:
                    m1.erase(key);
                    break;
                case 3:
                    try
                    {
                        m1.at(key);
                    }
                    catch (const std::out_of_range &)
                    {
                    }
                    break;
                default:
                    break;
                }
                //cout << "\n" << std::this_thread::get_id() << ":" << i;
            }
        }
    );

    auto t1 = std::thread(threadfunc);
    auto t2 = std::thread(threadfunc);
    auto t3 = std::thread(threadfunc);
    auto t4 = std::thread(threadfunc);

    // wait until all threads are on mark.
    while (concurrency < 4);

    // all threads go
    wait = false;

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // verify integrity of map
    for (auto item : m1)
    {
        if (item.first != item.second)
        FAIL_M("map data integrity");
    }
    ASSERT_M(m1[range_begin-2] == range_begin-2, "map data integrity");
    ASSERT_M(m1[range_begin-1] == range_begin-1, "map data integrity");
    ASSERT_M(m1[range_end] == range_end, "map data integrity");
    ASSERT_M(m1[range_end+1] == range_end+1, "map data integrity");
}

template<class Map>
void test_concurrency(Map & m)
{
    test_concurrent_writes(m);
    test_concurrent4x_read_write_modify(m);
}

//
// This std::map wrapper can be used to check the strength of concurrency tests
// Since std::map is not thread-safe, concurrency tests would not succeed on
// this map.
//
class MapNonMt :public std::map<int, int>
{
public:
    using implementation_type = std::map<int, int>;
    MapNonMt(const implementation_type & imp) : std::map<int, int>{ imp } {}
    MapNonMt() = default;
};


int main(int , char ** )
{
    lockfree::map<int, int> map_ord;
    test_interface(map_ord);
    test_concurrency(map_ord);

    lockfree::unordered_map<int, int> map_unord;
    test_interface(map_unord);
    test_concurrency(map_unord);

    test_myMap();

    // Enable this code to verify the strength of concurrency tests.
    // MapNonMt is a std::map wrapper.
    // Since std::map is not thread-safe, concurrency tests would not succeed
    // on this map.
    //MapNonMt map_nmt;
    //test_concurrency(map_nmt);

    cout << "\ndone\n";
    //getchar();
    return 0;
}
