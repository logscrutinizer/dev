/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <stddef.h>
#include <chrono>
#include <cmath>
#include <QDateTime>

using namespace std::chrono;

typedef struct {
    int64_t packTime;
    int64_t totalFilterTime;
}FilterExecTimes_t;

/*using ns100 = std::chrono::duration<int64_t, std::ratio<1, std::nano::den / 100>>; */

class CInternalTimer
{
public:
    std::chrono::time_point<steady_clock> clock;
    void ReadFromSystem();
};

/***********************************************************************************************************************
*   CTimeMeas
***********************************************************************************************************************/
class CTimeMeas
{
public:
    CTimeMeas(void)
    {
        m_startPoint.ReadFromSystem();
    }

    /****/
    void Restart(void)
    {
        m_startPoint.ReadFromSystem();
        m_triggerSet = false;
    }

    /****/
    void Trigger(void)
    {
        m_triggerPoint.ReadFromSystem();
        m_triggerSet = true;
    }

    /****/
    int64_t ms()
    {
        if (!m_triggerSet) {
            m_triggerPoint.ReadFromSystem();
        }

        auto diff = duration_cast<std::chrono::milliseconds>(m_triggerPoint.clock - m_startPoint.clock).count();
        return diff;
    }

    ~CTimeMeas(void) {}

private:
    bool m_triggerSet = {0};
    CInternalTimer m_startPoint;   /* This is the original starting point, it is used against the trigger point */
    CInternalTimer m_triggerPoint; /* This timer contains the last trigger point */
};

/***********************************************************************************************************************
*   CTimeFilter
***********************************************************************************************************************/
class CTimeFilter
{
    QDateTime m_next;
    int m_filterTime_ms;

public:
    CTimeFilter(int filterTime_ms) : m_next(QDateTime::currentDateTime()), m_filterTime_ms(filterTime_ms) {}

    /****/
    bool Check(void)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (QDateTime::currentDateTime() >= m_next) {
            m_next = (QDateTime::currentDateTime()).addMSecs(m_filterTime_ms);
            return true;
        }
        return false;
    }
};
