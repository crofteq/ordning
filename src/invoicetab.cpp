#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QBuffer>
#include <QFileDialog>
#include <QStandardPaths>

#include "data/database.h"

#include "data/invoicedraft.h"
#include "invoicefactory.h"
#include "invoiceprinter.h"
#include "pdfviewer.h"

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

    m_invoiceModel = new InvoiceTableModel(this);
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

        m_export = new QPushButton("Exportera");
        connect(m_export, &QPushButton::clicked, this, &InvoiceTab::onExport);
        hbox->addWidget(m_export);

        m_view = new QPushButton("Visa");
        connect(m_view, &QPushButton::clicked, this, &InvoiceTab::onView);
        hbox->addWidget(m_view);

        m_credit = new QPushButton("Kreditera");
        connect(m_credit, &QPushButton::clicked, this, &InvoiceTab::onCredit);
        hbox->addWidget(m_credit);

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
    bool hasSelection = m_invoices->currentIndex().row() >= 0;
    bool onListPage = m_stackedWidget->currentIndex() == 0;
    m_export->setVisible(onListPage);
    m_export->setDisabled(!hasSelection);
    m_export->setStyleSheet(!hasSelection ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");
    m_view->setVisible(onListPage);
    m_view->setDisabled(!hasSelection);
    m_view->setStyleSheet(!hasSelection ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");
    Invoice* sel = hasSelection ? m_invoiceModel->getInvoice(m_invoices->currentIndex()) : nullptr;
    bool canCredit = hasSelection && sel != nullptr && !sel->m_is_credit;
    m_credit->setVisible(onListPage);
    m_credit->setDisabled(!canCredit);
    m_credit->setStyleSheet(!canCredit ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");
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

    // Rendered into memory: looking at an invoice leaves no file behind.
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    InvoicePrinter::instance()->create_pdf(p_in, DataBase::instance()->db()->m_companyinfo.m_logo, false, &buffer);

    pdf_show(buffer.data(), QString("Faktura %1").arg(p_in->m_number));
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
void InvoiceTab::onExport(void)
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

    QString invoiceType = p_in->m_is_credit ? "kreditfaktura" : "faktura";
    QString suggestion =
        QString("%1/%2-%3-%4-%5.pdf")
                             .arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                             p_in->m_number,
                             invoiceType,
                             p_in->m_company_name.toLower().split(" ").at(0),
                             p_in->m_customer_name.toLower().split(" ").at(0));
    QString pdf_file = QFileDialog::getSaveFileName(this, "Exportera faktura", suggestion, "PDF-filer (*.pdf)");
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

//-----------------------------------------------------------------------------
void InvoiceTab::onCredit(void)
{
    QModelIndex index = m_invoices->currentIndex();
    if (index.row() < 0)
    {
        updateButtons();
        return;
    }

    Invoice* orig = m_invoiceModel->getInvoice(index);
    if (orig == nullptr)
    {
        qDebug() << "Failed to get invoice from model!!";
        return;
    }

    QMessageBox mb(this);
    mb.setWindowTitle("Kreditera faktura");
    mb.setText(QString("Vill du skapa en kreditfaktura för faktura nr %1?").arg(orig->m_number));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mb.setDefaultButton(QMessageBox::No);
    mb.setIcon(QMessageBox::Question);
    if (mb.exec() != QMessageBox::Yes)
        return;

    InvoiceDraft d;
    d.m_customer_id = orig->m_customer_id;
    d.m_invoice_date = QDate::currentDate();
    d.m_is_credit = true;
    d.m_credited_invoice_number = orig->m_number;

    std::unique_ptr<Invoice> created = InvoiceFactory::create(&d);
    if (created == nullptr)
    {
        QMessageBox::critical(this, "Fel", "Kunde inte skapa kreditfaktura.");
        return;
    }

    Invoice* p_in = DataBase::instance()->db()->m_invoices.append(std::move(created));
    DataBase::instance()->notifyUpdated();

    QMessageBox::information(this, "Kreditfaktura skapad", QString("Kreditfaktura nr %1 har skapats.").arg(p_in->m_number));
}
