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

    QPushButton* m_save;
    QPushButton* m_view;

    void updateButtons(void);
    void view(const QModelIndex &index);

private slots:
    void onInvoiceSelected(const QModelIndex &index);
    void onDoubleClicked(const QModelIndex &index);

    void onSave(void);
    void onView(void);
};

#endif // INVOICETAB_H
