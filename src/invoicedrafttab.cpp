#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QBuffer>
#include <QFileDialog>
#include <QStandardPaths>

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

#define EXPENSE_COLUMN_TEXT     0
#define EXPENSE_COLUMN_AMOUNT   1

//-----------------------------------------------------------------------------
// Hours are typed in hours and may carry a fraction, Swedish style with a comma.
// setValidator() does not take ownership, so the validator is parented to the
// line edit it validates.
static QValidator* hoursValidator(QObject* parent)
{
    QDoubleValidator* v = new QDoubleValidator(0.0, 300.0, 2, parent);
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

        m_export = new QPushButton("Exportera");
        connect(m_export, &QPushButton::clicked, this, &InvoiceDraftTab::onExport);
        hbox->addWidget(m_export);

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
    m_edit_hours_standard->setValidator(hoursValidator(m_edit_hours_standard));
    m_lineedits.append(m_edit_hours_standard);

    labels.append("Övertid (tim)");
    m_edit_hours_overtime = new QLineEdit();
    m_edit_hours_overtime->setPlaceholderText("0-300");
    m_edit_hours_overtime->setValidator(hoursValidator(m_edit_hours_overtime));
    m_lineedits.append(m_edit_hours_overtime);

    labels.append("Övertid kvalificerad (tim)");
    m_edit_hours_overtime_qualified = new QLineEdit();
    m_edit_hours_overtime_qualified->setPlaceholderText("0-300");
    m_edit_hours_overtime_qualified->setValidator(hoursValidator(m_edit_hours_overtime_qualified));
    m_lineedits.append(m_edit_hours_overtime_qualified);

    labels.append("Restid (tim)");
    m_edit_hours_traveltime = new QLineEdit();
    m_edit_hours_traveltime->setPlaceholderText("0-300");
    m_edit_hours_traveltime->setValidator(hoursValidator(m_edit_hours_traveltime));
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

    // Costs bought in the company's name and passed on to the customer. Typed
    // without the moms the receipt carried, since that is the company's to
    // deduct, and invoiced with the moms of the consultancy on top.
    {
        m_edit_expenses = new QTableWidget(0, 2);
        m_edit_expenses->setHorizontalHeaderLabels({ "Benämning", "Belopp exkl. moms" });
        m_edit_expenses->horizontalHeader()->setSectionResizeMode(EXPENSE_COLUMN_TEXT, QHeaderView::Stretch);
        m_edit_expenses->horizontalHeader()->setSectionResizeMode(EXPENSE_COLUMN_AMOUNT, QHeaderView::ResizeToContents);
        m_edit_expenses->verticalHeader()->setVisible(false);
        m_edit_expenses->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_edit_expenses->setSelectionMode(QAbstractItemView::SingleSelection);
        m_edit_expenses->setMaximumHeight(160);
        m_edit_expenses->setToolTip("Kostnader som företaget har betalat i uppdraget och som kunden ska betala.\n"
                                    "Ange beloppet utan den moms som står på kvittot: den drar företaget av.\n"
                                    "Fakturan lägger på 25 % moms, som på arbetstiden.");

        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel("Vidarefakturering:");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight | Qt::AlignTop);
        // The table is no taller than its maximum, and a layout centres a
        // widget in a row taller than it; all three hang from the top of the
        // row instead, so the label, the table and the buttons start level.
        hbox->addWidget(p_label, 0, Qt::AlignTop);
        hbox->addWidget(m_edit_expenses, 0, Qt::AlignTop);

        QVBoxLayout *vbox = new QVBoxLayout;
        QPushButton* p_add = new QPushButton("Lägg till kostnad");
        connect(p_add, &QPushButton::clicked, this, &InvoiceDraftTab::onAddExpense);
        vbox->addWidget(p_add);
        QPushButton* p_remove = new QPushButton("Ta bort kostnad");
        connect(p_remove, &QPushButton::clicked, this, &InvoiceDraftTab::onRemoveExpense);
        vbox->addWidget(p_remove);
        vbox->addStretch(1);
        hbox->addLayout(vbox);

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
void InvoiceDraftTab::loadCustomers(int customer_id)
{
    Customers& customers = DataBase::instance()->db()->m_customers;

    m_edit_customer_id->clear();
    for (int i=0; i<customers.size(); i++)
    {
        Customer* c = customers.at(i);
        bool invoiced = c->id() == m_edit_keep_customer_id;
        for (int j=0; j<c->m_agreements.size() && !invoiced; j++)
        {
            invoiced = c->m_agreements.at(j)->m_active;
        }
        if (invoiced)
        {
            m_edit_customer_id->addItem(c->m_name, c->id());
        }
    }

    const int index = m_edit_customer_id->findData(customer_id);
    if (index >= 0 || m_edit_customer_id->count() > 0)
    {
        m_edit_customer_id->setCurrentIndex(index >= 0 ? index : 0);
    }
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
    m_edit_keep_customer_id = -1;
    m_edit_keep_agreement_id = -1;

    // The first customer, if any, so the agreement combo gets populated.
    loadCustomers(-1);

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
    setExpenses({});
    m_edit_comment->clear();
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::openEditWidget(const QModelIndex &index)
{
    // fetch the clicked invoice from the model behind the table view
    InvoiceDraft* p = m_invoiceModel->getInvoice(index);

    m_invoice_id = p->id();
    m_edit_keep_customer_id = p->m_customer_id;
    m_edit_keep_agreement_id = p->m_agreement_id;

    loadCustomers(p->m_customer_id);
    m_edit_agreement_id->setCurrentIndex(m_edit_agreement_id->findData(p->m_agreement_id));

    loadEmployees(p->m_employee_id);

    m_edit_invoice_date_value = p->m_invoice_date;
    m_edit_invoice_date->setText(m_edit_invoice_date_value.toString(DATE_FORMAT_STRING));

    m_edit_month->setCurrentIndex(p->m_month);

    m_edit_hours_standard->setText(hours_format_hundredths(p->m_hours_standard));
    m_edit_hours_overtime->setText(hours_format_hundredths(p->m_hours_overtime));
    m_edit_hours_overtime_qualified->setText(hours_format_hundredths(p->m_hours_qualified));
    m_edit_hours_traveltime->setText(hours_format_hundredths(p->m_hours_traveltime));
    setExpenses(p->m_expenses);
    m_edit_comment->setText(p->m_comment);

    m_stackedWidget->setCurrentIndex(1);
    updateButtons();
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::setExpenses(const QList<InvoiceExpense>& expenses)
{
    m_edit_expenses->setRowCount(0);
    for (const InvoiceExpense& expense : expenses)
    {
        const int row = m_edit_expenses->rowCount();
        m_edit_expenses->insertRow(row);
        m_edit_expenses->setItem(row, EXPENSE_COLUMN_TEXT, new QTableWidgetItem(expense.text));
        m_edit_expenses->setItem(row, EXPENSE_COLUMN_AMOUNT, new QTableWidgetItem(amount_format_ore(expense.amount)));
    }
}

//-----------------------------------------------------------------------------
bool InvoiceDraftTab::readExpenses(QList<InvoiceExpense>* expenses)
{
    QList<QStringList> rows;
    for (int row = 0; row < m_edit_expenses->rowCount(); row++)
    {
        QStringList cells;
        for (int column : { EXPENSE_COLUMN_TEXT, EXPENSE_COLUMN_AMOUNT })
        {
            QTableWidgetItem* p_item = m_edit_expenses->item(row, column);
            cells.append(p_item ? p_item->text() : QString());
        }
        rows.append(cells);
    }

    QString error;
    if (!invoice_parse_expenses(rows, expenses, &error))
    {
        QMessageBox::warning(this, "Fakturaunderlag", error);
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onAddExpense(void)
{
    if (m_edit_expenses->rowCount() >= INVOICE_MAX_EXPENSES)
    {
        QMessageBox::information(this, "Fakturaunderlag", QString("En faktura rymmer högst %1 kostnader.").arg(INVOICE_MAX_EXPENSES));
        return;
    }

    const int row = m_edit_expenses->rowCount();
    m_edit_expenses->insertRow(row);
    m_edit_expenses->setItem(row, EXPENSE_COLUMN_TEXT, new QTableWidgetItem());
    m_edit_expenses->setItem(row, EXPENSE_COLUMN_AMOUNT, new QTableWidgetItem());
    // What the cost was is the first thing to say, and what the customer reads.
    m_edit_expenses->setCurrentCell(row, EXPENSE_COLUMN_TEXT);
    m_edit_expenses->editItem(m_edit_expenses->item(row, EXPENSE_COLUMN_TEXT));
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onRemoveExpense(void)
{
    const int row = m_edit_expenses->currentRow();
    if (row >= 0)
    {
        m_edit_expenses->removeRow(row);
    }
}

//-----------------------------------------------------------------------------
bool InvoiceDraftTab::checkFits(Invoice* p_in, const QString& title)
{
    if (InvoicePrinter::instance()->fits(p_in))
    {
        return true;
    }
    QMessageBox::warning(this, title, "Fakturan får inte plats på en sida. Korta kommentaren eller ta bort kostnader.");
    return false;
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

    m_export->setVisible(m_stackedWidget->currentIndex() == 0);
    m_export->setDisabled(m_invoicedrafts->currentIndex().row() < 0);
    m_export->setStyleSheet(m_invoicedrafts->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");

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
    Q_UNUSED(index)
    updateButtons();
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onCurrentCustomerChanged(int index)
{
    Q_UNUSED(index)
    if (m_edit_customer_id->currentIndex() >= 0)
    {
        int customerId = m_edit_customer_id->currentData().toInt();
        Customer* p_customer = DataBase::instance()->db()->m_customers.get(customerId);
        m_edit_agreement_id->clear();
        for (int i=0; i<p_customer->m_agreements.size(); i++)
        {
            const bool kept = customerId == m_edit_keep_customer_id
                              && p_customer->m_agreements.at(i)->id() == m_edit_keep_agreement_id;
            if (p_customer->m_agreements.at(i)->m_active || kept)
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

    QString error;
    std::unique_ptr<Invoice> p_in = InvoiceFactory::create(p_draft, &error);
    if (p_in == nullptr)
    {
        qDebug() << "Failed to convert invoice draft!!";
        QMessageBox::critical(this, "Fel", "Kunde inte skapa fakturan. " + error);
        return;
    }
    if (!checkFits(p_in.get(), "Godkänn faktura"))
    {
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
std::unique_ptr<Invoice> InvoiceDraftTab::createSelectedInvoice(void)
{
    QModelIndex index = m_invoicedrafts->currentIndex();
    if (index.row() < 0)
    {
        updateButtons();
        return nullptr;
    }

    InvoiceDraft *p_draft = m_invoiceModel->getInvoice(index);
    if (p_draft == nullptr)
    {
        qDebug() << "Failed to get draft from model!!";
        return nullptr;
    }

    QString error;
    std::unique_ptr<Invoice> p_in = InvoiceFactory::create(p_draft, &error);
    if (p_in == nullptr)
    {
        qDebug() << "Failed to convert invoice draft!!";
        QMessageBox::critical(this, "Fel", "Kunde inte skapa fakturan. " + error);
        return nullptr;
    }
    if (!checkFits(p_in.get(), "Fakturaunderlag"))
    {
        return nullptr;
    }
    return p_in;
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onPreview(void)
{
    std::unique_ptr<Invoice> p_in = createSelectedInvoice();
    if (p_in == nullptr)
    {
        return;
    }

    // Rendered into memory: a preview leaves no file behind.
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    InvoicePrinter::instance()->create_pdf(p_in.get(), DataBase::instance()->db()->m_companyinfo.m_logo, true, &buffer);

    pdf_show(buffer.data(), QString("Faktura %1 (utkast)").arg(p_in->m_number));
}

//-----------------------------------------------------------------------------
void InvoiceDraftTab::onExport(void)
{
    std::unique_ptr<Invoice> p_in = createSelectedInvoice();
    if (p_in == nullptr)
    {
        return;
    }

    // Named like an exported invoice, with "utkast" in it: the number is only
    // what the invoice would get if approved now.
    QString invoiceType = p_in->m_is_credit ? "kreditfaktura" : "faktura";
    QString suggestion =
        QString("%1/%2-%3-utkast-%4-%5.pdf")
                             .arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                             p_in->m_number,
                             invoiceType,
                             p_in->m_company_name.toLower().split(" ").at(0),
                             p_in->m_customer_name.toLower().split(" ").at(0));
    QString pdf_file = QFileDialog::getSaveFileName(this, "Exportera fakturaunderlag", suggestion, "PDF-filer (*.pdf)");
    if (!pdf_file.isEmpty())
    {
        InvoicePrinter::instance()->create_pdf(p_in.get(), DataBase::instance()->db()->m_companyinfo.m_logo, true, pdf_file);
    }
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

    QList<InvoiceExpense> expenses;
    if (!readExpenses(&expenses))
    {
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
    d.m_expenses = expenses;
    DataBase::instance()->db()->m_invoicedrafts.update(d);
    DataBase::instance()->setModified();

    m_invoiceModel->load();

    m_stackedWidget->setCurrentIndex(0);
    updateButtons();
}
