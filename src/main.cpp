#include <shm_ipc.hpp>
const std::string key = "/mem";
int main(int argc, char const *argv[])
{
    int data0 = 12;
    int data1[4] = {2,3,5,67};
    double data2 = 1.2;
    double data3[2] = {3.4, 4.5};

    int val0 = 43000; 
    int val1[4];

    std::string data5 = "hello";

    shmipc::SharedMemory shm;
    int w1 = shm.addToWriteList(key,data1,4);
    int r1 = shm.addToReadList(key, val1,4);

    shm.executeWriteCallback(w1);
    shm.executeReadCallback(r1);
    // sleep(0.5);
    // shm.executeWriteCallback(w1);
    shm.executeReadCallback(r1);
    
    // std::cout << "val = " << val << std::endl;
    for (int i = 0 ; i < 4; i++)
    {
        std::cout << val1[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}
