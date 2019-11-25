/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CDebug.h"
#include "CConfig.h"
#include "filemapping.h"
#include "CFilter.h"

#include <QRgb>
#include <QFileDevice>
#include <QFileInfo>
#include <QDateTime>

namespace FileMapping
{
    /* This function will open the file specified by TIA_File and then map memory to it */
    bool CreateTIA_MemMapped(QFile& Log_File, QFile& TIA_File, int *rows_p, TI_t *& TIA_mem_p, int64_t *fileSize_p,
                             bool check)
    {
        QFileInfo TIA_fileInfo(TIA_File);
        QFileInfo LOG_fileInfo(Log_File);
        TIA_fileInfo.refresh();
        LOG_fileInfo.refresh();
        *fileSize_p = 0;

        if (TRACEX_IS_ENABLED(LOG_LEVEL_DEBUG)) {
            TRACEX_D(QString("Creating TIA memory mapped using file: %1").arg(TIA_File.fileName()))
        }

        if (TIA_File.fileName().isEmpty()) {
            TRACEX_E("CreateTIA_MemMapped Failed - TIA File name is not setup")
            return false;
        }

        if (!TIA_File.exists()) {
            TRACEX_I("CreateTIA_MemMapped Failed - TIA File doesn't exist")
            return false;
        }

        if (!TIA_File.open(QIODevice::ReadOnly)) {
            TRACEX_QFILE(LOG_LEVEL_INFO, "FileMapping::CreateTIA_MemMapped - file could not be opened", &TIA_File)
            return false;
        }

        if (TIA_fileInfo.size() <= static_cast<int64_t>(sizeof(TIA_FileHeader_t))) {
            TRACEX_I("FileMapping::CreateTIA_MemMapped - Failed, file exists but is empty", &TIA_File)
            return false;
        }

        /* start by using the memory as the header, this will be later cast to the TIA array */
        TIA_FileHeader_t *header_p = reinterpret_cast<TIA_FileHeader_t *>(TIA_File.map(0, TIA_fileInfo.size()));
        if (header_p == nullptr) {
            TRACEX_E(QString("Failed to memory map TIA file - memory issue %1").arg(TIA_File.fileName()))
            return false;
        }

        /* Check the header */
        bool headerOK = true;
        *rows_p = header_p->numOfRows;

        if (header_p->fileVersion != TIA_FILE_VERSION) {
            headerOK = false;
        }

        auto fileSize = LOG_fileInfo.size();
        if (headerOK && (fileSize != header_p->fileSize)) {
            /* check the file size, must be the same (file size of the original log file, and the value in the TIA
             *  file header) */
            TRACEX_I(QString("TIA file size not matching TIA:%1 Log:%2 (Incremental?)")
                         .arg(header_p->fileSize).arg(fileSize))
            if (check) {
                headerOK = false;
            }
        }

        *fileSize_p = fileSize;

        if (headerOK) {
            /* Check the last changed file date, must be the same */

            QDateTime dateTime(LOG_fileInfo.lastModified());
            int64_t sinceEpoch = dateTime.toMSecsSinceEpoch();

            if (sinceEpoch != header_p->fileTime) {
                TRACEX_I(QString("TIA file time not matching, TIA:%1 Log:%2")
                             .arg(header_p->fileTime).arg(sinceEpoch))
                if (check) {
                    headerOK = false;
                }
            }
        }

        if (!headerOK) {
            TIA_File.unmap(reinterpret_cast<uint8_t *>(header_p));
            TIA_mem_p = nullptr;

            if (*rows_p != -1) {
                TRACEX_W(QString("The TIA file header seems corrupt.\n Please remove the TIA file %1 and try again")
                             .arg(TIA_File.fileName()))
            }

            return false;
        }

        *rows_p = header_p->numOfRows;

        /* Step pass the header, and let TIA_mem_p point to the first TIA row */
        TIA_mem_p = reinterpret_cast<TI_t *>(++header_p);
        return true;
    }

    /****/
    bool RemoveTIA_MemMapped(QFile& TIA_File, TI_t *& TIA_mem_p, bool forceRemove_TIA_file)
    {
        /* TIA is in a memory mapped file */

        TRACEX_D("FileMapping::RemoveTIA_MemMapped %s",
                 TIA_File.fileName().toLatin1().constData())

        if (!TIA_File.isOpen()) {
            TRACEX_E("Failed to remove the memory mapped TIA file, it is not open: s%",
                     TIA_File.fileName().toLatin1().constData())
            return false;
        }

        if (TIA_mem_p == nullptr) {
            TRACEX_E("Failed to remove the memory mapped TIA file, memory was nullptr: s%",
                     TIA_File.fileName().toLatin1().constData())
            return false;
        }

        /* stepping back the header */
        uint8_t *temp_p = (reinterpret_cast<uint8_t *>(TIA_mem_p) - sizeof(TIA_FileHeader_t));

        TIA_File.unmap(temp_p);
        TIA_File.close();

        TIA_mem_p = nullptr;

        if (forceRemove_TIA_file || !g_cfg_p->m_keepTIA_File) {
            if (!TIA_File.remove()) {
                TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to remove the memory mapped TIA file", &TIA_File)
            }
        }

        TRACEX_D("Unmapped the TIA file %s", TIA_File.fileName().toLatin1().constData())
        return true;
    }

    /****/
    bool CreateFIRA_MemMapped(QFile& FIRA_File, FIR_t *& FIRA_mem_p, const int rows)
    {
        if (FIRA_File.fileName().isEmpty()) {
            TRACEX_E("CreateFIRA_MemMapped Failed - FIRA File name is not setup")
            return false;
        }

        if (!FIRA_File.open(QIODevice::ReadWrite)) {
            /* On error */
            TRACEX_I(QString("FIRA file couldn't be open/created %1\n%2")
                         .arg(FIRA_File.fileName().toLatin1().constData()).arg(FIRA_File.errorString()))
            return false;
        }

        if (!FIRA_File.resize(static_cast<int>(sizeof(FIR_t)) * rows)) {
            TRACEX_E("Mapping FIRA file failed, file couldn't be resized %s, "
                     "please try again",
                     FIRA_File.fileName().toLatin1().constData())
            return false;
        }

        FIRA_mem_p = reinterpret_cast<FIR_t *>(FIRA_File.map(0, static_cast<int>(sizeof(FIR_t)) * rows));

        if (FIRA_mem_p == nullptr) {
            TRACEX_E("Mapping FIRA file failed, memory error")
            return false;
        }

        TRACEX_I("FIRA file memory mapped: %s", FIRA_File.fileName().toLatin1().constData())

        memset(FIRA_mem_p, 0, sizeof(FIR_t) * static_cast<size_t>(rows));    /* FIRA Use OK */
        return true;
    }

    /* This function extends the existing FIRA file with the additional rows that has recently been loaded */
    bool IncrementalFIRA_MemMap(QFile& FIRA_File, FIR_t *& FIRA_mem_p, const int totalRows)
    {
        if (FIRA_File.fileName().isEmpty()) {
            TRACEX_E(QString("%1 Failed - No FIRA file name").arg(__FUNCTION__))
            return false;
        }

        if (!FIRA_File.isOpen()) {
            if (!FIRA_File.open(QIODevice::ReadWrite)) {
                /* On error */
                TRACEX_I(QString("FIRA file couldn't be open/created %1\n%2")
                             .arg(FIRA_File.fileName().toLatin1().constData()).arg(FIRA_File.errorString()))
                return false;
            }
        } else {
            if (!FIRA_File.unmap(reinterpret_cast<uchar *>(FIRA_mem_p))) {
                TRACEX_I(QString("%1 Failed to unmap %2")
                             .arg(__FUNCTION__).arg(FIRA_File.fileName().toLatin1().constData()))
            }
        }

        /* Increase size of FIRA file */
        if (!FIRA_File.resize(static_cast<int64_t>(sizeof(FIR_t)) * totalRows)) {
            TRACEX_E(QString("%1 Mapping FIRA file failed, file couldn't be resized %2")
                         .arg(__FUNCTION__).arg(FIRA_File.fileName().toLatin1().constData()))
            return false;
        }

        FIRA_mem_p = reinterpret_cast<FIR_t *>(FIRA_File.map(0, static_cast<int64_t>(sizeof(FIR_t)) * totalRows));

        if (FIRA_mem_p == nullptr) {
            TRACEX_E(QString("%1 Mapping FIRA file failed, memory error").arg(__FUNCTION__))
            return false;
        }

        TRACEX_D("FIRA file memory mapped: %s", FIRA_File.fileName().toLatin1().constData())

        return true;
    }

    /****/
    bool RemoveFIRA_MemMapped(QFile& FIRA_File, FIR_t *& FIRA_mem_p, bool remove)
    {
        TRACEX_D("FileMapping::RemoveFIRA_MemMapped %s",
                 FIRA_File.fileName().toLatin1().constData())

        if (!FIRA_File.isOpen()) {
            TRACEX_E("Failed to remove the memory mapped FIRA file, it is not open: s%",
                     FIRA_File.fileName().toLatin1().constData())
            return false;
        }

        if (FIRA_mem_p == nullptr) {
            TRACEX_E("Failed to remove the memory mapped FIRA file, memory was nullptr: s%",
                     FIRA_File.fileName().toLatin1().constData())
            return false;
        }

        FIRA_File.unmap(reinterpret_cast<uchar *>(FIRA_mem_p));
        FIRA_File.close();

        FIRA_mem_p = nullptr;

        if (remove && !FIRA_File.remove()) {
            TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to remove the memory mapped TIA file", &FIRA_File)
        }

        TRACEX_D("Unmapped the FIRA file %s", FIRA_File.fileName().toLatin1().constData())
        return true;
    }
} /* namespace */
