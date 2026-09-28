#ifndef CUSTOMERTAB_H
#define CUSTOMERTAB_H

#include <QCheckBox>
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableView>
#include <QPushButton>
#include <QStackedWidget>

#include "customertablemodel.h"

class CustomerTab : public QWidget
{
    Q_OBJECT
public:
    explicit CustomerTab(QWidget *parent = nullptr);

    void load(void);

public slots:
    void onDatabaseUpdated(void);

signals:
    void editing(bool editing);

private:
    QStackedWidget *m_stackedWidget;

    QTableView* m_customers;
    CustomerTableModel* m_customerModel;

    QPushButton* m_new;
    QPushButton* m_edit;
    QPushButton* m_delete;
    QPushButton* m_cancel;
    QPushButton* m_ok;

    int m_customer_id = -1;
    QLineEdit* m_name;
    QLineEdit* m_orgnr;
    QLineEdit* m_adrs1;
    QLineEdit* m_adrs2;
    QLineEdit* m_email;
    QLineEdit* m_email_invoice;
    bool m_saveable = false;

    QList<QLineEdit*> m_lineedits;

    QWidget* editWidgetCreate(void);
    void openEditWidgetNew(void);
    void openEditWidget(const QModelIndex &index);

    void updateButtons(void);

    bool confirmNoAction(QWidget* parent, const QString& title, const QString& message);
    bool confirmAction(QWidget* parent, const QString& title, const QString& message);

private slots:
    void onStackedWidgetChanged(int index);
    void onDoubleClicked(const QModelIndex &index);
    void onCustomerSelected(const QModelIndex &index);
    void onCustomerSaveable(bool saveable);
    void onNew(void);
    void onEdit(void);
    void onDelete(void);
    void onCancel(void);
    void onOk(void);
    void onLineEditsEdited(void);
};

#endif // CUSTOMERTAB_H
