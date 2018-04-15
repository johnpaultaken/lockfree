#include <iostream>
using std::cout;

#include "map.h"

int main(int argc, char ** argv)
{
    lockfree::map<int,int> m;
    m.insert(std::make_pair(2,3));

    cout << "\ndone\n";
    //getchar();
    return 0;
}
