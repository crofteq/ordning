#ifndef COMPANYTAB_H
#define COMPANYTAB_H

#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QPair>
#include <QTableView>
#include <QTableWidget>

#include "taxtableyearsmodel.h"

class CompanyTab : public QWidget
{
    Q_OBJECT
public:
    explicit CompanyTab(QWidget *parent = nullptr);

public slots:
    void onDatabaseUpdated(void);

signals:

private:
    bool m_edit = false;

    QLineEdit* m_name;
    QLineEdit* m_adrs1;
    QLineEdit* m_adrs2;
    QLineEdit* m_orgnr;
    QLineEdit* m_momsregnr;
    QLineEdit* m_bankgiro;
    QLineEdit* m_swiftbic;
    QLineEdit* m_iban;
    QLineEdit* m_phone;
    QLineEdit* m_email;
    QLineEdit* m_web;
    QComboBox* m_fiscal_year;
    QString m_logo;

    QList<QLineEdit*> lineedits;

    QPushButton* m_logoselection;

    QPushButton* m_cancel;
    QPushButton* m_ok;

    QTableView* m_taxtables;
    TaxTableYearsModel* m_taxtableModel;
    QPushButton* m_taxtable_remove;

    QTableWidget* m_fees;
    QPushButton* m_fee_add;
    QPushButton* m_fee_remove;

    QTableWidget* m_trakt_inrikes;
    QPushButton* m_trakt_inrikes_add;
    QPushButton* m_trakt_inrikes_remove;

    QTableWidget* m_trakt_utrikes;
    QPushButton* m_trakt_utrikes_remove;

    QGroupBox* fieldGroup(const QString& title, const QList<QPair<QString, QLineEdit**>>& fields);
    void setLogo(QString svgString);
    void updateTaxTableYears(void);
    void updateTaxTableButtons(void);
    void setFees(void);
    bool readFees(void);
    void setInrikesTraktamente(void);
    bool readInrikesTraktamente(void);
    void updateUtrikesTraktamente(void);
    void updateUtrikesTraktamenteButtons(void);
    void updateButtons(void);

private slots:
    void onSelectLogo(bool clicked);
    void onEditSave(bool clicked);
    void onCancel(bool clicked);
    void onTaxTableSelected(const QModelIndex &index);
    void onImportTaxTable(bool clicked);
    void onAddFeeYear(void);
    void onRemoveFeeYear(void);
    void onRemoveTaxTable(bool clicked);
    void onAddTraktamenteYear(void);
    void onRemoveTraktamenteYear(void);
    void onImportTraktamente(bool clicked);
    void onRemoveUtrikesTraktamente(bool clicked);
    void onUtrikesTraktamenteSelected(void);
};

#endif // COMPANYTAB_H
