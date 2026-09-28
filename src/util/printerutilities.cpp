#include <QFontMetricsF>
#include <QPainter>
#include <QPdfWriter>
#include <QRectF>
#include <QSvgRenderer>
#include <QTextTable>
#include <QDebug>
#include <QtMath>

#include <cmath>

#include "amount.h"

//-----------------------------------------------------------------------------
void printer_util_painter_scale(QPainter* p_painter, QPdfWriter* p_printer)
{
    // This block transforms the size of the content, so it makes sense when applying e.g. a font size
    int logicalDotsPerInchX = p_printer->logicalDpiX();
    const int pointsPerInch = 72;
    QTransform t;
    float scaling=(float)logicalDotsPerInchX / pointsPerInch;
    t.scale(scaling, scaling);
    p_painter->setTransform(t);
}

//-----------------------------------------------------------------------------
void printer_util_insert_svg_string(QPainter* p_painter, QRectF area, const QString& svgString, const bool show_area = false)
{
    if (show_area)
    {
        QString area_boundary = QString::fromUtf8(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\">"
            "    <rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"transparent\" stroke=\"black\" stroke-width=\"2\"/>"
            "</svg>"
            );
        QSvgRenderer renderer(area_boundary.toUtf8());
        renderer.render(p_painter, area);
    }
    else
    {
        // A company without a logo is not a mistake, so nothing is said about
        // it; anything else that will not render is.
        if (svgString.isEmpty())
        {
            return;
        }

        QSvgRenderer renderer(svgString.toUtf8());
        if (!renderer.isValid()) {
            qWarning() << "Invalid SVG string";
            return;
        }
        QSize image_size = renderer.defaultSize();
        image_size.scale(area.width(), area.height(), Qt::KeepAspectRatio);
        qreal y_offset = (area.height() - image_size.height()) / 2.0;
        renderer.render(p_painter, QRectF( QPointF(area.x(), area.y() + y_offset), image_size));
    }
}

//-----------------------------------------------------------------------------
// The draft watermark, laid along the page's own diagonal. Both the size of the
// word and the point it was drawn from used to be written into the code, and
// "UTKAST" at 180 pt then ran off the right-hand edge of an A4: the word is
// measured here and given whatever the page has room for, so it is centred on
// the page whatever the page and the font turn out to be.
void printer_util_insert_draft(QPainter* p_painter)
{
    const QString text("UTKAST");

    // How much of the page the word may claim. The rest is what keeps it clear
    // of the edges.
    const qreal fill = 0.9;

    p_painter->save();

    // Device pixels: the watermark belongs to the page and not to the text
    // area, so it takes neither the margins nor the scaling the content is laid
    // out in.
    p_painter->resetTransform();

    const QPaintDevice* p_device = p_painter->device();
    const qreal page_width = p_device->width();
    const qreal page_height = p_device->height();

    // The page's own diagonal rather than a fixed 45 degrees, which on a
    // portrait page does not run corner to corner: this is the longest line the
    // page holds, and it is the line the word is given.
    const qreal angle = std::atan2(page_height, page_width);
    const qreal cos_angle = std::cos(angle);
    const qreal sin_angle = std::sin(angle);

    QFont font("Arial", 180);
    font.setBold(true);

    // A text of w x h turned by `angle` and centred on the page takes
    // w*cos + h*sin of the width and w*sin + h*cos of the height, so what fits
    // is worked out from the word as it is really laid out rather than guessed
    // at. Point sizes are rounded and the metrics are not quite linear in them,
    // so the size is taken down a point at a time until the fit holds instead
    // of being trusted to the one division.
    auto measure = [&](const QFont& f) { return QFontMetricsF(f, p_device).tightBoundingRect(text); };
    auto too_big = [&](const QRectF& r) {
        return r.width() * cos_angle + r.height() * sin_angle > fill * page_width
            || r.width() * sin_angle + r.height() * cos_angle > fill * page_height;
    };

    QRectF bounds = measure(font);
    if (bounds.width() > 0.0 && bounds.height() > 0.0)
    {
        const qreal scale = qMin(fill * page_width  / (bounds.width() * cos_angle + bounds.height() * sin_angle),
                                 fill * page_height / (bounds.width() * sin_angle + bounds.height() * cos_angle));
        font.setPointSizeF(qMax(1.0, font.pointSizeF() * scale));
        bounds = measure(font);
        while (font.pointSizeF() > 1.0 && too_big(bounds))
        {
            font.setPointSizeF(font.pointSizeF() - 1.0);
            bounds = measure(font);
        }
    }

    p_painter->setFont(font);
    p_painter->setPen(QColor(192, 192, 192, 64)); // Light gray, semi-transparent

    p_painter->translate(page_width / 2.0, page_height / 2.0);
    p_painter->rotate(-qRadiansToDegrees(angle));

    // drawText() starts the word at the baseline, and the measured box is
    // relative to that point, so this is what puts the ink of the word, rather
    // than the start of its baseline, in the middle of the page.
    p_painter->drawText(QPointF(-bounds.width() / 2.0 - bounds.x(), -bounds.height() / 2.0 - bounds.y()), text);

    p_painter->restore();
}

//-----------------------------------------------------------------------------
void printer_util_table_cell_align_text(QTextTable* p_table, int row, int column, Qt::Alignment alignment)
{
    QTextTableCell cell = p_table->cellAt(row, column);
    QTextCursor cellCursor = cell.firstCursorPosition();
    cellCursor.setPosition(cell.lastPosition(), QTextCursor::KeepAnchor);

    QTextBlockFormat format;
    format.setAlignment(alignment);
    cellCursor.mergeBlockFormat(format);
}

//-----------------------------------------------------------------------------
QString printer_util_format_money(long long ore)
{
    return amount_format_ore(ore);
}

//-----------------------------------------------------------------------------
QString printer_util_format_hours(int hundredths)
{
    return hours_format_hundredths(hundredths) + " tim";
}

//-----------------------------------------------------------------------------
QString printer_util_format_percent(int percent)
{
    return QString("%1 %").arg(percent);
}

