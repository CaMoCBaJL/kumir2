#ifndef EDITORPLANE_H
#define EDITORPLANE_H

#include <QtGui>

#include "interfaces/lexemtype.h"

namespace Editor {

class EditorPlane : public QWidget
{
    Q_OBJECT
public:
    explicit EditorPlane(class TextDocument * doc
                         , class TextCursor * cursor
                         , class Clipboard * clipboard
                         , const QList<QRegExp> &fileNamePattern
                         , QSettings * settings
                         , QScrollBar * horizontalSB
                         , QScrollBar * verticalSB
                         , bool hasAnalizer
                         , QWidget *parent = 0);
    int widthInChars() const;
    int charWidth() const;
    int lineHeight() const;
    inline int marginCharactersCount() const { return m_settings->value(MarginWidthKey, MarginWidthDefault).toInt(); }
    QRect cursorRect() const;
    static QString MarginWidthKey;
    static int MarginWidthDefault;
public slots:
    void selectAll();
    void copy();
    void paste();
    void cut();
    void removeLine();
    void removeLineTail();
    void setLineHighlighted(int lineNo, const QColor & color);
protected:
    static QPolygon errorUnderline(int x, int y, int len);
    void dragEventHandler(QDragMoveEvent * e);
    void wheelEvent(QWheelEvent *);
    void dragEnterEvent(QDragEnterEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *);
    void dropEvent(QDropEvent *e);
    void resizeEvent(QResizeEvent *e);
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);
    bool canDrop(const QPoint &pos, const QMimeData * data) const;
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void initMouseCursor();
    void updateScrollBars();
    void findCursor();
    void ensureCursorVisible();
    void ensureHighlightedLineVisible();
//    void updateOffset();
    void paintEvent(QPaintEvent *);
    QPoint offset() const;
    void paintBackground(QPainter *p, const QRect &rect);
    void paintMarginBackground(QPainter *p, const QRect & rect);
    void paintMarginText(QPainter *p, const QRect & rect);
    void paintCursor(QPainter *p, const QRect &rect);
    void paintText(QPainter *p, const QRect &rect);
    void paintSelection(QPainter *p, const QRect &rect);
    void paintRectSelection(QPainter *p, const QRect &rect);
    void paintLineNumbers(QPainter *p, const QRect &rect);
    void paintNewMarginLine(QPainter *p);
    void paintDropPosition(QPainter *p);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);
    void setProperFormat(QPainter * p, Shared::LexemType type, const QChar &c);
protected slots:
    void updateCursor();
    void updateText(int fromLine, int toLine);
private:
    int i_timerId;
    class TextDocument * m_document;
    class TextCursor * m_cursor;
    class Clipboard * m_clipboard;
    QList<QRegExp> rxFilenamePattern;
    QSettings * m_settings;
    QScrollBar * m_verticalScrollBar;
    QScrollBar * m_horizontalScrollBar;
    QPoint pnt_marginPress;
    QPoint pnt_textPress;
    QPoint pos_textPress;
    bool b_selectionInProgress;

    QPoint pnt_dropPosMarker;
    QPoint pnt_dropPosCorner;
    int i_marginAlpha;
    bool b_hasAnalizer;

    int i_highlightedLine;
    QColor color_highlightedLine;
signals:
    void urlsDragAndDropped(const QList<QUrl> &);

};

} // namespace Editor

#endif // EDITORPLANE_H
