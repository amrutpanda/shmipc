#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <vector>
#include <atomic>
#include <memory>

#include <Eigen/Dense>
#include <Eigen/Core>

#define MAX_STRING_SIZE 10

namespace shmipc
{

    enum SHOBJTYPE
    {
        STRING = 0,
        INT,
        DOUBLE,
        EIGEN,
        OTHER
    };

    template <typename T>
    struct SharedData
    {
        std::string _shm_name_flag;
        std::string _shm_name_1;
        std::string _shm_name_2;

        int fd_flag;
        int fd_1;
        int fd_2;

        size_t _shm_size_flag;
        size_t _shm_size_1;
        size_t _shm_size_2;

        std::atomic_int* flag;
        // std::shared_ptr<std::atomic_int*> flag;
        T* buf1;
        T* buf2;

        int _nele = 1;
        int _dtype;

        T* _dptr;

        SharedData() {};

        SharedData(const std::string& _shm_name, T* dataPointer, int _arr_len)
        {
            createInstance(_shm_name,dataPointer,_arr_len);
        };

        virtual ~SharedData() = default;

        void createInstance(const std::string& _shm_name, T* dataPointer, int _arr_len)
        {
            // fill data pointer and no. of elements.
            _dptr = dataPointer;
            _nele = _arr_len;
            _dtype = checkDataPtrType(dataPointer);

            // std::cout << _dtype << std::endl;

            // create shared memory object names; 
            _shm_name_flag = _shm_name + "_flag";
            _shm_name_1 = _shm_name + "_buf0";
            _shm_name_2 = _shm_name + "_buf1";

            // set the memory sizes.
            _shm_size_flag = sizeof(std::atomic_int);

            if (_dtype != 0)
            {
                _shm_size_1 = _nele * sizeof(T);
                _shm_size_2 = _shm_size_1;
            }
            else
            {
                _shm_size_1 = MAX_STRING_SIZE;
                _shm_size_1 = _shm_size_2;
            }
            
            // create shared memory buffers;
            createSharedMemory(_shm_name_flag,fd_flag,_shm_size_flag, flag);
            createSharedMemory(_shm_name_1, fd_1, _shm_size_1, buf1);
            createSharedMemory(_shm_name_2, fd_2, _shm_size_2, buf2);
        };

        int checkDataPtrType(T* dataptr)
        {
            using DataType = std::remove_extent_t<std::remove_pointer_t<decltype(dataptr)>>;
            if (std::is_same_v<DataType, int>) return SHOBJTYPE::INT;
            else if (std::is_same_v<DataType,double>) return SHOBJTYPE::DOUBLE;
            else if (std::is_same_v<DataType, std::string>) return SHOBJTYPE::STRING;
            else return -1;
        };



        void createSharedMemory(std::string& _name, int fd ,size_t _shm_size, T*& ptr)
        {
            if ((fd = shm_open(_name.c_str(),O_CREAT | O_RDWR, 0666)) == -1)
            {
                perror("shm_open");
                throw std::runtime_error("Failed to create shared memory. keyname: " + _name);
            }
            ftruncate(fd, _shm_size);
            void* _ptr;
            if ((_ptr = mmap(0, _shm_size,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0)) == MAP_FAILED)
            {
                perror("mmap");
                throw std::runtime_error("Failed during mmap. keyname: " + _name);
            }
            // get a shard memory casted as Template pointer type.
            ptr = static_cast<T*>(_ptr);
        };

        void createSharedMemory(std::string& _name, int fd, size_t _shm_size, std::atomic_int*& ptr)
        {
            const char* __name = _name.c_str();
            fd = shm_open(__name, O_CREAT | O_RDWR, 0666);
            if (fd == -1)
            {
                perror("shm_open");
                throw std::runtime_error("Failed to create shared memory. keyname: " + _name);
            }
            ftruncate(fd, _shm_size);
            void* _ptr;
            if ((_ptr = mmap(0, _shm_size,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0)) == MAP_FAILED)
            {
                perror("mmap");
                throw std::runtime_error("Failed during mmap. keyname: " + _name);
            }
            // get a shard memory casted as Template pointer type.
            ptr = static_cast<std::atomic_int*>(_ptr);
        }

        void setWriteFlag()
        {
            // std::cout << "initial flag value: " << flag->load() << std::endl;
            std::atomic_int flag_val = flag->load();
            if (flag_val != 0 && flag_val != 1)
                flag->store(0);
            // std::cout << "final flag value: " << flag->load() << std::endl;
        }

    };
    

    class SharedMemory
    {
    private:
        std::vector<std::string> _read_shd_obj_names;
        std::vector<void*>_read_shd_data_structs;
        std::vector<int> _read_data_types;
        std::vector<int> _read_inds;

        std::vector<std::string> _write_shd_obj_names;
        std::vector<void*> _write_shd_data_structs;
        std::vector<int> _write_data_types;
        std::vector<int> _write_inds;
    public:
        SharedMemory(/* args */){};
        ~SharedMemory(){};

        int addToReadList(const std::string& key, std::string& var);

        template <typename T>
        int addToReadList(const std::string& key, T& var, int arr_len = 1);

        template <typename _Scalar, int _Rows, int _Cols>
        int addEigenToReadList(const std::string& key, Eigen::Matrix<_Scalar, _Rows, _Cols>& var);

        int addToWriteList(const std::string& key, std::string& var);

        template <typename T>
        int addToWriteList(const std::string& key, T& var, int arr_len = 1);

        template <typename _Scalar, int _Rows, int _Cols>
        int addEigenToWriteList(const std::string& key, Eigen::Matrix<_Scalar, _Rows, _Cols>& var);

        void executeReadCallback(int cbn);
        void executeWriteCallback(int cbn);

        void executeAllReads();
        void executeAllWrites();
    };
    

    int SharedMemory::addToReadList(const std::string& key, std::string& var)
    {
        char* ptr = const_cast<char*>(var.c_str());
        return addToReadList(key,ptr,MAX_STRING_SIZE);
    }

    template <typename _Scalar, int _Rows, int _Cols>
    int SharedMemory::addEigenToReadList(const std::string& key, Eigen::Matrix<_Scalar, _Rows, _Cols>& mat)
    {
        return addToReadList(key, &mat.data(), mat.rows() * mat.cols());
    }


    template <typename T>
    int SharedMemory::addToReadList(const std::string& key, T& var, int arr_len)
    {
        SharedData<T>* _shd_obj_ptr = new SharedData<T>(key, &var, arr_len);
        // casting the object to void for storage.
        void* _ptr = static_cast<void*>(_shd_obj_ptr);
        
       _read_shd_obj_names.push_back(key);
       _read_shd_data_structs.push_back(_ptr);
       _read_data_types.push_back(_shd_obj_ptr->checkDataPtrType(&var));
       _read_inds.push_back(_read_inds.size());

        return _read_inds.size() - 1;
    }

    int SharedMemory::addToWriteList(const std::string& key, std::string& var)
    {
        char* ptr = const_cast<char*>(var.c_str());
        return addToWriteList(key,ptr,MAX_STRING_SIZE);
    }

    template <typename _Scalar, int _Rows, int _Cols>
    int SharedMemory::addEigenToWriteList(const std::string& key, Eigen::Matrix<_Scalar, _Rows, _Cols>& mat)
    {
        return addToWriteList(key, &mat.data(), mat.rows() * mat.cols());
    }

    
    template <typename T>
    int SharedMemory::addToWriteList(const std::string& key, T& var, int arr_len)
    {
        SharedData<T>* _shd_obj_ptr = new SharedData<T>(key, &var, arr_len);
        // Do not forget to setup write flag.
        _shd_obj_ptr->setWriteFlag();
        // casting the object to void for storage
        void* _ptr = static_cast<void*>(_shd_obj_ptr);

       _write_shd_obj_names.push_back(key);
       _write_shd_data_structs.push_back(_ptr);
       _write_data_types.push_back(_shd_obj_ptr->checkDataPtrType(&var));
       _write_inds.push_back(_write_inds.size());

        return _write_inds.size() - 1;
    }



    void SharedMemory::executeReadCallback(int cbn)
    {
        switch (_read_data_types[cbn])
        {
        case STRING:
            {
                SharedData<char>* _ptr = static_cast<SharedData<char>*>(_read_shd_data_structs[cbn]);
                std::atomic curr = _ptr->flag->load(std::memory_order_acquire);
                if (curr == 0)
                    strncpy(_ptr->_dptr, _ptr->buf1, MAX_STRING_SIZE);
                    // memcpy(_ptr->_dptr, _ptr->buf1, _ptr->_nele * sizeof(int));
                else
                    strncpy(_ptr->_dptr, _ptr->buf2, MAX_STRING_SIZE);
                    // memcpy(_ptr->_dptr, _ptr->buf2, _ptr->_nele * sizeof(int));
                break;
            }
        case INT:
            {
                SharedData<int>* _ptr = static_cast<SharedData<int>*>(_read_shd_data_structs[cbn]);
                std::atomic curr = _ptr->flag->load(std::memory_order_acquire);
                if (curr == 0)
                    memcpy(_ptr->_dptr, _ptr->buf1, _ptr->_nele * sizeof(int));
                else
                    memcpy(_ptr->_dptr, _ptr->buf2, _ptr->_nele * sizeof(int));
               
                std::cout << "------------------------------------" << std::endl;
                std::cout << "Execute Read callback " << std::endl;
                std::cout << "nele : " << _ptr->_nele << std::endl;
                std::cout << "flag value: " << _ptr->flag->load() << std::endl;
                for (int i = 0; i < _ptr->_nele; i++)
                {
                    std::cout << "buf1: ind" << i << ": " << *(_ptr->buf1 + i) << std::endl;
                    std::cout << "buf2: ind" << i << ": " << *(_ptr->buf2 + i) << std::endl;
                    std::cout << "dptr : ind" << i << ": "<< *(_ptr->_dptr + i) << std::endl;
                }
                
                break;
            }
        case DOUBLE:
            {
                SharedData<double>* _ptr = static_cast<SharedData<double>*>(_read_shd_data_structs[cbn]);
                std::atomic curr = _ptr->flag->load(std::memory_order_acquire);
                if (curr == 0)
                    memcpy(_ptr->_dptr, _ptr->buf1, _ptr->_nele * sizeof(double));
                else
                    memcpy(_ptr->_dptr, _ptr->buf2, _ptr->_nele * sizeof(double));
                break;
            }
        
        default:
            {
                throw std::runtime_error("Unknown Data type. Quitting ..");
                break;
            }
        }
    }

    void SharedMemory::executeWriteCallback(int cbn)
    {
        switch (_write_data_types[cbn])
        {
        case STRING:
            {
                SharedData<char>* _ptr = static_cast<SharedData<char>*>(_write_shd_data_structs[cbn]);
                std::atomic curr = _ptr->flag->load(std::memory_order_acquire);
                std::atomic next = 1 - curr;
                if (next == 0)
                    strncpy( _ptr->buf1, _ptr->_dptr, MAX_STRING_SIZE);
                else
                    strncpy(_ptr->buf2,_ptr->_dptr, MAX_STRING_SIZE);

                std::atomic_thread_fence(std::memory_order_release);
                _ptr->flag->store(next);
                break;
            }
        case INT:
            {
                SharedData<int>* _ptr = static_cast<SharedData<int>*>(_write_shd_data_structs[cbn]);
                std::atomic curr = _ptr->flag->load(std::memory_order_acquire);
                std::atomic next = 1 - curr;
                if (next == 0)
                    memcpy(_ptr->buf1, _ptr->_dptr, _ptr->_nele * sizeof(int));
                else
                    memcpy(_ptr->buf2, _ptr->_dptr, _ptr->_nele * sizeof(int));
                // put a memory fence while executing the above instruction.
                std::atomic_thread_fence(std::memory_order_release);
                _ptr->flag->store(next);
                // _ptr->flag->store(2);

                std::cout << "------------------------------------" << std::endl;
                std::cout << "Execute Write callback " << std::endl;
                std::cout << "nele : " << _ptr->_nele << std::endl;
                std::cout << "flag value: " << _ptr->flag->load() << std::endl;
                for (int i = 0; i < _ptr->_nele; i++)
                {
                    std::cout << "buf1: ind" << i << ": " << *(_ptr->buf1 + i) << std::endl;
                    std::cout << "buf2: ind" << i << ": " << *(_ptr->buf2 + i) << std::endl;
                    std::cout << "dptr : ind" << i << ": "<< *(_ptr->_dptr + i) << std::endl;
                }
                break;
            }
        case DOUBLE:
            {
                SharedData<double>* _ptr = static_cast<SharedData<double>*>(_write_shd_data_structs[cbn]);
                std::atomic curr = _ptr->flag->load(std::memory_order_acquire);
                std::atomic next = 1 - curr;
                if (next == 0)
                    memcpy(_ptr->buf1, _ptr->_dptr, _ptr->_nele * sizeof(double));
                else
                    memcpy(_ptr->buf2, _ptr->_dptr, _ptr->_nele * sizeof(double));
                
                // put a memory fence while executing the above instruction.
                std::atomic_thread_fence(std::memory_order_release);
                _ptr->flag->store(next);
 
                break;
            }
        
        default:
            {
                throw std::runtime_error("Unknown Data type. Quitting ..");
                break;
            }
        }
    }

    
} // namespace shmipc

