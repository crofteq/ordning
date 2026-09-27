#ifndef COMPANYREPORTPRINTER_H
#define COMPANYREPORTPRINTER_H

#include <QFont>
#include <QImage>
#include <QIODevice>
#include <QTextCursor>
#include <QTextTable>

#include "data/companyinfo.h"
#include "data/companyreport.h"

// Renders the company's years as a PDF: what was invoiced per year and per
// month, against what the salaries cost, and the years per customer and per
// employee. Laid out like the invoices, so it looks like it belongs, and over
// as many pages as it takes.
class CompanyReportPrinter
{
public:
    static void create_pdf(const CompanyReport& report, const CompanyInfo& company, QIODevice* device);

    // What a chart draws: what was invoiced, or what the salaries cost.
    enum Series { Invoiced, SalaryCost };

    // The charts on their own, which is also what makes them testable.
    // One series, a bar per year.
    static QImage yearChart(const CompanyReport& report, const QSize& size, Series series);
    // Both series, two bars a year, for the summary.
    static QImage comparisonChart(const CompanyReport& report, const QSize& size);
    // One series per month, one bar per year in each month.
    static QImage monthChart(const CompanyReport& report, const QSize& size, Series series);

    // The colours of the two series of the year chart, and the one a year is
    // drawn in on the month chart.
    static QColor invoicedColour(void);
    static QColor salaryColour(void);
    static QColor yearColour(int index);

private:
    CompanyReportPrinter();

    QFont m_font_header;
    QFont m_font_chapter;
    QFont m_font_super;
    QFont m_font_normal;
    QFont m_font_normal_bold;
    QFont m_font_caption;
    QFont m_font_table;
    QFont m_font_table_bold;
    QTextTableFormat m_table_format_default;

    void render(const CompanyReport& report, const CompanyInfo& company, QIODevice* device);

    void addHeader(const CompanyReport& report, const CompanyInfo& company, QTextCursor& cursor);
    // Returns where the heading was put, so that it can be moved to the next
    // page if it ends up too close to the foot of one.
    int addChapter(const QString& title, QTextCursor& cursor);
    // `caption` goes under the chart, saying what is counted along its axes.
    void addChart(const QImage& image, const QString& caption, QTextCursor& cursor, int width, int height);
    void addYears(const CompanyReport& report, QTextCursor& cursor);
    // The columns of amounts are measured from the widest figure that will
    // stand in them, and the first column takes whatever is left.
    void addSeries(const QString& heading, const QString& first_column,
                   const QList<CompanyReportSeries>& series, const CompanyReport& report, QTextCursor& cursor);

    QTextTableFormat tableFormat(const QList<qreal>& percentages, bool border = true) const;
    // Columns measured in points, for a table whose figures decide its shape.
    QTextTableFormat tableFormat(const QList<QTextLength>& lengths, bool border = true) const;
    // Moves a cell's text in from the left, for a line that belongs under the
    // one above it.
    void cellIndent(QTextTable* p_table, int row, int column, qreal margin);
    void cellText(QTextTable* p_table, int row, int column, const QString& text, const QFont& font, Qt::Alignment alignment = Qt::AlignLeft);
    void addSpace(QTextCursor& cursor, const QString& lines);
};

#endif // COMPANYREPORTPRINTER_H
