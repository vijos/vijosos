#include <clocale>
#include <iostream>

int main()
{
    std::cout << "hello, world" << std::endl;
    int a, b;
    std::cin >> a >> b;
    std::cout << a + b << std::endl;
    std::cout << "bye, world" << std::endl;
    try
    {
        throw "test";
    }
    catch (const char *s)
    {
        std::cout << "exception: " << s << std::endl;
    }
    return 0;
}
