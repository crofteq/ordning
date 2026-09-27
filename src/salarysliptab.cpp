#include <QBuffer>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "data/database.h"
#include "pdfviewer.h"
#include "salaryslipprinter.h"

#include "salarysliptab.h"

#define DATE_FORMAT_STRING  "yyyy-MM-dd"

//-----------------------------------------------------------------------------
SalarySlipTab::SalarySlipTab(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    m_slips = new QTableView();
    m_slips->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_slips->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_slips, &QTableView::clicked, this, &SalarySlipTab::onSlipSelected);
    connect(m_slips, &QTableView::doubleClicked, this, &SalarySlipTab::onView);
    m_slipModel = new SalarySlipTableModel(this);
    m_slips->setModel(m_slipModel);
    layout->addWidget(m_slips);

    {
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addStretch(1);

        m_export = new QPushButton("Export");
        connect(m_export, &QPushButton::clicked, this, &SalarySlipTab::onExport);
        hbox->addWidget(m_export);

        m_view = new QPushButton("View");
        connect(m_view, &QPushButton::clicked, this, &SalarySlipTab::onView);
        hbox->addWidget(m_view);

        layout->addLayout(hbox);
    }

    setLayout(layout);
    updateButtons();
}

//-----------------------------------------------------------------------------
void SalarySlipTab::load(void)
{
    m_slipModel->load();
    m_slips->resizeColumnsToContents();
    updateButtons();
}

//-----------------------------------------------------------------------------
void SalarySlipTab::onDatabaseUpdated(void)
{
    load();
}

//-----------------------------------------------------------------------------
void SalarySlipTab::updateButtons(void)
{
    const bool selected = m_slips->currentIndex().row() >= 0;
    for (QPushButton* p : { m_view, m_export })
    {
        p->setEnabled(selected);
        p->setStyleSheet(selected ? "" : "QPushButton:disabled { color: gray; background-color: lightgray; }");
    }
}

//-----------------------------------------------------------------------------
void SalarySlipTab::showPdf(const SalarySlip& slip, bool preview)
{
    const SalaryYearToDate ytd = DataBase::instance()->db()->m_salaryslips.yearToDate(slip);

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    SalarySlipPrinter::create_pdf(slip, DataBase::instance()->db()->m_companyinfo.m_logo, ytd, preview, &buffer);

    pdf_show(buffer.data(), QString("Lönebesked %1 %2").arg(slip.m_employee_name, slip.m_payment_date.toString(DATE_FORMAT_STRING)));
}

//-----------------------------------------------------------------------------
void SalarySlipTab::onSlipSelected(const QModelIndex &index)
{
    Q_UNUSED(index)
    updateButtons();
}

//-----------------------------------------------------------------------------
void SalarySlipTab::onView(void)
{
    SalarySlip* p_slip = m_slipModel->getSalarySlip(m_slips->currentIndex());
    if (p_slip != nullptr)
    {
        showPdf(*p_slip, false);
    }
}

//-----------------------------------------------------------------------------
void SalarySlipTab::onExport(void)
{
    SalarySlip* p_slip = m_slipModel->getSalarySlip(m_slips->currentIndex());
    if (p_slip == nullptr)
    {
        updateButtons();
        return;
    }

    QString suggestion = QString("%1/%2-lonebesked-%3.pdf")
        .arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
             p_slip->m_payment_date.toString(DATE_FORMAT_STRING),
             p_slip->m_employee_name.toLower().split(" ").at(0));
    QString pdf_file = QFileDialog::getSaveFileName(this, "Export lönebesked", suggestion, "PDF Files (*.pdf)");
    if (pdf_file.isEmpty())
    {
        return;
    }

    QFile file(pdf_file);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "Export lönebesked", QString("Could not write %1.").arg(pdf_file));
        return;
    }

    const SalaryYearToDate ytd = DataBase::instance()->db()->m_salaryslips.yearToDate(*p_slip);
    SalarySlipPrinter::create_pdf(*p_slip, DataBase::instance()->db()->m_companyinfo.m_logo, ytd, false, &file);
}
