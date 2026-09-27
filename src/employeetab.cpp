#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "data/database.h"
#include "util/personnummer.h"

#include "employeetab.h"

#define LABEL_WIDTH 140

// Columns of the tax-table list on the edit page.
#define TAX_COLUMN_YEAR     0
#define TAX_COLUMN_TABLE    1

//-----------------------------------------------------------------------------
EmployeeTab::EmployeeTab(QWidget *parent)
    : QWidget{parent}
{
    m_stackedWidget = new QStackedWidget();
    connect(m_stackedWidget, &QStackedWidget::currentChanged, this, &EmployeeTab::onStackedWidgetChanged);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    // First stacked widget
    m_employees = new QTableView();
    m_employees->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_employees->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_employees, &QTableView::doubleClicked, this, &EmployeeTab::onDoubleClicked);
    connect(m_employees, &QTableView::clicked, this, &EmployeeTab::onEmployeeSelected);
    m_employeeModel = new EmployeeTableModel(this);
    m_employees->setModel(m_employeeModel);
    m_stackedWidget->addWidget(m_employees);

    // Second stacked widget
    m_stackedWidget->addWidget(editWidgetCreate());

    m_stackedWidget->setCurrentIndex(0);

    // Add the stackedwidget
    layout->addWidget(m_stackedWidget);

    // Add the buttons at the bottom
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addStretch(1);
        m_new = new QPushButton("New");
        connect(m_new, &QPushButton::clicked, this, &EmployeeTab::onNew);
        hbox->addWidget(m_new);

        m_edit = new QPushButton("Edit");
        m_edit->setDisabled(true);
        connect(m_edit, &QPushButton::clicked, this, &EmployeeTab::onEdit);
        hbox->addWidget(m_edit);

        m_delete = new QPushButton("Delete");
        m_delete->setDisabled(true);
        connect(m_delete, &QPushButton::clicked, this, &EmployeeTab::onDelete);
        hbox->addWidget(m_delete);

        m_cancel = new QPushButton("Cancel");
        connect(m_cancel, &QPushButton::clicked, this, &EmployeeTab::onCancel);
        hbox->addWidget(m_cancel);

        m_ok = new QPushButton("Ok");
        m_ok->setDisabled(true);
        connect(m_ok, &QPushButton::clicked, this, &EmployeeTab::onOk);
        hbox->addWidget(m_ok);

        updateButtons();
        layout->addLayout(hbox);
    }

    setLayout(layout);
}

//-----------------------------------------------------------------------------
void EmployeeTab::load(void)
{
    m_employeeModel->load();
    m_employees->resizeColumnsToContents();
    updateButtons();
}

//-----------------------------------------------------------------------------
void EmployeeTab::onDatabaseUpdated(void)
{
    load();
}

//-----------------------------------------------------------------------------
QWidget* EmployeeTab::editWidgetCreate(void)
{
    QWidget* p_widget = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    QList<QString> labels;

    labels.append("First name");
    m_first_name = new QLineEdit();
    m_lineedits.append(m_first_name);

    labels.append("Last name");
    m_last_name = new QLineEdit();
    m_lineedits.append(m_last_name);

    labels.append("Address line 1");
    m_adrs1 = new QLineEdit();
    m_lineedits.append(m_adrs1);

    labels.append("Address line 2");
    m_adrs2 = new QLineEdit();
    m_lineedits.append(m_adrs2);

    // Assigned from the id when the employee is first stored, so it is shown
    // but cannot be edited.
    labels.append("Employment number");
    m_employment_number = new QLineEdit();
    m_employment_number->setReadOnly(true);
    m_employment_number->setPlaceholderText("Assigned when saved");
    m_lineedits.append(m_employment_number);

    labels.append("Personnummer");
    m_personnummer = new QLineEdit();
    m_personnummer->setPlaceholderText("YYMMDD-XXXX");
    m_lineedits.append(m_personnummer);

    labels.append("Bank account");
    m_bank_account = new QLineEdit();
    m_lineedits.append(m_bank_account);

    for(int i=0; i<labels.size(); i++)
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel(labels.at(i) + ":");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        hbox->addWidget(m_lineedits.at(i));
        connect(m_lineedits.at(i), &QLineEdit::textEdited, this, &EmployeeTab::onLineEditsEdited);
        layout->addLayout(hbox);
    }

    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel("Active:");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        m_active = new QCheckBox();
        hbox->addWidget(m_active);
        hbox->addStretch(1);
        layout->addLayout(hbox);
    }

    // The skattetabell per year, from the employee's skattebesked. Edited in
    // place.
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel("Skattetabell per year:");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight | Qt::AlignTop);
        hbox->addWidget(p_label);

        m_tax_tables = new QTableWidget(0, 2);
        m_tax_tables->setHorizontalHeaderLabels({ "Year", "Skattetabell" });
        m_tax_tables->horizontalHeader()->setStretchLastSection(true);
        m_tax_tables->verticalHeader()->setVisible(false);
        m_tax_tables->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tax_tables->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tax_tables->setMaximumHeight(160);
        hbox->addWidget(m_tax_tables);

        QVBoxLayout *vbox = new QVBoxLayout;
        QPushButton* p_add = new QPushButton("Add year");
        connect(p_add, &QPushButton::clicked, this, &EmployeeTab::onAddTaxYear);
        vbox->addWidget(p_add);
        QPushButton* p_remove = new QPushButton("Remove year");
        connect(p_remove, &QPushButton::clicked, this, &EmployeeTab::onRemoveTaxYear);
        vbox->addWidget(p_remove);
        vbox->addStretch(1);
        hbox->addLayout(vbox);

        layout->addLayout(hbox);
    }

    layout->addStretch(1);

    p_widget->setLayout(layout);

    return p_widget;
}

//-----------------------------------------------------------------------------
void EmployeeTab::setTaxTables(const QMap<int, int>& tax_tables)
{
    m_tax_tables->setRowCount(0);
    for (auto it = tax_tables.constBegin(); it != tax_tables.constEnd(); ++it)
    {
        int row = m_tax_tables->rowCount();
        m_tax_tables->insertRow(row);
        m_tax_tables->setItem(row, TAX_COLUMN_YEAR, new QTableWidgetItem(QString::number(it.key())));
        m_tax_tables->setItem(row, TAX_COLUMN_TABLE, new QTableWidgetItem(QString::number(it.value())));
    }
}

//-----------------------------------------------------------------------------
// Reads the tax-table list back. Tells the user what is wrong and returns
// false if a row does not make sense.
bool EmployeeTab::readTaxTables(QMap<int, int>* tax_tables)
{
    QList<QPair<QString, QString>> rows;
    for (int row = 0; row < m_tax_tables->rowCount(); row++)
    {
        QTableWidgetItem* p_year = m_tax_tables->item(row, TAX_COLUMN_YEAR);
        QTableWidgetItem* p_table = m_tax_tables->item(row, TAX_COLUMN_TABLE);
        rows.append({ p_year ? p_year->text() : QString(), p_table ? p_table->text() : QString() });
    }

    QString error;
    if (!employee_parse_tax_tables(rows, tax_tables, &error))
    {
        QMessageBox::warning(this, "Skattetabell per year", error);
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
void EmployeeTab::openEditWidgetNew(void)
{
    m_employee_id = -1;
    for(int i=0; i<m_lineedits.size(); i++)
    {
        m_lineedits.at(i)->clear();
    }
    m_active->setChecked(true);
    setTaxTables(QMap<int, int>());
    onLineEditsEdited();

    m_stackedWidget->setCurrentIndex(1);
    updateButtons();
}

//-----------------------------------------------------------------------------
void EmployeeTab::openEditWidget(const QModelIndex &index)
{
    Employee* p_employee = m_employeeModel->getEmployee(index);
    if (p_employee == nullptr)
    {
        return;
    }

    m_employee_id = p_employee->id();
    m_first_name->setText(p_employee->m_first_name);
    m_last_name->setText(p_employee->m_last_name);
    m_adrs1->setText(p_employee->m_adrsline1);
    m_adrs2->setText(p_employee->m_adrsline2);
    m_employment_number->setText(p_employee->employmentNumber());
    m_personnummer->setText(p_employee->m_personnummer);
    m_bank_account->setText(p_employee->m_bank_account);
    m_active->setChecked(p_employee->m_active);
    setTaxTables(p_employee->m_tax_tables);
    onLineEditsEdited();

    m_stackedWidget->setCurrentIndex(1);
    updateButtons();
}

//-----------------------------------------------------------------------------
void EmployeeTab::updateButtons(void)
{
    m_new->setVisible(m_stackedWidget->currentIndex() == 0);
    m_new->setEnabled(true);

    m_edit->setVisible(m_stackedWidget->currentIndex() == 0);
    m_edit->setDisabled(m_employees->currentIndex().row() < 0);
    m_edit->setStyleSheet(m_employees->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");

    m_delete->setVisible(m_stackedWidget->currentIndex() == 0);
    m_delete->setDisabled(m_employees->currentIndex().row() < 0);
    m_delete->setStyleSheet(m_employees->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");

    m_cancel->setVisible(m_stackedWidget->currentIndex() == 1);
    m_cancel->setEnabled(true);

    m_ok->setVisible(m_stackedWidget->currentIndex() == 1);
    m_ok->setEnabled(m_saveable);
    m_ok->setStyleSheet(m_saveable ? "" : "QPushButton:disabled { color: gray; background-color: lightgray; }");
}

//-----------------------------------------------------------------------------
bool EmployeeTab::confirmAction(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox messageBox(parent);
    messageBox.setWindowTitle(title);
    messageBox.setText(message);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    messageBox.setDefaultButton(QMessageBox::No);
    messageBox.setIcon(QMessageBox::Question);

    // This line makes the dialog modal, blocking the GUI until it's answered
    int result = messageBox.exec();

    return (result == QMessageBox::Yes);
}

//-----------------------------------------------------------------------------
void EmployeeTab::onStackedWidgetChanged(int index)
{
    emit editing(index > 0);
}

//-----------------------------------------------------------------------------
void EmployeeTab::onDoubleClicked(const QModelIndex &index)
{
    openEditWidget(index);
}

//-----------------------------------------------------------------------------
void EmployeeTab::onEmployeeSelected(const QModelIndex &index)
{
    Q_UNUSED(index)
    updateButtons();
}

//-----------------------------------------------------------------------------
void EmployeeTab::onNew(void)
{
    openEditWidgetNew();
}

//-----------------------------------------------------------------------------
void EmployeeTab::onEdit(void)
{
    openEditWidget(m_employees->currentIndex());
}

//-----------------------------------------------------------------------------
void EmployeeTab::onDelete(void)
{
    QModelIndex index = m_employees->currentIndex();
    if (index.row() < 0)
    {
        updateButtons();
        return;
    }

    Employee *p_employee = m_employeeModel->getEmployee(index);
    if (p_employee == nullptr)
    {
        return;
    }

    if (DataBase::instance()->db()->employeeInUse(p_employee->id()))
    {
        QMessageBox::information(this, "Action not allowed!", "Cannot delete an employee who has salary drafts or salary slips. Clear Active instead.");
        return;
    }

    if (confirmAction(this, QString("Delete employee"), QString("Do you want to delete employee %1 (%2)?").arg(p_employee->name(), p_employee->employmentNumber())))
    {
        DataBase::instance()->db()->m_employees.remove(p_employee);
        DataBase::instance()->setModified();
        m_employeeModel->load();
        m_employees->resizeColumnsToContents();
        updateButtons();
    }
}

//-----------------------------------------------------------------------------
void EmployeeTab::onCancel(void)
{
    m_stackedWidget->setCurrentIndex(0);
    updateButtons();
}

//-----------------------------------------------------------------------------
void EmployeeTab::onOk(void)
{
    Employee e;

    if (!readTaxTables(&e.m_tax_tables))
    {
        return;
    }

    e.setId(m_employee_id);
    e.m_first_name = m_first_name->text().trimmed();
    e.m_last_name = m_last_name->text().trimmed();
    e.m_adrsline1 = m_adrs1->text();
    e.m_adrsline2 = m_adrs2->text();
    e.m_personnummer = personnummer_normalize(m_personnummer->text());
    e.m_bank_account = m_bank_account->text().trimmed();
    e.m_active = m_active->isChecked();

    DataBase::instance()->db()->m_employees.update(e);
    DataBase::instance()->setModified();
    m_employeeModel->load();
    m_employees->resizeColumnsToContents();
    m_stackedWidget->setCurrentIndex(0);
    updateButtons();
}

//-----------------------------------------------------------------------------
void EmployeeTab::onLineEditsEdited(void)
{
    // What the salary statement cannot do without; the rest may be filled
    // in later.
    Employee e;
    e.m_first_name = m_first_name->text();
    e.m_last_name = m_last_name->text();
    e.m_personnummer = m_personnummer->text();
    const bool pnr_ok = personnummer_is_valid(m_personnummer->text());
    m_personnummer->setStyleSheet(pnr_ok || m_personnummer->text().isEmpty() ? "" : "QLineEdit { color: red; }");

    m_saveable = employee_is_complete(e);
    updateButtons();
}

//-----------------------------------------------------------------------------
void EmployeeTab::onAddTaxYear(void)
{
    // Suggest the year after the latest one listed, but not a past year, and
    // carry its table over since it rarely changes.
    int latest = 0;
    int table = 0;
    for (int row = 0; row < m_tax_tables->rowCount(); row++)
    {
        QTableWidgetItem* p_year = m_tax_tables->item(row, TAX_COLUMN_YEAR);
        QTableWidgetItem* p_table = m_tax_tables->item(row, TAX_COLUMN_TABLE);
        if (p_year && p_year->text().toInt() > latest)
        {
            latest = p_year->text().toInt();
            table = p_table ? p_table->text().toInt() : 0;
        }
    }
    const int year = qMax(QDate::currentDate().year(), latest + 1);

    int row = m_tax_tables->rowCount();
    m_tax_tables->insertRow(row);
    m_tax_tables->setItem(row, TAX_COLUMN_YEAR, new QTableWidgetItem(QString::number(year)));
    m_tax_tables->setItem(row, TAX_COLUMN_TABLE, new QTableWidgetItem(table > 0 ? QString::number(table) : QString()));
    m_tax_tables->setCurrentCell(row, TAX_COLUMN_TABLE);
    m_tax_tables->editItem(m_tax_tables->item(row, TAX_COLUMN_TABLE));
}

//-----------------------------------------------------------------------------
void EmployeeTab::onRemoveTaxYear(void)
{
    int row = m_tax_tables->currentRow();
    if (row >= 0)
    {
        m_tax_tables->removeRow(row);
    }
}
