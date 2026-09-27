#ifndef INVOICEPRINTER_H
#define INVOICEPRINTER_H

#include <QIODevice>
#include <QPdfWriter>
#include <QTextCursor>
#include <QMutex>

#include <memory>

#include "data/invoice.h"

class InvoicePrinter
{
public:
    // Public static method to access the singleton instance
    static InvoicePrinter* instance() {
        static QMutex mutex;
        if (!m_instance) {
            QMutexLocker locker(&mutex);
            if (!m_instance) {
                m_instance = new InvoicePrinter();
            }
        }
        return m_instance;
    }

    // Deleting copy constructor and assignment operator to prevent copying
    InvoicePrinter(const InvoicePrinter&) = delete;
    InvoicePrinter& operator=(const InvoicePrinter&) = delete;

    // Writes the invoice to `filename`, or to `device` when it is only going
    // to be looked at: a preview then leaves no file behind.
    void create_pdf(Invoice* p_in, QString logo, bool b_preview, QString filename);
    void create_pdf(Invoice* p_in, QString logo, bool b_preview, QIODevice* device);

protected:
    // Protected constructor to prevent instantiation from outside the class
    InvoicePrinter();

private:
    static InvoicePrinter* m_instance;

    bool m_border_colapse;
    QBrush m_border_brush;
    QTextFrameFormat::BorderStyle m_border_style;
    qreal m_border_size;
    int m_cell_spacing;
    int m_cell_padding;
    QFont m_font_header;
    QFont m_font_super;
    QFont m_font_normal;
    QFont m_font_normal_bold;
    QTextTableFormat m_table_format_default;

    void render(Invoice* p_in, QString logo, bool b_preview, QPdfWriter& printer);
    std::unique_ptr<QTextDocument> createDocument(Invoice* p_in, QPdfWriter &printer);

    void addHeader(Invoice* p_in, QTextCursor& cursor);
    void addSenderReceiver(Invoice* p_in, QTextCursor& cursor);
    void addCustomerInfo(Invoice* p_in, QTextCursor& cursor);
    void addItems(Invoice* p_in, QTextCursor& cursor);
    void addComments(Invoice* p_in, QTextCursor& cursor);
    void addSummary(Invoice* p_in, QTextCursor& cursor);
    void addCompanyInfo(Invoice* p_in, QTextCursor& cursor);

    void addFooter(QTextDocument *document, const QString &footerText);
};

#endif // INVOICEPRINTER_H
