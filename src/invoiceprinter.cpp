#include <QApplication>
#include <QtCore>
#include <QPdfWriter>
#include <QPainter>
#include <QTextDocument>
#include <QTextCursor>
#include <QTransform>
#include <QTextTable>
#include <QTextTableCell>

#include <memory>

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
    // The body text is 7 pt rather than 8, because the page is only 453 pt
    // wide between the margins and the longest things an invoice carries do
    // not all fit across it at 8 pt: an e-mail address, a Swedish IBAN with
    // its spaces and "Innehar F-skattsedel" stand side by side along the foot
    // and come to more than the page holds. Since the document is laid out to
    // the width of the page they would wrap rather than run off it, and a
    // wrapped e-mail address reads worse than a small one. The labels above
    // the values stay at 5 pt.
    m_font_normal = QFont("Arial", 7, QFont::Normal, false);
    m_font_normal_bold = QFont("Arial", 7, QFont::ExtraBold, false);

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
    QPdfWriter printer(filename);
    render(p_in, logo, b_preview, printer);
}

//-----------------------------------------------------------------------------
void InvoicePrinter::create_pdf(Invoice* p_in, QString logo, bool b_preview, QIODevice* device)
{
    QPdfWriter printer(device);
    render(p_in, logo, b_preview, printer);
}

//-----------------------------------------------------------------------------
void InvoicePrinter::render(Invoice* p_in, QString logo, bool b_preview, QPdfWriter& printer)
{
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
    std::unique_ptr<QTextDocument> p_doc = createDocument(p_in, printer);

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
std::unique_ptr<QTextDocument> InvoicePrinter::createDocument(Invoice* p_in, QPdfWriter &printer)
{
    const int available_width = printer.pageLayout().paintRectPoints().width();
    const int available_height = printer.pageLayout().paintRectPoints().height();

    qDebug() << "available size" << available_width << available_height;

    // update the default table width relative the available width
    m_table_format_default.setWidth(QTextLength(QTextLength::FixedLength, available_width));

    // Lays the whole invoice out in one document, with an empty frame of `fill`
    // points between the comments and the summary, which pushes the summary and
    // the company details to the foot of the page.
    auto build = [&](qreal fill) {
        auto doc = std::make_unique<QTextDocument>();

        // Without a text width the document is laid out as if the page were
        // endless, and a cell whose text is too long for its column widens the
        // table past the right edge of the page instead of wrapping. The margin
        // is added back so the tables, which are exactly the printable width,
        // sit where they have always sat.
        doc->setTextWidth(available_width + 2 * doc->documentMargin());

        QTextCursor cursor(doc.get());
        addHeader(p_in, cursor);
        addSenderReceiver(p_in, cursor);
        addCustomerInfo(p_in, cursor);
        addItems(p_in, cursor);
        addComments(p_in, cursor);

        cursor.movePosition(QTextCursor::End);
        QTextFrameFormat tff;
        tff.setHeight(fill);
        cursor.insertFrame(tff);

        // insertFrame() leaves the cursor inside the frame, so the foot is
        // written with a cursor of its own, at the end of the document.
        QTextCursor foot(doc.get());
        foot.movePosition(QTextCursor::End);
        addSummary(p_in, foot);
        addCompanyInfo(p_in, foot);
        return doc;
    };

    // Measured on the page as it will be printed: measuring the head and the
    // foot as documents of their own, as this used to, counts their margins
    // twice and gets a wrapped line wrong. The 17 pt past the printable height
    // keeps the footer where it was, which is where the lönebesked has it too
    // (test_salaryslipprinter holds the two to the same pixels).
    const qreal natural = build(0)->size().height();
    auto main_doc = build(qMax<qreal>(0, available_height + 17 - natural));

    qDebug() << "used size" << main_doc->size().width() << main_doc->size().height();

    return main_doc;
}

//-----------------------------------------------------------------------------
void InvoicePrinter::addHeader(Invoice* p_in, QTextCursor& cursor)
{
    QTextTableFormat table_format = m_table_format_default;

    table_format.setBorder(0);

    // The first column is the logo area, which is 130 pt wide; the last one
    // has to hold "Krediterar faktura nr: 2501", the longest of the lines
    // beside the heading, on one line.
    QList<QTextLength> columnWidthConstraintList;
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 33.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 33.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 34.0));
    table_format.setColumnWidthConstraints(columnWidthConstraintList);

    cursor.movePosition(QTextCursor::End);
    QTextTable *p_table = cursor.insertTable(3, 3, table_format);

    p_table->mergeCells(0, 0, 2, 1); // merge the 'LOGO' cells
    {
        // The logo is placed where this cell resides later in the factory
    }

    // For credit invoices, span the heading across all 3 rows so the row heights are
    // distributed evenly and the 3 metadata items in col 2 keep equal spacing.
    p_in->m_is_credit ? p_table->mergeCells(0, 1, 3, 1) : p_table->mergeCells(0, 1, 2, 1);
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 1);

        QTextCharFormat format;
        format.setFont(m_font_header);
        format.setVerticalAlignment(QTextCharFormat::AlignTop);
        cell.setFormat(format);

        QTextCursor cellCursor = cell.firstCursorPosition();

        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(Qt::AlignCenter);
        cellCursor.mergeBlockFormat(blockFormat);

        if (p_in->m_is_credit) {
            cellCursor.insertText("KREDIT", format);
            cellCursor.insertBlock(blockFormat);
            cellCursor.insertText("FAKTURA", format);
        } else {
            cellCursor.insertText("FAKTURA", format);
        }
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(0, 2);

        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.setFormat(format);

        QTextCursor cellCursor = cell.firstCursorPosition();

        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(Qt::AlignRight);
        cellCursor.mergeBlockFormat(blockFormat);

        cellCursor.insertText(QString("Fakturanr: %1").arg(p_in->m_number), format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(1, 2);

        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.setFormat(format);

        QTextCursor cellCursor = cell.firstCursorPosition();

        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(Qt::AlignRight);
        cellCursor.mergeBlockFormat(blockFormat);

        cellCursor.insertText(QString("Fakturadatum: %1").arg(p_in->m_date), format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(2, 2);

        QTextCharFormat format;
        format.setFont(m_font_normal);
        cell.setFormat(format);

        QTextCursor cellCursor = cell.firstCursorPosition();

        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(Qt::AlignRight);
        cellCursor.mergeBlockFormat(blockFormat);

        if (p_in->m_is_credit) {
            cellCursor.insertText(QString("Krediterar faktura nr: %1").arg(p_in->m_credited_invoice_number), format);
        } else {
            cellCursor.insertText(QString("Förfallodatum: %1").arg(p_in->m_due_date), format);
        }
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

    // Antal holds "160,0 tim", Á-pris and Belopp whole amounts with their
    // thousands separators — Belopp the widest of them, since it is a month of
    // them added up — and Moms only "25 %". The first column takes what is
    // left, which is what the consultant and the period are written across.
    QList<QTextLength> columnWidthConstraintList;
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 49.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 13.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 13.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 7.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 18.0));
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
        cell.firstCursorPosition().insertText(printer_util_format_money(p_in->m_agreement_standard_hourly_rate), format);
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
        cell.firstCursorPosition().insertText(printer_util_format_money((p_in->m_sum_standard)), format);
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
            cell.firstCursorPosition().insertText(printer_util_format_money(p_in->m_agreement_overtime_hourly_rate), format);
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
            cell.firstCursorPosition().insertText(printer_util_format_money((p_in->m_sum_overtime)), format);
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
            cell.firstCursorPosition().insertText(printer_util_format_money(p_in->m_agreement_qualified_hourly_rate), format);
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
            cell.firstCursorPosition().insertText(printer_util_format_money((p_in->m_sum_qualified)), format);
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
            cell.firstCursorPosition().insertText(printer_util_format_money(p_in->m_agreement_traveltime_hourly_rate), format);
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
            cell.firstCursorPosition().insertText(printer_util_format_money((p_in->m_sum_traveltime)), format);
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
        cell.firstCursorPosition().insertText(printer_util_format_money(p_in->m_sum), format);
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
        cell.firstCursorPosition().insertText(printer_util_format_money(p_in->m_vat), format);
        printer_util_table_cell_align_text(p_table, 1, 2, Qt::AlignRight);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(2, 1);
        QTextCharFormat format;
        format.setFont(m_font_normal_bold);
        cell.firstCursorPosition().insertText(p_in->m_is_credit ? "Er tillgodo (SEK)" : "Att betala (SEK)", format);
    }
    {
        QTextTableCell cell = p_table->QTextTable::cellAt(2, 2);
        QTextCharFormat format;
        format.setFont(m_font_normal_bold);
        cell.firstCursorPosition().insertText(printer_util_format_money(p_in->m_sum_including_vat), format);
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

    // Three columns of very different content: numbers of the company on the
    // left, the bank details in the middle, where a Swedish IBAN written in
    // groups of four is the widest thing on the page but one, and the ways of
    // reaching the company on the right, where that one is: the e-mail
    // address. Equal thirds leave the last two wrapping.
    QList<QTextLength> columnWidthConstraintList;
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 23.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 37.0));
    columnWidthConstraintList.append(QTextLength(QTextLength::PercentageLength, 40.0));
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
        QTextTableCell cell = p_table->QTextTable::cellAt(2, 2);
        QTextCharFormat format;
        format.setFont(m_font_super);
        cell.firstCursorPosition().insertText("Web\n", format);
        format.setFont(m_font_normal);
        cell.lastCursorPosition().insertText(p_in->m_company_web, format);
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
