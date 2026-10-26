#ifndef SALARYSLIPPRINTER_H
#define SALARYSLIPPRINTER_H

#include <QFont>
#include <QIODevice>
#include <QPdfWriter>
#include <QTextCursor>
#include <QTextTable>

#include <memory>

#include "data/salaryslips.h"

// Renders a salary slip as a one-page PDF lönebesked, laid out like the
// invoices from InvoicePrinter: the same page, fonts, tables and logo area.
class SalarySlipPrinter
{
public:
    // Writes the PDF to `device`, which must be open for writing. `logo` is the
    // company's SVG, placed where the invoices have it. `ytd` holds the "Totalt
    // under året" figures, see SalarySlips::yearToDate(). A preview carries a
    // UTKAST watermark.
    static void create_pdf(const SalarySlip& slip, const QString& logo, const SalaryYearToDate& ytd, bool preview, QIODevice* device);

    // Whether the lönebesked fits its one page. SALARY_MAX_LINES short lines
    // always do, but long benämningar wrap and take more room.
    static bool fits(const SalarySlip& slip);

private:
    SalarySlipPrinter();

    QFont m_font_header;
    QFont m_font_super;
    QFont m_font_normal;
    QFont m_font_normal_bold;
    QTextTableFormat m_table_format_default;
    qreal m_spare = 0; // room left on the page by the last createDocument()

    void render(const SalarySlip& slip, const QString& logo, const SalaryYearToDate& ytd, bool preview, QIODevice* device);
    std::unique_ptr<QTextDocument> createDocument(const SalarySlip& slip, const SalaryYearToDate& ytd, QPdfWriter& writer);

    void addHeader(const SalarySlip& slip, QTextCursor& cursor);
    void addSenderReceiver(const SalarySlip& slip, QTextCursor& cursor);
    void addEmployeeInfo(const SalarySlip& slip, QTextCursor& cursor);
    void addLines(const SalarySlip& slip, QTextCursor& cursor);
    void addSummary(const SalarySlip& slip, const SalaryYearToDate& ytd, QTextCursor& cursor);
    void addFooter(QTextCursor& cursor);

    QTextTableFormat tableFormat(const QList<qreal>& percentages, bool border = true) const;
    void cellText(QTextTable* p_table, int row, int column, const QString& text, const QFont& font, Qt::Alignment alignment = Qt::AlignLeft);
    void cellLabelled(QTextTable* p_table, int row, int column, const QString& label, const QString& value);
    void cellCommented(QTextTable* p_table, int row, int column, const QString& text, const QString& comment);
    void addSpace(QTextCursor& cursor, const QString& lines);
};

#endif // SALARYSLIPPRINTER_H
