#ifndef INVOICETAB_H
#define INVOICETAB_H

#include <QWidget>

#include <QDialog>
#include <QCalendarWidget>
#include <QCheckBox>
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableView>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextEdit>

#include "invoicetablemodel.h"

class InvoiceTab : public QWidget
{
    Q_OBJECT
public:
    explicit InvoiceTab(QWidget *parent = nullptr);

    void load(void);

public slots:
    void onDatabaseUpdated(void);

private:
    QStackedWidget* m_stackedWidget;

    QTableView* m_invoices;
    InvoiceTableModel* m_invoiceModel;

    QPushButton* m_export;
    QPushButton* m_view;
    QPushButton* m_credit;

    void updateButtons(void);
    void view(const QModelIndex &index);

private slots:
    void onInvoiceSelected(const QModelIndex &index);
    void onDoubleClicked(const QModelIndex &index);

    void onExport(void);
    void onView(void);
    void onCredit(void);
};

#endif // INVOICETAB_H
