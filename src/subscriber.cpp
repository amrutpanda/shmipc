#include <shmipc.hpp>
#include <signal.h>
const std::string key = "/mem";

bool runloop = true;
void sighandler(int signum) {runloop = false;}

int main(int argc, char const *argv[])
{
    shmipc::SharedMemory shm;
    int val;
    int n = shm.createIntReadCallback(key,val);
    int prev_val = val;
    
    while (runloop)
    {
        shm.executeIntReadCallback(n);
        std::cout << val << std::endl;
        if (prev_val > val)
            break;
        prev_val = val;
    }
    
    
    return 0;
}
