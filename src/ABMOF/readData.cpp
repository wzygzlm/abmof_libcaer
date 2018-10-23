#include <fstream>
#include <string>
#include <iostream>

int main() 
{ 
    std::ifstream file("jAER-events.txt");
    std::string str; 
    while (std::getline(file, str))
    {
	std::cout << "str is : " << std::endl;        
    }
}
