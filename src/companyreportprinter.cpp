#include <QAbstractTextDocumentLayout>
#include <QDateTime>
#include <QTextBlock>
#include <QPainter>
#include <QSet>
#include <QTextDocument>
#include <QPdfWriter>
#include <QTextDocument>
#include <QUrl>

#include "util/printerutilities.h"
#include "version.h"

#include "companyreportprinter.h"

#define MONTH_NAMES { "Jan", "Feb", "Mar", "Apr", "Maj", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dec" }

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
QColor CompanyReportPrinter::yearColour(int index)
{
    // Enough colours to tell a handful of years apart; after that they come
    // round again, which is still readable since the years are labelled.
    static const QList<QColor> colours = {
        QColor(51, 102, 153), QColor(196, 120, 60), QColor(94, 140, 87),
        QColor(150, 90, 140), QColor(120, 120, 120), QColor(186, 160, 60),
    };
    return colours.at(index % colours.size());
}

//-----------------------------------------------------------------------------
CompanyReportPrinter::CompanyReportPrinter()
{
    m_font_header = QFont("Arial", 14, QFont::Normal, false);
    m_font_chapter = QFont("Arial", 11, QFont::ExtraBold, false);
    m_font_super = QFont("Arial", 5, QFont::Normal, false);
    m_font_normal = QFont("Arial", 8, QFont::Normal, false);
    m_font_normal_bold = QFont("Arial", 8, QFont::ExtraBold, false);
    // The tables carry a figure per year across the page and a customer or an
    // uppdrag in the first column, so they are the one thing that runs out of
    // width. A point smaller than the rest of the report buys that room.
    m_font_caption = QFont("Arial", 6, QFont::Normal, true);
    m_font_table = QFont("Arial", 7, QFont::Normal, false);
    m_font_table_bold = QFont("Arial", 7, QFont::ExtraBold, false);

    m_table_format_default.setBorderCollapse(false);
    m_table_format_default.setCellSpacing(0);
    m_table_format_default.setCellPadding(3);
    m_table_format_default.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    m_table_format_default.setBorder(0.5);
    m_table_format_default.setBorderBrush(QBrush(QColor(245, 245, 245)));
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
                       size.height() - size.height() / 8 - size.height() / 6);
    frame.scale = chartScale(highest);
    frame.font = QFont("Arial");
    frame.font.setPixelSize(qMax(9, size.height() / 34));
    p_painter->setFont(frame.font);

    for (int i = 0; i <= 5; i++)
    {
        const int y = frame.plot.bottom() - frame.plot.height() * i / 5;
        const int grey = (i == 0) ? 120 : 225;
        p_painter->setPen(QPen(QColor(grey, grey, grey), 1));
        p_painter->drawLine(frame.plot.left(), y, frame.plot.right(), y);
        p_painter->setPen(QColor(90, 90, 90));
        p_painter->drawText(QRect(0, y - frame.font.pixelSize(), frame.plot.left() - 6, frame.font.pixelSize() * 2),
                            Qt::AlignRight | Qt::AlignVCenter,
                            printer_util_format_money(frame.scale * i / 5).split(',').first());
    }
    return frame;
}

//-----------------------------------------------------------------------------
// The legend under the plot: a swatch and a name per series.
static void drawLegend(QPainter* p_painter, const ChartFrame& frame, const QList<QPair<QColor, QString>>& series)
{
    const int y = frame.plot.bottom() + frame.font.pixelSize() * 3;
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
// What the chart draws for a year, and what to call it.
static long long valueOf(const CompanyReportYear& year, CompanyReportPrinter::Series series)
{
    return series == CompanyReportPrinter::Invoiced ? year.invoiced : year.salary_cost();
}

static long long valueOf(const CompanyReportMonth& month, CompanyReportPrinter::Series series)
{
    return series == CompanyReportPrinter::Invoiced ? month.invoiced : month.salary_cost();
}

static QString nameOf(CompanyReportPrinter::Series series)
{
    return series == CompanyReportPrinter::Invoiced
        ? QString("Fakturerat (exkl. moms)")
        : QString("Lönekostnad (bruttolön + arbetsgivaravgift)");
}

//-----------------------------------------------------------------------------
QImage CompanyReportPrinter::yearChart(const CompanyReport& report, const QSize& size, Series series)
{
    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::white);

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
    const int bar = qMax(3, slot / 3);
    const QColor colour = (series == Invoiced) ? invoicedColour() : salaryColour();
    for (int i = 0; i < report.years.size(); i++)
    {
        const long long value = valueOf(report.years.at(i), series);
        const int centre = frame.plot.left() + slot * i + slot / 2;
        if (value > 0)
        {
            const int height = (int)(frame.plot.height() * value / frame.scale);
            painter.fillRect(QRect(centre - bar / 2, frame.plot.bottom() - height, bar, height), colour);
        }

        painter.setPen(QColor(60, 60, 60));
        painter.drawText(QRect(frame.plot.left() + slot * i, frame.plot.bottom() + 4, slot, frame.font.pixelSize() * 2),
                         Qt::AlignHCenter | Qt::AlignTop, report.years.at(i).name);
    }

    drawLegend(&painter, frame, { qMakePair(colour, nameOf(series)) });
    painter.end();
    return image;
}

//-----------------------------------------------------------------------------
QImage CompanyReportPrinter::comparisonChart(const CompanyReport& report, const QSize& size)
{
    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::white);

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
    const int bar = qMax(3, slot / 3);
    for (int i = 0; i < report.years.size(); i++)
    {
        const CompanyReportYear& year = report.years.at(i);
        const int centre = frame.plot.left() + slot * i + slot / 2;

        auto drawBar = [&](long long value, const QColor& colour, int offset) {
            if (value <= 0)
            {
                return;
            }
            const int height = (int)(frame.plot.height() * value / frame.scale);
            painter.fillRect(QRect(centre + offset, frame.plot.bottom() - height, bar, height), colour);
        };
        drawBar(year.invoiced, invoicedColour(), -bar - 1);
        drawBar(year.salary_cost(), salaryColour(), 1);

        painter.setPen(QColor(60, 60, 60));
        painter.drawText(QRect(frame.plot.left() + slot * i, frame.plot.bottom() + 4, slot, frame.font.pixelSize() * 2),
                         Qt::AlignHCenter | Qt::AlignTop, year.name);
    }

    drawLegend(&painter, frame, { qMakePair(invoicedColour(), QString("Fakturerat (exkl. moms)")),
                                  qMakePair(salaryColour(), QString("Lönekostnad (bruttolön + arbetsgivaravgift)")) });
    painter.end();
    return image;
}

//-----------------------------------------------------------------------------
QImage CompanyReportPrinter::monthChart(const CompanyReport& report, const QSize& size, Series series)
{
    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    long long highest = 0;
    for (const CompanyReportYear& year : report.years)
    {
        for (const CompanyReportMonth& month : year.months)
        {
            highest = qMax(highest, valueOf(month, series));
        }
    }
    const ChartFrame frame = drawFrame(&painter, size, highest);

    // Twelve months, and within each month one bar per year.
    const QStringList names = MONTH_NAMES;
    const int years = qMax(1, (int)report.years.size());
    const int slot = frame.plot.width() / 12;
    const int bar = qMax(2, (slot - slot / 4) / years);
    for (int m = 0; m < 12; m++)
    {
        const int left = frame.plot.left() + slot * m + (slot - bar * years) / 2;
        for (int y = 0; y < report.years.size(); y++)
        {
            const CompanyReportYear& year = report.years.at(y);
            if (m >= year.months.size() || valueOf(year.months.at(m), series) <= 0)
            {
                continue;
            }
            const int height = (int)(frame.plot.height() * valueOf(year.months.at(m), series) / frame.scale);
            painter.fillRect(QRect(left + bar * y, frame.plot.bottom() - height, qMax(2, bar - 1), height), yearColour(y));
        }

        // The months in the order the fiscal year has them, which is where
        // they are in the report.
        const int calendar_month = report.years.isEmpty() ? m + 1 : report.years.first().months.at(m).month;
        painter.setPen(QColor(60, 60, 60));
        painter.drawText(QRect(frame.plot.left() + slot * m, frame.plot.bottom() + 4, slot, frame.font.pixelSize() * 2),
                         Qt::AlignHCenter | Qt::AlignTop, names.at(calendar_month - 1));
    }

    QList<QPair<QColor, QString>> legend;
    for (int y = 0; y < report.years.size(); y++)
    {
        legend.append(qMakePair(yearColour(y), report.years.at(y).name));
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
// How wide a column has to be for none of `texts` to wrap: the widest of them
// plus the padding on either side, the cell borders, and enough over that a
// figure as wide as its column is not broken by a rounding. A column measured
// exactly is a column that wraps.
// The tables never go below this, however many years there are. It is small,
// but a report with a column per year over a long history has to choose
// between small figures and broken ones, and a broken amount is worse.
static const int TABLE_FONT_MINIMUM = 4;

// How much of a table's width is left unclaimed by its columns. Columns that
// claim all of it leave nothing for the borders between them, and the last
// figure wraps.
static const qreal RESERVE = 2.0;

static qreal columnWidthFor(const QStringList& texts, const QFont& font, qreal padding)
{
    const QFontMetricsF metrics(font);
    qreal widest = 0.0;
    for (const QString& text : texts)
    {
        widest = qMax(widest, metrics.horizontalAdvance(text));
    }
    return widest + 2.0 * padding + 4.0;
}

//-----------------------------------------------------------------------------
QTextTableFormat CompanyReportPrinter::tableFormat(const QList<QTextLength>& lengths, bool border) const
{
    QTextTableFormat format = m_table_format_default;
    if (!border)
    {
        format.setBorder(0);
    }
    format.setColumnWidthConstraints(lengths);
    return format;
}

//-----------------------------------------------------------------------------
QTextTableFormat CompanyReportPrinter::tableFormat(const QList<qreal>& percentages, bool border) const
{
    QList<QTextLength> lengths;
    for (qreal p : percentages)
    {
        lengths.append(QTextLength(QTextLength::PercentageLength, p));
    }
    return tableFormat(lengths, border);
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::cellIndent(QTextTable* p_table, int row, int column, qreal margin)
{
    QTextCursor cursor = p_table->cellAt(row, column).firstCursorPosition();
    QTextBlockFormat format = cursor.blockFormat();
    format.setLeftMargin(margin);
    cursor.setBlockFormat(format);
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::cellText(QTextTable* p_table, int row, int column, const QString& text, const QFont& font, Qt::Alignment alignment)
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
void CompanyReportPrinter::addSpace(QTextCursor& cursor, const QString& lines)
{
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    format.setFont(m_font_super);
    cursor.insertText(lines, format);
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::render(const CompanyReport& report, const CompanyInfo& company, QIODevice* device)
{
    QPdfWriter writer(device);
    writer.setPageSize(QPageSize::A4Small);
    writer.setPageMargins(QMarginsF(25, 25, 25, 15));
    writer.setResolution(300);
    const QString period = report.years.isEmpty()
        ? QString()
        : (report.years.size() == 1
               ? report.years.first().name
               : QString("%1-%2").arg(report.years.first().name, report.years.last().name));

    // What the reader sees in the document, and what a file manager or a PDF
    // reader shows about it. Qt writes the creation date itself.
    writer.setTitle(period.isEmpty() ? QString("Rapport") : QString("Rapport %1").arg(period));
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

    // Three chapters: what came in, what the salaries cost, and the two put
    // together.
    const int chart_height = available_width * 2 / 5;
    const QSize chart_size(available_width * 4, chart_height * 4);

    // Where each chapter begins, and whether it is one that always starts a
    // page of its own.
    QList<QPair<int, bool>> chapters;
    chapters.append(qMakePair(addChapter("Fakturerat", cursor), false));
    addChart(yearChart(report, chart_size, Invoiced), "Fakturerat per räkenskapsår, exklusive moms", cursor, available_width, chart_height);
    addChart(monthChart(report, chart_size, Invoiced), "Fakturerat per månad", cursor, available_width, chart_height);
    addSeries("Per kund och år (exkl. moms)", "Kund", report.customers, report, cursor);
    addSeries("Per uppdrag och år (exkl. moms)", "Uppdrag", report.assignments, report, cursor);

    chapters.append(qMakePair(addChapter("Lönekostnader", cursor), false));
    addChart(yearChart(report, chart_size, SalaryCost), "Lönekostnad per räkenskapsår, bruttolön och arbetsgivaravgifter", cursor, available_width, chart_height);
    addChart(monthChart(report, chart_size, SalaryCost), "Lönekostnad per månad", cursor, available_width, chart_height);
    addSeries("Bruttolön per anställd och år", "Anställd", report.employees, report, cursor);

    // The summary is what the two chapters before it add up to, so it is read
    // on its own page.
    chapters.append(qMakePair(addChapter("Summering", cursor), true));
    addChart(comparisonChart(report, chart_size), "Fakturerat mot lönekostnad per räkenskapsår", cursor, available_width, chart_height);
    addYears(report, cursor);

    // The page number and the build line go at the foot of every page, drawn
    // on the page rather than flowing with the text, so they sit at the bottom
    // whatever the page holds. The last two lines of the page are left for
    // them: the page number in black, since it is for the reader, and the
    // build line in grey below it, since it is not.
    QFont footer_font("Arial");
    footer_font.setPixelSize(6);
    QFont page_font("Arial");
    page_font.setPixelSize(8);
    const int footer_height = 24;   // two lines, as on an invoice
    const int page_height = 16;
    const qreal body_height = page.height() - footer_height - page_height;

    doc.setPageSize(QSizeF(available_width + 2 * doc.documentMargin(), body_height));

    // A chapter heading close to the foot of a page would be left there on
    // its own, so it is moved to the next page, and a chapter that asks for a
    // page of its own is moved whatever it sits next to. A heading already at
    // the top of a page is left alone: breaking there would leave the page
    // before it empty.
    {
        const qreal keep_together = chart_height + 30;
        bool moved = true;
        for (int pass = 0; moved && pass < 4; pass++)
        {
            moved = false;
            for (const QPair<int, bool>& chapter : chapters)
            {
                const int position = chapter.first;
                const QTextBlock block = doc.findBlock(position);
                const qreal top = doc.documentLayout()->blockBoundingRect(block).top();
                const qreal on_page = top - body_height * (int)(top / body_height);
                const bool at_the_top = on_page < 1.0;
                const bool too_low = on_page + keep_together > body_height;
                if (at_the_top || !(too_low || chapter.second))
                {
                    continue;
                }

                QTextCursor c(&doc);
                c.setPosition(position);
                QTextBlockFormat format = c.blockFormat();
                if (format.pageBreakPolicy() == QTextFormat::PageBreak_AlwaysBefore)
                {
                    continue;
                }
                format.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
                c.setBlockFormat(format);
                moved = true;
            }
        }
    }

    const int pages = qMax(1, doc.pageCount());
    for (int i = 0; i < pages; i++)
    {
        painter.save();
        painter.translate(0, -body_height * i);
        painter.setClipRect(QRectF(0, body_height * i, available_width + 2 * doc.documentMargin(), body_height));
        doc.drawContents(&painter, QRectF(0, body_height * i, available_width + 2 * doc.documentMargin(), body_height));
        painter.restore();

        // The logo sits where it does on an invoice, on the first page.
        if (i == 0)
        {
            printer_util_insert_svg_string(&painter, QRectF(5.0, 3.0, 130.0, 35.0), company.m_logo, false);
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
    QTextTable* p_table = cursor.insertTable(3, 3, tableFormat({ 27.0, 43.0, 30.0 }, false));
    p_table->mergeCells(0, 0, 3, 1);

    p_table->mergeCells(0, 1, 3, 1);
    {
        QTextTableCell cell = p_table->cellAt(0, 1);
        QTextCharFormat format;
        format.setFont(m_font_header);
        QTextCursor c = cell.firstCursorPosition();
        QTextBlockFormat block;
        block.setAlignment(Qt::AlignCenter);
        c.mergeBlockFormat(block);
        c.insertText("RAPPORT", format);
    }

    const QString period = report.years.isEmpty()
        ? QString()
        : (report.years.size() == 1
               ? report.years.first().name
               : QString("%1-%2").arg(report.years.first().name, report.years.last().name));

    cellText(p_table, 0, 2, company.m_name, m_font_normal, Qt::AlignRight);
    cellText(p_table, 1, 2, period, m_font_normal, Qt::AlignRight);
    // A report is of the day it was made: the same year looks different a
    // month later.
    cellText(p_table, 2, 2, QString("Genererad %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm")),
             m_font_super, Qt::AlignRight);
}

//-----------------------------------------------------------------------------
// Every part of the report puts its space before itself rather than after, so
// that the document ends with the last table. A trailing empty line would run
// over onto a page of its own, and that page would hold nothing but its
// footer.
int CompanyReportPrinter::addChapter(const QString& title, QTextCursor& cursor)
{
    addSpace(cursor, " \n \n");
    cursor.movePosition(QTextCursor::End);
    cursor.insertBlock(QTextBlockFormat());

    const int position = cursor.position();
    QTextCharFormat heading;
    heading.setFont(m_font_chapter);
    cursor.insertText(title, heading);
    return position;
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
    cursor.insertImage(format);

    // Under the chart, saying what it shows: the axes carry years and months,
    // not what is being counted along them.
    QTextBlockFormat block;
    block.setAlignment(Qt::AlignHCenter);
    cursor.insertBlock(block);

    QTextCharFormat text;
    text.setFont(m_font_caption);
    text.setForeground(QBrush(QColor(90, 90, 90)));
    cursor.insertText(caption, text);

    // Back to the left for whatever comes next.
    QTextBlockFormat plain;
    cursor.insertBlock(plain);
    cursor.setBlockCharFormat(QTextCharFormat());
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addYears(const CompanyReport& report, QTextCursor& cursor)
{
    addSpace(cursor, " \n");
    cursor.movePosition(QTextCursor::End);
    QTextTable* p_table = cursor.insertTable(1 + report.years.size(), 5, tableFormat({ 20.0, 20.0, 20.0, 20.0, 20.0 }));

    cellText(p_table, 0, 0, "År", m_font_super);
    cellText(p_table, 0, 1, "Fakturerat", m_font_super, Qt::AlignRight);
    cellText(p_table, 0, 2, "Bruttolön", m_font_super, Qt::AlignRight);
    cellText(p_table, 0, 3, "Arbetsgivaravgifter", m_font_super, Qt::AlignRight);
    cellText(p_table, 0, 4, "Kvar efter löner", m_font_super, Qt::AlignRight);

    for (int i = 0; i < report.years.size(); i++)
    {
        const CompanyReportYear& year = report.years.at(i);
        cellText(p_table, i + 1, 0, year.name, m_font_table);
        cellText(p_table, i + 1, 1, printer_util_format_money(year.invoiced), m_font_table, Qt::AlignRight);
        cellText(p_table, i + 1, 2, printer_util_format_money(year.salary_gross), m_font_table, Qt::AlignRight);
        cellText(p_table, i + 1, 3, printer_util_format_money(year.employer_fee), m_font_table, Qt::AlignRight);
        cellText(p_table, i + 1, 4, printer_util_format_money(year.after_salaries()), m_font_table_bold, Qt::AlignRight);
    }
}

//-----------------------------------------------------------------------------
void CompanyReportPrinter::addSeries(const QString& heading, const QString& first_column,
                                     const QList<CompanyReportSeries>& series, const CompanyReport& report, QTextCursor& cursor)
{
    const QList<int> years = report.yearNumbers();
    if (series.isEmpty())
    {
        return;
    }

    addSpace(cursor, " \n");
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat heading_format;
    heading_format.setFont(m_font_normal_bold);
    cursor.insertText(heading + "\n", heading_format);

    // One column per year, and the total of them all last. An amount has to
    // stand on one line, so the columns holding them are measured from the
    // widest figure that will actually be in them and the name takes what is
    // left. An even share is what broke here: an uppdrag carries its
    // customer's name as well, and the wide first column that needs squeezed a
    // seven-figure amount over two lines.
    const int columns = 2 + years.size();
    const qreal table_width = m_table_format_default.width().rawValue();
    const qreal padding = m_table_format_default.cellPadding();

    QStringList amounts;
    QStringList totals;
    QStringList headings;
    for (const CompanyReportSeries& entry : series)
    {
        for (int i = 0; i < years.size(); i++)
        {
            const long long value = entry.per_year.value(years.at(i));
            amounts.append(value == 0 ? QString("-") : printer_util_format_money(value));
        }
        totals.append(printer_util_format_money(entry.total()));
    }
    for (int i = 0; i < years.size(); i++)
    {
        headings.append(report.years.at(i).name);
    }

    // The largest size at which it all still fits. A table with a column per
    // year is the one thing in the report that runs out of width, and a
    // company with a long history runs out sooner, so the size follows what is
    // being shown rather than being fixed and hoping.
    // A name may wrap over a few lines; an amount may not wrap at all. So when
    // the years crowd the page it is the name column that gives way, down to
    // this much of the width and no further.
    const qreal name_minimum = 0.15 * table_width;
    const qreal reserve = RESERVE / 100.0 * table_width;
    QFont font = m_font_table;
    QFont font_bold = m_font_table_bold;
    qreal amount_points = 0.0;
    qreal total_points = 0.0;
    for (int points = m_font_table.pointSize(); ; points--)
    {
        font.setPointSize(points);
        font_bold.setPointSize(points);

        amount_points = qMax(columnWidthFor(amounts, font, padding),
                             columnWidthFor(headings, m_font_super, padding));
        total_points = qMax(columnWidthFor(totals, font_bold, padding),
                            columnWidthFor({ QString("Totalt") }, m_font_super, padding));

        if (years.size() * amount_points + total_points <= table_width - name_minimum - reserve
            || points <= TABLE_FONT_MINIMUM)
        {
            break;
        }
    }

    // Even at the smallest size there can be years enough not to fit. The
    // amounts then give up their slack in proportion rather than the names
    // disappearing altogether.
    const qreal wanted = years.size() * amount_points + total_points;
    if (wanted > table_width - name_minimum - reserve)
    {
        const qreal shrink = (table_width - name_minimum - reserve) / qMax(1.0, wanted);
        amount_points *= shrink;
        total_points *= shrink;
    }

    // As shares of the width, with a little of it left over: shares that come
    // to exactly the whole leave nothing for the cell borders, and the figure
    // wraps after all. The name column takes what the amounts do not.
    const qreal amount_width = 100.0 * amount_points / table_width;
    const qreal total_width = 100.0 * total_points / table_width;
    const qreal name_width = 100.0 - RESERVE - years.size() * amount_width - total_width;

    QList<qreal> widths;
    widths.append(name_width);
    for (int i = 0; i < years.size(); i++)
    {
        widths.append(amount_width);
    }
    widths.append(total_width);

    // A customer whose uppdrag are listed heads them with a row of its own, so
    // the name is not repeated on every line of it.
    QStringList groups;
    for (const CompanyReportSeries& entry : series)
    {
        if (!entry.group.isEmpty() && !groups.contains(entry.group))
        {
            groups.append(entry.group);
        }
    }

    cursor.movePosition(QTextCursor::End);
    QTextTable* p_table = cursor.insertTable(1 + series.size() + groups.size(), columns, tableFormat(widths));

    cellText(p_table, 0, 0, first_column, m_font_super);
    for (int i = 0; i < years.size(); i++)
    {
        cellText(p_table, 0, 1 + i, report.years.at(i).name, m_font_super, Qt::AlignRight);
    }
    cellText(p_table, 0, columns - 1, "Totalt", m_font_super, Qt::AlignRight);

    int row = 0;
    QString current;
    for (const CompanyReportSeries& entry : series)
    {
        if (!entry.group.isEmpty() && entry.group != current)
        {
            current = entry.group;
            row++;
            cellText(p_table, row, 0, current, font_bold);
        }

        row++;
        cellText(p_table, row, 0, entry.name, font);
        if (!entry.group.isEmpty())
        {
            cellIndent(p_table, row, 0, 2.0 * padding);
        }
        for (int i = 0; i < years.size(); i++)
        {
            const long long value = entry.per_year.value(years.at(i));
            cellText(p_table, row, 1 + i, value == 0 ? QString("-") : printer_util_format_money(value),
                     font, Qt::AlignRight);
        }
        cellText(p_table, row, columns - 1, printer_util_format_money(entry.total()), font_bold, Qt::AlignRight);
    }
}
