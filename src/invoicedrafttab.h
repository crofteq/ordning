#ifndef INVOICEDRAFTTAB_H
#define INVOICEDRAFTTAB_H

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

#include "invoicedrafttablemodel.h"

class InvoiceDraftTab : public QWidget
{
    Q_OBJECT
public:
    explicit InvoiceDraftTab(QWidget *parent = nullptr);

    void load(void);

public slots:
    void onDatabaseUpdated(void);

signals:
    void editing(bool editing);

private:
    QStackedWidget* m_stackedWidget;

    QTableView* m_invoicedrafts;
    InvoiceDraftTableModel* m_invoiceModel;

    QPushButton* m_approve;
    QPushButton* m_preview;
    QPushButton* m_new;
    QPushButton* m_edit;
    QPushButton* m_delete;
    QPushButton* m_cancel;
    QPushButton* m_ok;

    int m_invoice_id = -1;
    QComboBox* m_edit_customer_id;
    QComboBox* m_edit_agreement_id;
    QPushButton* m_edit_invoice_date;
    QDate m_edit_invoice_date_value;

    QDialog* m_edit_invoice_date_dialog;
    QCalendarWidget* m_edit_invoice_date_calendar;

    QComboBox* m_edit_month;

    QLineEdit* m_edit_hours_standard;
    QLineEdit* m_edit_hours_overtime;
    QLineEdit* m_edit_hours_overtime_qualified;
    QLineEdit* m_edit_hours_traveltime;
    QTextEdit* m_edit_comment;

    QList<QLineEdit*> m_lineedits;

    QWidget* editWidgetCreate(void);
    void openEditWidgetNew(void);
    void openEditWidget(const QModelIndex &index);

    void updateButtons(void);
    bool confirmAction(QWidget* parent, const QString& title, const QString& message);

private slots:
    void onStackedWidgetChanged(int index);
    void onDoubleClicked(const QModelIndex &index);
    void onInvoiceSelected(const QModelIndex &index);

    void onApprove(void);
    void onPreview(void);
    void onNew(void);
    void onEdit(void);
    void onDelete(void);
    void onCancel(void);
    void onOk(void);

    void onCurrentCustomerChanged(int index);
    void onPickInvoiceDate(void);
    void onInvoiceDatePicked(const QDate &date);

};

#endif // INVOICEDRAFTTAB_H
