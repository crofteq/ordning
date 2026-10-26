#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QActionGroup>
#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QMenuBar>
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include "aboutdialog.h"
#include "agreementtab.h"
#include "companytab.h"
#include "customertab.h"
#include "employeetab.h"
#include "invoicedrafttab.h"
#include "invoicetab.h"
#include "salarydrafttab.h"
#include "reportstab.h"
#include "salarysliptab.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onTabSelected(int current);
    void onTabEditing(bool editing);
    void updateWindowTitle();

    void newFile();
    void open();
    bool save();
    bool saveAs();
    void about();

private:
    QString m_settingsFile;
    QString m_dbfilename;

    void createActions();
    void createMenus();

    // Remembers `filename` as the business currently being edited, both in the
    // window title and in the settings used to reopen it on the next start.
    void setDatabaseFilename(const QString& filename);

    // Offers to save unsaved work. Returns false when the caller should abort
    // whatever it was about to do.
    bool maybeSave();

    // Invoices written before the uppdrag was kept with them carry none, and
    // the reports cannot then tell one project from another. On opening such a
    // business this offers to work them out from the contact person each
    // invoice was addressed to, having first put a copy of the file aside. The
    // answer is only a guess from a name, so it is asked rather than done, and
    // a no is remembered for that file.
    void offerToMatchAgreements(const QString& filename);

    QMenu *fileMenu;
    QMenu *helpMenu;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *aboutAct;

    QTabWidget* m_tabWidget;
    CompanyTab* m_companyTab;
    CustomerTab* m_customerTab;
    AgreementTab* m_agreementTab;
    EmployeeTab* m_employeeTab;
    SalaryDraftTab* m_salaryDraftTab;
    SalarySlipTab* m_salarySlipTab;
    ReportsTab* m_reportsTab;
    InvoiceDraftTab* m_invoiceDraftTab;
    InvoiceTab* m_invoiceTab;
};
#endif // MAINWINDOW_H
