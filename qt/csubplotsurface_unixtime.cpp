/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <stdlib.h>
#include <array>

#include "math.h"

#include "csubplotsurface.h"
#include "CLogScrutinizerDoc.h"

#include "CDebug.h"
#include "globals.h"
#include "CFontCtrl.h"

extern QPen *g_plotWnd_defaultPen_p;

/* X-axis - based on Unix Time
 *
 * This file contains the implementation of how to present the plot X-axis.
 *
 * To present the x-axis with easy to understand time scale 4 different time-units/levels was introduced.
 *
 * 1. Anchor - The most important point in time that is currently visible in the surfaceZoom.
 *      - All other drawn time-units will be related to the anchor
 *      - The anchor is also high-lighted, and more time attributes are added such as year, month, date
 * 2. Major point - The highest largest stepping across the X-axis that is put in text
 * 3. Minor point - Less or equal step to Major, is also shown but with less. When there is not enough space to show all
 *      the minor lables some are stepped over.
 * 4. Micro point - This is the smallest step through the x-axis, shown with just a line.
 *
 *
 *  The idea is
 *
 * 1. Identify the highest granularity time-crossing that occurs in the visible zoom. E.g., if there is a year crossing
 *      this is considered more important than a month crossing. This crossing is set to anchor.
 * 2. Define the granularity to use for major, minor and micro stepping. This is determined by how much space that
 *      is available for printing the time labels, and of course the time span.
 *      It's enough that one or two major lables are shown, however minor lables
 *      should be a lot more. Micro stepping is
 * 3. From the anchor, first step forward in time and draw the major/minor labels and micro lines. The do the same
 *      while stepping backwards.
 *
 * What to print for major and minor lables are different, for major lables more information is shown, such that even
 * if the major step is in years, both the month and date is shown as well. However for minor labels just the year
 * number
 * is shown.
 *
 */

/***********************************************************************************************************************
***********************************************************************************************************************/
bool CSubPlotSurface::Draw_X_Axis_UnixTime(void)
{
    /*
     * The possible values for the std::tm struct
     *
     * tm_sec   int seconds after the minute    0-60*
     *  tm_min  int minutes after the hour  0-59
     *  tm_hour int hours since midnight    0-23
     *  tm_mday int day of the month    1-31
     *  tm_mon  int months since January    0-11
     *  tm_year int years since 1900
     *  tm_wday int days since Sunday   0-6
     *  tm_yday int days since January 1    0-365
     *  tm_isdst    int Daylight Saving Time flag
     */

    auto x_min_sec = std::max(static_cast<std::time_t>(0), static_cast<std::time_t>(m_surfaceZoom.x_min));
    auto x_max_sec = static_cast<std::time_t>(m_surfaceZoom.x_max);
    auto x_min_msec = std::max(static_cast<qint64>(0), static_cast<qint64>(std::round(m_surfaceZoom.x_min * 1000.0)));
    auto x_max_msec = static_cast<qint64>(std::round(m_surfaceZoom.x_max * 1000.0));
    auto span_ms = x_max_msec - x_min_msec;
    auto x_min_tm = *std::localtime(&x_min_sec);
    auto x_max_tm = *std::localtime(&x_max_sec);
    XLineConfig_t xconfig;
    qint64 anchor_ms = 0;
    QDateTime anchor_dt;

    /* Locate best position for the Anchor, this is where we have a time crossing, e.g. over a year
     * month, minutes etc. */

    if (x_min_tm.tm_year != x_max_tm.tm_year) {
        xconfig.anchorTime.tm_year = x_min_tm.tm_year + 1;
        xconfig.anchorCfg = allowedTimePeriods[static_cast<int>(TimePeriod::Year)];
    } else {
        xconfig.anchorTime.tm_year = x_min_tm.tm_year;

        if (x_min_tm.tm_mon != x_max_tm.tm_mon) {
            xconfig.anchorTime.tm_mon = x_min_tm.tm_mon + 1;
            xconfig.anchorCfg = allowedTimePeriods[static_cast<int>(TimePeriod::Month)];
        } else {
            xconfig.anchorTime.tm_mon = x_min_tm.tm_mon;

            if (x_min_tm.tm_mday != x_max_tm.tm_mday) {
                xconfig.anchorTime.tm_mday = x_min_tm.tm_mday + 1;
                xconfig.anchorCfg = allowedTimePeriods[static_cast<int>(TimePeriod::Day)];
            } else {
                xconfig.anchorTime.tm_mday = x_min_tm.tm_mday;

                if (x_min_tm.tm_hour != x_max_tm.tm_hour) {
                    xconfig.anchorTime.tm_hour = x_min_tm.tm_hour + 1;
                    xconfig.anchorCfg = allowedTimePeriods[static_cast<int>(TimePeriod::Hour)];
                } else {
                    xconfig.anchorTime.tm_hour = x_min_tm.tm_hour;

                    if (x_min_tm.tm_min != x_max_tm.tm_min) {
                        xconfig.anchorTime.tm_min = x_min_tm.tm_min + 1;
                        xconfig.anchorCfg = allowedTimePeriods[static_cast<int>(TimePeriod::Min)];
                    } else {
                        xconfig.anchorTime.tm_min = x_min_tm.tm_min;

                        if ((x_min_tm.tm_sec != x_max_tm.tm_sec) && (span_ms > 1000)) {
                            xconfig.anchorTime.tm_sec = x_min_tm.tm_sec + 1;
                            xconfig.anchorCfg = allowedTimePeriods[static_cast<int>(TimePeriod::Sec)];
                        } else {
                            if (span_ms > 1000) {
                                TRACEX_I(QString("Failed to draw using UnixTime, interval diff error %1").arg(
                                             span_ms));
                                return false;
                            }
                            if (span_ms < 10) {
                                PRINT_CPLOTWIDGET_GRAPHICS(
                                    QString("Diff less than 10ms, fallback to legacy: diff  %1").arg(span_ms));
                                return false;
                            }
                            if (span_ms < 100) {
                                xconfig.anchorCfg = allowedTimePeriods[static_cast<int>(TimePeriod::Milli10)];
                            } else {
                                xconfig.anchorCfg = allowedTimePeriods[static_cast<int>(TimePeriod::Milli100)];
                            }

                            /* Check if the sec based anchor is inside millisec range */

                            xconfig.anchorTime.tm_sec = x_min_tm.tm_sec;
                            anchor_dt.setSecsSinceEpoch(std::mktime(&xconfig.anchorTime));
                            anchor_ms = anchor_dt.toMSecsSinceEpoch();

                            auto anchor_ms_p1 = anchor_ms + 1000; /* could be that anchor is not part of tm_sec but plus
                                                                   * 1sec */

                            if ((anchor_ms_p1 > x_min_msec) && (anchor_ms_p1 < x_max_msec)) {
                                xconfig.anchorTime.tm_sec = x_min_tm.tm_sec + 1;
                                anchor_dt.setSecsSinceEpoch(std::mktime(&xconfig.anchorTime));
                                anchor_ms = anchor_dt.toMSecsSinceEpoch();
                            } else if ((anchor_ms < x_min_msec) || (anchor_ms > x_max_msec)) {
                                /* Now special special... to support milli-second granularity
                                 * Since there were no "second" diff between min and max it is then less than 1000ms */
                                anchor_ms = static_cast<qint64>(std::floor(x_min_msec / 100.0) * 100.0);
                                if (span_ms < 100) {
                                    while ((anchor_ms <= x_min_msec + span_ms / 2) && (anchor_ms + 10 < x_max_msec)) {
                                        anchor_ms += 10;
                                    }
                                } else {
                                    while ((anchor_ms <= x_min_msec + span_ms / 2) && (anchor_ms + 100 < x_max_msec)) {
                                        anchor_ms += 100;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    EnumerateUnixTimeStringLengths();

    /* seconds since epoc */
    if (anchor_ms == 0) {
        anchor_dt.setSecsSinceEpoch(std::mktime(&xconfig.anchorTime));
        anchor_ms = anchor_dt.toMSecsSinceEpoch();
    } else {
        anchor_dt = anchor_dt.fromMSecsSinceEpoch(anchor_ms);
    }

    auto ts_str = anchor_dt.toString(Qt::ISODateWithMs /*SystemLocaleShortDate*/);
    PRINT_CPLOTWIDGET_GRAPHICS(QString("Anchor: %1").arg(ts_str));

    if ((anchor_ms < x_min_msec) || (anchor_ms > x_max_msec)) {
        QDateTime timestamp;
        timestamp.fromMSecsSinceEpoch(x_min_msec);

        auto min = timestamp.toString(Qt::ISODateWithMs /*SystemLocaleShortDate*/);
        timestamp.fromMSecsSinceEpoch(x_max_msec);

        auto max = timestamp.toString(Qt::ISODateWithMs /*SystemLocaleShortDate*/);
        PRINT_CPLOTWIDGET_GRAPHICS(QString("Anchor outside min %1   max %2").arg(min).arg(max));
        return false;
    }

    auto idx = xconfig.anchorCfg.idx;

    /* Decide the best sized Major time step. There should be at least one Major label */
    while (idx < MAX_ALLOWED_TIME_PERIODS) {
        auto lines = span_ms / allowedTimePeriods[idx].msecInPeriod;
        if (lines <= m_maxLengthArray[idx].majorMaxWordCount) {
            xconfig.majorCfg = allowedTimePeriods[idx];
            PRINT_CPLOTWIDGET_GRAPHICS(
                QString("major:%1 lines:%2 idx:%3 req_max:%4")
                    .arg(xconfig.majorCfg.periodToStr())
                    .arg(lines)
                    .arg(idx)
                    .arg(m_maxLengthArray[idx].majorMaxWordCount));
            if (lines >= 1) {
                break;
            }
            idx++;
        } else {
            idx--;
            break;
        }
    }

    if (xconfig.majorCfg.period == TimePeriod::NA) {
        xconfig.majorCfg = xconfig.anchorCfg;
    }

    /* Decide the best sized Minor time step.. The minor step starts at one finer granularity level compare to major.
     *  Note, by setting minorSkips we allow a finer minor-step where we skip some of the labelsThe get a proper
     * skipping we used
     *  the skipConfig. */
    int minorSkips = 0;
    idx++;
    while (idx < MAX_ALLOWED_TIME_PERIODS) {
        auto lines = span_ms / allowedTimePeriods[idx].msecInPeriod;
        if (lines > m_maxLengthArray[idx].minorMaxWordCount) {
            /* If no minor step config has been found previously we configure time/step skipping.
             * This isn't necessary if we in previous loop found a config that would fit. */
            if (xconfig.minorCfg.period == TimePeriod::NA) {
                xconfig.minorCfg = allowedTimePeriods[idx];

                auto less_lines = lines;
                int skipConfigIdx = 0;

                while (less_lines > m_maxLengthArray[idx].minorMaxWordCount) {
                    if ((allowedTimePeriods[idx].skipConfig_p != nullptr) &&
                        (skipConfigIdx < allowedTimePeriods[idx].skipConfig_p->length)) {
                        minorSkips = allowedTimePeriods[idx].skipConfig_p->recSkips[skipConfigIdx];
                        skipConfigIdx++;
                    } else {
                        minorSkips += 1;
                    }

                    less_lines = lines / (1.0 + minorSkips);
                }
                PRINT_CPLOTWIDGET_GRAPHICS(QString("minor:%1 lines:%2 less_lines:%3 idx:%4 skips:%5").arg(
                                               xconfig.minorCfg.periodToStr()).arg(
                                               lines).arg(less_lines).arg(idx).arg(minorSkips));
            } else {
                idx--;
            }
            break;
        }
        if (lines >= 10) {
            xconfig.minorCfg = allowedTimePeriods[idx];
            PRINT_CPLOTWIDGET_GRAPHICS(QString("minor:%1 lines:%2 idx:%3").arg(xconfig.minorCfg.periodToStr()).arg(
                                           lines).arg(idx));
        }
        idx++;
    }

    if (xconfig.minorCfg.period == TimePeriod::NA) {
        xconfig.minorCfg = xconfig.majorCfg;
    }

    /* Micro */
    auto numMicroLinesEstimate = static_cast<int>(m_viewPortRect.width() / 10);
    while (idx < MAX_ALLOWED_TIME_PERIODS) {
        auto lines = span_ms / allowedTimePeriods[idx].msecInPeriod;
        if (lines < numMicroLinesEstimate) {
            if (allowedTimePeriods[idx].micro_allowed) {
                xconfig.microCfg = allowedTimePeriods[idx];
                PRINT_CPLOTWIDGET_GRAPHICS(QString("micro:%1 lines:%2").arg(xconfig.microCfg.periodToStr()).arg(lines));
                if (lines >= numMicroLinesEstimate) {
                    break;
                }
            }
            idx++;
        } else {
            break;
        }
    }

    if (xconfig.microCfg.period == TimePeriod::NA) {
        xconfig.microCfg = xconfig.minorCfg;
    }

    QPoint startPoint;
    QPoint endPoint;
    endPoint.setY(m_viewPortRect.bottom());
    startPoint.setY(endPoint.y() - m_avgPixPerLetterHeight);

    m_painter_p->setPen(*g_plotWnd_defaultPen_p);
    g_plotWnd_defaultPen_p->setColor(Q_RGB(g_cfg_p->m_plot_GrayIntensity,
                                           g_cfg_p->m_plot_GrayIntensity,
                                           g_cfg_p->m_plot_GrayIntensity));

    /* Step forward from anchor */

    QDateTime nextMajor_dt = anchor_dt;
    QDateTime nextMinor_dt = anchor_dt;
    QDateTime nextMicro_dt = anchor_dt;
    StepTime(nextMajor_dt, xconfig.majorCfg.period, true);
    StepTime(nextMinor_dt, xconfig.minorCfg.period, true);
    StepTime(nextMicro_dt, xconfig.microCfg.period, true);

    /* 1. Draw the anchor first
     * 2. From the anchor step forward with micro steps, when passing a major boarder show that label,
     *      then check the same for the minor and draw that if it was crossed.
     * 3. After reaching the end of the visible zoom step backwards from the anchor and do the same as
     *      in (2).
     */
    auto slist = GetMajorStrings(anchor_dt, xconfig.majorCfg.period, true, 1);
    Draw_UniXTimeLabel(slist, anchor_ms);

    auto tempSkips = minorSkips;
    auto ts = nextMicro_dt.toMSecsSinceEpoch();
    auto major_ts = nextMajor_dt.toMSecsSinceEpoch();
    auto minor_ts = nextMinor_dt.toMSecsSinceEpoch();

    while (ts < x_max_msec) {
        if (ts >= major_ts) {
            auto appendCount = xconfig.majorCfg.period == xconfig.minorCfg.period ? 0 : 1;
            auto slist = GetMajorStrings(nextMajor_dt, xconfig.majorCfg.period, false, appendCount);
            Draw_UniXTimeLabel(slist, major_ts);
            PRINT_CPLOTWIDGET_GRAPHICS(QString("major ts: %1").arg(nextMajor_dt.toString("hh:mm:ss.zzz")));

            nextMinor_dt = nextMajor_dt;
            nextMicro_dt = nextMajor_dt;
            StepTime(nextMajor_dt, xconfig.majorCfg.period, true);
            StepTime(nextMinor_dt, xconfig.minorCfg.period, true);
            tempSkips = minorSkips;
            major_ts = nextMajor_dt.toMSecsSinceEpoch();
            minor_ts = nextMinor_dt.toMSecsSinceEpoch();
        } else if (ts >= minor_ts) {
            if (tempSkips == 0) {
                auto slist = GetMinorStrings(nextMinor_dt, xconfig.minorCfg.period);
                Draw_UniXTimeLabel(slist, minor_ts);
                tempSkips = minorSkips;
            } else {
                Draw_UnixTimeXLine(ts);
                tempSkips--;
            }

            nextMicro_dt = nextMinor_dt;
            StepTime(nextMinor_dt, xconfig.minorCfg.period, true);
            minor_ts = nextMinor_dt.toMSecsSinceEpoch();
        } else {
            Draw_UnixTimeXLine(ts);
        }

        StepTime(nextMicro_dt, xconfig.microCfg.period, true);
        ts = nextMicro_dt.toMSecsSinceEpoch();
    }

    /* Step back from anchor */

    nextMajor_dt = anchor_dt;
    nextMinor_dt = anchor_dt;
    nextMicro_dt = anchor_dt;
    StepTime(nextMajor_dt, xconfig.majorCfg.period, false);
    StepTime(nextMinor_dt, xconfig.minorCfg.period, false);

    StepTime(nextMicro_dt, xconfig.microCfg.period, false);

    tempSkips = minorSkips;
    major_ts = nextMajor_dt.toMSecsSinceEpoch();
    minor_ts = nextMinor_dt.toMSecsSinceEpoch();
    ts = nextMicro_dt.toMSecsSinceEpoch();

    while (ts > x_min_msec) {
        if (ts <= major_ts) {
            auto slist = GetMajorStrings(nextMajor_dt, xconfig.majorCfg.period);
            Draw_UniXTimeLabel(slist, major_ts);
            nextMinor_dt = nextMajor_dt;
            nextMicro_dt = nextMajor_dt;
            StepTime(nextMajor_dt, xconfig.majorCfg.period, false);
            StepTime(nextMinor_dt, xconfig.minorCfg.period, false);
            tempSkips = minorSkips;
            major_ts = nextMajor_dt.toMSecsSinceEpoch();
            minor_ts = nextMinor_dt.toMSecsSinceEpoch();
        } else if (ts <= minor_ts) {
            if (tempSkips == 0) {
                auto slist = GetMinorStrings(nextMinor_dt, xconfig.minorCfg.period);
                Draw_UniXTimeLabel(slist, minor_ts);
                tempSkips = minorSkips;
            } else {
                Draw_UnixTimeXLine(ts);
                tempSkips--;
            }
            nextMicro_dt = nextMinor_dt;
            StepTime(nextMinor_dt, xconfig.minorCfg.period, false);
            minor_ts = nextMinor_dt.toMSecsSinceEpoch();
        } else {
            Draw_UnixTimeXLine(ts);
        }

        StepTime(nextMicro_dt, xconfig.microCfg.period, false);
        ts = nextMicro_dt.toMSecsSinceEpoch();
    }

    m_painter_p->setBackgroundMode(Qt::TransparentMode);
    return true;
}

/***********************************************************************************************************************
*   EnumerateUnixTimeStringLengths
*
* In order to identify how many pixels the different major/minor lables would require the allowedTimePeriods array
* is decorated with the values, based on the different time periods (stepping).
***********************************************************************************************************************/
void CSubPlotSurface::EnumerateUnixTimeStringLengths(void)
{
    QDate date(2021, 1, 12);
    QDateTime dt;
    dt.setDate(date);

    CLogScrutinizerDoc *doc_p = GetTheDoc();

	auto& a = m_maxLengthArray;

	auto hasValues = true;
	for (auto& item : a) {
		if (item.majorMaxWordCount == 0 || item.majorWidth == 0 || item.minorMaxWordCount == 0 || item.minorWidth == 0) {
			hasValues = false;
		}
	}

	for (auto month = 0; month < 12; month++) {
        dt = dt.addMonths(1);

        for (size_t idx = 0; idx < a.size(); idx++) {
            auto sl = GetMajorStrings(dt, allowedTimePeriods[idx].period);
            QString max_s("");
            for (auto& s : sl) {
                if (s.length() > max_s.length()) {
                    max_s = s;
                }
            }

            auto lineSize = doc_p->m_fontCtrl.GetTextPixelLength(m_painter_p, max_s);
            auto w = std::max(a[idx].majorWidth, lineSize.width() + 5);

			if (hasValues && w == a[idx].majorWidth) {
				return; // early exit
			}

			a[idx].majorWidth = w;

            sl = GetMinorStrings(dt, allowedTimePeriods[idx].period);
            max_s = QString("");
            for (auto& s : sl) {
                if (s.length() > max_s.length()) {
                    max_s = s;
                }
            }
            lineSize = doc_p->m_fontCtrl.GetTextPixelLength(m_painter_p, max_s);

			if (lineSize.width() > 640) {
				return;
			}

            a[idx].minorWidth = std::max(a[idx].minorWidth, lineSize.width() + 5);
        }
    }

    for (size_t i = 0; i < a.size(); i++) {
        a[i].minorMaxWordCount = m_viewPortRect.width() / a[i].minorWidth;
        a[i].majorMaxWordCount = m_viewPortRect.width() / a[i].majorWidth;
    }
}

/***********************************************************************************************************************
*   Draw_UnixTimeXLine
*
* * Just draw the basic time line, used for Micro steps.
***********************************************************************************************************************/
void CSubPlotSurface::Draw_UnixTimeXLine(qint64 ts /*millisec since epoc*/)
{
    QPoint startPoint;
    QPoint endPoint;

    endPoint.setY(m_viewPortRect.bottom());
    startPoint.setY(endPoint.y() - m_avgPixPerLetterHeight);

    startPoint.setX(static_cast<int>(m_viewPortRect.left() + ((ts / 1000.0) - m_surfaceZoom.x_min) *
                                     m_unitsPerPixel_X_inv));
    endPoint.setX(startPoint.x());
    m_painter_p->drawLine(startPoint, endPoint);
}

/***********************************************************************************************************************
*   Draw_XLabel
*
* The provided QStringList is printed at the time-stamp (ts). By adding empty lines to the list
* the labels will be moved higher up, this is used to e.g. get a better high-lighting for Anchors and Major labels
***********************************************************************************************************************/
void CSubPlotSurface::Draw_UniXTimeLabel(const QStringList& labels, qint64 ts /*milli since epoc*/)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    QPoint startPoint;
    QPoint endPoint;

    endPoint.setY(m_viewPortRect.bottom());
    startPoint.setY(endPoint.y() - m_avgPixPerLetterHeight);

    startPoint.setX(static_cast<int>(m_viewPortRect.left() + ((ts / 1000.0) - m_surfaceZoom.x_min) *
                                     m_unitsPerPixel_X_inv));
    endPoint.setX(startPoint.x());

    auto len = labels.length();
    auto y_offset = startPoint.y() - m_avgPixPerLetterHeight * len;
    for (auto& s : labels) {
        if (s.length() > 0) {
            auto lineSize = doc_p->m_fontCtrl.GetTextPixelLength(m_painter_p, s);
            auto x_text = std::max(m_viewPortRect.left(), startPoint.x() - lineSize.width() / 2);
            x_text = std::min(m_viewPortRect.right() - lineSize.width(), x_text);
            m_painter_p->drawText(QPoint(x_text, y_offset), s);
            y_offset += m_avgPixPerLetterHeight;
        }
    }

    startPoint.setY(y_offset - m_avgPixPerLetterHeight / 3);
    m_painter_p->drawLine(startPoint, endPoint);
}

/***********************************************************************************************************************
*   StepTime
* Step based on the period
***********************************************************************************************************************/
bool CSubPlotSurface::StepTime(QDateTime& dtime, const TimePeriod period, const bool forward)
{
    switch (period)
    {
        case TimePeriod::Milli:
            dtime = dtime.addMSecs(forward ? 1 : -1);
            break;

        case TimePeriod::Milli10:
            dtime = dtime.addMSecs(forward ? 10 : -10);
            break;

        case TimePeriod::Milli100:
            dtime = dtime.addMSecs(forward ? 100 : -100);
            break;

        case TimePeriod::HalfSec:
            dtime = dtime.addMSecs(forward ? 500 : -500);
            break;

        case TimePeriod::Sec:
            dtime = dtime.addSecs(forward ? 1 : -1);
            break;

        case TimePeriod::HalfMin:
            dtime = dtime.addSecs(forward ? 30 : -30);
            break;

        case TimePeriod::Min:
            dtime = dtime.addSecs(forward ? 60 : -60);
            break;

        case TimePeriod::TenMin:
            dtime = dtime.addSecs(forward ? 60 * 10 : -60 * 10);
            break;

        case TimePeriod::HalfHour:
            dtime = dtime.addSecs(forward ? 60 * 30 : -60 * 30);
            break;

        case TimePeriod::Hour:
            dtime = dtime.addSecs(forward ? 60 * 60 : -60 * 60);
            break;

        case TimePeriod::HalfDay:
            dtime = dtime.addSecs(forward ? 60 * 60 * 12 : -60 * 60 * 12);
            break;

        case TimePeriod::Day:
            dtime = dtime.addDays(forward ? 1 : -1);
            break;

        case TimePeriod::Week:
            dtime = dtime.addDays(forward ? 7 : -7);
            break;

        case TimePeriod::Month:
            dtime = dtime.addMonths(forward ? 1 : -1);
            break;

        case TimePeriod::Year:
            dtime = dtime.addYears(forward ? 1 : -1);
            break;

        default:
            assert(false);
            return false;
            break;
    }
    return true;
}

/***********************************************************************************************************************
*   GetMilliSecsPart
*
* It seems that using the microseconds time formatting doesn't really work with Qt. This is why this function has
* been introduced. (instead of using ".zz")
***********************************************************************************************************************/
QString CSubPlotSurface::GetMilliSecsString(const QDateTime& dtime, int numDecimals)
{
    /* 12345 msec / 1000 -> 12.345 sec */

    auto dtime_ms = dtime.toMSecsSinceEpoch();
    auto ms = dtime_ms - static_cast<qint64>((std::floor(dtime_ms / 1000.0) * 1000.0));
    if (numDecimals == 1) {
        ms = static_cast<int>(std::round(ms / 100.0));
    } else if (numDecimals == 2) {
        ms = static_cast<int>(std::round(ms / 10.0));
    }
    return QString("%1").arg(ms, numDecimals, 10, QLatin1Char('0'));
}

/***********************************************************************************************************************
*   GetMajorStrings
***********************************************************************************************************************/
QStringList CSubPlotSurface::GetMajorStrings(const QDateTime& dtime,
                                             const TimePeriod& period,
                                             bool isAnchor,
                                             int appendEmptyCount)
{
    QStringList slist;

    /*
     * dd.MM.yyyy   21.05.2001
     *
     *  ddd MMMM d yy   Tue May 21 01
     *  hh:mm:ss.zzz    14:13:09.120
     *  hh:mm:ss.z  14:13:09.12
     *  h:m:s ap    2:13:9 pm
     *
     * d    The day as a number without a leading zero (1 to 31)
     *  dd  The day as a number with a leading zero (01 to 31)
     *  ddd The abbreviated localized day name (e.g. 'Mon' to 'Sun'). Uses the system locale to localize the name, i.e.
     * QLocale::system().
     *  dddd    The long localized day name (e.g. 'Monday' to 'Sunday'). Uses the system locale to localize the name,
     * i.e. QLocale::system().
     *  M   The month as a number without a leading zero (1 to 12)
     *  MM  The month as a number with a leading zero (01 to 12)
     *  MMM The abbreviated localized month name (e.g. 'Jan' to 'Dec'). Uses the system locale to localize the name,
     * i.e. QLocale::system().
     *  MMMM    The long localized month name (e.g. 'January' to 'December'). Uses the system locale to localize the
     * name, i.e. QLocale::system().
     *  yy  The year as a two digit number (00 to 99)
     *  yyyy    The year as a four digit number, possibly plus a leading minus sign for negative years.
     */

    if (isAnchor) {
        slist.append(dtime.toString("yyyy"));
        slist.append(dtime.toString("MMM d"));
    }

    switch (period)
    {
        case TimePeriod::Milli:
        case TimePeriod::Milli10:
            slist.append(dtime.toString("hh:mm:ss") + QString(".%1").arg(GetMilliSecsString(dtime, 3)));
            break;

        case TimePeriod::Milli100:
        case TimePeriod::HalfSec:
            slist.append(dtime.toString("hh:mm:ss") + QString(".%1").arg(GetMilliSecsString(dtime, 1)));
            break;

        case TimePeriod::Sec:
        case TimePeriod::HalfMin:
            slist.append(dtime.toString("hh:mm:ss"));
            break;

        case TimePeriod::Min:
        case TimePeriod::TenMin:
        case TimePeriod::HalfHour:
        case TimePeriod::Hour:
        case TimePeriod::HalfDay:
            slist.append(dtime.toString("hh:mm"));
            break;

        case TimePeriod::Day:
        case TimePeriod::Week:
            if (!isAnchor) {
                slist.append(dtime.toString("MMM"));
                slist.append(dtime.toString("d"));
            }
            break;

        case TimePeriod::Month:
            if (!isAnchor) {
                slist.append(dtime.toString("MMMM"));
                slist.append(dtime.toString("d"));
            }
            break;

        case TimePeriod::Year:
            if (!isAnchor) {
                slist.append(dtime.toString("yyyy"));
                slist.append(dtime.toString("MMM"));
                slist.append(dtime.toString("d"));
            }
            break;

        default:
            slist.append(QString("BAD period %1").arg(static_cast<int>(period)));
            break;
    }

    for (auto i = 0; i < appendEmptyCount; i++) {
        slist.append(QString("")); /* This creates some extra space under the labels */
    }

    return slist;
}

/***********************************************************************************************************************
*   GetMinorStrings
***********************************************************************************************************************/
QStringList CSubPlotSurface::GetMinorStrings(const QDateTime& dtime, const TimePeriod& period)
{
    QStringList slist;

    switch (period)
    {
        case TimePeriod::Milli:
        case TimePeriod::Milli10:
            slist.append(dtime.toString("hh:mm:ss") + QString(".%1").arg(GetMilliSecsString(dtime, 3)));
            break;

        case TimePeriod::Milli100:
        case TimePeriod::HalfSec:
            slist.append(dtime.toString("hh:mm:ss") + QString(".%1").arg(GetMilliSecsString(dtime, 1)));
            break;

        case TimePeriod::Sec:
        case TimePeriod::HalfMin:
            slist.append(dtime.toString("hh:mm:ss"));
            break;

        case TimePeriod::Min:
        case TimePeriod::TenMin:
        case TimePeriod::HalfHour:
        case TimePeriod::Hour:
        case TimePeriod::HalfDay:
            slist.append(dtime.toString("hh:mm"));
            break;

        case TimePeriod::Day:
        case TimePeriod::Week:
            slist.append(dtime.toString("MMM"));
            slist.append(dtime.toString("d"));
            break;

        case TimePeriod::Month:
            slist.append(dtime.toString("MMM"));
            break;

        case TimePeriod::Year:
            slist.append(dtime.toString("yyyy"));
            break;

        default:
            slist.append(QString("BAD period %1").arg(static_cast<int>(period)));
            break;
    }

    return slist;
}
