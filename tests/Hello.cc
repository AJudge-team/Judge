#include <iostream>

#include <cstdio>
#include <algorithm>

using namespace std;

int main()
{
    long long counter = 0;
    for(long long i = 0; i < 4000000000LL; ++i) {
        counter = (counter + i)%124 + (i*i)%12;
    }
    cout<<counter<<endl;
    return 0;
}
