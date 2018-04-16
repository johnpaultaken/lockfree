#include "map.h"

using std::cout;

int main(int argc, char ** argv)
{
    lockfree::map<int,int> m;
    m.set_value(2, 3);
    cout << m.at(2);

    cout << "\ndone\n";
    getchar();
    return 0;
}
