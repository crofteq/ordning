#ifndef AGREEMENTTAB_H
#define AGREEMENTTAB_H

#include <QCheckBox>
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableView>
#include <QPushButton>
#include <QStackedWidget>

#include "agreementtablemodel.h"

class AgreementTab : public QWidget
{
    Q_OBJECT
public:
    explicit AgreementTab(QWidget *parent = nullptr);

    void load(void);

public slots:
    void onDatabaseUpdated(void);

signals:
    void editing(bool editing);

private:
    QComboBox* m_customers;

    QStackedWidget *m_stackedWidget;

    QTableView* m_agreements;
    AgreementTableModel* m_agreementModel;

    QCheckBox* m_show_inactive;

    QPushButton* m_new;
    QPushButton* m_edit;
    QPushButton* m_delete;
    QPushButton* m_cancel;
    QPushButton* m_ok;

    int m_agreement_id = -1;
    QLineEdit* m_edit_name;
    QLineEdit* m_edit_agreementId;
    QLineEdit* m_edit_referenceName;
    QLineEdit* m_edit_referenceEmail;
    QLineEdit* m_edit_referencePhone;
    QLineEdit* m_edit_referenceNameOurs;
    QLineEdit* m_edit_hourlyRateStandard;
    QLineEdit* m_edit_hourlyRateOvertime;
    QLineEdit* m_edit_hourlyRateOvertimeQualified;
    QLineEdit* m_edit_hourlyRateTraveltime;
    QLineEdit* m_edit_paymentTermsDays;
    QLineEdit* m_edit_latePaymentInterest;
    QCheckBox* m_edit_activated;
    bool m_saveable = false;

    QList<QLineEdit*> m_lineedits;

    QWidget* editWidgetCreate(void);
    void openEditWidgetNew(void);
    void openEditWidget(const QModelIndex &index);

    void updateButtons(void);
    bool confirmAction(QWidget* parent, const QString& title, const QString& message);

private slots:
    void onStackedWidgetChanged(int index);
    void onDoubleClicked(const QModelIndex &index);
    void onAgreementSelected(const QModelIndex &index);

    void onCurrentCustomerChanged(int index);
    void onShowInactiveCheckStateChanged(int state);
    void onNew(void);
    void onEdit(void);
    void onDelete(void);
    void onCancel(void);
    void onOk(void);
    void onLineEditsEdited(void);
};

#endif // AGREEMENTTAB_H
