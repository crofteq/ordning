#ifndef EMPLOYEETAB_H
#define EMPLOYEETAB_H

#include <QCheckBox>
#include <QWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableView>
#include <QPushButton>
#include <QStackedWidget>

#include "employeetablemodel.h"

class EmployeeTab : public QWidget
{
    Q_OBJECT
public:
    explicit EmployeeTab(QWidget *parent = nullptr);

    void load(void);

public slots:
    void onDatabaseUpdated(void);

signals:
    void editing(bool editing);

private:
    QStackedWidget *m_stackedWidget;

    QTableView* m_employees;
    EmployeeTableModel* m_employeeModel;

    QPushButton* m_new;
    QPushButton* m_edit;
    QPushButton* m_delete;
    QPushButton* m_cancel;
    QPushButton* m_ok;

    int m_employee_id = -1;
    QLineEdit* m_first_name;
    QLineEdit* m_last_name;
    QLineEdit* m_adrs1;
    QLineEdit* m_adrs2;
    QLineEdit* m_employment_number;
    QLineEdit* m_personnummer;
    QLineEdit* m_bank_account;
    QCheckBox* m_active;
    QTableWidget* m_tax_tables;
    bool m_saveable = false;

    QList<QLineEdit*> m_lineedits;

    QWidget* editWidgetCreate(void);
    void openEditWidgetNew(void);
    void openEditWidget(const QModelIndex &index);
    void setTaxTables(const QMap<int, int>& tax_tables);
    bool readTaxTables(QMap<int, int>* tax_tables);

    void updateButtons(void);

    bool confirmAction(QWidget* parent, const QString& title, const QString& message);

private slots:
    void onStackedWidgetChanged(int index);
    void onDoubleClicked(const QModelIndex &index);
    void onEmployeeSelected(const QModelIndex &index);
    void onNew(void);
    void onEdit(void);
    void onDelete(void);
    void onCancel(void);
    void onOk(void);
    void onLineEditsEdited(void);
    void onAddTaxYear(void);
    void onRemoveTaxYear(void);
};

#endif // EMPLOYEETAB_H
