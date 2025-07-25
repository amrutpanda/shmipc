#include <shmipc.hpp>
#include <signal.h>
const std::string key = "/mem";

bool runloop = true;
void sighandler(int signum) {runloop = false;}

int main(int argc, char const *argv[])
{
    shmipc::SharedMemory shm;
    std::string s;
    int n = shm.createStringReadCallback(key,s);
    
    while (runloop)
    {
        shm.executeStringReadCallback(n);
        std::cout << "s: " << s << std::endl;
    }
    
    
    return 0;
}
