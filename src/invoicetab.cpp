#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPdfView>
#include <QPdfDocument>
#include <QFileDialog>
#include <QStandardPaths>

#include "data/database.h"

#include "data/agreement.h"
#include "data/customer.h"
#include "invoicefactory.h"
#include "invoiceprinter.h"

#include "invoicetab.h"

//-----------------------------------------------------------------------------
InvoiceTab::InvoiceTab(QWidget *parent) : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    m_stackedWidget = new QStackedWidget();

    // First stacked widget
    m_invoices = new QTableView();
    m_invoices->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_invoices->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_invoices, &QTableView::clicked, this, &InvoiceTab::onInvoiceSelected);
    connect(m_invoices, &QTableView::doubleClicked, this, &InvoiceTab::onDoubleClicked);

    m_invoiceModel = new InvoiceTableModel();
    m_invoices->setModel(m_invoiceModel);
    m_stackedWidget->addWidget(m_invoices);
    m_stackedWidget->setCurrentIndex(0);

    // Setup the callback for the stackedwidgetchanged
    // connect(m_stackedWidget, &QStackedWidget::currentChanged, this, &InvoiceTab::onStackedWidgetChanged);

    // Add the stackedwidget
    layout->addWidget(m_stackedWidget);

    // Add the buttons at the bottom
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addStretch(1);

        m_save = new QPushButton("Save");
        connect(m_save, &QPushButton::clicked, this, &InvoiceTab::onSave);
        hbox->addWidget(m_save);

        m_view = new QPushButton("View");
        connect(m_view, &QPushButton::clicked, this, &InvoiceTab::onView);
        hbox->addWidget(m_view);

        updateButtons();
        layout->addLayout(hbox);
    }

    setLayout(layout);
}

//-----------------------------------------------------------------------------
void InvoiceTab::load(void)
{
    m_invoiceModel->load();
    m_invoices->resizeColumnsToContents();
    updateButtons();
}

//-----------------------------------------------------------------------------
void InvoiceTab::onDatabaseUpdated(void)
{
    load();
}

//-----------------------------------------------------------------------------
void InvoiceTab::updateButtons(void)
{
    m_save->setVisible(m_stackedWidget->currentIndex() == 0);
    m_save->setDisabled(m_invoices->currentIndex().row() < 0);
    m_save->setStyleSheet(m_invoices->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");
    m_view->setVisible(m_stackedWidget->currentIndex() == 0);
    m_view->setDisabled(m_invoices->currentIndex().row() < 0);
    m_view->setStyleSheet(m_invoices->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");
}

//-----------------------------------------------------------------------------
void InvoiceTab::view(const QModelIndex &index)
{
    Invoice *p_in = m_invoiceModel->getInvoice(index);
    if (p_in == nullptr)
    {
        qDebug() << "Failed to get invoice from model!!";
        return;
    }

    QString pdf_file = "view.pdf";
    InvoicePrinter::instance()->create_pdf(p_in, DataBase::instance()->db()->m_companyinfo.m_logo, false, pdf_file);

    QPdfDocument *document = new QPdfDocument;
    document->load(pdf_file);

    QPdfView *view = new QPdfView;
    view->setDocument(document);
    view->setDocumentMargins(QMargins(30, 30, 30, 30));
    view->setMinimumHeight(1200);
    view->setMinimumWidth(900);
    view->show();
}

//-----------------------------------------------------------------------------
void InvoiceTab::onInvoiceSelected(const QModelIndex &index)
{
    updateButtons();
}

//-----------------------------------------------------------------------------
void InvoiceTab::onDoubleClicked(const QModelIndex &index)
{
    view(index);
}

//-----------------------------------------------------------------------------
void InvoiceTab::onSave(void)
{
    QModelIndex index = m_invoices->currentIndex();
    if (index.row() < 0)
    {
        updateButtons();
        return;
    }

    Invoice *p_in = m_invoiceModel->getInvoice(index);
    if (p_in == nullptr)
    {
        qDebug() << "Failed to get invoice from model!!";
        return;
    }

    QString suggestion =
        QString("%1/%2-faktura-%3-%4.pdf")
                             .arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                             p_in->m_number,
                             p_in->m_company_name.toLower().split(" ").at(0),
                             p_in->m_customer_name.toLower().split(" ").at(0));
    QString pdf_file = QFileDialog::getSaveFileName(this, "Save Invoice", suggestion, "PDF Files (*.pdf)");
    if (!pdf_file.isEmpty())
    {
        InvoicePrinter::instance()->create_pdf(p_in, DataBase::instance()->db()->m_companyinfo.m_logo, false, pdf_file);
    }
}

//-----------------------------------------------------------------------------
void InvoiceTab::onView(void)
{
    QModelIndex index = m_invoices->currentIndex();
    if (index.row() < 0)
    {
        updateButtons();
        return;
    }

    view(index);
}
