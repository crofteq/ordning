#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QDoubleValidator>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "database.h"

#include "util/amount.h"

#include "agreementtab.h"

#define LABEL_WIDTH 200

//-----------------------------------------------------------------------------
// Rates are typed in kronor and may carry ören, Swedish style with a comma.
static QValidator* rateValidator(void)
{
    QDoubleValidator* v = new QDoubleValidator(10.0, 20000.0, 2);
    v->setNotation(QDoubleValidator::StandardNotation);
    v->setLocale(QLocale(QLocale::Swedish, QLocale::Sweden));
    return v;
}


//-----------------------------------------------------------------------------
AgreementTab::AgreementTab(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    m_customers = new QComboBox();
    connect(m_customers, &QComboBox::currentIndexChanged, this, &AgreementTab::onCurrentCustomerChanged);
    layout->addWidget(m_customers);

    m_stackedWidget = new QStackedWidget();
    connect(m_stackedWidget, &QStackedWidget::currentChanged, this, &AgreementTab::onStackedWidgetChanged);

    // First stacked widget
    m_agreements = new QTableView();
    m_agreements->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_agreements->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_agreements, &QTableView::doubleClicked, this, &AgreementTab::onDoubleClicked);
    connect(m_agreements, &QTableView::clicked, this, &AgreementTab::onAgreementSelected);
    m_agreementModel = new AgreementTableModel(this);
    m_agreements->setModel(m_agreementModel);
    m_stackedWidget->addWidget(m_agreements);

    // Second stacked widget
    m_stackedWidget->addWidget(editWidgetCreate());
    m_stackedWidget->setCurrentIndex(0);

    // Add the stackedwidget
    layout->addWidget(m_stackedWidget);

    // Add the buttons at the bottom
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addStretch(1);

        m_show_inactive = new QCheckBox("Visa inaktiva avtal");
        connect(m_show_inactive, &QCheckBox::checkStateChanged, this, &AgreementTab::onShowInactiveCheckStateChanged);
        hbox->addWidget(m_show_inactive);

        m_new = new QPushButton("Ny");
        connect(m_new, &QPushButton::clicked, this, &AgreementTab::onNew);
        hbox->addWidget(m_new);

        m_edit = new QPushButton("Ändra");
        m_edit->setDisabled(true);
        connect(m_edit, &QPushButton::clicked, this, &AgreementTab::onEdit);
        hbox->addWidget(m_edit);

        m_delete = new QPushButton("Ta bort");
        m_delete->setDisabled(true);
        connect(m_delete, &QPushButton::clicked, this, &AgreementTab::onDelete);
        hbox->addWidget(m_delete);

        m_cancel = new QPushButton("Avbryt");
        connect(m_cancel, &QPushButton::clicked, this, &AgreementTab::onCancel);
        hbox->addWidget(m_cancel);

        m_ok = new QPushButton("Ok");
        m_ok->setDisabled(true);
        connect(m_ok, &QPushButton::clicked, this, &AgreementTab::onOk);
        hbox->addWidget(m_ok);

        updateButtons();
        layout->addLayout(hbox);
    }

    setLayout(layout);
}

//-----------------------------------------------------------------------------
void AgreementTab::load(void)
{
    Customers& customers = DataBase::instance()->db()->m_customers;

    m_customers->clear();
    for (int i=0; i<customers.size(); i++)
    {
        m_customers->addItem(customers.at(i)->m_name, customers.at(i)->id());
    }
    m_agreementModel->load(customers.size() == 0 ? -1 : customers.at(0)->id(), m_show_inactive->isChecked());
    m_agreements->resizeColumnsToContents();
    updateButtons();
}

//-----------------------------------------------------------------------------
void AgreementTab::onDatabaseUpdated(void)
{
    load();
}

//-----------------------------------------------------------------------------
QWidget* AgreementTab::editWidgetCreate(void)
{
    QWidget* p_widget = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    QList<QString> labels;

    labels.append("Namn");
    m_edit_name = new QLineEdit();
    m_lineedits.append(m_edit_name);

    labels.append("Avtalsnummer/id/datum");
    m_edit_agreementId = new QLineEdit();
    m_lineedits.append(m_edit_agreementId);

    labels.append("Kundens referens");
    m_edit_referenceName = new QLineEdit();
    m_lineedits.append(m_edit_referenceName);

    labels.append("Referens e-post");
    m_edit_referenceEmail = new QLineEdit();
    m_lineedits.append(m_edit_referenceEmail);

    labels.append("Referens telefon");
    m_edit_referencePhone = new QLineEdit();
    m_lineedits.append(m_edit_referencePhone);

    labels.append("Vår referens");
    m_edit_referenceNameOurs = new QLineEdit();
    m_lineedits.append(m_edit_referenceNameOurs);

    labels.append("Timpris ordinarie (kr)");
    m_edit_hourlyRateStandard = new QLineEdit();
    m_edit_hourlyRateStandard->setValidator(rateValidator());
    m_lineedits.append(m_edit_hourlyRateStandard);

    labels.append("Timpris övertid (kr)");
    m_edit_hourlyRateOvertime = new QLineEdit();
    m_edit_hourlyRateOvertime->setValidator(rateValidator());
    m_lineedits.append(m_edit_hourlyRateOvertime);

    labels.append("Timpris övertid kvalificerad (kr)");
    m_edit_hourlyRateOvertimeQualified = new QLineEdit();
    m_edit_hourlyRateOvertimeQualified->setValidator(rateValidator());
    m_lineedits.append(m_edit_hourlyRateOvertimeQualified);

    labels.append("Timpris restid (kr)");
    m_edit_hourlyRateTraveltime = new QLineEdit();
    m_edit_hourlyRateTraveltime->setValidator(rateValidator());
    m_lineedits.append(m_edit_hourlyRateTraveltime);

    labels.append("Betalningsvillkor (dagar)");
    m_edit_paymentTermsDays = new QLineEdit();
    m_edit_paymentTermsDays->setPlaceholderText("10-30");
    m_edit_paymentTermsDays->setValidator(new QIntValidator(10, 30));
    m_lineedits.append(m_edit_paymentTermsDays);

    labels.append("Dröjsmålsränta (%)");
    m_edit_latePaymentInterest = new QLineEdit();
    m_edit_latePaymentInterest->setPlaceholderText("0-30");
    m_edit_latePaymentInterest->setValidator(new QIntValidator(0, 30));
    m_lineedits.append(m_edit_latePaymentInterest);

    for(int i=0; i<labels.size(); i++)
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel(labels.at(i) + ":");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        hbox->addWidget(m_lineedits.at(i));
        layout->addLayout(hbox);
        connect(m_lineedits.at(i), &QLineEdit::textEdited, this, &AgreementTab::onLineEditsEdited);
    }

    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel("Aktiv:");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        m_edit_activated = new QCheckBox();
        hbox->addWidget(m_edit_activated);
        hbox->addStretch(1);
        layout->addLayout(hbox);
    }

    layout->addStretch(1);

    p_widget->setLayout(layout);

    return p_widget;
}

//-----------------------------------------------------------------------------
void AgreementTab::openEditWidgetNew(void)
{
    m_agreement_id = -1;
    for(int i=0; i<m_lineedits.size(); i++)
    {
        m_lineedits.at(i)->clear();
    }
    m_edit_activated->setChecked(true);
}

//-----------------------------------------------------------------------------
void AgreementTab::openEditWidget(const QModelIndex &index)
{
    // fetch the clicked agreement from the model behind the table view
    Agreement* p_agreeement = m_agreementModel->getAgreement(index);
    if (p_agreeement == nullptr)
    {
        return;
    }

    m_agreement_id = p_agreeement->id();
    m_edit_name->setText(p_agreeement->m_agreement_name);
    m_edit_agreementId->setText(p_agreeement->m_agreement_id);
    m_edit_referenceName->setText(p_agreeement->m_reference_name);
    m_edit_referenceEmail->setText(p_agreeement->m_reference_email);
    m_edit_referencePhone->setText(p_agreeement->m_reference_phone);
    m_edit_referenceNameOurs->setText(p_agreeement->m_reference_name_ours);
    m_edit_hourlyRateStandard->setText(amount_format_ore(p_agreeement->m_standard_hourly_rate));
    m_edit_hourlyRateOvertime->setText(amount_format_ore(p_agreeement->m_overtime_hourly_rate));
    m_edit_hourlyRateOvertimeQualified->setText(amount_format_ore(p_agreeement->m_qualified_hourly_rate));
    m_edit_hourlyRateTraveltime->setText(amount_format_ore(p_agreeement->m_traveltime_hourly_rate));
    m_edit_paymentTermsDays->setText(QString("%1").arg(p_agreeement->m_payment_terms_days));
    m_edit_latePaymentInterest->setText(QString("%1").arg(p_agreeement->m_late_payment_interest));
    m_edit_activated->setChecked(p_agreeement->m_active);
    onLineEditsEdited();

    m_stackedWidget->setCurrentIndex(1);
    updateButtons();
}

//-----------------------------------------------------------------------------
void AgreementTab::updateButtons(void)
{
    m_show_inactive->setVisible(m_stackedWidget->currentIndex() == 0);

    m_customers->setEnabled(m_stackedWidget->currentIndex() == 0);

    m_new->setVisible(m_stackedWidget->currentIndex() == 0);
    m_new->setEnabled(true);

    m_edit->setVisible(m_stackedWidget->currentIndex() == 0);
    m_edit->setDisabled(m_agreements->currentIndex().row() < 0);
    m_edit->setStyleSheet(m_agreements->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");

    m_delete->setVisible(m_stackedWidget->currentIndex() == 0);
    m_delete->setDisabled(m_agreements->currentIndex().row() < 0);
    m_delete->setStyleSheet(m_agreements->currentIndex().row() < 0 ? "QPushButton:disabled { color: gray; background-color: lightgray; }": "");

    m_cancel->setVisible(m_stackedWidget->currentIndex() == 1);
    m_cancel->setEnabled(true);

    m_ok->setVisible(m_stackedWidget->currentIndex() == 1);
    m_ok->setEnabled(m_saveable);
    m_ok->setStyleSheet(m_saveable ? "" : "QPushButton:disabled { color: gray; background-color: lightgray; }");
}

//-----------------------------------------------------------------------------
bool AgreementTab::confirmAction(QWidget* parent, const QString& title, const QString& message) {
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
void AgreementTab::onStackedWidgetChanged(int index)
{
    emit editing(index > 0);
}

//-----------------------------------------------------------------------------
void AgreementTab::onDoubleClicked(const QModelIndex &index)
{
    openEditWidget(index);
}

//-----------------------------------------------------------------------------
void AgreementTab::onAgreementSelected(const QModelIndex &index)
{
    updateButtons();
}

//-----------------------------------------------------------------------------
void AgreementTab::onCurrentCustomerChanged(int index)
{
    if (m_customers->count() == 0)
    {
        return;
    }
    m_agreementModel->load(m_customers->currentData().toInt(), m_show_inactive->isChecked());
}

//-----------------------------------------------------------------------------
void AgreementTab::onShowInactiveCheckStateChanged(int state)
{
    Q_UNUSED(state)
    m_agreementModel->load(m_customers->currentData().toInt(), m_show_inactive->isChecked());
}

//-----------------------------------------------------------------------------
void AgreementTab::onNew(void)
{
    openEditWidgetNew();
    m_stackedWidget->setCurrentIndex(1);
    updateButtons();
}

//-----------------------------------------------------------------------------
void AgreementTab::onEdit(void)
{
    openEditWidget(m_agreements->currentIndex());
}

//-----------------------------------------------------------------------------
void AgreementTab::onDelete(void)
{
    QModelIndex index = m_agreements->currentIndex();
    if (index.row() < 0)
    {
        updateButtons();
        return;
    }

    Agreement *p_agreement = m_agreementModel->getAgreement(index);
    if (p_agreement == nullptr)
    {
        return;
    }

    if (confirmAction(this, QString("Ta bort avtal"), QString("Vill du ta bort avtalet %1 - %2?").arg(p_agreement->m_agreement_name, p_agreement->m_agreement_id)))
    {
        int customerId = m_customers->currentData().toInt();
        Customer* p_customer = DataBase::instance()->db()->m_customers.get(customerId);
        if (p_customer == nullptr)
        {
            qWarning() << QString("The currentdata (%1) in combobox cannot be found in customers.").arg(customerId);
            return;
        }
        p_customer->deleteAgreement(p_agreement);
        DataBase::instance()->setModified();
        m_agreementModel->load(customerId, m_show_inactive->isChecked());
    }
}

//-----------------------------------------------------------------------------
void AgreementTab::onCancel(void)
{
    m_stackedWidget->setCurrentIndex(0);
    updateButtons();
}

//-----------------------------------------------------------------------------
void AgreementTab::onOk(void)
{
    Customer* p_customer = DataBase::instance()->db()->m_customers.get(m_customers->currentData().toInt());
    if (p_customer == nullptr)
    {
        qWarning() << QString("The currentdata (%1) in combobox cannot be found in customers.").arg(m_customers->currentData().toInt());
        QMessageBox::warning(this, "Åtgärden är inte tillåten", "Avtalet kan inte sparas: kunden finns inte kvar.");
        m_stackedWidget->setCurrentIndex(0);
        updateButtons();
        return;
    }

    Agreement a;
    a.setId(m_agreement_id);
    a.m_active = m_edit_activated->isChecked();
    a.m_agreement_name = m_edit_name->text();
    a.m_agreement_id = m_edit_agreementId->text();
    a.m_reference_name = m_edit_referenceName->text();
    a.m_reference_email = m_edit_referenceEmail->text();
    a.m_reference_phone = m_edit_referencePhone->text();
    a.m_reference_name_ours = m_edit_referenceNameOurs->text();
    a.m_standard_hourly_rate = amount_parse_ore(m_edit_hourlyRateStandard->text());
    a.m_overtime_hourly_rate = amount_parse_ore(m_edit_hourlyRateOvertime->text());
    a.m_qualified_hourly_rate = amount_parse_ore(m_edit_hourlyRateOvertimeQualified->text());
    a.m_traveltime_hourly_rate = amount_parse_ore(m_edit_hourlyRateTraveltime->text());
    a.m_payment_terms_days = m_edit_paymentTermsDays->text().toInt();
    a.m_late_payment_interest = m_edit_latePaymentInterest->text().toInt();
    p_customer->updateAgreement(a);
    DataBase::instance()->setModified();

    m_agreementModel->load(m_customers->currentData().toInt(), m_show_inactive->isChecked());

    m_stackedWidget->setCurrentIndex(0);
    updateButtons();
}

//-----------------------------------------------------------------------------
void AgreementTab::onLineEditsEdited(void)
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
