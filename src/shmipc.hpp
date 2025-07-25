#ifndef _SHMIPC_H
#define _SHMIPC_H

#include <iostream>
#include "shmipc.hpp"
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>           
#include <unistd.h>
#include <cstring>

#include <vector>
#include <atomic>



namespace shmipc
{

    enum SHMOBJTYPE
    {
        STRING = 0,
        INT,
        DOUBLE,
        EIGEN
    };

       class SharedMemory
       {
       private:
            // save all shared memory
            std::vector<std::string> _read_shd_obj_names;
            std::vector<int> _read_inds;
            std::vector<int> _read_shd_obj_fds;
            std::vector<void*> _read_shd_obj_ptrs;
            std::vector<int> _read_shd_obj_types;
            std::vector<int> _read_shd_obj_sizes;
            std::vector<void*> _read_data_ptrs;

            std::vector<std::string> _write_shd_obj_names;
            std::vector<int> _write_inds;
            std::vector<int> _write_shd_obj_fds;
            std::vector<void*> _write_shd_obj_ptrs;
            std::vector<int> _write_shd_obj_types;
            std::vector<int> _write_shd_obj_sizes;
            std::vector<void*> _write_data_ptrs;

            
       public:
        SharedMemory(/* args */) {};
        ~SharedMemory() {};

        void clearSharedObjects();

        int createStringReadCallback(const std::string& _varName, std::string& var);
        int createIntReadCallback(const std::string& _varName, int& var);
        int createDoubleReadCallback(const std::string& _varName, double& var);
        
        int createStringWriteCallback(const std::string& _varName, std::string& var);
        int createIntWriteCallback(const std::string& _varName, int& var);
        int createDoubleWriteCallback(const std::string& _varName, double& var);

        void executeStringReadCallback(int cbn);
        void executeStringWriteCallback(int cbn);

        void executeAllReadCallbacks();
        void executeAllWriteCallbacks();

        void getValue(const std::string& _varName, void* var); // it works if the _varName shared object already created.
        void setValue(const std::string& _varName, void* var); // it works if the _varName shared object already created.

        // public variables.
        int max_string_size = 10;

       };
       
       
} // namespace shmipc


#endif