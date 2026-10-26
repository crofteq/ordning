#ifndef COMPANYREPORTPRINTER_H
#define COMPANYREPORTPRINTER_H

#include <QFont>
#include <QImage>
#include <QIODevice>
#include <QTextCursor>
#include <QTextTable>

#include "data/companyinfo.h"
#include "data/companyreport.h"

// Renders the company's years as a PDF, an ekonomisk översikt: the key
// figures of the latest year against the one before and a flerårsöversikt
// first, then what was invoiced, per customer, uppdrag and consultant, what
// the salaries cost, and what it all rests on. Laid out like the invoices, so
// it looks like it belongs, and over as many pages as it takes.
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
    // One series per month, for the last year and the one before it.
    static QImage monthChart(const CompanyReport& report, const QSize& size, Series series);

    // The colours of the two series, and the lighter one the year before the
    // last is drawn in on the month chart.
    static QColor invoicedColour(void);
    static QColor salaryColour(void);
    static QColor previousYearColour(Series series);

    // How the report writes its figures: whole thousands of kronor, grouped
    // with a space that does not break, and a real minus sign.
    static QString formatThousands(long long ore);
    static QString formatWhole(long long value);

private:
    CompanyReportPrinter();

    // What a row of a table is, which decides how it is set.
    enum RowKind { Body, Emphasis, Group, Indented, Total };
    struct Row
    {
        QStringList cells;
        RowKind kind = Body;
    };

    QFont m_font_title;
    QFont m_font_chapter;
    QFont m_font_heading;
    QFont m_font_super;
    QFont m_font_normal;
    QFont m_font_normal_bold;
    QFont m_font_note;
    QFont m_font_table;
    QFont m_font_table_bold;
    QTextTableFormat m_table_format_default;
    // Where each chart is, so that one whose caption falls on the next page
    // can be moved there with it.
    QList<int> m_charts;
    // Where each table's heading is, so that one at the foot of a page can be
    // moved to the next with the table it heads.
    QList<int> m_headings;

    void render(const CompanyReport& report, const CompanyInfo& company, QIODevice* device);

    void addHeader(const CompanyReport& report, const CompanyInfo& company, QTextCursor& cursor);
    // Returns where the heading was put, so that it can be moved to the next
    // page if it ends up too close to the foot of one.
    int addChapter(const QString& title, QTextCursor& cursor);
    void addHeading(const QString& title, QTextCursor& cursor);
    // A line of small grey type, under a chart or a table, saying what it
    // counts or what it is compared with.
    void addNote(const QString& text, QTextCursor& cursor, Qt::Alignment alignment = Qt::AlignLeft);
    void addChart(const QImage& image, const QString& caption, QTextCursor& cursor, int width, int height);

    void addKeyFigures(const CompanyReport& report, QTextCursor& cursor);
    void addOverview(const CompanyReport& report, QTextCursor& cursor);
    void addSalaryYears(const CompanyReport& report, QTextCursor& cursor);
    // A line per customer, uppdrag, employee or consultant, a column per year,
    // the total and the change, and a sum last. `hours` says the figures are
    // hundredths of an hour rather than öre.
    void addSeries(const QString& heading, const QString& first_column, const QList<CompanyReportSeries>& series,
                   const CompanyReport& report, bool hours, QTextCursor& cursor);
    void addPrinciples(const CompanyReport& report, QTextCursor& cursor);

    // The name column takes what the figures leave; the figures are measured
    // from what stands in them and may not wrap.
    void addTable(const QStringList& headings, const QList<Row>& rows, QTextCursor& cursor);
    void cellText(QTextTable* p_table, int row, int column, const QString& text, const QFont& font,
                  Qt::Alignment alignment = Qt::AlignLeft, const QColor& colour = Qt::black);
    void addSpace(QTextCursor& cursor, const QString& lines);
};

#endif // COMPANYREPORTPRINTER_H
