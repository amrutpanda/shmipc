#include <shmipc.hpp>
#include <signal.h>

const std::string key = "/mem";

bool runloop = true;
void sighandler(int signum) {runloop = false;}

int main(int argc, char const *argv[])
{
    signal(SIGINT, sighandler);
    shmipc::SharedMemory shm;
    int count = 0;
    int n = shm.createIntWriteCallback(key,count);
    while (runloop)
    {
        shm.executeIntWriteCallback(n);
        count++;
    }
    
    
    return 0;
}
