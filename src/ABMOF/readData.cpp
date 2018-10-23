#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

int main() 
{ 
    std::ifstream file("jAER-events.txt");
    std::string str; 
    std::vector<int> values;

    while (std::getline(file, str))
    {
        std::stringstream stream(str);
        int n;
        while(stream >> n){
            std::cout << n << std::endl;
            values.push_back(n);
        }
    }
}
