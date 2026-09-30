#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QBuffer>

#include "data/database.h"

#include "data/agreement.h"
#include "data/customer.h"
#include "data/employee.h"
#include "util/amount.h"

#include "invoicedrafttab.h"
#include "invoicefactory.h"
#include "invoiceprinter.h"
#include "pdfviewer.h"

#define LABEL_WIDTH 140

#define DATE_FORMAT_STRING          "yyyy-MM-dd"

//-----------------------------------------------------------------------------
// Hours are typed in hours and may carry a fraction, Swedish style with a comma.
static QValidator* hoursValidator(void)
{
    QDoubleValidator* v = new QDoubleValidator(0.0, 300.0, 2);
    v->setNotation(QDoubleValidator::StandardNotation);
    v->setLocale(QLocale(QLocale::Swedish, QLocale::Sweden));
    return v;
}


//-----------------------------------------------------------------------------
InvoiceDraftTab::InvoiceDraftTab(QWidget *parent) : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    m_stackedWidget = new QStackedWidget();

    // First stacked widget
    m_invoicedrafts = new QTableView();
    m_invoicedrafts->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_invoicedrafts->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_invoicedrafts, &QTableView::doubleClicked, this, &InvoiceDraftTab::onDoubleClicked);
    connect(m_invoicedrafts, &QTableView::clicked, this, &InvoiceDraftTab::onInvoiceSelected);
    m_invoiceModel = new InvoiceDraftTableModel(this);
    m_invoicedrafts->setModel(m_invoiceModel);
    m_stackedWidget->addWidget(m_invoicedrafts);

    // Second stacked widget
    m_stackedWidget->addWidget(editWidgetCreate());
    m_stackedWidget->setCurrentIndex(0);

    // Setup the callback for the stackedwidgetchanged
    connect(m_stackedWidget, &QStackedWidget::currentChanged, this, &InvoiceDraftTab::onStackedWidgetChanged);

    // Add the stackedwidget
    layout->addWidget(m_stackedWidget);

    // Add the buttons at the bottom
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addStretch(1);

        m_approve = new QPushButton("Godkänn");
        connect(m_approve, &QPushButton::clicked, this, &InvoiceDraftTab::onApprove);
        hbox->addWidget(m_approve);

        m_preview = new QPushButton("Förhandsgranska");
        connect(m_preview, &QPushButton::clicked, this, &InvoiceDraftTab::onPreview);
        hbox->addWidget(m_preview);

        m_new = new QPushButton("Ny");
        connect(m_new, &QPushButton::clicked, this, &InvoiceDraftTab::onNew);
        hbox->addWidget(m_new);

        m_edit = new QPushButton("Ändra");
        m_edit->setDisabled(true);
        connect(m_edit, &QPushButton::clicked, this, &InvoiceDraftTab::onEdit);
        hbox->addWidget(m_edit);

        m_delete = new QPushButton("Ta bort");
        m_delete->setDisabled(true);
        connect(m_delete, &QPushButton::clicked, this, &InvoiceDraftTab::onDelete);
        hbox->addWidget(m_delete);

        m_cancel = new QPushButton("Avbryt");
        connect(m_cancel, &QPushButton::clicked, this, &InvoiceDraftTab::onCancel);
        hbox->addWidget(m_cancel);

        m_ok = new QPushButton("Ok");
        m_ok->setDisabled(true);
        connect(m_ok, &QPushButton::clicked, this, &InvoiceDraftTab::onOk);
        hbox->addWidget(m_ok);

        updateButtons();
        layout->addLayout(hbox);
    }

    setLayout(layout);
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::load(void)
{
    m_invoiceModel->load();
    m_invoicedrafts->resizeColumnsToContents();
    updateButtons();
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onDatabaseUpdated(void)
{
    load();
}

//-----------------------------------------------------------------------------
QWidget* InvoiceDraftTab::editWidgetCreate(void)
{
    QWidget* p_widget = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel("Kund:");
        p_label->setFixedWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        m_edit_customer_id = new QComboBox();
        connect(m_edit_customer_id, &QComboBox::currentIndexChanged, this, &InvoiceDraftTab::onCurrentCustomerChanged);
        hbox->addWidget(m_edit_customer_id);
        layout->addLayout(hbox);
    }

    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel("Avtal:");
        p_label->setFixedWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        m_edit_agreement_id = new QComboBox();
        m_edit_agreement_id->setMinimumWidth(100);
        hbox->addWidget(m_edit_agreement_id);
        layout->addLayout(hbox);
    }

    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel("Konsult:");
        p_label->setFixedWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        m_edit_employee_id = new QComboBox();
        m_edit_employee_id->setMinimumWidth(100);
        hbox->addWidget(m_edit_employee_id);
        layout->addLayout(hbox);
    }

    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel("Fakturadatum:");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        m_edit_invoice_date_value = QDate::currentDate();
        m_edit_invoice_date = new QPushButton(m_edit_invoice_date_value.toString("yyyy-MM-dd"));
        connect(m_edit_invoice_date, &QPushButton::clicked, this, &InvoiceDraftTab::onPickInvoiceDate);
        hbox->addWidget(m_edit_invoice_date);
        hbox->addStretch(1);
        layout->addLayout(hbox);
    }

    {
        // Create calendar widget in a separate dialog
        m_edit_invoice_date_dialog = new QDialog(this);
        m_edit_invoice_date_dialog->setWindowTitle("Välj datum");
        m_edit_invoice_date_dialog->setModal(true);

        QVBoxLayout *dialogLayout = new QVBoxLayout(m_edit_invoice_date_dialog);
        m_edit_invoice_date_calendar = new QCalendarWidget(m_edit_invoice_date_dialog);
        dialogLayout->addWidget(m_edit_invoice_date_calendar);

        // Connect calendar date selection
        connect(m_edit_invoice_date_calendar, &QCalendarWidget::clicked, this, &InvoiceDraftTab::onInvoiceDatePicked);
    }

    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel("Period:");
        p_label->setFixedWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        m_edit_month = new QComboBox();
        QStringList periods = { "Januari", "Februari", "Mars", "April", "Maj", "Juni", "Juli", "Augusti", "September", "Oktober", "November", "December" };
        m_edit_month->addItems(periods);
        hbox->addWidget(m_edit_month);
        layout->addLayout(hbox);
    }

    QList<QString> labels;

    labels.append("Normaltid (tim)");
    m_edit_hours_standard = new QLineEdit();
    m_edit_hours_standard->setPlaceholderText("0-300");
    m_edit_hours_standard->setValidator(hoursValidator());
    m_lineedits.append(m_edit_hours_standard);

    labels.append("Övertid (tim)");
    m_edit_hours_overtime = new QLineEdit();
    m_edit_hours_overtime->setPlaceholderText("0-300");
    m_edit_hours_overtime->setValidator(hoursValidator());
    m_lineedits.append(m_edit_hours_overtime);

    labels.append("Övertid kvalificerad (tim)");
    m_edit_hours_overtime_qualified = new QLineEdit();
    m_edit_hours_overtime_qualified->setPlaceholderText("0-300");
    m_edit_hours_overtime_qualified->setValidator(hoursValidator());
    m_lineedits.append(m_edit_hours_overtime_qualified);

    labels.append("Restid (tim)");
    m_edit_hours_traveltime = new QLineEdit();
    m_edit_hours_traveltime->setPlaceholderText("0-300");
    m_edit_hours_traveltime->setValidator(hoursValidator());
    m_lineedits.append(m_edit_hours_traveltime);

    for(int i=0; i<labels.size(); i++)
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel(labels.at(i) + ":");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        hbox->addWidget(m_lineedits.at(i));
        layout->addLayout(hbox);
    }

    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel("Kommentar:");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        m_edit_comment = new QTextEdit();
        hbox->addWidget(m_edit_comment);
        layout->addLayout(hbox);
    }

    p_widget->setLayout(layout);

    return p_widget;
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::loadEmployees(int employee_id)
{
    Employees& employees = DataBase::instance()->db()->m_employees;

    m_edit_employee_id->clear();
    for (int i=0; i<employees.size(); i++)
    {
        Employee* e = employees.at(i);
        // Somebody who has left is still on the invoices they were the
        // consultant on, but no new invoice is written for them.
        if (e->m_active || e->id() == employee_id)
        {
            m_edit_employee_id->addItem(e->name(), e->id());
        }
    }

    for (int i=0; i<m_edit_employee_id->count(); i++)
    {
        if (m_edit_employee_id->itemData(i).toInt() == employee_id)
        {
            m_edit_employee_id->setCurrentIndex(i);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::openEditWidgetNew(void)
{
    m_invoice_id = -1;
    Customers& customers = DataBase::instance()->db()->m_customers;

    m_edit_customer_id->clear();
    for (int i=0; i<customers.size(); i++)
    {
        Customer* c = customers.at(i);
        for (int j=0; j<c->m_agreements.size(); j++)
        {
            if (c->m_agreements.at(j)->m_active)
            {
                // this customer has active agreements, lets add it to the list of possible customers, to send an invoice to.
                m_edit_customer_id->addItem(c->m_name, c->id());
                break;
            }
        }
    }

    // Select the first customer (if any) so the agreement combo gets populated
    if (m_edit_customer_id->count() > 0)
    {
        m_edit_customer_id->setCurrentIndex(0);
    }

    loadEmployees(-1);

    // Reset input fields to sensible defaults for a new invoice draft
    for(int i=0; i<m_lineedits.size(); i++)
    {
        m_lineedits.at(i)->clear();
    }
    m_edit_hours_standard->setText("0,0");
    m_edit_hours_overtime->setText("0,0");
    m_edit_hours_overtime_qualified->setText("0,0");
    m_edit_hours_traveltime->setText("0,0");

    m_edit_invoice_date_value = QDate::currentDate();
    m_edit_invoice_date->setText(m_edit_invoice_date_value.toString(DATE_FORMAT_STRING));
    m_edit_month->setCurrentIndex(m_edit_invoice_date_value.month() - 1);
    m_edit_comment->clear();
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::openEditWidget(const QModelIndex &index)
{
    // fetch the clicked invoice from the model behind the table view
    InvoiceDraft* p = m_invoiceModel->getInvoice(index);

    m_invoice_id = p->id();

    Customers& customers = DataBase::instance()->db()->m_customers;

    m_edit_customer_id->clear();
    for (int i=0; i<customers.size(); i++)
    {
        Customer* c = customers.at(i);
        for (int j=0; j<c->m_agreements.size(); j++)
        {
            if (c->m_agreements.at(j)->m_active)
            {
                // this customer has active agreements, lets add it to the list of possible customers, to send an invoice to.
                m_edit_customer_id->addItem(c->m_name, c->id());
                break;
            }
        }
    }

    for (int i=0; i < m_edit_customer_id->count(); i++)
    {
        m_edit_customer_id->setCurrentIndex(i);
        if (m_edit_customer_id->currentData().toInt() == p->m_customer_id)
        {
            break;
        }
    }

    for (int i=0; i < m_edit_agreement_id->count(); i++)
    {
        m_edit_agreement_id->setCurrentIndex(i);
        if (m_edit_agreement_id->currentData().toInt() == p->m_agreement_id)
        {
            break;
        }
    }

    loadEmployees(p->m_employee_id);

    m_edit_invoice_date_value = p->m_invoice_date;
    m_edit_invoice_date->setText(m_edit_invoice_date_value.toString(DATE_FORMAT_STRING));

    m_edit_month->setCurrentIndex(p->m_month);

    m_edit_hours_standard->setText(hours_format_hundredths(p->m_hours_standard));
    m_edit_hours_overtime->setText(hours_format_hundredths(p->m_hours_overtime));
    m_edit_hours_overtime_qualified->setText(hours_format_hundredths(p->m_hours_qualified));
    m_edit_hours_traveltime->setText(hours_format_hundredths(p->m_hours_traveltime));
    m_edit_comment->setText(p->m_comment);

    m_stackedWidget->setCurrentIndex(1);
    updateButtons();
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::updateButtons(void)
{
    m_approve->setVisible(m_stackedWidget->currentIndex() == 0);
    m_approve->setDisabled(m_invoicedrafts->currentIndex().row() < 0);
    m_approve->setStyleSheet(m_invoicedrafts->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");

    m_preview->setVisible(m_stackedWidget->currentIndex() == 0);
    m_preview->setDisabled(m_invoicedrafts->currentIndex().row() < 0);
    m_preview->setStyleSheet(m_invoicedrafts->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");

    m_new->setVisible(m_stackedWidget->currentIndex() == 0);
    m_new->setEnabled(true);

    m_edit->setVisible(m_stackedWidget->currentIndex() == 0);
    m_edit->setDisabled(m_invoicedrafts->currentIndex().row() < 0);
    m_edit->setStyleSheet(m_invoicedrafts->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");

    m_delete->setVisible(m_stackedWidget->currentIndex() == 0);
    bool disable_delete = m_invoicedrafts->currentIndex().row() < 0;
    m_delete->setDisabled(disable_delete);
    m_delete->setStyleSheet(disable_delete ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");

    m_cancel->setVisible(m_stackedWidget->currentIndex() == 1);
    m_cancel->setEnabled(true);

    m_ok->setVisible(m_stackedWidget->currentIndex() == 1);
    m_ok->setEnabled(true);
}

//-----------------------------------------------------------------------------
bool InvoiceDraftTab::confirmAction(QWidget* parent, const QString& title, const QString& message)
{
    QMessageBox messageBox(parent);
    messageBox.setWindowTitle(title);
    messageBox.setText(message);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    messageBox.setDefaultButton(QMessageBox::No);
    messageBox.setIcon(QMessageBox::Question);

    // Qt's own translations are not shipped with the application, so Yes and No
    // would read in English. The buttons are named here instead.
    messageBox.button(QMessageBox::Yes)->setText("Ja");
    messageBox.button(QMessageBox::No)->setText("Nej");

    // This line makes the dialog modal, blocking the GUI until it's answered
    int result = messageBox.exec();

    return (result == QMessageBox::Yes);
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onStackedWidgetChanged(int index)
{
    if (index == 0)
    {
        if (m_invoiceModel != nullptr)
        {
            m_invoiceModel->load();
        }
    }
    emit editing(index > 0);
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onDoubleClicked(const QModelIndex &index)
{
    openEditWidget(index);
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onInvoiceSelected(const QModelIndex &index)
{
    updateButtons();
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onCurrentCustomerChanged(int index)
{
    if (m_edit_customer_id->currentIndex() >= 0)
    {
        int customerId = m_edit_customer_id->currentData().toInt();
        Customer* p_customer = DataBase::instance()->db()->m_customers.get(customerId);
        m_edit_agreement_id->clear();
        for (int i=0; i<p_customer->m_agreements.size(); i++)
        {
            if (p_customer->m_agreements.at(i)->m_active)
            {
                m_edit_agreement_id->addItem(QString("%1 - %2 (%3)").arg(p_customer->m_agreements.at(i)->m_agreement_id, p_customer->m_agreements.at(i)->m_agreement_name, p_customer->m_agreements.at(i)->m_reference_name), p_customer->m_agreements.at(i)->id());
            }
        }
    }
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onPickInvoiceDate(void)
{
    // Position dialog near the button
    QPoint globalPos = mapToGlobal(m_edit_invoice_date->pos());
    m_edit_invoice_date_dialog->move(globalPos + QPoint(0, m_edit_invoice_date->height()));
    m_edit_invoice_date_dialog->exec();
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onInvoiceDatePicked(const QDate &date)
{
    // Update pushbutton with selected date
    m_edit_invoice_date_value = date;
    m_edit_invoice_date->setText(m_edit_invoice_date_value.toString(DATE_FORMAT_STRING));
    m_edit_invoice_date_dialog->hide();
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onApprove(void)
{
    QModelIndex index = m_invoicedrafts->currentIndex();
    if (index.row() < 0)
    {
        updateButtons();
        return;
    }

    InvoiceDraft *p_draft = m_invoiceModel->getInvoice(index);
    if (p_draft == nullptr)
    {
        qDebug() << "Failed to get draft from model!!";
        return;
    }

    std::unique_ptr<Invoice> p_in = InvoiceFactory::create(p_draft);
    if (p_in == nullptr)
    {
        qDebug() << "Failed to convert invoice draft!!";
        QMessageBox::critical(this, "Fel", "Kunde inte skapa fakturan. Kontrollera att kund, avtal och konsult finns kvar.");
        return;
    }

    if (confirmAction(this, QString("Godkänn faktura"), QString("Vill du godkänna faktura nr %1 med datum %2?").arg(p_in->m_number, p_in->m_date)))
    {
        DataBase::instance()->db()->m_invoices.append(std::move(p_in));
        DataBase::instance()->db()->m_invoicedrafts.remove(p_draft);
        DataBase::instance()->setModified();
        m_invoiceModel->load();
    }
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onPreview(void)
{
    QModelIndex index = m_invoicedrafts->currentIndex();
    if (index.row() < 0)
    {
        updateButtons();
        return;
    }

    InvoiceDraft *p_draft = m_invoiceModel->getInvoice(index);
    if (p_draft == nullptr)
    {
        qDebug() << "Failed to get draft from model!!";
        return;
    }

    std::unique_ptr<Invoice> p_in = InvoiceFactory::create(p_draft);
    if (p_in == nullptr)
    {
        qDebug() << "Failed to convert invoice draft!!";
        QMessageBox::critical(this, "Fel", "Kunde inte skapa fakturan. Kontrollera att kund, avtal och konsult finns kvar.");
        return;
    }

    // Rendered into memory: a preview leaves no file behind.
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    InvoicePrinter::instance()->create_pdf(p_in.get(), DataBase::instance()->db()->m_companyinfo.m_logo, true, &buffer);

    pdf_show(buffer.data(), QString("Faktura %1 (utkast)").arg(p_in->m_number));
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onNew(void)
{
    openEditWidgetNew();
    m_stackedWidget->setCurrentIndex(1);
    updateButtons();
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onEdit(void)
{
    openEditWidget(m_invoicedrafts->currentIndex());
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onDelete(void)
{
    QModelIndex index = m_invoicedrafts->currentIndex();
    if (index.row() < 0)
    {
        updateButtons();
        return;
    }

    InvoiceDraft *p = m_invoiceModel->getInvoice(index);
    if (p == nullptr)
    {
        return;
    }

    if (confirmAction(this, QString("Ta bort fakturaunderlag"), QString("Vill du ta bort fakturaunderlaget med datum %1 för kund %2?").arg(p->m_invoice_date.toString(DATE_FORMAT_STRING)).arg(p->m_customer_id)))
    {
        DataBase::instance()->db()->m_invoicedrafts.remove(p);
        DataBase::instance()->setModified();
        m_invoiceModel->load();
    }
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onCancel(void)
{
    m_edit_customer_id->clear();
    m_edit_employee_id->clear();
    m_stackedWidget->setCurrentIndex(0);
    updateButtons();
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onOk(void)
{
    // Without a consultant the draft cannot become an invoice, so it is not
    // worth storing one that has none.
    if (m_edit_employee_id->currentIndex() < 0)
    {
        QMessageBox::warning(this, "Konsult saknas", "Välj en konsult. Finns ingen att välja, lägg upp den anställde på fliken Anställda först.");
        return;
    }

    InvoiceDraft d;
    d.setId(m_invoice_id);
    d.m_invoice_number = 0;
    d.m_invoice_year = QDate::currentDate().year() % 100;
    d.m_customer_id = m_edit_customer_id->currentData().toInt();
    d.m_agreement_id = m_edit_agreement_id->currentData().toInt();
    d.m_employee_id = m_edit_employee_id->currentData().toInt();
    d.m_invoice_date = m_edit_invoice_date_value;
    d.m_month = m_edit_month->currentIndex();
    d.m_hours_standard = hours_parse_hundredths(m_edit_hours_standard->text());
    d.m_hours_overtime = hours_parse_hundredths(m_edit_hours_overtime->text());
    d.m_hours_qualified = hours_parse_hundredths(m_edit_hours_overtime_qualified->text());
    d.m_hours_traveltime = hours_parse_hundredths(m_edit_hours_traveltime->text());
    d.m_comment = m_edit_comment->toPlainText();
    DataBase::instance()->db()->m_invoicedrafts.update(d);
    DataBase::instance()->setModified();

    m_invoiceModel->load();

    m_stackedWidget->setCurrentIndex(0);
    updateButtons();
}
