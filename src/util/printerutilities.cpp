#include <QPainter>
#include <QPdfWriter>
#include <QRectF>
#include <QSvgRenderer>
#include <QTextTable>
#include <QDebug>

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
void printer_util_insert_draft(QPainter* p_painter)
{
    // Now draw the watermark on top

    // Save the painter state
    p_painter->save();

    // Revoke all settings
    p_painter->resetTransform();

    // Set up the font and opacity for the watermark
    QFont font("Arial", 180);
    font.setBold(true);
    p_painter->setFont(font);
    p_painter->setPen(QColor(192, 192, 192, 64)); // Light gray, semi-transparent

    // Move to the center of the page and rotate
    p_painter->translate(280, 2500);
    p_painter->rotate(-45);

    // Draw the watermark text
    p_painter->drawText(0, 0, "DRAFT");

    // Restore the painter state
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
QString printer_util_format_money(double amount)
{
    // Use QString::arg to format the number
    // First, split the amount into whole kronor and ören
    int wholePart = static_cast<int>(std::abs(amount));
    int fractionalPart = static_cast<int>(std::round(std::abs(amount - wholePart) * 100));

    // Format whole part with dot as thousand separator
    QString formattedWholePart;
    if (wholePart >= 1000) {
        // Use recursive approach to add dots for thousands
        if (wholePart >= 1000000) {
            formattedWholePart = QString("%1.%2").arg(wholePart / 1000000)
                                     .arg((wholePart % 1000000) / 1000, 3, 10, QChar('0'));
        } else {
            formattedWholePart = QString::number(wholePart / 1000) + "." +
                                 QString("%1").arg(wholePart % 1000, 3, 10, QChar('0'));
        }
    } else {
        formattedWholePart = QString::number(wholePart);
    }

    // Combine whole part and fractional part
    QString formattedAmount = QString("%1,%2")
                                  .arg(formattedWholePart)
                                  .arg(fractionalPart, 2, 10, QChar('0'));

    // Handle negative numbers by adding a minus sign
    if (amount < 0) {
        formattedAmount.prepend("-");
    }

    return formattedAmount;
}

//-----------------------------------------------------------------------------
QString printer_util_format_hours(double amount)
{
    int wholePart = static_cast<int>(std::abs(amount));
    int fractionalPart = static_cast<int>(std::round(std::abs(amount - wholePart) * 100));

    // Combine whole part and fractional part
    QString formattedAmount = QString("%1,%2 tim")
                                  .arg(wholePart)
                                  .arg(fractionalPart, 1, 10, QChar('0'));

    // Handle negative numbers by adding a minus sign
    if (amount < 0) {
        formattedAmount.prepend("-");
    }

    return formattedAmount;
}

//-----------------------------------------------------------------------------
QString printer_util_format_percent(int percent)
{
    return QString("%1 %").arg(percent);
}

