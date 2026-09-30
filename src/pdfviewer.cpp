#include <QBuffer>
#include <QPdfDocument>
#include <QPdfView>

#include "pdfviewer.h"

//-----------------------------------------------------------------------------
void pdf_show(const QByteArray& pdf, const QString& title)
{
    // Read from memory, so nothing is left behind in the working directory.
    // The buffer belongs to the document, which has to outlive the view.
    QPdfDocument *document = new QPdfDocument;
    QBuffer *buffer = new QBuffer(document);
    buffer->setData(pdf);
    buffer->open(QIODevice::ReadOnly);
    document->load(buffer);

    // The window owns itself: it is destroyed when the user closes it.
    QPdfView *view = new QPdfView;
    view->setAttribute(Qt::WA_DeleteOnClose);
    view->setWindowTitle(title);

    // The document must NOT be a child of the view: see the note in CLAUDE.md.
    // It is deleted once the view is fully destroyed.
    view->setDocument(document);
    QObject::connect(view, &QObject::destroyed, document, &QObject::deleteLater);
    // Every page, one under the other: the company report runs over as many
    // pages as it needs, and a single-page view would show the first of them
    // and nothing else.
    view->setPageMode(QPdfView::PageMode::MultiPage);
    view->setDocumentMargins(QMargins(30, 30, 30, 30));
    view->setMinimumHeight(1200);
    view->setMinimumWidth(900);
    view->show();
}
