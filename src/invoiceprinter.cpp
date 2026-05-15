#include <QApplication>
#include <QtCore>
#include <QPdfWriter>
#include <QPainter>
#include <QTextDocument>
#include <QTextCursor>
#include <QTransform>
#include <QTextTable>
#include <QTextTableCell>
#include <QTextDocumentFragment>

#include "util/printerutilities.h"
#include "version.h"

#include "invoiceprinter.h"

// Define the static member outside the class
InvoicePrinter* InvoicePrinter::m_instance = nullptr;

//-----------------------------------------------------------------------------
InvoicePrinter::InvoicePrinter()
{
    m_border_style = QTextFrameFormat::BorderStyle_Solid;
    m_border_brush = QBrush(QColor(245,245,245));
    m_border_size = 0.5;

    m_border_colapse = false;
    m_cell_spacing = 0;
    m_cell_padding = 3;

    m_font_header = QFont("Arial", 18, QFont::Normal, false);
    m_font_super = QFont("Arial", 5, QFont::Normal, false);
    m_font_normal = QFont("Arial", 8, QFont::Normal, false);
    m_font_normal_bold = QFont("Arial", 8, QFont::ExtraBold, false);

    m_table_format_default.setBorderCollapse(m_border_colapse);
    m_table_format_default.setCellSpacing(m_cell_spacing);
    m_table_format_default.setCellPadding(m_cell_padding);
    m_table_format_default.setBorderStyle(m_border_style);
    m_table_format_default.setBorder(m_border_size);
    m_table_format_default.setBorderBrush(m_border_brush);
}

//-----------------------------------------------------------------------------
void InvoicePrinter::create_pdf(Invoice* p_in, QString logo, bool b_preview, QString filename)
{
    // Set up the QPdfWriter with the output file
    QPdfWriter printer(filename);
    printer.setPageSize(QPageSize::A4Small);  // Set page size to A4
    printer.setPageMargins(QMarginsF(25, 25, 25, 15));
    printer.setResolution(300);  // Set resolution to 300 DPI

    qDebug() << "Actual size of an A4: 210 × 297 mm";
    qDebug() << "printer size in mm after margins" << printer.widthMM() << printer.heightMM();
    qDebug() << "printer size in points after setResolution" << printer.width() << printer.height();
    qDebug() << "printer margins" << printer.pageLayout().margins();

    // Create a QPainter to draw on the PDF
    QPainter painter(&printer);
    printer_util_painter_scale(&painter, &printer);

    if (b_preview)
    {
        // Draw the draft watermark first so it ends up in the background.
        printer_util_insert_draft(&painter);
    }

    // Create and paint the text document on the PDF using the painter
    QTextDocument* p_doc = createDocument(p_in, printer);

    {
        // This block specifies the area in which an eventual logo can be placed.

        // Area for logo
        qreal aleft = 5.0;
        qreal atop = 3.0;
        qreal awidth = 130.0;
        qreal aheight = 35.0;

        printer_util_insert_svg_string(&painter, QRectF(aleft, atop, awidth, aheight), logo, false);
    }

    p_doc->drawContents(&painter);


    // End the painter session
    painter.end();
}

//-----------------------------------------------------------------------------
QTextDocument* InvoicePrinter::createDocument(Invoice* p_in, QPdfWriter &printer)
{
    int available_width = printer.pageLayout().paintRectPoints().width();
    int available_height = printer.pageLayout().paintRectPoints().height();

    qDebug() << "available size" << available_width << available_height;

    // Create QTextDocument fragments to be put together into the main doc
    QTextDocument *p_head_doc = new QTextDocument();
    QTextDocument *p_fill_doc = new QTextDocument();
    QTextDocument *p_foot_doc = new QTextDocument();
    QTextDocument *p_main_doc = new QTextDocument();

    // update the default table width relative the available width
    m_table_format_default.setWidth(QTextLength(QTextLength::FixedLength, available_width));

    {
        QTextCursor cursor(p_head_doc);
        addHeader(p_in, cursor);
        addSenderReceiver(p_in, cursor);
        addCustomerInfo(p_in, cursor);
        addItems(p_in, cursor);
        addComments(p_in, cursor);
    }

    {
        QTextCursor cursor(p_foot_doc);
        addSummary(p_in, cursor);
        addCompanyInfo(p_in, cursor);
    }

    // This block will fill up the A4 so the footer is placed at the bottom
    // The extra 25 seem to be needed for some reason.
    {
        QTextFrameFormat tff;
        tff.setHeight(available_height + 25 - p_head_doc->size().height() - p_foot_doc->size().height());
        QTextCursor cursor(p_fill_doc);
        cursor.insertFrame(tff);
    }

    {
        QTextCursor cursor(p_main_doc);
        cursor.insertFragment(QTextDocumentFragment(p_head_doc));
        cursor.insertFragment(QTextDocumentFragment(p_fill_doc));
        cursor.insertFragment(QTextDocumentFragment(p_foot_doc));
    }

    qDebug() << "used size" << p_main_doc->size().width() << p_main_doc->size().height();

    return p_main_doc;
}

//-----------------------------------------------------------------------------
void InvoicePrinter::addHeader(Invoice* p_in, QTextCursor& cursor)
{
    QTextTableFormat table_format = m_table_format_default;

    table_format.setBorder(0);

    QList<QTextLength> columnWidthConstraintList;
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 33.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 34.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 33.0));
    table_format.setColumnWidthConstraints(columnWidthConstraintList);

    cursor.movePosition(QTextCursor::End);
    QTextTable *p_table = cursor.insertTable(3, 3, table_format);

    p_table->mergeCells(0, 0, 2, 1); // merge the 'LOGO' cells
    {
        // The logo is placed where this cell resides later in the factory
    }

    p_table->mergeCells(0, 1, 2, 1); // merge the 'FAKTURA' cells
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 1);

        QTextCharFormat format;
        format.setFont(m_font_header);
        format.setVerticalAlignment(QTextCharFormat::AlignTop);  // Vertical centering
        cell.setFormat(format);

        QTextCursor cellCursor = cell.firstCursorPosition();

        // Create block format for horizontal and vertical alignment
        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(Qt::AlignCenter);  // Horizontal centering
        cellCursor.mergeBlockFormat(blockFormat);

        cellCursor.insertText("FAKTURA");
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 2);

        QTextCharFormat format;
        format.setFont(m_font_normal);
        // format.setVerticalAlignment(QTextCharFormat::AlignMiddle);  // Vertical centering
        cell.setFormat(format);

        QTextCursor cellCursor = cell.firstCursorPosition();

        // Create block format for horizontal and vertical alignment
        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(Qt::AlignRight);  // Horizontal centering
        cellCursor.mergeBlockFormat(blockFormat);

        cellCursor.insertText(QString("Fakturanr: %1").arg(p_in->m_number), format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 2);

        QTextCharFormat format;
        format.setFont(m_font_normal);
        // format.setVerticalAlignment(QTextCharFormat::AlignMiddle);  // Vertical centering
        cell.setFormat(format);

        QTextCursor cellCursor = cell.firstCursorPosition();

        // Create block format for horizontal and vertical alignment
        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(Qt::AlignRight);  // Horizontal centering
        cellCursor.mergeBlockFormat(blockFormat);

        cellCursor.insertText(QString("Fakturadatum: %1").arg(p_in->m_date), format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(2, 2);

        QTextCharFormat format;
        format.setFont(m_font_normal);
        // format.setVerticalAlignment(QTextCharFormat::AlignMiddle);  // Vertical centering
        cell.setFormat(format);

        QTextCursor cellCursor = cell.firstCursorPosition();

        // Create block format for horizontal and vertical alignment
        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(Qt::AlignRight);  // Horizontal centering
        cellCursor.mergeBlockFormat(blockFormat);

        cellCursor.insertText(QString("Förfallodatum: %1").arg(p_in->m_due_date), format);
    }



    // Add some space to next table
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    format.setFont(m_font_normal);
    cursor.insertText(" \n \n", format);
}

//-----------------------------------------------------------------------------
void InvoicePrinter::addSenderReceiver(Invoice* p_in, QTextCursor& cursor)
{
    QTextTableFormat table_format = m_table_format_default;

    QList<QTextLength> columnWidthConstraintList;
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 50.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 50.0));
    table_format.setColumnWidthConstraints(columnWidthConstraintList);

    cursor.movePosition(QTextCursor::End);
    QTextTable *p_table = cursor.insertTable(1, 2, table_format);

    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 0);
        QTextCursor c = cell.firstCursorPosition();
        QTextCharFormat format;
        format.setFont(m_font_super);
        c.insertText("Avsändare\n", format);
        format.setFont(m_font_normal);
        c.insertText(p_in->m_company_name + "\n", format);
        c.insertText(p_in->m_company_adrsline1 + "\n", format);
        c.insertText(p_in->m_company_adrsline2, format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 1);
        QTextCursor c = cell.firstCursorPosition();
        QTextCharFormat format;
        format.setFont(m_font_super);
        c.insertText("Mottagare\n", format);
        format.setFont(m_font_normal);
        c.insertText(p_in->m_customer_name + "\n", format);
        c.insertText(p_in->m_customer_adrsline1 + "\n", format);
        c.insertText(p_in->m_customer_adrsline2, format);
    }

    // Add some space to next table
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    format.setFont(m_font_super);
    cursor.insertText(" \n", format);
}

//-----------------------------------------------------------------------------
void InvoicePrinter::addCustomerInfo(Invoice* p_in, QTextCursor& cursor)
{
    QTextTableFormat table_format = m_table_format_default;

    QList<QTextLength> columnWidthConstraintList;
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 50.4));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 25.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 25.0));
    table_format.setColumnWidthConstraints(columnWidthConstraintList);

    cursor.movePosition(QTextCursor::End);
    QTextTable *p_table = cursor.insertTable(2, 3, table_format);

    // first row
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 0);
        QTextCursor c = cell.firstCursorPosition();
        QTextCharFormat format;
        format.setFont(m_font_super);
        c.insertText("Vår referens\n", format);
        format.setFont(m_font_normal);
        c.insertText(p_in->m_company_reference_name, format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 1);
        QTextCursor c = cell.firstCursorPosition();
        QTextCharFormat format;
        format.setFont(m_font_super);
        c.insertText("Kundnr\n", format);
        format.setFont(m_font_normal);
        c.insertText(QString("%1").arg(p_in->m_customer_id), format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 2);
        QTextCursor c = cell.firstCursorPosition();
        QTextCharFormat format;
        format.setFont(m_font_super);
        c.insertText("Betalningsvillkor\n", format);
        format.setFont(m_font_normal);
        c.insertText(QString("%1 dagar").arg(p_in->m_agreement_payment_terms_days), format);
    }
    // second row
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 0);
        QTextCursor c = cell.firstCursorPosition();
        QTextCharFormat format;
        format.setFont(m_font_super);
        c.insertText("Er referens\n", format);
        format.setFont(m_font_normal);
        c.insertText(p_in->m_customer_reference_name, format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 1);
        QTextCursor c = cell.firstCursorPosition();
        QTextCharFormat format;
        format.setFont(m_font_super);
        c.insertText("Avtalsid/Avtalsdatum\n", format);
        format.setFont(m_font_normal);
        c.insertText(p_in->m_agreement_id, format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 2);
        QTextCursor c = cell.firstCursorPosition();
        QTextCharFormat format;
        format.setFont(m_font_super);
        c.insertText("Dröjsmålsränta\n", format);
        format.setFont(m_font_normal);
        c.insertText(printer_util_format_percent(p_in->m_agreement_late_payment_interest), format);
    }

    // Add some space to next table
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    format.setFont(m_font_super);
    cursor.insertText(" \n", format);
}

//-----------------------------------------------------------------------------
void InvoicePrinter::addItems(Invoice* p_in, QTextCursor& cursor)
{
    QTextTableFormat table_format = m_table_format_default;

    QList<QTextLength> columnWidthConstraintList;
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 49.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 13.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 13.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 10.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 15.0));
    table_format.setColumnWidthConstraints(columnWidthConstraintList);

    cursor.movePosition(QTextCursor::End);
    QTextTable *p_table = cursor.insertTable(2, 5, table_format);

    // header row
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 0);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText(p_in->m_description_consultant_period, format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 1);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("Antal", format);
        printer_util_table_cell_align_text(p_table, 0, 1, Qt::AlignCenter);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 2);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("Á-pris (SEK)", format);
        printer_util_table_cell_align_text(p_table, 0, 2, Qt::AlignCenter);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 3);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("Moms", format);
        printer_util_table_cell_align_text(p_table, 0, 3, Qt::AlignCenter);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 4);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("Belopp (SEK)", format);
    }

    // First item row
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 0);
        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.firstCursorPosition().insertText(p_in->m_description_standard, format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 1);
        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.firstCursorPosition().insertText(printer_util_format_hours(p_in->m_hours_standard), format);
        printer_util_table_cell_align_text(p_table, 1, 1, Qt::AlignCenter);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 2);
        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.firstCursorPosition().insertText(printer_util_format_money((double)p_in->m_agreement_standard_hourly_rate), format);
        printer_util_table_cell_align_text(p_table, 1, 2, Qt::AlignCenter);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 3);
        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.firstCursorPosition().insertText(printer_util_format_percent(p_in->m_agreement_standard_vat), format);
        printer_util_table_cell_align_text(p_table, 1, 3, Qt::AlignCenter);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 4);
        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.firstCursorPosition().insertText(printer_util_format_money((double)(p_in->m_sum_standard)), format);
    }

    if (p_in->m_hours_overtime > 0)
    {
        p_table->appendRows(1);

        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 0);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(p_in->m_description_overtime, format);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 1);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_hours(p_in->m_hours_overtime), format);
            printer_util_table_cell_align_text(p_table, p_table->rows() - 1, 1, Qt::AlignCenter);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 2);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_money((double)p_in->m_agreement_overtime_hourly_rate), format);
            printer_util_table_cell_align_text(p_table, p_table->rows() - 1, 2, Qt::AlignCenter);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 3);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_percent(p_in->m_agreement_overtime_vat), format);
            printer_util_table_cell_align_text(p_table, p_table->rows() - 1, 3, Qt::AlignCenter);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 4);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_money((double)(p_in->m_sum_overtime)), format);
        }
    }

    if (p_in->m_hours_qualified > 0)
    {
        p_table->appendRows(1);

        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 0);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(p_in->m_description_qualified, format);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 1);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_hours(p_in->m_hours_qualified), format);
            printer_util_table_cell_align_text(p_table, p_table->rows() - 1, 1, Qt::AlignCenter);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 2);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_money((double)p_in->m_agreement_qualified_hourly_rate), format);
            printer_util_table_cell_align_text(p_table, p_table->rows() - 1, 2, Qt::AlignCenter);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 3);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_percent(p_in->m_agreement_qualified_vat), format);
            printer_util_table_cell_align_text(p_table, p_table->rows() - 1, 3, Qt::AlignCenter);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 4);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_money((double)(p_in->m_sum_qualified)), format);
        }
    }

    if (p_in->m_hours_traveltime > 0)
    {
        p_table->appendRows(1);

        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 0);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(p_in->m_description_traveltime, format);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 1);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_hours(p_in->m_hours_traveltime), format);
            printer_util_table_cell_align_text(p_table, p_table->rows() - 1, 1, Qt::AlignCenter);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 2);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_money((double)p_in->m_agreement_traveltime_hourly_rate), format);
            printer_util_table_cell_align_text(p_table, p_table->rows() - 1, 2, Qt::AlignCenter);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 3);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_percent(p_in->m_agreement_traveltime_vat), format);
            printer_util_table_cell_align_text(p_table, p_table->rows() - 1, 3, Qt::AlignCenter);
        }
        {
            QTextTableCell cell = p_table->QTextTable::cellAt(p_table->rows() - 1, 4);
            QTextCharFormat format;
            format.setFont(m_font_normal);
            cell.firstCursorPosition().insertText(printer_util_format_money((double)(p_in->m_sum_traveltime)), format);
        }
    }

    // Add some space to next table
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    format.setFont(m_font_super);
    cursor.insertText(" \n", format);
}

//-----------------------------------------------------------------------------
void InvoicePrinter::addComments(Invoice* p_in, QTextCursor& cursor)
{
    QTextTableFormat table_format = m_table_format_default;

    cursor.movePosition(QTextCursor::End);
    QTextTable *p_table = cursor.insertTable(2, 1, table_format);

    // header row
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 0);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("Kommentarer", format);
    }

    // First item row
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 0);
        QTextCharFormat format;
        format.setFont(m_font_normal);
        QTextCursor c = cell.firstCursorPosition();
        c.insertText(p_in->m_comment, format);
    }

    // Add some space to next table
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    format.setFont(m_font_super);
    cursor.insertText(" \n", format);
}

//-----------------------------------------------------------------------------
void InvoicePrinter::addSummary(Invoice* p_in, QTextCursor& cursor)
{
    QTextTableFormat table_format = m_table_format_default;

    table_format.setBorder(0);

    QList<QTextLength> columnWidthConstraintList;
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 50));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 25));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 25));
    table_format.setColumnWidthConstraints(columnWidthConstraintList);

    cursor.movePosition(QTextCursor::End);
    QTextTable *p_table = cursor.insertTable(3, 3, table_format);

    p_table->mergeCells(0, 0, 3, 1);
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 0);
        QTextCharFormat format;
        format.setFont(m_font_header);
        cell.firstCursorPosition().insertText("", format);
    }

    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 1);
        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.firstCursorPosition().insertText("Belopp exkl. moms", format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 2);
        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.firstCursorPosition().insertText(printer_util_format_money((double)p_in->m_sum), format);
        printer_util_table_cell_align_text(p_table, 0, 2, Qt::AlignRight);
    }

    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 1);
        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.firstCursorPosition().insertText("Total moms", format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 2);
        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.firstCursorPosition().insertText(printer_util_format_money((double)p_in->m_vat), format);
        printer_util_table_cell_align_text(p_table, 1, 2, Qt::AlignRight);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(2, 1);
        QTextCharFormat format;
        format.setFont(m_font_normal_bold);
        cell.firstCursorPosition().insertText("Att betala (SEK)", format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(2, 2);
        QTextCharFormat format;
        format.setFont(m_font_normal_bold);
        cell.firstCursorPosition().insertText(printer_util_format_money((double)p_in->m_sum_including_vat), format);
        printer_util_table_cell_align_text(p_table, 2, 2, Qt::AlignRight);
    }

    // Add some extra space to the footer
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    format.setFont(m_font_super);
    cursor.insertText(" \n \n \n", format);
}

//-----------------------------------------------------------------------------
void InvoicePrinter::addCompanyInfo(Invoice* p_in, QTextCursor& cursor)
{
    QTextTableFormat table_format = m_table_format_default;
    table_format.setCellPadding(1);

    QList<QTextLength> columnWidthConstraintList;
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 33.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 33.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 34.0));
    table_format.setColumnWidthConstraints(columnWidthConstraintList);

    QTextTable *p_table = cursor.insertTable(3, 3, table_format);

    // first row
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 0);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("Organisationsnummer\n", format);
        format.setFont(m_font_normal);
        cell.lastCursorPosition().insertText(p_in->m_company_org_number, format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 1);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("Bankgiro\n", format);
        format.setFont(m_font_normal);
        cell.lastCursorPosition().insertText(p_in->m_company_bankgiro, format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 2);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("Telefon\n", format);
        format.setFont(m_font_normal);
        cell.lastCursorPosition().insertText(p_in->m_company_phone, format);
    }
    // second row
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 0);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("Momsregistreringsnummer\n", format);
        format.setFont(m_font_normal);
        cell.lastCursorPosition().insertText(p_in->m_company_vat_number, format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 1);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("SWIFT/BIC\n", format);
        format.setFont(m_font_normal);
        cell.lastCursorPosition().insertText(p_in->m_company_swift_bic, format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 2);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("E-mail\n", format);
        format.setFont(m_font_normal);
        cell.lastCursorPosition().insertText(p_in->m_company_email, format);
    }
    // third row
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(2, 0);
        QTextCharFormat format;
        // format.setVerticalAlignment(QTextCharFormat::AlignMiddle);
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText(" \n", format);
        format.setFont(m_font_normal);
        cell.lastCursorPosition().insertText(p_in->m_company_f_skatt, format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(2, 1);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("IBAN\n", format);
        format.setFont(m_font_normal);
        cell.lastCursorPosition().insertText(p_in->m_company_iban, format);
    }
    {
        // QTextTableCell cell = p_table->QTextTable::cellAt(2, 2);
        // QTextCharFormat format;
        // format.setFont(m_font_super);
        // cell.firstCursorPosition().insertText("\n", format);
        // format.setFont(m_font_normal);
        // cell.lastCursorPosition().insertText("", format);
    }

    // Add footer with repository URL and version
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat footerFormat;
    footerFormat.setFont(m_font_super);
    footerFormat.setForeground(QBrush(QColor(186, 186, 186)));  // Medium grey
    cursor.insertText("\n\n", footerFormat);

    QTextTableFormat footerTableFormat = m_table_format_default;
    footerTableFormat.setBorder(0);
    footerTableFormat.setCellPadding(0);
    footerTableFormat.setCellSpacing(0);

    QList<QTextLength> footerColumnWidthConstraintList;
    footerColumnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 50.0));
    footerColumnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 50.0));
    footerTableFormat.setColumnWidthConstraints(footerColumnWidthConstraintList);

    cursor.movePosition(QTextCursor::End);
    QTextTable *p_footer_table = cursor.insertTable(1, 2, footerTableFormat);

    // Left cell: Repository URL
    {
        QTextTableCell cell = p_footer_table->QTextTable::cellAt(0, 0);
        QTextCharFormat format;
        format.setFont(m_font_super);
        format.setForeground(QBrush(QColor(186, 186, 186)));  // Medium grey
        cell.firstCursorPosition().insertText(REPOSITORY_URL, format);
    }

    // Right cell: Application name, version and git hash
    {
        QTextTableCell cell = p_footer_table->QTextTable::cellAt(0, 1);
        QTextCharFormat format;
        format.setFont(m_font_super);
        format.setForeground(QBrush(QColor(186, 186, 186)));  // Medium grey
        QTextCursor cellCursor = cell.firstCursorPosition();
        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(Qt::AlignRight);
        cellCursor.mergeBlockFormat(blockFormat);
        cellCursor.insertText(QString("ordning v%1 (%2)").arg(BUILD_VERSION, BUILD_DATE), format);
        cellCursor.insertBlock(blockFormat);
        cellCursor.insertText(BUILD_HASH, format);
    }
}
