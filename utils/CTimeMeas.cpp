/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <iostream>
#include <chrono>

#include "CTimeMeas.h"
#include "CDebug.h"

using namespace std;
using namespace std::chrono;

/***********************************************************************************************************************
*   ReadFromSystem
***********************************************************************************************************************/
void CInternalTimer::ReadFromSystem()
{
    clock = std::chrono::steady_clock::now();
}
