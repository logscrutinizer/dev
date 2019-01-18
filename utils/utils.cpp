/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "utils.h"
#include <QtGlobal>
#include <cctype>

bool g_upperChar_LUT_init = false;
unsigned char g_upperChar_LUT[256];

/***********************************************************************************************************************
*   initUpperChar
***********************************************************************************************************************/
void initUpperChar(void)
{
    for (int i = 0; i < 256; i++) {
        g_upperChar_LUT[i] = static_cast<unsigned char>(toupper(i));
    }
    g_upperChar_LUT_init = true;
}

/***********************************************************************************************************************
*   MakeScreenPoint
***********************************************************************************************************************/
ScreenPoint_t MakeScreenPoint(QMouseEvent *event, int dx, int dy)
{
    ScreenPoint_t screenPoint;
    screenPoint.mouse = QPoint(event->x(), event->y());
    screenPoint.ClientToScreen = QPoint(event->globalX(), event->globalY());

    screenPoint.DCBMP.setX(screenPoint.mouse.x() + dx);
    screenPoint.DCBMP.setY(screenPoint.mouse.y() + dy);

    return screenPoint;
}

/***********************************************************************************************************************
*   MakeScreenPoint
***********************************************************************************************************************/
ScreenPoint_t MakeScreenPoint(QWheelEvent *event, int dx, int dy)
{
    ScreenPoint_t screenPoint;
    screenPoint.mouse = QPoint(event->x(), event->y());
    screenPoint.ClientToScreen = QPoint(event->globalX(), event->globalY());

    screenPoint.DCBMP.setX(screenPoint.mouse.x() + dx);
    screenPoint.DCBMP.setY(screenPoint.mouse.y() + dy);

    return screenPoint;
}

/***********************************************************************************************************************
*   MakeScreenPoint
***********************************************************************************************************************/
ScreenPoint_t MakeScreenPoint(QWidget *widget_p, const QPoint& cursorPos, int dx, int dy)
{
    ScreenPoint_t screenPoint;
    screenPoint.mouse = cursorPos;
    screenPoint.ClientToScreen = widget_p->mapToGlobal(cursorPos);

    screenPoint.DCBMP.setX(screenPoint.mouse.x() + dx);
    screenPoint.DCBMP.setY(screenPoint.mouse.y() + dy);

    return screenPoint;
}

/***********************************************************************************************************************
*   MakeScreenPoint_FromGlobal
***********************************************************************************************************************/
ScreenPoint_t MakeScreenPoint_FromGlobal(QWidget *widget_p, const QPoint& cursorPos, int dx, int dy)
{
    ScreenPoint_t screenPoint;
    screenPoint.mouse = widget_p->mapFromGlobal(cursorPos);
    screenPoint.ClientToScreen = cursorPos;

    screenPoint.DCBMP.setX(screenPoint.mouse.x() + dx);
    screenPoint.DCBMP.setY(screenPoint.mouse.y() + dy);

    return screenPoint;
}

/***********************************************************************************************************************
*   MakeScreenPoint
***********************************************************************************************************************/
ScreenPoint_t MakeScreenPoint(QContextMenuEvent *event, int dx, int dy)
{
    ScreenPoint_t screenPoint;
    screenPoint.mouse = QPoint(event->x(), event->x());
    screenPoint.ClientToScreen = QPoint(event->globalX(), event->globalY());

    screenPoint.DCBMP.setX(screenPoint.mouse.x() + dx);
    screenPoint.DCBMP.setY(screenPoint.mouse.y() + dy);

    return screenPoint;
}

/***********************************************************************************************************************
*   MakeTransparantImage
***********************************************************************************************************************/
QImage MakeTransparantImage(QString resource, QRgb transparencyColor)
{
    QImage temp = QImage(resource);
    if (temp.isNull()) {
        g_DebugLib->TRACEX(
#ifdef _DEBUG
            LOG_LEVEL_ERROR,
#else
            LOG_LEVEL_WARNING,
#endif
            QString("MakeTransparantImage  Bitmap %1 load FAILED").arg(resource));
    }

    if (temp.hasAlphaChannel()) {
        return temp;
    }

    TRACEX_D(QString("MakeTransparantImage  %1 not alpha channel").arg(resource));

    QImage temp2 = temp.convertToFormat(QImage::Format_ARGB32);

    /* Loop over all pixels and set alpha to 0 if there is a transparency color match (making those pixels transparent)
     * */
    const int width = temp2.width();
    const int height = temp2.height();

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            QRgb current_color = temp2.pixel(x, y) & RGB_MASK;
            if (current_color == transparencyColor) {
                temp2.setPixel(x, y, qRgba(0x7c, 0xfc, 0, 0));
            }
        }
    }
    return temp2;
}
