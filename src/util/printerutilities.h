#ifndef PRINTERUTILITIES_H
#define PRINTERUTILITIES_H

#include <QPainter>
#include <QPdfWriter>
#include <QTextTable>

void printer_util_painter_scale(QPainter* p_painter, QPdfWriter* p_printer);
void printer_util_insert_svg_string(QPainter* p_painter, QRectF area, const QString& svgString, const bool show_area = false);
void printer_util_insert_draft(QPainter* p_painter);

void printer_util_table_cell_align_text(QTextTable* p_table, int row, int column, Qt::Alignment alignment);

// `ore` and `hundredths` are the scaled integers the data layer stores; see
// util/amount.h.
QString printer_util_format_money(long long ore);
QString printer_util_format_hours(int hundredths);
QString printer_util_format_percent(int percent);

#endif // PRINTERUTILITIES_H
