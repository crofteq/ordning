#include <QBuffer>
#include <QPainter>
#include <QStringList>
#include <QPdfWriter>
#include <QTextDocument>
#include <QTextFrame>

#include "util/printerutilities.h"
#include "version.h"

#include "salaryslipprinter.h"

#define DATE_FORMAT_STRING  "yyyy-MM-dd"

//-----------------------------------------------------------------------------
// The page, the fonts and the table style are those of InvoicePrinter, so a
// lönebesked and an invoice from the same company look like they belong
// together.
SalarySlipPrinter::SalarySlipPrinter()
{
    // "LÖNEBESKED" is half again as long as the invoice's "FAKTURA", so it is
    // set smaller to sit as calmly beside the logo as the invoice heading does.
    m_font_header = QFont("Arial", 14, QFont::Normal, false);
    m_font_super = QFont("Arial", 5, QFont::Normal, false);
    // The body is the invoice's 8 pt. The lönebesked has no company-details
    // strip along its foot, which is the one thing on an invoice that has to
    // stay smaller, so the whole statement is set at the one size.
    m_font_normal = QFont("Arial", 8, QFont::Normal, false);
    m_font_normal_bold = QFont("Arial", 8, QFont::ExtraBold, false);

    m_table_format_default.setBorderCollapse(false);
    m_table_format_default.setCellSpacing(0);
    m_table_format_default.setCellPadding(3);
    m_table_format_default.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    m_table_format_default.setBorder(0.5);
    m_table_format_default.setBorderBrush(QBrush(QColor(245, 245, 245)));
}

//-----------------------------------------------------------------------------
void SalarySlipPrinter::create_pdf(const SalarySlip& slip, const QString& logo, const SalaryYearToDate& ytd, bool preview, QIODevice* device)
{
    SalarySlipPrinter printer;
    printer.render(slip, logo, ytd, preview, device);
}

//-----------------------------------------------------------------------------
bool SalarySlipPrinter::fits(const SalarySlip& slip)
{
    // Laid out for the same page as create_pdf(), but nothing is painted.
    QBuffer scratch;
    scratch.open(QIODevice::WriteOnly);
    QPdfWriter writer(&scratch);
    writer.setPageSize(QPageSize::A4Small);
    writer.setPageMargins(QMarginsF(20, 25, 20, 15));
    writer.setResolution(300);

    SalarySlipPrinter printer;
    printer.createDocument(slip, SalaryYearToDate(), writer);
    return printer.m_spare >= 0;
}

//-----------------------------------------------------------------------------
QTextTableFormat SalarySlipPrinter::tableFormat(const QList<qreal>& percentages, bool border) const
{
    QTextTableFormat format = m_table_format_default;
    if (!border)
    {
        format.setBorder(0);
    }
    QList<QTextLength> widths;
    for (qreal p : percentages)
    {
        widths.append(QTextLength(QTextLength::PercentageLength, p));
    }
    format.setColumnWidthConstraints(widths);
    return format;
}

//-----------------------------------------------------------------------------
void SalarySlipPrinter::cellText(QTextTable* p_table, int row, int column, const QString& text, const QFont& font, Qt::Alignment alignment)
{
    QTextCharFormat format;
    format.setFont(font);
    p_table->cellAt(row, column).firstCursorPosition().insertText(text, format);
    if (alignment != Qt::AlignLeft)
    {
        printer_util_table_cell_align_text(p_table, row, column, alignment);
    }
}

//-----------------------------------------------------------------------------
// A small label above its value, as in the boxes of the invoice.
void SalarySlipPrinter::cellLabelled(QTextTable* p_table, int row, int column, const QString& label, const QString& value)
{
    QTextCursor c = p_table->cellAt(row, column).firstCursorPosition();
    QTextCharFormat format;
    format.setFont(m_font_super);
    c.insertText(label + "\n", format);
    format.setFont(m_font_normal);
    c.insertText(value, format);
}

//-----------------------------------------------------------------------------
// A value with what it was for under it, the other way round from
// cellLabelled(): the benämning as on any line, then the comment in the small
// type on a line of its own.
void SalarySlipPrinter::cellCommented(QTextTable* p_table, int row, int column, const QString& text, const QString& comment)
{
    QTextCursor c = p_table->cellAt(row, column).firstCursorPosition();
    QTextCharFormat format;
    format.setFont(m_font_normal);
    c.insertText(text + "\n", format);
    format.setFont(m_font_super);
    c.insertText(comment, format);
}

//-----------------------------------------------------------------------------
void SalarySlipPrinter::addSpace(QTextCursor& cursor, const QString& lines)
{
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    format.setFont(m_font_super);
    cursor.insertText(lines, format);
}

//-----------------------------------------------------------------------------
void SalarySlipPrinter::render(const SalarySlip& slip, const QString& logo, const SalaryYearToDate& ytd, bool preview, QIODevice* device)
{
    QPdfWriter writer(device);
    writer.setPageSize(QPageSize::A4Small);
    writer.setPageMargins(QMarginsF(20, 25, 20, 15));
    writer.setResolution(300);
    writer.setTitle(QString("Lönebesked %1 %2").arg(slip.m_employee_name, slip.m_payment_date.toString(DATE_FORMAT_STRING)));

    QPainter painter(&writer);
    printer_util_painter_scale(&painter, &writer);

    if (preview)
    {
        // Drawn first so it ends up in the background.
        printer_util_insert_draft(&painter);
    }

    std::unique_ptr<QTextDocument> p_doc = createDocument(slip, ytd, writer);

    // The same logo area as on the invoices.
    printer_util_insert_svg_string(&painter, QRectF(5.0, 3.0, 130.0, 35.0), logo, false);

    p_doc->drawContents(&painter);
    painter.end();
}

//-----------------------------------------------------------------------------
std::unique_ptr<QTextDocument> SalarySlipPrinter::createDocument(const SalarySlip& slip, const SalaryYearToDate& ytd, QPdfWriter& writer)
{
    const int available_width = writer.pageLayout().paintRectPoints().width();
    const int available_height = writer.pageLayout().paintRectPoints().height();

    m_table_format_default.setWidth(QTextLength(QTextLength::FixedLength, available_width));

    // Lays out the whole page with an empty frame of `fill` points between
    // the top and the bottom, which pushes the bottom to the foot of the page.
    auto build = [&](qreal fill) {
        auto doc = std::make_unique<QTextDocument>();

        // Without a text width a document is laid out as if the page were
        // endless, and a cell whose text is too long for its column widens the
        // table past the edge instead of wrapping. The margin is added back so
        // the tables, which are exactly the printable width, sit where they do
        // on an invoice.
        doc->setTextWidth(available_width + 2 * doc->documentMargin());

        QTextCursor cursor(doc.get());
        addHeader(slip, cursor);
        addSenderReceiver(slip, cursor);
        addEmployeeInfo(slip, cursor);
        addLines(slip, cursor);

        cursor.movePosition(QTextCursor::End);
        QTextFrameFormat format;
        format.setHeight(fill);
        cursor.insertFrame(format);

        QTextCursor foot(doc.get());
        foot.movePosition(QTextCursor::End);
        addSummary(slip, ytd, foot);
        addFooter(foot);
        return doc;
    };

    // Measured on the page as it will be printed, since measuring the parts
    // on their own, as InvoicePrinter does, gets wrapped lines wrong. The 17 pt
    // past the printable height puts the foot where the invoice has it; the
    // invoice gets there with 25 pt measured its own way.
    // test_salaryslipprinter holds the two to the same pixels.
    const qreal natural = build(0)->size().height();
    m_spare = available_height + 17 - natural;
    auto doc = build(qMax<qreal>(0, m_spare));
    return doc;
}

//-----------------------------------------------------------------------------
void SalarySlipPrinter::addHeader(const SalarySlip& slip, QTextCursor& cursor)
{
    // "LÖNEBESKED" is wider than "FAKTURA", so the middle column is wider. The
    // left column still holds the 130 pt logo area.
    QTextTable* p_table = cursor.insertTable(3, 3, tableFormat({ 27.0, 43.0, 30.0 }, false));

    // The logo is painted over the merged cell on the left afterwards.
    p_table->mergeCells(0, 0, 2, 1);

    p_table->mergeCells(0, 1, 2, 1);
    {
        QTextTableCell cell = p_table->cellAt(0, 1);
        QTextCharFormat format;
        format.setFont(m_font_header);
        format.setVerticalAlignment(QTextCharFormat::AlignTop);
        cell.setFormat(format);
        QTextCursor c = cell.firstCursorPosition();
        QTextBlockFormat block;
        block.setAlignment(Qt::AlignCenter);
        c.mergeBlockFormat(block);
        c.insertText("LÖNEBESKED", format);
    }

    cellText(p_table, 0, 2, "Utbetalningsdag", m_font_normal, Qt::AlignRight);
    cellText(p_table, 1, 2, slip.m_payment_date.toString(DATE_FORMAT_STRING), m_font_normal, Qt::AlignRight);

    addSpace(cursor, " \n \n");
}

//-----------------------------------------------------------------------------
void SalarySlipPrinter::addSenderReceiver(const SalarySlip& slip, QTextCursor& cursor)
{
    cursor.movePosition(QTextCursor::End);
    // Each party's number in the row right under it: the same table, so the
    // two boxes meet with only the cell border between them.
    QTextTable* p_table = cursor.insertTable(2, 2, tableFormat({ 50.0, 50.0 }));

    cellLabelled(p_table, 0, 0, "Arbetsgivare",
                 slip.m_company_name + "\n" + slip.m_company_adrsline1 + "\n" + slip.m_company_adrsline2);
    cellLabelled(p_table, 0, 1, "Anställd",
                 slip.m_employee_name + "\n" + slip.m_employee_adrsline1 + "\n" + slip.m_employee_adrsline2);
    cellLabelled(p_table, 1, 0, "Organisationsnummer", slip.m_company_org_number);
    cellLabelled(p_table, 1, 1, "Personnummer", slip.m_personnummer);

    addSpace(cursor, " \n");
}

//-----------------------------------------------------------------------------
void SalarySlipPrinter::addEmployeeInfo(const SalarySlip& slip, QTextCursor& cursor)
{
    cursor.movePosition(QTextCursor::End);
    // Bankkonto starts half way across, under Anställd and Personnummer.
    QTextTable* p_table = cursor.insertTable(1, 3, tableFormat({ 25.0, 25.0, 50.0 }));

    cellLabelled(p_table, 0, 0, "Anställningsnummer", slip.m_employment_number);
    cellLabelled(p_table, 0, 1, "Skattetabell", slip.m_tax_table > 0 ? QString::number(slip.m_tax_table) : QString());
    cellLabelled(p_table, 0, 2, "Bankkonto", slip.m_bank_account);

    addSpace(cursor, " \n");
}

//-----------------------------------------------------------------------------
void SalarySlipPrinter::addLines(const SalarySlip& slip, QTextCursor& cursor)
{
    cursor.movePosition(QTextCursor::End);
    // Each column is given what its widest figure needs and a little over, and
    // the benämning keeps the rest — 211 pt of the 481, which is what it had at
    // 7 pt on the narrower page, so what a line has to say did not lose by the
    // body growing.
    //
    // Á-pris is the column that had this wrong: it was given a tenth of the
    // page, which is an hourly rate on an invoice but a whole month's salary
    // here, and "250.000,00" was broken over two lines. It was broken at 7 pt
    // too — the tests read the page for the figure and found the line total,
    // which is the same figure whenever the antal is 1,00.
    QTextTable* p_table = cursor.insertTable(1 + slip.m_lines.size(), 5, tableFormat({ 8.0, 46.4, 11.1, 15.9, 18.6 }));

    cellText(p_table, 0, 0, "Löneart", m_font_super);
    cellText(p_table, 0, 1, "Benämning", m_font_super);
    cellText(p_table, 0, 2, "Antal", m_font_super, Qt::AlignCenter);
    cellText(p_table, 0, 3, "Á-pris (SEK)", m_font_super, Qt::AlignCenter);
    cellText(p_table, 0, 4, "Belopp (SEK)", m_font_super, Qt::AlignRight);

    for (int i = 0; i < slip.m_lines.size(); i++)
    {
        const SalaryLine& line = slip.m_lines.at(i);
        const int row = i + 1;
        // A line of an older slip may carry no löneart: they were free text
        // before, and a code that means nothing now is printed as nothing.
        cellText(p_table, row, 0, line.code != SALARY_TYPE_NONE ? QString::number(line.code) : QString(), m_font_normal);
        // What the line was for goes under its benämning, in the small type: an
        // explanation of a line is not another line. What the line applies to
        // comes first — for a traktamente the place it was paid for — and what
        // it was for after it: the employer has to be able to show both, and
        // they are not the same thing, which is why the line keeps them apart.
        QStringList about;
        if (!line.applies_to.isEmpty())
        {
            about.append(line.applies_to);
        }
        if (!line.comment.isEmpty())
        {
            about.append(line.comment);
        }
        if (about.isEmpty())
        {
            cellText(p_table, row, 1, line.text, m_font_normal);
        }
        else
        {
            cellCommented(p_table, row, 1, line.text, about.join(" – "));
        }
        cellText(p_table, row, 2, printer_util_format_money(line.quantity), m_font_normal, Qt::AlignCenter);
        cellText(p_table, row, 3, printer_util_format_money(line.amount), m_font_normal, Qt::AlignCenter);
        cellText(p_table, row, 4, printer_util_format_money(line.total_ore()), m_font_normal, Qt::AlignRight);
    }

    addSpace(cursor, " \n");
}

//-----------------------------------------------------------------------------
void SalarySlipPrinter::addSummary(const SalarySlip& slip, const SalaryYearToDate& ytd, QTextCursor& cursor)
{
    // The year so far on the left and this period on the right, where the
    // invoice has its total. One table for both, so that Bruttolön and Skatt
    // share their rows; the empty middle column keeps the two apart.
    //
    // A slip with no skattefri ersättning on it has no row for one, so a
    // lönebesked of ordinary lön is laid out exactly as it was before there
    // were two kinds of line.
    const bool any_tax_free = slip.m_tax_free != 0 || ytd.tax_free != 0;
    const int row_tax_free = any_tax_free ? 3 : -1;
    const int row_net = any_tax_free ? 4 : 3;

    QTextTable* p_table = cursor.insertTable(row_net + 1, 5, tableFormat({ 22.0, 20.0, 8.0, 25.0, 25.0 }, false));

    cellText(p_table, 0, 0, "Totalt under året", m_font_super);
    cellText(p_table, 0, 3, "Denna period", m_font_super);

    cellText(p_table, 1, 0, "Bruttolön", m_font_normal);
    cellText(p_table, 1, 1, printer_util_format_money(ytd.gross), m_font_normal, Qt::AlignRight);
    cellText(p_table, 1, 3, "Bruttolön", m_font_normal);
    cellText(p_table, 1, 4, printer_util_format_money(slip.m_gross), m_font_normal, Qt::AlignRight);

    cellText(p_table, 2, 0, "Skatt", m_font_normal);
    cellText(p_table, 2, 1, printer_util_format_money(ytd.tax), m_font_normal, Qt::AlignRight);
    cellText(p_table, 2, 3, "Skatt", m_font_normal);
    cellText(p_table, 2, 4, printer_util_format_money(-slip.m_tax), m_font_normal, Qt::AlignRight);

    if (any_tax_free)
    {
        // Paid out on top of the lön rather than taken out of it, so it stands
        // with a plus where the skatt stands with a minus.
        cellText(p_table, row_tax_free, 0, "Skattefritt", m_font_normal);
        cellText(p_table, row_tax_free, 1, printer_util_format_money(ytd.tax_free), m_font_normal, Qt::AlignRight);
        cellText(p_table, row_tax_free, 3, "Skattefritt", m_font_normal);
        cellText(p_table, row_tax_free, 4, printer_util_format_money(slip.m_tax_free), m_font_normal, Qt::AlignRight);
    }

    cellText(p_table, row_net, 3, "Utbetalt (SEK)", m_font_normal_bold);
    cellText(p_table, row_net, 4, printer_util_format_money(slip.m_net), m_font_normal_bold, Qt::AlignRight);

    addSpace(cursor, " \n \n \n");
}

//-----------------------------------------------------------------------------
void SalarySlipPrinter::addFooter(QTextCursor& cursor)
{
    // The same build line as on the invoices.
    addSpace(cursor, "\n\n");
    QTextTableFormat footer = tableFormat({ 50.0, 50.0 }, false);
    footer.setCellPadding(0);
    cursor.movePosition(QTextCursor::End);
    QTextTable* p_footer = cursor.insertTable(1, 2, footer);

    QTextCharFormat grey;
    grey.setFont(m_font_super);
    grey.setForeground(QBrush(QColor(186, 186, 186)));
    p_footer->cellAt(0, 0).firstCursorPosition().insertText(REPOSITORY_URL, grey);

    QTextCursor c = p_footer->cellAt(0, 1).firstCursorPosition();
    QTextBlockFormat right;
    right.setAlignment(Qt::AlignRight);
    c.mergeBlockFormat(right);
    c.insertText(QString("ordning v%1 (%2)").arg(BUILD_VERSION, BUILD_DATE), grey);
    c.insertBlock(right);
    c.insertText(BUILD_HASH, grey);
}
