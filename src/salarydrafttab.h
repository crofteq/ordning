#ifndef SALARYDRAFTTAB_H
#define SALARYDRAFTTAB_H

#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableView>
#include <QTableWidget>
#include <QWidget>

#include "salarydrafttablemodel.h"

class SalaryDraftTab : public QWidget
{
    Q_OBJECT
public:
    explicit SalaryDraftTab(QWidget *parent = nullptr);

    void load(void);

public slots:
    void onDatabaseUpdated(void);

signals:
    void editing(bool editing);

private:
    QStackedWidget* m_stackedWidget;

    QTableView* m_drafts;
    SalaryDraftTableModel* m_draftModel;

    QPushButton* m_approve;
    QPushButton* m_preview;
    QPushButton* m_new;
    QPushButton* m_edit;
    QPushButton* m_delete;
    QPushButton* m_cancel;
    QPushButton* m_ok;

    int m_draft_id = -1;
    QComboBox* m_employee;
    QDateEdit* m_payment_date;
    QTableWidget* m_lines;
    QLabel* m_gross;
    QLabel* m_taxfree;

    QWidget* editWidgetCreate(void);
    void openEditWidget(const SalaryDraft& d);
    void fillEmployees(int selected_id);
    void setLines(const QList<SalaryLine>& lines);
    void addLineRow(const SalaryLine& line);
    QTableWidgetItem* lineTypeItemCreate(const SalaryLine& line);
    int lineTypeOf(int row) const;
    bool readLines(QList<SalaryLine>* lines);

    void updateButtons(void);
    bool confirmAction(QWidget* parent, const QString& title, const QString& message);

private slots:
    void onStackedWidgetChanged(int index);
    void onDoubleClicked(const QModelIndex &index);
    void onDraftSelected(const QModelIndex &index);
    void onApprove(void);
    void onPreview(void);
    void onNew(void);
    void onEdit(void);
    void onDelete(void);
    void onCancel(void);
    void onOk(void);
    void onAddLine(void);
    void onAddTraktamenteLine(void);
    void onPaymentDateChanged(void);
    void onRemoveLine(void);
    void onLinesChanged(void);
};

#endif // SALARYDRAFTTAB_H
