#include <shmipc.hpp>
const std::string key = "/mem";
int main(int argc, char const *argv[])
{
    void* p = new int[10];
    std::atomic_int* ptr = reinterpret_cast<std::atomic_int*>(p);
    ptr->store(10);
    std::cout << ptr->load() << std::endl;

    return 0;
}
