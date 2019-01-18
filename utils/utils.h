
#ifndef UTILS_H
#define UTILS_H

#include <QWidget>
#include <QPoint>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPainter>
#include <QImage>

#include "CConfig.h"

extern unsigned char g_upperChar_LUT[256];
extern bool g_upperChar_LUT_init;

typedef struct {
    QPoint  mouse;              // as captured (in the current window, normalized to the top 0,0 of the window)
    QPoint  DCBMP;              // Position in the DC Bitmap, e.h mouse + current bitmap vert/horiz offsets
    QPoint  ClientToScreen;     // mouse converted into screen coordinates, normalized to the top 0,0 of the entire screen
}ScreenPoint_t;

class LS_Painter : public QPainter {
public:
    LS_Painter(QPaintDevice* paintDevice) : QPainter(paintDevice), m_currentFontItem_p(nullptr) {}
    LS_Painter() {}
    void setCurrentFontItem(FontItem_t* fontItem_p) { m_currentFontItem_p = fontItem_p; }
    bool isCurrentFontItem(FontItem_t* fontItem_p) { return (fontItem_p == m_currentFontItem_p ? true : false); }
    void clearCurrentFontItem() { m_currentFontItem_p = nullptr; }
    FontItem_t* getCurrentFontItem(void) { return m_currentFontItem_p; }

private:
    FontItem_t* m_currentFontItem_p;
};


void initUpperChar(void);

// Use dx/dy to compensate for e.g. m_hbmpOffset/m_vbmpOffset
extern ScreenPoint_t   MakeScreenPoint(QMouseEvent* event, int dx = 0, int dy = 0);
extern ScreenPoint_t   MakeScreenPoint(QWheelEvent* event, int dx = 0, int dy = 0);
extern ScreenPoint_t   MakeScreenPoint(QWidget* widget_p, const QPoint& cursorPos, int dx = 0, int dy = 0);
extern ScreenPoint_t   MakeScreenPoint_FromGlobal(QWidget* widget_p, const QPoint& cursorPos, int dx = 0, int dy = 0);
extern ScreenPoint_t   MakeScreenPoint(QContextMenuEvent* event, int dx = 0, int dy = 0);

QImage MakeTransparantImage(QString resource, QRgb transparencyColor);

#define DELETE_AND_CLEAR(A) { if (A != nullptr) { delete(A); A = nullptr;}}

#define SAFE_STR_MEMCPY(DEST, DEST_SIZE, SRC, SRC_SIZE) { \
    const size_t copy_size =  DEST_SIZE - 1 < SRC_SIZE ? DEST_SIZE - 1 : SRC_SIZE; \
    memcpy(DEST, SRC, copy_size); \
    DEST[copy_size] = 0; }
#endif
