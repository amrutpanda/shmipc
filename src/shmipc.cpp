#include <shmipc.hpp>

namespace shmipc
{

    void SharedMemory::clearSharedObjects()
    {
        // cleaning up read shared memory objects. 
        for (int i = 0 ; i < _read_shd_obj_names.size() ; ++i)
        {
            munmap(_read_shd_obj_ptrs[i],_read_shd_obj_sizes[i]);
            close(_read_shd_obj_fds[i]);
        }
        // cleaning up write shared memory objects.
        for (int i = 0 ; i < _write_shd_obj_names.size() ; ++i)
        {
            munmap(_write_shd_obj_ptrs[i],_write_shd_obj_sizes[i]);
            close(_write_shd_obj_fds[i]);
        }
        
    }

    int SharedMemory::createStringReadCallback(const std::string& _varName, std::string& var)
    {
        const char* shm_name = _varName.c_str();
        // use double buffer with atomic variable for race free read and write.
        const size_t shm_size = sizeof(int) + 2*max_string_size ;

        int fd = shm_open(shm_name,O_CREAT | O_RDWR, 0666);
        if (fd == -1)
        {
            perror("shm_open: ");
            throw std::runtime_error("Error occurred in CreateStringReadCallback");
        }
        ftruncate(fd,shm_size);
        // map the shared memory in process address space.
        void* ptr = mmap(0, shm_size,PROT_READ,MAP_SHARED,fd,0);
        if (ptr == MAP_FAILED)
        {
            perror("mmap: ");
            throw std::runtime_error("Error occured during createStringReadCallback. Quitting..");
        }

        // do the book keeping.
        _read_shd_obj_names.push_back(shm_name);
        _read_inds.push_back(_read_inds.size() + 1);
        _read_shd_obj_fds.push_back(fd);
        _read_shd_obj_ptrs.push_back(ptr);
        _read_shd_obj_sizes.push_back(shm_size);
        _read_shd_obj_types.push_back(SHMOBJTYPE::STRING);
        
        return _read_inds.size();

    }

    int SharedMemory::createStringWriteCallback(const std::string& _varName, std::string& var)
    {
        const char* shm_name = _varName.c_str();
        // use double buffer with atomic variable for race free read and write.
        const size_t shm_size = sizeof(int) + 2*max_string_size ;

        int fd = shm_open(shm_name,O_CREAT | O_RDWR, 0666);
        if (fd == -1)
        {
            perror("shm_open: ");
            throw std::runtime_error("Error occurred in CreateStringWriteCallback");
        }
        ftruncate(fd,shm_size);
        // map the shared memory in process address space.
        void* ptr = mmap(0, shm_size,PROT_READ,MAP_SHARED,fd,0);
        if (ptr == MAP_FAILED)
        {
            perror("mmap: ");
            throw std::runtime_error("Error occured during createStringWriteCallback. Quitting..");
        }

        // store 0 as initial value in atomic variable.
        auto* active_index = reinterpret_cast<std::atomic<int>*>(ptr);
        active_index->store(0,std::memory_order_relaxed);

        // do the book keeping.
        _write_shd_obj_names.push_back(shm_name);
        _write_inds.push_back(_read_inds.size() + 1);
        _write_shd_obj_fds.push_back(fd);
        _write_shd_obj_ptrs.push_back(ptr);
        _write_shd_obj_sizes.push_back(shm_size);
        _write_shd_obj_types.push_back(SHMOBJTYPE::STRING);
        
        return _write_inds.size();

    }


    void SharedMemory::executeStringReadCallback(int cbn)
    {
        void* ptr = _read_shd_obj_ptrs[cbn];
        auto active_index = reinterpret_cast<std::atomic<int>*>(ptr);
        char* buf0 = reinterpret_cast<char*>(ptr + sizeof(int));
        char* buf1 = reinterpret_cast<char*>(ptr + max_string_size);

        std::string* dstptr = static_cast<std::string*>(_read_data_ptrs[cbn]);
        // get the atomic var value.
        int current = active_index->load(std::memory_order_acquire);
        char* srcptr = (current == 0) ? buf0 : buf1;

        // copy the address value of src to sptr.
        memcpy(dstptr,srcptr,max_string_size);
    }

    void SharedMemory::executeStringWriteCallback(int cbn)
    {
        void* ptr = _write_shd_obj_ptrs[cbn];
        auto active_index = reinterpret_cast<std::atomic<int>*>(ptr);
        char* buf0 = reinterpret_cast<char*>(ptr + sizeof(int));
        char* buf1 = reinterpret_cast<char*>(ptr + max_string_size);

        std::string* srcptr = static_cast<std::string*>(_write_data_ptrs[cbn]);
        // perform the atomic operation.
        int current = active_index->load(std::memory_order_acquire);
        int next = 1 - current;

        char* dstptr = (next == 0) ? buf0 : buf1;
        memcpy(dstptr,srcptr,max_string_size);

        // use atomic fence to ensure the active_index store happens after the memcpy.
        // this instruction does not do anything accept fencing.
        std::atomic_thread_fence(std::memory_order_release);
        active_index->store(next, std::memory_order_release);
    }
    
} // namespace shmipc
