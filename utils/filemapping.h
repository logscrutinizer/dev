/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include "CConfig.h"
#include "globals.h"
#include "CFilter.h"

typedef struct {
    int32_t headerSize; /**< sizeof(TIA_FileHeader_t); */
    int32_t fileVersion; /**< TIA_FILE_VERSION */
    int32_t numOfRows;
    int64_t fileSize; /**< Size of the log file */
    int64_t fileTime;
}TIA_FileHeader_t;

namespace FileMapping
{
/* if checkFileSize is true then the TIA mapping will fail if its not equal. This shoule be used when initially
 * loading the TIA file, whereas later on the file can be tracked (in-case data is being streamed). */
    bool CreateTIA_MemMapped(QFile& Log_File, QFile& TIA_File, int *rows_p, TI_t *& TIA_mem_p, int64_t *fileSize,
                             bool check = false);

    bool RemoveTIA_MemMapped(QFile& TIA_File, TI_t *& TIA_mem_p, bool forceRemove_TIA_file);
    bool CreateFIRA_MemMapped(QFile& FIRA_File, FIR_t *& FIRA_mem_p, const int rows);
    bool IncrementalFIRA_MemMap(QFile& FIRA_File, FIR_t *& FIRA_mem_p, const int totalRows);
    bool RemoveFIRA_MemMapped(QFile& FIRA_File, FIR_t *& FIRA_mem_p, bool remove = true);
}
