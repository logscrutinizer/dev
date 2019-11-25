/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
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
    enum timeSpan {
        sec,
        milli,
        micro
    };

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

    /****/
    int64_t elapsed(timeSpan ts)
    {
        if (!m_triggerSet) {
            m_triggerPoint.ReadFromSystem();
        }

        int64_t diff;

        if (ts == milli) {
            diff = duration_cast<std::chrono::milliseconds>(m_triggerPoint.clock - m_startPoint.clock).count();
        } else if (ts == micro) {
            diff = duration_cast<std::chrono::microseconds>(m_triggerPoint.clock - m_startPoint.clock).count();
        } else {
            diff = duration_cast<std::chrono::seconds>(m_triggerPoint.clock - m_startPoint.clock).count();
        }
        return diff;
    }

    ~CTimeMeas(void) {}

private:
    bool m_triggerSet = {0};
    CInternalTimer m_startPoint;   /* This is the original starting point, it is used against the trigger point */
    CInternalTimer m_triggerPoint; /* This timer contains the last trigger point */
};

/***********************************************************************************************************************
*   CTimeAverage
***********************************************************************************************************************/
class CTimeAverage : public CTimeMeas
{
public:
    /****/
    void start(void)
    {
        Restart();
        count++;
    }

    /****/
    void stop(timeSpan ts = milli)
    {
        auto last_elapsed = elapsed(ts);
        sum += last_elapsed;
        if ((min == -1) || (min > last_elapsed)) {
            min = last_elapsed;
        }
        if ((max == -1) || (max < last_elapsed)) {
            max = last_elapsed;
        }
    }

    /****/
    void reset(void)
    {
        if (!count) {
            stop();
        }
        sum = 0;
        count = 0;
        min = -1;
        max = -1;
    }

    /****/
    int64_t avg(void)
    {
        return sum / count;
    }

private:
    int count = 0;
    int64_t sum = 0;
    int64_t min = -1;
    int64_t max = -1;
};

/***********************************************************************************************************************
*   CTimePeriod
***********************************************************************************************************************/
class CTimePeriod
{
    QDateTime m_next;
    int m_filterTime_ms;

public:
    CTimePeriod(int filterTime_ms) : m_next(QDateTime::currentDateTime()), m_filterTime_ms(filterTime_ms) {}

    /****/
    bool isNext(void)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (QDateTime::currentDateTime() >= m_next) {
            m_next = (QDateTime::currentDateTime()).addMSecs(m_filterTime_ms);
            return true;
        }
        return false;
    }
};
