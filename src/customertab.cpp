#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "data/database.h"

#include "customertab.h"

#define LABEL_WIDTH 140

//-----------------------------------------------------------------------------
CustomerTab::CustomerTab(QWidget *parent)
    : QWidget{parent}
{
    m_stackedWidget = new QStackedWidget();
    connect(m_stackedWidget, &QStackedWidget::currentChanged, this, &CustomerTab::onStackedWidgetChanged);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    // First stacked widget
    m_customers = new QTableView();
    m_customers->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_customers->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_customers, &QTableView::doubleClicked, this, &CustomerTab::onDoubleClicked);
    connect(m_customers, &QTableView::clicked, this, &CustomerTab::onCustomerSelected);
    m_customerModel  = new CustomerTableModel(this);
    m_customers->setModel(m_customerModel);
    m_stackedWidget->addWidget(m_customers);

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
        connect(m_new, &QPushButton::clicked, this, &CustomerTab::onNew);
        hbox->addWidget(m_new);

        m_edit = new QPushButton("Edit");
        m_edit->setDisabled(true);
        connect(m_edit, &QPushButton::clicked, this, &CustomerTab::onEdit);
        hbox->addWidget(m_edit);

        m_delete = new QPushButton("Delete");
        m_delete->setDisabled(true);
        connect(m_delete, &QPushButton::clicked, this, &CustomerTab::onDelete);
        hbox->addWidget(m_delete);

        m_cancel = new QPushButton("Cancel");
        connect(m_cancel, &QPushButton::clicked, this, &CustomerTab::onCancel);
        hbox->addWidget(m_cancel);

        m_ok = new QPushButton("Ok");
        m_ok->setDisabled(true);
        connect(m_ok, &QPushButton::clicked, this, &CustomerTab::onOk);
        hbox->addWidget(m_ok);

        updateButtons();
        layout->addLayout(hbox);
    }

    setLayout(layout);
}

//-----------------------------------------------------------------------------
void CustomerTab::load(void)
{
    m_customerModel->load();
    m_customers->resizeColumnsToContents();
    updateButtons();
}

//-----------------------------------------------------------------------------
void CustomerTab::onDatabaseUpdated(void)
{
    load();
}

//-----------------------------------------------------------------------------
QWidget* CustomerTab::editWidgetCreate(void)
{
    QWidget* p_widget = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    QList<QString> labels;

    labels.append("Name");
    m_name = new QLineEdit();
    m_lineedits.append(m_name);

    labels.append("Address line 1");
    m_adrs1 = new QLineEdit();
    m_lineedits.append(m_adrs1);

    labels.append("Address line 2");
    m_adrs2 = new QLineEdit();
    m_lineedits.append(m_adrs2);

    labels.append("Orgnummer");
    m_orgnr = new QLineEdit();
    m_lineedits.append(m_orgnr);

    labels.append("Email");
    m_email = new QLineEdit();
    m_lineedits.append(m_email);

    labels.append("Email (invoice recipient)");
    m_email_invoice = new QLineEdit();
    m_lineedits.append(m_email_invoice);

    for(int i=0; i<labels.size(); i++)
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel(labels.at(i) + ":");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        hbox->addWidget(m_lineedits.at(i));
        connect(m_lineedits.at(i), &QLineEdit::textEdited, this, &CustomerTab::onLineEditsEdited);
        layout->addLayout(hbox);
    }

    layout->addStretch(1);

    p_widget->setLayout(layout);

    return p_widget;
}

//-----------------------------------------------------------------------------
void CustomerTab::openEditWidgetNew(void)
{
    m_customer_id = -1;
    for(int i=0; i<m_lineedits.size(); i++)
    {
        m_lineedits.at(i)->clear();
    }
    onLineEditsEdited();

    m_stackedWidget->setCurrentIndex(1);
    updateButtons();
}

//-----------------------------------------------------------------------------
void CustomerTab::openEditWidget(const QModelIndex &index)
{
    // fetch the clicked customer from the model behind the table view
    Customer* p_customer = m_customerModel->getCustomer(index);
    if (p_customer == nullptr)
    {
        return;
    }

    m_customer_id = p_customer->id();
    m_name->setText(p_customer->m_name);
    m_orgnr->setText(p_customer->m_org_number);
    m_adrs1->setText(p_customer->m_adrsline1);
    m_adrs2->setText(p_customer->m_adrsline2);
    m_email->setText(p_customer->m_email);
    m_email_invoice->setText(p_customer->m_email_invoice);
    onLineEditsEdited();

    m_stackedWidget->setCurrentIndex(1);
    updateButtons();
}

//-----------------------------------------------------------------------------
void CustomerTab::updateButtons(void)
{
    m_new->setVisible(m_stackedWidget->currentIndex() == 0);
    m_new->setEnabled(true);

    m_edit->setVisible(m_stackedWidget->currentIndex() == 0);
    m_edit->setDisabled(m_customers->currentIndex().row() < 0);
    m_edit->setStyleSheet(m_customers->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");

    m_delete->setVisible(m_stackedWidget->currentIndex() == 0);
    m_delete->setDisabled(m_customers->currentIndex().row() < 0);
    m_delete->setStyleSheet(m_customers->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");

    m_cancel->setVisible(m_stackedWidget->currentIndex() == 1);
    m_cancel->setEnabled(true);

    m_ok->setVisible(m_stackedWidget->currentIndex() == 1);
    m_ok->setEnabled(m_saveable);
    m_ok->setStyleSheet(m_saveable ? "" : "QPushButton:disabled { color: gray; background-color: lightgray; }");
}

//-----------------------------------------------------------------------------
bool CustomerTab::confirmNoAction(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox messageBox(parent);
    messageBox.setWindowTitle(title);
    messageBox.setText(message);
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.setDefaultButton(QMessageBox::Ok);
    messageBox.setIcon(QMessageBox::Information);

    // This line makes the dialog modal, blocking the GUI until it's answered
    int result = messageBox.exec();

    return (result == QMessageBox::Ok);
}

//-----------------------------------------------------------------------------
bool CustomerTab::confirmAction(QWidget* parent, const QString& title, const QString& message) {
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
void CustomerTab::onStackedWidgetChanged(int index)
{
    emit editing(index > 0);
}

//-----------------------------------------------------------------------------
void CustomerTab::onDoubleClicked(const QModelIndex &index)
{
    openEditWidget(index);
}

//-----------------------------------------------------------------------------
void CustomerTab::onCustomerSelected(const QModelIndex &index)
{
    Q_UNUSED(index)
    updateButtons();
}

//-----------------------------------------------------------------------------
void CustomerTab::onCustomerSaveable(bool saveable)
{
    m_ok->setEnabled(saveable);
}

//-----------------------------------------------------------------------------
void CustomerTab::onNew(void)
{
    openEditWidgetNew();
}

//-----------------------------------------------------------------------------
void CustomerTab::onEdit(void)
{
    openEditWidget(m_customers->currentIndex());
}

//-----------------------------------------------------------------------------
void CustomerTab::onDelete(void)
{
    QModelIndex index = m_customers->currentIndex();
    if (index.row() < 0)
    {
        updateButtons();
        return;
    }

    Customer *p_customer = m_customerModel->getCustomer(index);
    if (p_customer == nullptr)
    {
        return;
    }

    if (p_customer->m_agreements.size() > 0)
    {
        confirmNoAction(this, QString("Action not allowed!"), QString("Cannot delete customer with agreements."));
        return;
    }

    if (confirmAction(this, QString("Delete customer"), QString("Do you want to delete customer %1 (%2)?").arg(p_customer->m_name, p_customer->m_org_number)))
    {
        DataBase::instance()->db()->m_customers.remove(p_customer);
        DataBase::instance()->setModified();
        m_customerModel->load();
        m_customers->resizeColumnsToContents();
    }
}

//-----------------------------------------------------------------------------
void CustomerTab::onCancel(void)
{
    m_stackedWidget->setCurrentIndex(0);
    updateButtons();
}

//-----------------------------------------------------------------------------
void CustomerTab::onOk(void)
{
    Customer c;

    c.setId(m_customer_id);
    c.m_name = m_name->text();
    c.m_org_number = m_orgnr->text();
    c.m_adrsline1 = m_adrs1->text();
    c.m_adrsline2 = m_adrs2->text();
    c.m_email = m_email->text();
    c.m_email_invoice = m_email_invoice->text();

    DataBase::instance()->db()->m_customers.update(c);
    DataBase::instance()->setModified();
    m_customerModel->load();
    m_customers->resizeColumnsToContents();
    m_stackedWidget->setCurrentIndex(0);
    updateButtons();
}

//-----------------------------------------------------------------------------
void CustomerTab::onLineEditsEdited(void)
{
    bool saveable = true;

    // something in all fields
    for(int i=0; i<m_lineedits.size(); i++)
    {
        if (m_lineedits.at(i)->text().isEmpty())
        {
            saveable = false;
            break;
        }
    }
    m_saveable = saveable;
    updateButtons();
}
