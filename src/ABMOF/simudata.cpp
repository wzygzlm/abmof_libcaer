#include <iostream>
#include <cstdlib>
#include <stdint.h>

using namespace std;

void creatEventdata(int event_num, uint64_t *data)
{
    int x = 10;
    int y = 12;
    int width = 45;
    int lenth = 72;
    int polarity = 1;
    int bit = 1;
    uint64_t temp = 0;

    for(int i = x; i < width; i++)
    {
        for (int j = y; j < lenth; j++)
        {
            temp = i << 17 + j << 2 + polarity << 1 + 1;
            *data++ = temp;
        }
    }
}


int main()
{
    //std::cout << "Hello World\n";
    return 0;
}