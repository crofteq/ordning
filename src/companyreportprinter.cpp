#include <QAbstractTextDocumentLayout>
#include <QDateTime>
#include <QTextBlock>
#include <QPainter>
#include <QPdfWriter>
#include <QTextDocument>
#include <QUrl>

#include <algorithm>

#include "data/fiscalyear.h"
#include "util/printerutilities.h"
#include "version.h"

#include "companyreportprinter.h"

#define MONTH_NAMES { "Jan", "Feb", "Mar", "Apr", "Maj", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dec" }
#define MONTH_NAMES_LONG { "januari", "februari", "mars", "april", "maj", "juni", \
                           "juli", "augusti", "september", "oktober", "november", "december" }

// What the report is called, on its first page, over every other page and in
// the PDF's metadata.
static const char* TITLE = "Ekonomisk översikt";

// A space inside a figure must not become a line break, and a minus sign is a
// minus sign rather than a hyphen. A dash stands for nothing.
static const QChar NBSP(0x00A0);
static const QChar MINUS(0x2212);
static const QString NONE = QString::fromUtf8("–");

// The room above a chapter heading, in points.
static const qreal CHAPTER_SPACE = 20.0;

// The rules of the tables, and the fill behind their headings.
static const QColor RULE_STRONG(70, 70, 70);
static const QColor RULE_LIGHT(222, 222, 222);
static const QColor HEADING_FILL(240, 242, 245);
// The chapter headings and the title, in the colour of what was invoiced.
static const QColor ACCENT(35, 70, 110);
static const QColor NOTE_GREY(100, 100, 100);

//-----------------------------------------------------------------------------
QColor CompanyReportPrinter::invoicedColour(void)
{
    return QColor(51, 102, 153);    // fakturerat
}

//-----------------------------------------------------------------------------
QColor CompanyReportPrinter::salaryColour(void)
{
    return QColor(196, 120, 60);    // lönekostnad
}

//-----------------------------------------------------------------------------
QColor CompanyReportPrinter::previousYearColour(Series series)
{
    // The same colour, lighter, so the two years read as one series.
    return series == Invoiced ? QColor(163, 190, 217) : QColor(230, 196, 168);
}

//-----------------------------------------------------------------------------
// Rounded half away from zero, as everything else in Ordning is.
static long long divideRounded(long long value, long long divisor)
{
    const long long half = divisor / 2;
    return value >= 0 ? (value + half) / divisor : -((-value + half) / divisor);
}

//-----------------------------------------------------------------------------
QString CompanyReportPrinter::formatWhole(long long value)
{
    QString digits = QString::number(value < 0 ? -value : value);
    for (int i = digits.size() - 3; i > 0; i -= 3)
    {
        digits.insert(i, NBSP);
    }
    return value < 0 ? QString(MINUS) + digits : digits;
}

//-----------------------------------------------------------------------------
QString CompanyReportPrinter::formatThousands(long long ore)
{
    return formatWhole(divideRounded(ore, 100000));
}

//-----------------------------------------------------------------------------
// Hundredths of an hour as whole hours.
static QString formatHours(long long hundredths)
{
    return CompanyReportPrinter::formatWhole(divideRounded(hundredths, 100));
}

//-----------------------------------------------------------------------------
// Hundredths as a figure with one decimal, and its sign when `sign` says so:
// "34,5", or "+2,1" for a change.
static QString formatTenths(long long hundredths, bool sign)
{
    const long long tenths = divideRounded(hundredths, 10);
    const long long whole = (tenths < 0 ? -tenths : tenths);
    QString text = QString("%1,%2").arg(CompanyReportPrinter::formatWhole(whole / 10)).arg(whole % 10);
    if (tenths < 0)
    {
        text.prepend(MINUS);
    }
    else if (sign && tenths > 0)
    {
        text.prepend('+');
    }
    return text;
}

//-----------------------------------------------------------------------------
// A share in hundredths of a percent.
static QString formatPercent(long long hundredths)
{
    return formatTenths(hundredths, false) + NBSP + "%";
}

//-----------------------------------------------------------------------------
// How much `current` changed from `previous`, in percent. A change from
// nothing, or from less than nothing, is no percentage at all.
static QString formatChange(long long current, long long previous)
{
    if (previous <= 0)
    {
        return NONE;
    }
    return formatTenths(divideRounded((current - previous) * 10000, previous), true) + NBSP + "%";
}

//-----------------------------------------------------------------------------
// How much a share changed, in procentenheter: a share going from 30 % to
// 33 % grew by 3 procentenheter, not by 10 %.
static QString formatPoints(long long current, long long previous)
{
    return formatTenths(current - previous, true) + NBSP + "p.e.";
}

//-----------------------------------------------------------------------------
// What a year is called in a heading: the name, and the months it holds when
// it is still running, which stay on the line with it.
static QString yearHeading(const CompanyReport& report, int index)
{
    const CompanyReportYear& year = report.years.at(index);
    if (report.running && index == report.years.size() - 1)
    {
        return year.name + NBSP + report.elapsedMonths();
    }
    return year.name;
}

//-----------------------------------------------------------------------------
// The years the report covers: "2025", or "2021–2025".
static QString periodOf(const CompanyReport& report)
{
    if (report.years.isEmpty())
    {
        return QString();
    }
    if (report.years.size() == 1)
    {
        return report.years.first().name;
    }
    return QString("%1–%2").arg(report.years.first().name, report.years.last().name);
}

//-----------------------------------------------------------------------------
CompanyReportPrinter::CompanyReportPrinter()
{
    m_font_title = QFont("Arial", 14, QFont::Normal, false);
    m_font_chapter = QFont("Arial", 12, QFont::Bold, false);
    m_font_heading = QFont("Arial", 8, QFont::Bold, false);
    m_font_super = QFont("Arial", 5, QFont::Normal, false);
    m_font_normal = QFont("Arial", 8, QFont::Normal, false);
    m_font_normal_bold = QFont("Arial", 8, QFont::Bold, false);
    m_font_note = QFont("Arial", 6, QFont::Normal, true);
    // The tables carry a figure per year across the page and a customer or an
    // uppdrag in the first column, so they are the one thing that runs out of
    // width. A point smaller than the rest of the report buys that room.
    m_font_table = QFont("Arial", 7, QFont::Normal, false);
    m_font_table_bold = QFont("Arial", 7, QFont::Bold, false);

    // The tables are ruled the way a financial statement is: across, under
    // the headings, between the lines and over the sum, and never down.
    m_table_format_default.setBorderCollapse(true);
    m_table_format_default.setCellSpacing(0);
    m_table_format_default.setCellPadding(3);
    m_table_format_default.setBorder(0);
}

//-----------------------------------------------------------------------------
// The tallest bar decides the scale, rounded up to something a person reads
// easily: 1, 2 or 5 times a power of ten, in kronor.
static long long chartScale(long long highest_ore)
{
    if (highest_ore <= 0)
    {
        return 100000; // 1 000 kr, so an empty chart still has an axis
    }

    long long step = 100; // 1 krona
    while (true)
    {
        for (long long factor : { 1LL, 2LL, 5LL })
        {
            if (step * factor >= highest_ore)
            {
                return step * factor;
            }
        }
        step *= 10;
    }
}

//-----------------------------------------------------------------------------
// A figure on the axis, in the unit the scale reads best in: millions once it
// reaches one, thousands below that, and kronor for a chart with next to
// nothing on it.
static QString axisLabel(long long ore, long long scale)
{
    if (scale >= 100000000LL)
    {
        QString text = formatTenths(divideRounded(ore, 1000000), false);
        if (text.endsWith(",0"))
        {
            text.chop(2);
        }
        return text + NBSP + "Mkr";
    }
    if (scale >= 1000000LL)
    {
        return CompanyReportPrinter::formatThousands(ore) + NBSP + "tkr";
    }
    return CompanyReportPrinter::formatWhole(divideRounded(ore, 100)) + NBSP + "kr";
}

//-----------------------------------------------------------------------------
// The frame every chart is drawn in: the plot, its lines and its scale, plus
// the font to label it with.
struct ChartFrame
{
    QRect plot;
    long long scale = 0;
    QFont font;
};

static ChartFrame drawFrame(QPainter* p_painter, const QSize& size, long long highest)
{
    ChartFrame frame;
    frame.plot = QRect(size.width() / 10, size.height() / 8,
                       size.width() - size.width() / 10 - size.width() / 40,
                       size.height() - size.height() / 8 - size.height() / 5);
    frame.scale = chartScale(highest);
    frame.font = QFont("Arial");
    frame.font.setPixelSize(qMax(9, size.height() / 34));
    p_painter->setFont(frame.font);

    for (int i = 0; i <= 5; i++)
    {
        const int y = frame.plot.bottom() - frame.plot.height() * i / 5;
        const int grey = (i == 0) ? 120 : 230;
        p_painter->setPen(QPen(QColor(grey, grey, grey), 1));
        p_painter->drawLine(frame.plot.left(), y, frame.plot.right(), y);
        p_painter->setPen(QColor(90, 90, 90));
        p_painter->drawText(QRect(0, y - frame.font.pixelSize(), frame.plot.left() - 6, frame.font.pixelSize() * 2),
                            Qt::AlignRight | Qt::AlignVCenter, axisLabel(frame.scale * i / 5, frame.scale));
    }
    return frame;
}

//-----------------------------------------------------------------------------
// The legend under the plot: a swatch and a name per series.
static void drawLegend(QPainter* p_painter, const ChartFrame& frame, const QList<QPair<QColor, QString>>& series)
{
    const int y = frame.plot.bottom() + frame.font.pixelSize() * 4;
    int x = frame.plot.left();
    for (const QPair<QColor, QString>& entry : series)
    {
        p_painter->fillRect(QRect(x, y, frame.font.pixelSize(), frame.font.pixelSize()), entry.first);
        p_painter->setPen(QColor(60, 60, 60));
        const int width = p_painter->fontMetrics().horizontalAdvance(entry.second);
        p_painter->drawText(QRect(x + frame.font.pixelSize() * 3 / 2, y, width, frame.font.pixelSize() * 3 / 2),
                            Qt::AlignLeft | Qt::AlignVCenter, entry.second);
        x += frame.font.pixelSize() * 3 + width;
    }
}

//-----------------------------------------------------------------------------
// The years along the foot of a chart, the one still running with the months
// it holds under its name.
static void drawYearLabels(QPainter* p_painter, const ChartFrame& frame, const CompanyReport& report)
{
    const int count = qMax(1, (int)report.years.size());
    const int slot = frame.plot.width() / count;
    p_painter->setPen(QColor(60, 60, 60));
    for (int i = 0; i < report.years.size(); i++)
    {
        QString label = report.years.at(i).name;
        if (report.running && i == report.years.size() - 1)
        {
            label += "\n" + report.elapsedMonths();
        }
        p_painter->drawText(QRect(frame.plot.left() + slot * i, frame.plot.bottom() + 4, slot, frame.font.pixelSize() * 3),
                            Qt::AlignHCenter | Qt::AlignTop, label);
    }
}

//-----------------------------------------------------------------------------
// A bar, and what it comes to in tkr above it. Nothing is drawn for nothing.
static void drawBar(QPainter* p_painter, const ChartFrame& frame, const QRect& slot, int width, long long value,
                    const QColor& colour, bool labelled)
{
    if (value <= 0)
    {
        return;
    }
    const int height = (int)(frame.plot.height() * value / frame.scale);
    const QRect bar(slot.center().x() - width / 2, frame.plot.bottom() - height, width, height);
    p_painter->fillRect(bar, colour);

    if (labelled)
    {
        QFont font = frame.font;
        font.setPixelSize(frame.font.pixelSize() * 9 / 10);
        p_painter->setFont(font);
        p_painter->setPen(QColor(40, 40, 40));
        p_painter->drawText(QRect(slot.left(), bar.top() - font.pixelSize() * 2, slot.width(), font.pixelSize() * 2),
                            Qt::AlignHCenter | Qt::AlignBottom, CompanyReportPrinter::formatThousands(value));
        p_painter->setFont(frame.font);
    }
}

//-----------------------------------------------------------------------------
// What a chart is drawn on. Nothing: the page shows through. A white ground
// goes into the PDF compressed along with the bars and comes out a shade off
// white, a grey box round every chart.
static QImage chartImage(const QSize& size)
{
    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    return image;
}

//-----------------------------------------------------------------------------
// What the chart draws for a year or a month, and what to call it.
static long long valueOf(const CompanyReportFigures& figures, CompanyReportPrinter::Series series)
{
    return series == CompanyReportPrinter::Invoiced ? figures.invoiced : figures.salary_cost();
}

static QString nameOf(CompanyReportPrinter::Series series)
{
    return series == CompanyReportPrinter::Invoiced
        ? QString("Fakturerat (exkl. moms)")
        : QString("Lönekostnad (bruttolön, arbetsgivaravgifter och skattefria ersättningar)");
}

//-----------------------------------------------------------------------------
QImage CompanyReportPrinter::yearChart(const CompanyReport& report, const QSize& size, Series series)
{
    QImage image = chartImage(size);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    long long highest = 0;
    for (const CompanyReportYear& year : report.years)
    {
        highest = qMax(highest, valueOf(year, series));
    }
    const ChartFrame frame = drawFrame(&painter, size, highest);

    const int count = qMax(1, (int)report.years.size());
    const int slot = frame.plot.width() / count;
    const QColor colour = (series == Invoiced) ? invoicedColour() : salaryColour();
    for (int i = 0; i < report.years.size(); i++)
    {
        const QRect area(frame.plot.left() + slot * i, frame.plot.top(), slot, frame.plot.height());
        drawBar(&painter, frame, area, qMax(3, qMin(slot / 3, size.width() / 12)), valueOf(report.years.at(i), series), colour, true);
    }
    drawYearLabels(&painter, frame, report);

    drawLegend(&painter, frame, { qMakePair(colour, nameOf(series)) });
    painter.end();
    return image;
}

//-----------------------------------------------------------------------------
QImage CompanyReportPrinter::comparisonChart(const CompanyReport& report, const QSize& size)
{
    QImage image = chartImage(size);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    long long highest = 0;
    for (const CompanyReportYear& year : report.years)
    {
        highest = qMax(highest, qMax(year.invoiced, year.salary_cost()));
    }
    const ChartFrame frame = drawFrame(&painter, size, highest);

    const int count = qMax(1, (int)report.years.size());
    const int slot = frame.plot.width() / count;
    const int bar = qMax(3, qMin(slot / 3, size.width() / 16));
    for (int i = 0; i < report.years.size(); i++)
    {
        const CompanyReportYear& year = report.years.at(i);
        // Each bar has half the slot to itself, so its label stays over it.
        const QRect left(frame.plot.left() + slot * i + slot / 2 - bar - 1, frame.plot.top(), bar, frame.plot.height());
        const QRect right(frame.plot.left() + slot * i + slot / 2 + 1, frame.plot.top(), bar, frame.plot.height());
        drawBar(&painter, frame, left, bar, year.invoiced, invoicedColour(), false);
        drawBar(&painter, frame, right, bar, year.salary_cost(), salaryColour(), false);
    }
    drawYearLabels(&painter, frame, report);

    drawLegend(&painter, frame, { qMakePair(invoicedColour(), QString("Fakturerat (exkl. moms)")),
                                  qMakePair(salaryColour(), QString("Lönekostnad")) });
    painter.end();
    return image;
}

//-----------------------------------------------------------------------------
QImage CompanyReportPrinter::monthChart(const CompanyReport& report, const QSize& size, Series series)
{
    QImage image = chartImage(size);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    // The last year and the one before it, a pair of bars a month: with a bar
    // for every year the months turn into a comb nobody reads, and what is
    // asked of a month is how it went against the same month last year.
    QList<const CompanyReportYear*> years;
    for (int i = qMax(0, (int)report.years.size() - 2); i < report.years.size(); i++)
    {
        years.append(&report.years.at(i));
    }
    const QList<QColor> colours = (years.size() == 2)
        ? QList<QColor>({ previousYearColour(series), series == Invoiced ? invoicedColour() : salaryColour() })
        : QList<QColor>({ series == Invoiced ? invoicedColour() : salaryColour() });

    long long highest = 0;
    for (const CompanyReportYear* p_year : years)
    {
        for (const CompanyReportMonth& month : p_year->months)
        {
            highest = qMax(highest, valueOf(month, series));
        }
    }
    const ChartFrame frame = drawFrame(&painter, size, highest);

    const QStringList names = MONTH_NAMES;
    const int slot = frame.plot.width() / 12;
    const int bar = qMax(2, (slot - slot / 3) / qMax(1, (int)years.size()));
    for (int m = 0; m < 12; m++)
    {
        const int left = frame.plot.left() + slot * m + (slot - bar * years.size()) / 2;
        for (int y = 0; y < years.size(); y++)
        {
            if (m >= years.at(y)->months.size())
            {
                continue;
            }
            const long long value = valueOf(years.at(y)->months.at(m), series);
            if (value <= 0)
            {
                continue;
            }
            const int height = (int)(frame.plot.height() * value / frame.scale);
            painter.fillRect(QRect(left + bar * y, frame.plot.bottom() - height, qMax(2, bar - 1), height), colours.at(y));
        }

        // The months in the order the fiscal year has them, which is where
        // they are in the report.
        const int calendar_month = report.years.isEmpty() ? m + 1 : report.years.first().months.at(m).month;
        painter.setPen(QColor(60, 60, 60));
        painter.drawText(QRect(frame.plot.left() + slot * m, frame.plot.bottom() + 4, slot, frame.font.pixelSize() * 2),
                         Qt::AlignHCenter | Qt::AlignTop, names.at(calendar_month - 1));
    }

    QList<QPair<QColor, QString>> legend;
    for (int y = 0; y < years.size(); y++)
    {
        legend.append(qMakePair(colours.at(y), years.at(y)->name));
    }
    drawLegend(&painter, frame, legend);
    painter.end();
    return image;
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::create_pdf(const CompanyReport& report, const CompanyInfo& company, QIODevice* device)
{
    CompanyReportPrinter printer;
    printer.render(report, company, device);
}

//-----------------------------------------------------------------------------
// The tables never go below this, however many years there are. It is small,
// but a report with a column per year over a long history has to choose
// between small figures and broken ones, and a broken figure is worse.
static const int TABLE_FONT_MINIMUM = 4;

// How much of a table's width is left unclaimed by its columns. Columns that
// claim all of it leave nothing for the rules between them, and the last
// figure wraps.
static const qreal RESERVE = 2.0;

// How wide a column has to be for `text` not to wrap: its width plus the
// padding on either side, and enough over that a figure as wide as its column
// is not broken by a rounding. A column measured exactly is a column that
// wraps. The text is measured by a document laid out as the report is, since
// QFontMetrics reports widths in other units than the layout then uses.
static qreal columnWidthFor(const QString& text, const QFont& font, qreal padding)
{
    QTextDocument probe;
    probe.setDefaultFont(font);
    QTextOption option;
    option.setWrapMode(QTextOption::NoWrap);
    probe.setDefaultTextOption(option);
    probe.setPlainText(text);
    return probe.idealWidth() - 2.0 * probe.documentMargin() + 2.0 * padding + 4.0;
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::cellText(QTextTable* p_table, int row, int column, const QString& text, const QFont& font,
                                    Qt::Alignment alignment, const QColor& colour)
{
    QTextCharFormat format;
    format.setFont(font);
    format.setForeground(colour);
    p_table->cellAt(row, column).firstCursorPosition().insertText(text, format);
    if (alignment != Qt::AlignLeft)
    {
        printer_util_table_cell_align_text(p_table, row, column, alignment);
    }
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addSpace(QTextCursor& cursor, const QString& lines)
{
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    format.setFont(m_font_super);
    cursor.insertText(lines, format);
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addTable(const QStringList& headings, const QList<Row>& rows, QTextCursor& cursor)
{
    const int columns = headings.size();
    const qreal table_width = m_table_format_default.width().rawValue();
    const qreal padding = m_table_format_default.cellPadding();
    const qreal reserve = RESERVE / 100.0 * table_width;
    // A name may wrap over a few lines; a figure may not wrap at all. So when
    // the figures crowd the page it is the name column that gives way, down
    // to this much of the width and no further.
    const qreal name_minimum = 0.15 * table_width;

    // Each figure column is as wide as the widest figure in it, and a heading
    // may wrap between its words. The size is the largest at which they all
    // fit beside the smallest name column.
    QFont font = m_font_table;
    QFont font_bold = m_font_table_bold;
    QList<qreal> figures;
    qreal name = 0.0;
    auto needed = [&figures](void) {
        qreal sum = 0.0;
        for (qreal width : figures)
        {
            sum += width;
        }
        return sum;
    };
    // A heading is kept on one line where there is room for that, and
    // wrapped between its words before the type is made any smaller.
    for (int attempt = 0; ; attempt++)
    {
        const int points = m_font_table.pointSize() - attempt / 2;
        const bool whole_headings = (attempt % 2 == 0);
        font.setPointSize(points);
        font_bold.setPointSize(points);

        figures = QList<qreal>(columns - 1, 0.0);
        for (int c = 1; c < columns; c++)
        {
            const QStringList parts = whole_headings ? QStringList({ headings.at(c) })
                                                     : headings.at(c).split(' ', Qt::SkipEmptyParts);
            for (const QString& part : parts)
            {
                figures[c - 1] = qMax(figures.at(c - 1), columnWidthFor(part, font_bold, padding));
            }
        }
        name = columnWidthFor(headings.at(0), font_bold, padding);
        for (const Row& row : rows)
        {
            const QFont& f = (row.kind == Body || row.kind == Indented) ? font : font_bold;
            for (int c = 1; c < row.cells.size() && c < columns; c++)
            {
                figures[c - 1] = qMax(figures.at(c - 1), columnWidthFor(row.cells.at(c), f, padding));
            }
            if (!row.cells.isEmpty())
            {
                name = qMax(name, columnWidthFor(row.cells.at(0), f, padding) + (row.kind == Indented ? 2.0 * padding : 0.0));
            }
        }

        if (needed() <= table_width - name_minimum - reserve || (points <= TABLE_FONT_MINIMUM && !whole_headings))
        {
            break;
        }
    }

    // Even at the smallest size there can be years enough not to fit. The
    // figures then give up their slack rather than the names disappearing.
    if (needed() > table_width - name_minimum - reserve)
    {
        const qreal shrink = (table_width - name_minimum - reserve) / needed();
        for (qreal& width : figures)
        {
            width *= shrink;
        }
    }

    // The name takes what it needs, up to what the figures leave it; what is
    // over is shared out among the figures, so that a table with short names
    // spreads across the page rather than crowding its figures at the right.
    const qreal left = table_width - reserve - needed();
    const qreal name_width = qMin(left, qMax(name, name_minimum));
    for (qreal& width : figures)
    {
        width += (left - name_width) / figures.size();
    }

    QList<QTextLength> lengths;
    lengths.append(QTextLength(QTextLength::PercentageLength, 100.0 * name_width / table_width));
    for (qreal width : figures)
    {
        lengths.append(QTextLength(QTextLength::PercentageLength, 100.0 * width / table_width));
    }
    QTextTableFormat format = m_table_format_default;
    format.setColumnWidthConstraints(lengths);
    // A table that runs over onto the next page repeats its headings there.
    format.setHeaderRowCount(1);

    cursor.movePosition(QTextCursor::End);
    QTextTable* p_table = cursor.insertTable(1 + rows.size(), columns, format);

    auto rule = [p_table, columns](int row, bool top, qreal width, const QColor& colour) {
        for (int c = 0; c < columns; c++)
        {
            QTextTableCell cell = p_table->cellAt(row, c);
            QTextTableCellFormat f = cell.format().toTableCellFormat();
            if (top)
            {
                f.setTopBorder(width);
                f.setTopBorderBrush(colour);
                f.setTopBorderStyle(QTextFrameFormat::BorderStyle_Solid);
            }
            else
            {
                f.setBottomBorder(width);
                f.setBottomBorderBrush(colour);
                f.setBottomBorderStyle(QTextFrameFormat::BorderStyle_Solid);
            }
            cell.setFormat(f);
        }
    };

    for (int c = 0; c < columns; c++)
    {
        QTextTableCell cell = p_table->cellAt(0, c);
        QTextTableCellFormat f = cell.format().toTableCellFormat();
        f.setBackground(HEADING_FILL);
        cell.setFormat(f);
        cellText(p_table, 0, c, headings.at(c), font_bold, c == 0 ? Qt::AlignLeft : Qt::AlignRight);
    }
    rule(0, false, 0.75, RULE_STRONG);

    for (int r = 0; r < rows.size(); r++)
    {
        const Row& row = rows.at(r);
        const int at = r + 1;
        const QFont& f = (row.kind == Body || row.kind == Indented) ? font : font_bold;
        for (int c = 0; c < row.cells.size() && c < columns; c++)
        {
            cellText(p_table, at, c, row.cells.at(c), f, c == 0 ? Qt::AlignLeft : Qt::AlignRight);
        }
        if (row.kind == Indented)
        {
            QTextCursor c = p_table->cellAt(at, 0).firstCursorPosition();
            QTextBlockFormat block = c.blockFormat();
            block.setLeftMargin(2.0 * padding);
            c.setBlockFormat(block);
        }

        if (row.kind == Total)
        {
            rule(at, true, 0.75, RULE_STRONG);
        }
        else if (r + 1 < rows.size() && rows.at(r + 1).kind != Total)
        {
            rule(at, false, 0.25, RULE_LIGHT);
        }
    }
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::render(const CompanyReport& report, const CompanyInfo& company, QIODevice* device)
{
    QPdfWriter writer(device);
    writer.setPageSize(QPageSize::A4Small);
    // The side margins of the invoices, so the report lines up with them.
    writer.setPageMargins(QMarginsF(20, 25, 20, 15));
    writer.setResolution(300);
    const QString period = periodOf(report);
    const QString title = period.isEmpty() ? QString(TITLE) : QString("%1 %2").arg(TITLE, period);

    // What the reader sees in the document, and what a file manager or a PDF
    // reader shows about it. Qt writes the creation date itself.
    writer.setTitle(title);
    writer.setCreator(QString("ordning v%1").arg(BUILD_VERSION));

    QPainter painter(&writer);
    printer_util_painter_scale(&painter, &writer);

    const QRectF page = writer.pageLayout().paintRectPoints();
    const int available_width = page.width();

    QTextDocument doc;
    doc.setTextWidth(available_width + 2 * doc.documentMargin());
    m_table_format_default.setWidth(QTextLength(QTextLength::FixedLength, available_width));

    QTextCursor cursor(&doc);
    addHeader(report, company, cursor);

    const int chart_height = available_width * 2 / 5;
    const QSize chart_size(available_width * 4, chart_height * 4);

    // Where each chapter begins, whether it always starts a page of its own,
    // and how much of what follows it has to have under it on its page.
    struct Chapter
    {
        int position;
        bool new_page;
        qreal keep;
    };
    // A heading opening with a chart keeps the chart and its caption with it.
    const qreal keep_chart = chart_height + 50;
    const qreal keep_table = 110;
    QList<Chapter> chapters;

    // What the reader is after comes first: the latest year against the one
    // before, and the years side by side.
    chapters.append({ addChapter("Sammanfattning", cursor), false, 0 });
    addKeyFigures(report, cursor);
    addHeading("Flerårsöversikt", cursor);
    addOverview(report, cursor);
    // A lower chart than the chapters have, so that the summary is the first
    // page and all of it.
    const int summary_height = available_width * 3 / 10;
    addChart(comparisonChart(report, QSize(available_width * 4, summary_height * 4)),
             "Fakturerat mot lönekostnad per räkenskapsår, tkr", cursor, available_width, summary_height);

    // Then what it rests on, beginning on a page of its own.
    chapters.append({ addChapter("Fakturering", cursor), true, keep_chart });
    addChart(yearChart(report, chart_size, Invoiced), "Fakturerat per räkenskapsår, tkr exklusive moms", cursor, available_width, chart_height);
    addChart(monthChart(report, chart_size, Invoiced), "Fakturerat per månad, i år mot föregående år", cursor, available_width, chart_height);

    chapters.append({ addChapter("Kunder och uppdrag", cursor), false, keep_table });
    addSeries("Fakturerat per kund, tkr exklusive moms", "Kund", report.customers, report, false, cursor);
    addSeries("Fakturerat per uppdrag, tkr exklusive moms", "Uppdrag", report.assignments, report, false, cursor);

    if (!report.consultants.isEmpty())
    {
        chapters.append({ addChapter("Konsulter och timmar", cursor), false, keep_table });
        addSeries("Debiterade timmar per konsult", "Konsult", report.consultants, report, true, cursor);
    }

    // A company that has never paid a salary has no chapter of charts of
    // nothing; the summary says what the salaries came to either way.
    if (!report.employees.isEmpty())
    {
        chapters.append({ addChapter("Lönekostnader", cursor), false, keep_chart });
        addChart(yearChart(report, chart_size, SalaryCost), "Lönekostnad per räkenskapsår, tkr", cursor, available_width, chart_height);
        addChart(monthChart(report, chart_size, SalaryCost), "Lönekostnad per månad, i år mot föregående år", cursor, available_width, chart_height);
        addHeading("Lönekostnad per räkenskapsår, tkr", cursor);
        addSalaryYears(report, cursor);
        addSeries("Bruttolön per anställd, tkr", "Anställd", report.employees, report, false, cursor);
    }

    chapters.append({ addChapter("Underlag och principer", cursor), false, keep_table });
    addPrinciples(report, cursor);

    // Every page but the first carries the company and the title across its
    // top, so that a page on its own can still be placed; the page number and
    // the build line go at the foot of every page. Both are drawn on the page
    // rather than flowing with the text, so they sit where they sit whatever
    // the page holds: the page number in black, since it is for the reader,
    // and the build line in grey below it, since it is not.
    QFont footer_font("Arial");
    footer_font.setPixelSize(6);
    QFont page_font("Arial");
    page_font.setPixelSize(8);
    QFont running_font("Arial");
    running_font.setPixelSize(7);
    const int header_height = 22;
    const int footer_height = 24;   // two lines, as on an invoice
    const int page_height = 16;
    const qreal body_height = page.height() - header_height - footer_height - page_height;
    const qreal body_width = available_width + 2 * doc.documentMargin();

    doc.setPageSize(QSizeF(body_width, body_height));

    // A chapter heading close to the foot of a page would be left there on
    // its own, so it is moved to the next page, and a chapter that asks for a
    // page of its own is moved whatever it sits next to. A heading already at
    // the top of a page, with nothing above it but empty lines, is left alone:
    // breaking there would leave the page before it empty. A chart whose
    // caption falls on the next page goes there with it, since a caption on
    // its own at the top of a page says nothing.
    //
    // Each break moves everything after it, so they are put in one at a time
    // from the top, and the document is laid out again after each: a change
    // to a block's format lays it out only part of the way, and what follows
    // is left with no position to be measured by and nothing to draw.
    struct Keep
    {
        int position;
        bool new_page;
        qreal keep;     // how much has to follow a heading on its page
        bool chart;
    };
    QList<Keep> keeps;
    for (const Chapter& chapter : chapters)
    {
        keeps.append({ chapter.position, chapter.new_page, chapter.keep, false });
    }
    for (int position : m_charts)
    {
        keeps.append({ position, false, 0, true });
    }
    // A table's heading, its note, the row of its headings and a line or two.
    for (int position : m_headings)
    {
        keeps.append({ position, false, 70, false });
    }
    std::sort(keeps.begin(), keeps.end(), [](const Keep& a, const Keep& b) { return a.position < b.position; });

    auto pageOf = [&doc, body_height](const QTextBlock& block) {
        return (int)(doc.documentLayout()->blockBoundingRect(block).top() / body_height);
    };
    auto needsBreak = [&](const Keep& keep) {
        const QTextBlock block = doc.findBlock(keep.position);
        if (block.blockFormat().pageBreakPolicy() == QTextFormat::PageBreak_AlwaysBefore)
        {
            return false;
        }
        if (keep.chart)
        {
            // Where the caption ends, since a caption that begins on the
            // chart's page can still end on the next. A page keeps the
            // document's margin clear at its foot, and a line that reaches
            // into it is put on the next page when it is drawn.
            const qreal caption_bottom = doc.documentLayout()->blockBoundingRect(block.next()).bottom();
            return pageOf(block) != (int)((caption_bottom + doc.documentMargin()) / body_height);
        }

        const qreal top = doc.documentLayout()->blockBoundingRect(block).top();
        const qreal page_top = body_height * (int)(top / body_height);
        for (QTextBlock above = block.previous(); above.isValid(); above = above.previous())
        {
            if (doc.documentLayout()->blockBoundingRect(above).bottom() <= page_top)
            {
                break;
            }
            if (!above.text().trimmed().isEmpty())
            {
                return keep.new_page || top - page_top + keep.keep > body_height;
            }
        }
        return false;
    };

    for (int step = 0; step < keeps.size(); step++)
    {
        const auto found = std::find_if(keeps.begin(), keeps.end(), needsBreak);
        if (found == keeps.end())
        {
            break;
        }
        QTextCursor c(&doc);
        c.setPosition(found->position);
        QTextBlockFormat format = c.blockFormat();
        format.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
        c.setBlockFormat(format);
        doc.markContentsDirty(0, doc.characterCount());
    }

    const int pages = qMax(1, doc.pageCount());
    for (int i = 0; i < pages; i++)
    {
        // The first page has no line across its top, so the report's own
        // header starts where the logo does, at the top of the page, and the
        // room kept for that line is left over at its foot instead.
        painter.save();
        painter.translate(0, (i == 0 ? 0 : header_height) - body_height * i);
        painter.setClipRect(QRectF(0, body_height * i, body_width, body_height));
        doc.drawContents(&painter, QRectF(0, body_height * i, body_width, body_height));
        painter.restore();

        if (i == 0)
        {
            // The logo sits where it does on an invoice, on the first page.
            printer_util_insert_svg_string(&painter, QRectF(5.0, 3.0, 130.0, 35.0), company.m_logo, false);
        }
        else
        {
            painter.setFont(running_font);
            painter.setPen(NOTE_GREY);
            const QRectF band(doc.documentMargin(), 0, available_width, header_height - 8);
            painter.drawText(band, Qt::AlignLeft | Qt::AlignBottom, company.m_name);
            painter.drawText(band, Qt::AlignRight | Qt::AlignBottom, title);
            painter.setPen(QPen(RULE_LIGHT, 0.5));
            painter.drawLine(QPointF(doc.documentMargin(), header_height - 5),
                             QPointF(doc.documentMargin() + available_width, header_height - 5));
        }

        painter.setFont(page_font);
        painter.setPen(Qt::black);
        painter.drawText(QRectF(0, page.height() - footer_height - page_height, available_width, page_height),
                         Qt::AlignRight | Qt::AlignBottom, QString("Sida %1 (%2)").arg(i + 1).arg(pages));

        // The same build metadata as on an invoice: the repository on the
        // left, the version and the build it came from on the right.
        painter.setFont(footer_font);
        painter.setPen(QColor(186, 186, 186));
        const QRectF footer(0, page.height() - footer_height, available_width, footer_height);
        painter.drawText(footer, Qt::AlignLeft | Qt::AlignBottom, REPOSITORY_URL);
        painter.drawText(footer, Qt::AlignRight | Qt::AlignBottom,
                         QString("ordning v%1 (%2)\n%3").arg(BUILD_VERSION, BUILD_DATE, BUILD_HASH));

        if (i + 1 < pages)
        {
            writer.newPage();
            printer_util_painter_scale(&painter, &writer);
        }
    }

    painter.end();
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addHeader(const CompanyReport& report, const CompanyInfo& company, QTextCursor& cursor)
{
    // The logo is drawn where it is on an invoice, over the left column; the
    // title and who and what it is about stand to the right of it, where a
    // title of any length has the room it needs.
    QTextTableFormat format = m_table_format_default;
    format.setColumnWidthConstraints({ QTextLength(QTextLength::PercentageLength, 35.0),
                                       QTextLength(QTextLength::PercentageLength, 65.0) });
    QTextTable* p_table = cursor.insertTable(5, 2, format);
    p_table->mergeCells(0, 0, 5, 1);

    cellText(p_table, 0, 1, QString(TITLE).toUpper(), m_font_title, Qt::AlignRight, ACCENT);
    cellText(p_table, 1, 1, company.m_name, m_font_normal_bold, Qt::AlignRight);
    if (!company.m_org_number.isEmpty())
    {
        cellText(p_table, 2, 1, QString("Org.nr %1").arg(company.m_org_number), m_font_normal, Qt::AlignRight);
    }
    cellText(p_table, 3, 1, QString("Räkenskapsår %1").arg(periodOf(report)), m_font_normal, Qt::AlignRight);
    // A report is of the day it was made: the same year looks different a
    // month later.
    cellText(p_table, 4, 1, QString("Genererad %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm")),
             m_font_note, Qt::AlignRight, NOTE_GREY);
}

//-----------------------------------------------------------------------------
// Every part of the report puts its space before itself rather than after, so
// that the document ends with the last of it. A trailing empty line would run
// over onto a page of its own, and that page would hold nothing but its
// footer.
int CompanyReportPrinter::addChapter(const QString& title, QTextCursor& cursor)
{
    // The space above a chapter is the heading's own margin rather than empty
    // lines before it: lines that fall at the foot of a page run over onto the
    // next, and a heading moved to a new page after them leaves that page
    // holding nothing but them.
    cursor.movePosition(QTextCursor::End);
    QTextBlockFormat block;
    block.setTopMargin(CHAPTER_SPACE);
    cursor.insertBlock(block);

    const int position = cursor.position();
    QTextCharFormat heading;
    heading.setFont(m_font_chapter);
    heading.setForeground(ACCENT);
    cursor.insertText(title, heading);
    return position;
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addHeading(const QString& title, QTextCursor& cursor)
{
    addSpace(cursor, " \n");
    cursor.movePosition(QTextCursor::End);
    m_headings.append(cursor.position());
    QTextCharFormat format;
    format.setFont(m_font_heading);
    cursor.insertText(title + "\n", format);
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addNote(const QString& text, QTextCursor& cursor, Qt::Alignment alignment)
{
    // On a line of its own: the empty one left after a heading or a table,
    // or a new one after a chart.
    cursor.movePosition(QTextCursor::End);
    QTextBlockFormat block;
    block.setAlignment(alignment);
    block.setTopMargin(2);
    if (cursor.block().text().isEmpty())
    {
        cursor.setBlockFormat(block);
    }
    else
    {
        cursor.insertBlock(block);
    }

    QTextCharFormat format;
    format.setFont(m_font_note);
    format.setForeground(NOTE_GREY);
    cursor.insertText(text, format);

    // Back to the left for whatever comes next.
    cursor.insertBlock(QTextBlockFormat());
    cursor.setBlockCharFormat(QTextCharFormat());
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addChart(const QImage& image, const QString& caption, QTextCursor& cursor, int width, int height)
{
    addSpace(cursor, " \n");
    cursor.movePosition(QTextCursor::End);
    // Each chart needs a name of its own, or the second would show the first.
    const QString name = QString("chart%1").arg(cursor.position());
    cursor.document()->addResource(QTextDocument::ImageResource, QUrl(name), image);

    QTextImageFormat format;
    format.setName(name);
    format.setWidth(width);
    format.setHeight(height);
    m_charts.append(cursor.position());
    cursor.insertImage(format);

    // Under the chart, saying what it shows: the axes carry years and months,
    // not what is being counted along them.
    addNote(caption, cursor, Qt::AlignHCenter);
}

//-----------------------------------------------------------------------------
// One key figure of one period: what it reads as, and what a change is worked
// out from. A share of nothing is no figure at all.
struct KeyValue
{
    QString text;
    long long value = 0;
    bool defined = true;
};

static QList<KeyValue> keyValues(const CompanyReportFigures& f, int largest_share)
{
    const KeyValue none = { NONE, 0, false };
    const long long salary_share = f.invoiced > 0 ? divideRounded(f.salary_cost() * 10000, f.invoiced) : 0;
    const long long per_hour = f.hours > 0 ? divideRounded(f.invoiced, f.hours) : 0;
    return {
        { CompanyReportPrinter::formatThousands(f.invoiced), f.invoiced },
        { CompanyReportPrinter::formatThousands(f.salary_cost()), f.salary_cost() },
        { CompanyReportPrinter::formatThousands(f.after_salaries()), f.after_salaries() },
        f.invoiced > 0 ? KeyValue{ formatPercent(salary_share), salary_share } : none,
        { formatHours(f.hours), f.hours },
        // Öre over hundredths of an hour is kronor an hour.
        f.hours > 0 ? KeyValue{ CompanyReportPrinter::formatWhole(per_hour), per_hour } : none,
        { CompanyReportPrinter::formatWhole(f.invoices), f.invoices },
        { CompanyReportPrinter::formatWhole(f.credit_invoices), f.credit_invoices },
        f.invoiced > 0 ? KeyValue{ formatPercent(largest_share), largest_share } : none,
    };
}

void CompanyReportPrinter::addKeyFigures(const CompanyReport& report, QTextCursor& cursor)
{
    if (report.years.isEmpty())
    {
        return;
    }

    enum Change { Percent, Points, Nothing };
    const QStringList labels = {
        "Fakturerat, tkr",
        "Lönekostnad, tkr",
        "Kvar efter löner, tkr",
        "Lönekostnad av fakturerat",
        "Debiterade timmar",
        "Fakturerat per debiterad timme, kr",
        "Antal fakturor",
        "Antal kreditfakturor",
        "Största kundens andel av fakturerat",
    };
    const QList<Change> changes = { Percent, Percent, Percent, Points, Percent, Percent, Percent, Nothing, Points };

    // The last year so far, against the same months of the one before it,
    // and that year whole beside them when the last one is still running.
    const int last = report.years.size() - 1;
    const int months = report.months_complete;
    const CompanyReportYear& year = report.years.at(last);
    const QList<KeyValue> current = keyValues(year.firstMonths(months),
                                              report.largestCustomerShare(year.year, report.running));

    QStringList headings = { "Nyckeltal", yearHeading(report, last) };
    QList<KeyValue> previous;
    QList<KeyValue> previous_whole;
    if (last >= 1)
    {
        const CompanyReportYear& before = report.years.at(last - 1);
        previous = keyValues(before.firstMonths(months), report.largestCustomerShare(before.year, report.running));
        headings.append(report.running ? before.name + NBSP + report.elapsedMonths() : before.name);
        headings.append("Förändring");
        if (report.running)
        {
            previous_whole = keyValues(before, report.largestCustomerShare(before.year, false));
            headings.append(before.name + NBSP + "helår");
        }
    }

    QList<Row> rows;
    for (int i = 0; i < labels.size(); i++)
    {
        Row row;
        row.kind = (i == 2) ? Emphasis : Body;
        row.cells = QStringList({ labels.at(i), current.at(i).text });
        if (!previous.isEmpty())
        {
            row.cells.append(previous.at(i).text);
            QString change = NONE;
            if (current.at(i).defined && previous.at(i).defined)
            {
                if (changes.at(i) == Percent)
                {
                    change = formatChange(current.at(i).value, previous.at(i).value);
                }
                else if (changes.at(i) == Points)
                {
                    change = formatPoints(current.at(i).value, previous.at(i).value);
                }
            }
            row.cells.append(change);
        }
        if (!previous_whole.isEmpty())
        {
            row.cells.append(previous_whole.at(i).text);
        }
        rows.append(row);
    }

    addTable(headings, rows, cursor);

    if (last >= 1 && report.running)
    {
        addNote(QString("%1 pågår och jämförs med samma månader %2.").arg(year.name, report.years.at(last - 1).name), cursor);
    }
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addOverview(const CompanyReport& report, QTextCursor& cursor)
{
    const QStringList headings = { "Räkenskapsår", "Fakturerat", "Lönekostnad", "Kvar efter löner",
                                   "Lönekostnad av fakturerat", "Debiterade timmar", "Kr per timme" };
    QList<Row> rows;
    for (int i = 0; i < report.years.size(); i++)
    {
        const CompanyReportYear& year = report.years.at(i);
        Row row;
        row.cells = QStringList({
            yearHeading(report, i),
            formatThousands(year.invoiced),
            formatThousands(year.salary_cost()),
            formatThousands(year.after_salaries()),
            year.invoiced > 0 ? formatPercent(divideRounded(year.salary_cost() * 10000, year.invoiced)) : NONE,
            formatHours(year.hours),
            year.hours > 0 ? formatWhole(divideRounded(year.invoiced, year.hours)) : NONE,
        });
        rows.append(row);
    }
    addTable(headings, rows, cursor);
    addNote("Belopp i tkr, fakturerat exklusive moms.", cursor);
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addSalaryYears(const CompanyReport& report, QTextCursor& cursor)
{
    const QStringList headings = { "Räkenskapsår", "Bruttolön", "Arbetsgivaravgifter",
                                   "Skattefria ersättningar", "Lönekostnad" };
    QList<Row> rows;
    for (int i = 0; i < report.years.size(); i++)
    {
        const CompanyReportYear& year = report.years.at(i);
        Row row;
        row.cells = QStringList({
            yearHeading(report, i),
            formatThousands(year.salary_gross),
            formatThousands(year.employer_fee),
            year.tax_free == 0 ? NONE : formatThousands(year.tax_free),
            formatThousands(year.salary_cost()),
        });
        rows.append(row);
    }
    addTable(headings, rows, cursor);
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addSeries(const QString& heading, const QString& first_column, const QList<CompanyReportSeries>& series,
                                     const CompanyReport& report, bool hours, QTextCursor& cursor)
{
    if (series.isEmpty())
    {
        return;
    }
    addHeading(heading, cursor);

    auto format = [hours](long long value) {
        return hours ? formatHours(value) : formatThousands(value);
    };

    // One column per year, the total of them all, and how the last year went
    // against the one before, over the same months.
    const QList<int> years = report.yearNumbers();
    const bool compared = years.size() >= 2;
    QStringList headings = { first_column };
    for (int i = 0; i < years.size(); i++)
    {
        headings.append(yearHeading(report, i));
    }
    headings.append("Totalt");
    if (compared)
    {
        headings.append("Förändring");
    }

    auto cells = [&](const QString& name, const QMap<int, long long>& per_year, const QMap<int, long long>& to_date) {
        QStringList line = { name };
        long long total = 0;
        for (int year : years)
        {
            const long long value = per_year.value(year);
            total += value;
            line.append(value == 0 ? NONE : format(value));
        }
        line.append(format(total));
        if (compared)
        {
            line.append(formatChange(to_date.value(years.last()), to_date.value(years.at(years.size() - 2))));
        }
        return line;
    };

    // A customer whose uppdrag are listed heads them with a row of its own,
    // so the name is not repeated on every line of it.
    QList<Row> rows;
    QString current;
    QMap<int, long long> sum;
    QMap<int, long long> sum_to_date;
    for (const CompanyReportSeries& entry : series)
    {
        if (!entry.group.isEmpty() && entry.group != current)
        {
            current = entry.group;
            rows.append({ QStringList({ current }), Group });
        }
        rows.append({ cells(entry.name, entry.per_year, entry.to_date), entry.group.isEmpty() ? Body : Indented });
        for (int year : years)
        {
            sum[year] += entry.per_year.value(year);
            sum_to_date[year] += entry.to_date.value(year);
        }
    }
    rows.append({ cells("Summa", sum, sum_to_date), Total });

    // What the change is worked out over goes under the heading rather than
    // under the table, where it would be left on a page of its own whenever
    // the table ends at the foot of one.
    if (compared)
    {
        const QString last = report.years.last().name;
        const QString before = report.years.at(report.years.size() - 2).name;
        addNote(report.running
                    ? QString("Förändring: %1 %2 mot samma månader %3.").arg(last, report.elapsedMonths(), before)
                    : QString("Förändring: %1 mot %2.").arg(last, before),
                cursor);
    }
    addTable(headings, rows, cursor);
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addPrinciples(const CompanyReport& report, QTextCursor& cursor)
{
    const QStringList months = MONTH_NAMES_LONG;
    QStringList points = {
        "Belopp anges i tusentals kronor (tkr) och avrundas var för sig, så en summa kan skilja sig "
        "någon tkr från summan av raderna ovanför den.",
        "Fakturerat är exklusive moms och räknas efter fakturadatum. En kreditfaktura dras av i den "
        "månad den är daterad.",
        "Lönekostnad är bruttolön, arbetsgivaravgifter och skattefria ersättningar, räknat efter "
        "utbetalningsdag. Bruttolön per anställd är bruttolönen ensam.",
        "Kvar efter löner är fakturerat minus lönekostnad. Det är inget resultat: övriga kostnader, "
        "som lokaler, utrustning, resor och bokslutsposter, finns inte i underlaget.",
        "Debiterade timmar är alla timmar på fakturorna, övertid, kvalificerad tid och restid "
        "inräknade. Fakturerat per debiterad timme är hela det fakturerade beloppet delat med dem.",
        report.start_month == 1
            ? QString("Räkenskapsåret är kalenderåret.")
            : QString("Räkenskapsåret börjar i %1.").arg(months.at(report.start_month - 1)),
    };
    if (report.running && !report.years.isEmpty())
    {
        points.append(QString("Räkenskapsåret %1 pågår. Rapporten tar med de månader som är slut, "
                              "här %2, och jämför dem med samma månader året innan. En månad som "
                              "pågår kommer med när den är slut.")
                          .arg(report.years.last().name, report.elapsedMonths()));
    }

    addSpace(cursor, " \n");
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat text;
    text.setFont(m_font_table);
    // Each point hangs from its bullet, so that a point running over a line
    // lines up under its own text.
    QTextBlockFormat block;
    block.setLeftMargin(8);
    block.setTextIndent(-8);
    block.setBottomMargin(3);
    for (const QString& point : points)
    {
        cursor.insertBlock(block, text);
        cursor.insertText(QString::fromUtf8("•\u2002") + point, text);
    }
}
