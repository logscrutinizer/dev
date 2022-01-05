/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CMemPool.h"
#include "CConfig.h"

qint64 g_pool_total_size = 0;

/***********************************************************************************************************************
*   Operation
***********************************************************************************************************************/
bool CWorkMem::Operation(WorkMem_Operation_t operation)
{
    if ((m_mem_p == nullptr) && (operation == WORK_MEM_OPERATION_FREE)) {
        return false;
    }

    if (operation == WORK_MEM_OPERATION_COMMIT) {
        qulonglong free, total, avail;

        assert(m_mem_p == nullptr);
        if (m_mem_p != nullptr) {
            VirtualMem::Free(m_mem_p);
            m_mem_p = nullptr;
        }

        if (!VirtualMem::GetMemoryStatus(free, total, avail)) {
            TRACEX_E("WorkMem failure, internal, couldn't stat memory")
                return false;
        }

        /* Start with requesting 70% of free */
        m_size = static_cast<int64_t>(static_cast<double>(free) * 0.7);
 
        if (m_size > DEFAULT_MAX_MEM_SIZE /*600MB*/) {
            m_size = DEFAULT_MAX_MEM_SIZE;
        }

        if (g_cfg_p->m_workMemSize != 0 && g_cfg_p->m_workMemSize < m_size) {
            m_size = g_cfg_p->m_workMemSize;
            if (m_size > 1024 * 1000) {
                TRACEX_I(QString("WorkMem reservation OVERRIDE  size:%1MB").arg(m_size >> 20))
            }
            else {
                TRACEX_I(QString("WorkMem reservation OVERRIDE  size:%1").arg(m_size))
            }
        } 
        
        /* Try to do the allocation, step down in size until success */
        while (m_mem_p == nullptr && m_size > DEFAULT_MIN_MEM_SIZE /*100kB*/) {
            m_mem_p = reinterpret_cast<char*>(VirtualMem::Alloc(m_size));
            if (m_mem_p == nullptr) {
                TRACEX_D(QString("Failed to allocated Work Memory, size:%1MB").arg(m_size >> 20))
                m_size -= static_cast<int>(static_cast<double>(m_size) * 0.10);
            }
        }

        if (m_mem_p == nullptr) {
            TRACEX_E("Failed to allocated Work Memory")
                return false;
        }

        m_status = WORK_MEM_OPERATION_COMMIT;
        TRACEX_D(QString("Work memory in use:%1MB (%2MB)").arg(m_size >> 20).arg(total >> 20))
        return true;
    }
    else if (operation == WORK_MEM_OPERATION_TINY_COMMIT) {
        assert(m_mem_p == nullptr);
        if (m_mem_p != nullptr) {
            VirtualMem::Free(m_mem_p);
            m_mem_p = nullptr;
        }

        m_size = DEFAULT_TINY_MEM_SIZE;
        m_mem_p = reinterpret_cast<char *>(VirtualMem::Alloc(m_size));
        if (m_mem_p == nullptr) {
            TRACEX_E("Failed to allocated Work Memory")
            return false;
        }

        m_status = WORK_MEM_OPERATION_TINY_COMMIT;
        TRACEX_D(QString("Work memory in use:%1").arg(m_size))
        return true;
    } else if (operation == WORK_MEM_OPERATION_FREE) {
        Q_ASSERT(m_mem_p != nullptr);
        Q_ASSERT(m_status != WORK_MEM_OPERATION_FREE);
        VirtualMem::Free(m_mem_p);
        m_mem_p = nullptr;
        m_size = 0;
        m_status = WORK_MEM_OPERATION_FREE;
        TRACEX_D("Released work memory")
        return true;
    } else {
        TRACEX_E("CLogScrutinizerDoc::WorkMem, Unknown operation - %d", operation)
        return false;
    }
}
