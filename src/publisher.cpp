#include <shmipc.hpp>
#include <signal.h>

const std::string key = "/mem";

bool runloop = true;
void sighandler(int signum) {runloop = false;}

int main(int argc, char const *argv[])
{
    signal(SIGINT, sighandler);
    shmipc::SharedMemory shm;
    std::string s;
    int n = shm.createStringWriteCallback(key,s);
    int count = 0;
    while (runloop)
    {
        s = std::to_string(count);
        // shm.executeStringWriteCallback(n);
        count++;
    }
    
    
    return 0;
}
