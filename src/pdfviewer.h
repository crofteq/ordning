#ifndef PDFVIEWER_H
#define PDFVIEWER_H

#include <QByteArray>
#include <QString>

// Opens `pdf` in a window of its own, titled `title`. The window owns itself
// and is destroyed when the user closes it; the document and the bytes behind
// it go with it.
void pdf_show(const QByteArray& pdf, const QString& title);

#endif // PDFVIEWER_H
