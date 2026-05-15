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
#include "invoicedrafttab.h"
#include "invoicetab.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTabSelected(int current);
    void onTabEditing(bool editing);

    void newFile();
    void open();
    void save();
    void saveAs();
    void about();

private:
    QString m_settingsFile;

    void createActions();
    void createMenus();

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
    InvoiceDraftTab* m_invoiceDraftTab;
    InvoiceTab* m_invoiceTab;
};
#endif // MAINWINDOW_H
