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

#include <memory>

#include "data/invoice.h"
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
    QPushButton* m_export;
    QPushButton* m_new;
    QPushButton* m_edit;
    QPushButton* m_delete;
    QPushButton* m_cancel;
    QPushButton* m_ok;

    int m_invoice_id = -1;
    QComboBox* m_edit_customer_id;
    QComboBox* m_edit_agreement_id;
    QComboBox* m_edit_employee_id;
    QPushButton* m_edit_invoice_date;
    QDate m_edit_invoice_date_value;

    QDialog* m_edit_invoice_date_dialog;
    QCalendarWidget* m_edit_invoice_date_calendar;

    QComboBox* m_edit_month;

    QLineEdit* m_edit_hours_standard;
    QLineEdit* m_edit_hours_overtime;
    QLineEdit* m_edit_hours_overtime_qualified;
    QLineEdit* m_edit_hours_traveltime;
    // The costs passed on to the customer: Benämning and Belopp exkl. moms.
    QTableWidget* m_edit_expenses;
    QTextEdit* m_edit_comment;

    QList<QLineEdit*> m_lineedits;

    // The customer and the agreement of the draft being edited, or -1 for a
    // new one. They are offered even when they are no longer invoiced, as a
    // consultant who has left is: a draft keeps what it was written for.
    int m_edit_keep_customer_id = -1;
    int m_edit_keep_agreement_id = -1;

    QWidget* editWidgetCreate(void);
    // Fills the customer combo with the customers that have an active
    // agreement, plus the one being kept, and picks `customer_id`; which fills
    // the agreement combo through onCurrentCustomerChanged().
    void loadCustomers(int customer_id);
    // Fills the consultant combo with the employees still employed, and picks
    // `employee_id` if it is one of them.
    void loadEmployees(int employee_id);
    void openEditWidgetNew(void);
    void openEditWidget(const QModelIndex &index);
    void setExpenses(const QList<InvoiceExpense>& expenses);
    // Reads the cost table back. Tells the user what is wrong and returns false
    // if a row does not make sense.
    bool readExpenses(QList<InvoiceExpense>* expenses);
    // Whether `p_in` fits its page; says so, under `title`, when it does not.
    bool checkFits(Invoice* p_in, const QString& title);

    // The invoice the selected draft would become, not stored; nullptr, after
    // saying why, when there is none.
    std::unique_ptr<Invoice> createSelectedInvoice(void);

    void updateButtons(void);
    bool confirmAction(QWidget* parent, const QString& title, const QString& message);

private slots:
    void onStackedWidgetChanged(int index);
    void onDoubleClicked(const QModelIndex &index);
    void onInvoiceSelected(const QModelIndex &index);

    void onApprove(void);
    void onPreview(void);
    void onExport(void);
    void onNew(void);
    void onEdit(void);
    void onDelete(void);
    void onCancel(void);
    void onOk(void);

    void onCurrentCustomerChanged(int index);
    void onAddExpense(void);
    void onRemoveExpense(void);
    void onPickInvoiceDate(void);
    void onInvoiceDatePicked(const QDate &date);

};

#endif // INVOICEDRAFTTAB_H
