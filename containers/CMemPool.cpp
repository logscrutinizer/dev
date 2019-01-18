/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
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

        if (!VirtualMem::GetMemoryStatus(free, total, avail)) {
            TRACEX_E("WorkMem failure, internal, couldn't stat memory");
            return false;
        }

        /* Set maximum workmemory to the max value of a signed 32-bit int (31 bits), about 2GB */
        m_size = static_cast<int64_t>((double)free * 0.7);

        /* Override */
        if (g_cfg_p->m_workMemSize != 0) {
            m_size = g_cfg_p->m_workMemSize;
            TRACEX_I("WorkMem reservation OVERRIDE  size:%dMB", m_size >> 20);
        } else if (m_size > DEFAULT_MAX_MEM_SIZE) {
            /* if not overriden, limit the memory alloc to DEFAULT_MAX_MEM_SIZE */
            m_size = DEFAULT_MAX_MEM_SIZE;
        }

        m_mem_p = nullptr;
        while (m_mem_p == nullptr && m_size > 100000) {
            m_mem_p = (char *)VirtualMem::Alloc(m_size);

            if (m_mem_p == nullptr) {
                TRACEX_D("Failed to allocated Work Memory, size:%dMB", m_size >> 20);
                m_size -= (int)(m_size * 0.10);
            }
        }

        if (m_mem_p == nullptr) {
            TRACEX_E("Failed to allocated Work Memory");
            return false;
        }

        m_status = WORK_MEM_OPERATION_COMMIT;
        TRACEX_D("Work memory in use:%dMB (%dMB)",
                 static_cast<uint32_t>(m_size >> 20), static_cast<uint32_t>(total >> 20));
        return true;
    } else if (operation == WORK_MEM_OPERATION_FREE) {
        VirtualMem::Free(m_mem_p);

        m_mem_p = nullptr;
        m_status = WORK_MEM_OPERATION_FREE;
        TRACEX_D("Released work memory");
        return true;
    } else {
        TRACEX_E("CLogScrutinizerDoc::WorkMem, Unknown operation - %d", operation);
        return false;
    }
}
