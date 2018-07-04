#include "singleton.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <future>

using lockfree::singleton;
using std::async;
using std::future;
using std::logic_error;
using std::vector;
using std::cout;

class c
{
public:
    c() :m_i(0)
    {
    }

    c(int i) :m_i(i)
    {
    }

    c & operator=(int i)
    {
        m_i = i;
        return *this;
    }

    operator int()
    {
        return m_i;
    }
private:
    int m_i;
};

void test_basic()
{
    constexpr int expected = 17;
    auto instance = singleton<c>::instance(expected);
    auto sameinstance = singleton<c>::instance();
    if (*sameinstance == expected)
    {
        cout << "\nOK";
    }
    else
    {
        cout << "\nFAIL";
    }
}

void test_manythreads()
{
    constexpr int expected = 21;
    auto instance = singleton<c>::instance(expected);

    constexpr unsigned int num_tasks = 4;

    auto fetchInstanceVal = [expected]() {
        auto instance = singleton<c>::instance();
        if(*instance != expected)
        {
            throw logic_error{"instance fetched is NOT the expected one!"};
        }
    };

    {
        bool ok = true;
        vector<future<void>> vt;
        for (size_t i = 0; i < 4; ++i)
        {
            vt.push_back(async(fetchInstanceVal));
        }
        for (auto & task : vt)
        {
            try
            {
                task.get();
            }
            catch(const logic_error &)
            {
                cout << "\nFAIL";
                ok = false;
                break;
            }
        }

        if (ok) cout << "\nOK";
    }
}

int main(int argc, char ** argv)
{
    test_basic();
    test_manythreads();
    cout << "\ndone";
    //getchar();
}
