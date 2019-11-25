/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include "CDebug.h"
#include "errno.h"

#ifdef _WIN32
 #include "Windows.h"
#else
 #include <sys/mman.h>
#endif

#include <stdint.h>

#define MEMPOOL_MAX_NUM_OF_BINS     6

extern qint64 g_pool_total_size;

typedef struct {
    int numOfRanges;
    int startNumPerRange[MEMPOOL_MAX_NUM_OF_BINS];
    int ranges[MEMPOOL_MAX_NUM_OF_BINS];
}CMemPool_Config_t;

/***********************************************************************************************************************
*   CMemPoolItem
***********************************************************************************************************************/

/* If you inherit from this then make sure to add virtual destructors */
class CMemPoolItem final
{
public:
    CMemPoolItem() = delete;

    explicit CMemPoolItem(int size)
    {
        m_data_p = reinterpret_cast<int *>(malloc(static_cast<size_t>(size)));
        m_size = size;

        g_pool_total_size += size;
    }

    inline ~CMemPoolItem()
    {
        if (m_data_p != nullptr) {
            g_pool_total_size -= m_size;
            free(m_data_p);
        }
    }

    /****/
    inline void MemSet(void)
    {
        if ((m_data_p != nullptr) && (m_size != 0)) {
            memset(m_data_p, 0, static_cast<size_t>(m_size));
        }
    }

    inline int GetDataSize(void) {return m_size;}
    inline void *GetDataRef(void) {return reinterpret_cast<void *>(m_data_p);}

private:
    int *m_data_p;
    int m_size;
};

/***********************************************************************************************************************
*   CMemPoolItemBin
***********************************************************************************************************************/

/* If you inherit from this then make sure to add virtual destructors */
class CMemPoolItemBin final
{
public:
    CMemPoolItemBin(int minSize, int maxSize, int numOfStartItems)
    {
        m_minSize = minSize;
        m_maxSize = maxSize;

        for (int index = 0; index < numOfStartItems; ++index) {
            m_poolItemList.insert(0, new CMemPoolItem(m_maxSize));
        }
    }

    ~CMemPoolItemBin()
    {
        while (!m_poolItemList.isEmpty()) {
            auto item = m_poolItemList.takeLast();
            if (nullptr != item) {
                delete (item);
            }
        }
    }

    /****/
    inline CMemPoolItem *AllocMem(void)
    {
        if (!m_poolItemList.isEmpty()) {
            return m_poolItemList.takeFirst();
        } else {
            CMemPoolItem *item_p = new CMemPoolItem(m_maxSize);
            return item_p;
        }
    }

    /****/
    inline void ReturnMem(CMemPoolItem **poolItem_pp)
    {
        m_poolItemList.insert(0, *poolItem_pp);
        *poolItem_pp = nullptr;
    }

    inline int GetMaxSize(void) {return m_maxSize;}

protected:
    CMemPoolItemBin() {}

    void Init(void) {m_minSize = 0; m_maxSize = 0;}

private:
    int m_minSize;
    int m_maxSize;
    QList<CMemPoolItem *> m_poolItemList;
};

/***********************************************************************************************************************
*   CMemPool
***********************************************************************************************************************/

/* If you inherit from this then make sure to add virtual destructors */
class CMemPool final
{
public:
    CMemPool() = delete;

    explicit CMemPool(const CMemPool_Config_t *config_p)
    {
        Init();

        m_config = *config_p;

        int minSize = 0;

        for (int index = 0; index < m_config.numOfRanges; ++index) {
            if (index > 0) {
                minSize = m_config.ranges[index - 1] + 1; /* take previous max size and add 1 */
            }

            TRACEX_D("CMemPool::CMemPool    Range %6d - %6d NumOfItems:%-4d",
                     minSize, m_config.ranges[index], m_config.startNumPerRange[index])

            m_bins_p[index] = new CMemPoolItemBin(minSize, m_config.ranges[index], m_config.startNumPerRange[index]);

            if (m_bins_p[index] == nullptr) {
                TRACEX_E("CMemPool::CMemPool    m_bins_p[index] is nullptr")
            }
        }

        m_numOfRanges = m_config.numOfRanges;
    }

    ~CMemPool()
    {
        for (int index = 0; index < m_numOfRanges; ++index) {
            if (m_bins_p[index] != nullptr) {
                delete m_bins_p[index];
            }
        }
    }

    /****/
    inline CMemPoolItem *AllocMem(qint64 minimumSize)
    {
        for (int index = 0; index < m_numOfRanges; ++index) {
            if ((m_bins_p[index] != nullptr) && (minimumSize <= m_bins_p[index]->GetMaxSize())) {
                return m_bins_p[index]->AllocMem();
            }
        }

        if ((m_numOfRanges > 0) && (m_numOfRanges <= MEMPOOL_MAX_NUM_OF_BINS) &&
            (m_bins_p[m_numOfRanges - 1] != nullptr)) {
            TRACEX_E(QString("CMemPool::AllocMem    size outside range %1 -> memory nullptr").arg(minimumSize))
            return nullptr;
        }

        TRACEX_E("CMemPool::AllocMem    Memory corruption, ranges badly setup")
        return nullptr;
    }

    /****/
    inline void ReturnMem(CMemPoolItem **poolItem_pp)
    {
        if ((nullptr == poolItem_pp) || (nullptr == *poolItem_pp)) {
            TRACEX_E("CMemPool::ReturnMem   nullptr")
            return;
        }

        for (int index = 0; index < m_numOfRanges; ++index) {
            if ((*poolItem_pp)->GetDataSize() <= m_bins_p[index]->GetMaxSize()) {
                return m_bins_p[index]->ReturnMem(poolItem_pp);
            }
        }
    }

    qint64 GetTotalSize(void) {return g_pool_total_size;}

private:
    /****/
    void Init(void)
    {
        memset(&m_config, 0, sizeof(m_config));
        memset(m_bins_p, 0, sizeof(CMemPoolItemBin *) * MEMPOOL_MAX_NUM_OF_BINS);
        m_numOfRanges = 0;
    }

    int m_numOfRanges; /* the last range always maps to sizes larger than configured */
    CMemPoolItemBin *m_bins_p[MEMPOOL_MAX_NUM_OF_BINS];  /* directly maps to the config */
    CMemPool_Config_t m_config;
};

/* Used for allocating virtual memory on the platform */
class VirtualMem final
{
private:
    /* Not possible to create an instance of this memory, this is just an interface */
    VirtualMem() {}
    ~VirtualMem() {}

public:
    /****/
    static void *Alloc(int64_t size)
    {
        void *voidPtr = nullptr;
        size += sizeof(uint64_t) * 4; /* head + size + footer  + address align */
        size = (size >> 2) << 2; /* Clear the 2 LSBs */
#ifdef _WIN32
        voidPtr = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (voidPtr == nullptr) {
            DWORD error = GetLastError();
            TRACEX_E("VirtualMem    allocation failed code:%d", error)
            return nullptr;
        }
#else

        /* void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset); */
        voidPtr = mmap(nullptr, static_cast<size_t>(size), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        if ((voidPtr == MAP_FAILED) || (voidPtr == nullptr)) {
            TRACEX_E("VirtualMem    allocation failed code:%d", errno)
            return nullptr;
        }
#endif

#ifdef _DEBUG
        Q_CHECK_PTR(voidPtr);
#endif

#ifdef _WIN32
 #ifdef _DEBUG
        MEMORY_BASIC_INFORMATION info;
        VirtualQuery(voidPtr, &info, sizeof(info));

        if ((info.State & MEM_COMMIT) == 0) {
            TRACEX_E("VirtualMem    allocation, memory not commit status")
        }

        if (info.Protect != PAGE_READWRITE) {
            TRACEX_E("VirtualMem    allocation, memory protection wrong")
        }
 #endif
#endif

        uint64_t *headPtr = reinterpret_cast<uint64_t *>(voidPtr);
        uint64_t *sizePtr = headPtr + 1;
        *headPtr = 0x5555555555555555;
        *sizePtr = static_cast<uint64_t>(size);

        uint64_t *footerPtr = headPtr + (size >> 3) - 1;
        *footerPtr = 0x5555555555555555;
        voidPtr = reinterpret_cast<void *>(headPtr + 2);
        Q_CHECK_PTR(voidPtr);
        return voidPtr;
    }

    /****/
    static bool Free(void *mem_p)   /* non-zero on success */
    {
        uint64_t *headPtr = (reinterpret_cast<uint64_t *>(mem_p)) - 2;

        if (*headPtr != 0x5555555555555555) {
            TRACEX_E("VirtualMem    Free - header wrong")
        }

        *headPtr = 0; /* to trap double free */

        uint64_t *sizePtr = headPtr + 1;
        uint64_t *footerPtr = headPtr + ((*sizePtr) >> 3) - 1;

        if (*footerPtr != 0x5555555555555555) {
            TRACEX_E("VirtualMem    Free - footer wrong")
        }

#ifdef _WIN32
        bool result = VirtualFree(headPtr, 0, MEM_RELEASE);

        if (!result) {
            DWORD dwError = GetLastError();
            MEMORY_BASIC_INFORMATION info;
            VirtualQuery(headPtr, &info, sizeof(info));

            TRACEX_E("VirtualMem    free failed code:%d size:%llu", dwError, *sizePtr)
        }
        return result;
#else
        return munmap(headPtr, *sizePtr) == 0 ? true : false;
#endif
    }

    /****/
    static bool CheckMem(void *mem_p)   /* non-zero on success */
    {
        uint64_t *headPtr = (reinterpret_cast<uint64_t *>(mem_p)) - 2;

        if (*headPtr != 0x5555555555555555) {
            TRACEX_E("VirtualMem  CheckMem    - header wrong")
            return false;
        }

        uint64_t *sizePtr = headPtr + 1;
        uint64_t *footerPtr = headPtr + ((*sizePtr) >> 3) - 1;

        if (*footerPtr != 0x5555555555555555) {
            TRACEX_E("VirtualMem  CheckMeme   - footer wrong")
            return false;
        }

        return true;
    }

    /* Not thread safe */
    static bool GetMemoryStatus(qulonglong& free, qulonglong& total, qulonglong& avail)
    {
#ifdef _WIN32

        /*
         *  typedef struct _MEMORYSTATUSEX {
         *  DWORD dwLength;
         *  DWORD dwMemoryLoad;
         *  DWORDLONG ullTotalPhys;
         *  DWORDLONG ullAvailPhys;
         *  DWORDLONG ullTotalPageFile;
         *  DWORDLONG ullAvailPageFile;
         *  DWORDLONG ullTotalVirtual;
         *  DWORDLONG ullAvailVirtual;
         *  DWORDLONG ullAvailExtendedVirtual;
         *  } MEMORYSTATUSEX, *LPMEMORYSTATUSEX;
         */
        MEMORYSTATUSEX memoryStatus;

        memoryStatus.dwLength = sizeof(memoryStatus);

        if (GlobalMemoryStatusEx(&memoryStatus)) {
            total = memoryStatus.ullTotalPhys;
            avail = memoryStatus.ullAvailVirtual;
            free = memoryStatus.ullAvailPhys;
            return true;
        } else {
            return false;
        }
#else

        /* Linux and others */
        static QFile memInfo("/proc/meminfo");
        static QTextStream in (&memInfo);
        static QString total_s;
        static QString avail_s;
        static QString free_s;
        static QTextStream reader;
        static QString header;

        reader.setIntegerBase(10);

        if (!memInfo.isOpen()) {
            memInfo.open(QIODevice::ReadOnly);
        }

        if (!memInfo.isOpen()) {
            TRACEX_W("VirtualMem  Failed to open /proc/meminfo")
            return false;
        }

        in.seek(0);
        total_s = in.readLine();
        free_s = in.readLine();
        avail_s = in.readLine();

        reader.setString(&total_s);
        reader >> header;
        reader >> total;
        total <<= 10; /* *1024 */

        reader.setString(&free_s);
        reader >> header;
        reader >> free;
        free <<= 10; /* *1024 */

        reader.setString(&avail_s);
        reader >> header;
        reader >> avail;
        avail <<= 10; /* *1024 */

        return true;
#endif
    }

    /****/
    static bool test()
    {
#ifdef _DEBUG
        for (int index = 0; index < 100; index++) {
            auto *myData_p = reinterpret_cast<char *>(VirtualMem::Alloc(1000 * 1024));
            if (myData_p == nullptr) {
                return false;
            }

            if (!VirtualMem::Free(myData_p)) {
                return false;
            }
        }
#endif
        return true;
    }
};

typedef enum {
    WORK_MEM_OPERATION_NO = 0,
    WORK_MEM_OPERATION_COMMIT,
    WORK_MEM_OPERATION_FREE
} WorkMem_Operation_t;

/***********************************************************************************************************************
*   CWorkMem
***********************************************************************************************************************/
class CWorkMem
{
public:
    CWorkMem() : m_mem_p(nullptr), m_status(WORK_MEM_OPERATION_NO), m_size(0) {}
    ~CWorkMem()
    {
        if (m_mem_p != nullptr) {
            TRACEX_E("CWorkMem destructor, not freed")
        }
    }

    char *GetRef() {return m_mem_p;}
    int64_t GetSize() {return m_size;}
    bool Operation(WorkMem_Operation_t operation);

private:
    char *m_mem_p;
    WorkMem_Operation_t m_status;
    int64_t m_size;
};
