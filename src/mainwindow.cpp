#include <QDebug>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QPdfView>
#include <QStyle>
#include <QFileDialog>
#include <QSettings>
#include <QtPdf/QPdfDocument>
#include "version.h"

#include "data/database.h"
#include "aboutdialog.h"

#include "mainwindow.h"

//-----------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *p_widget = new QWidget;
    setCentralWidget(p_widget);

    // Create the main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(p_widget);

    // Create the QTabWidget
    m_tabWidget = new QTabWidget(this);

    // Add tabs with QStackedWidget inside each tab

    m_companyTab = new CompanyTab();
    connect(DataBase::instance(), &DataBase::updated, m_companyTab, &CompanyTab::onDatabaseUpdated);
    m_tabWidget->addTab(m_companyTab, "Company");

    m_customerTab = new CustomerTab();
    connect(DataBase::instance(), &DataBase::updated, m_customerTab, &CustomerTab::onDatabaseUpdated);
    connect(m_customerTab, &CustomerTab::editing, this, &MainWindow::onTabEditing);
    m_tabWidget->addTab(m_customerTab, "Customers");

    m_agreementTab = new AgreementTab();
    connect(DataBase::instance(), &DataBase::updated, m_agreementTab, &AgreementTab::onDatabaseUpdated);
    connect(m_agreementTab, &AgreementTab::editing, this, &MainWindow::onTabEditing);
    m_tabWidget->addTab(m_agreementTab, "Agreements");

    m_invoiceDraftTab = new InvoiceDraftTab();
    connect(DataBase::instance(), &DataBase::updated, m_invoiceDraftTab, &InvoiceDraftTab::onDatabaseUpdated);
    connect(m_invoiceDraftTab, &InvoiceDraftTab::editing, this, &MainWindow::onTabEditing);
    m_tabWidget->addTab(m_invoiceDraftTab, "InvoiceDrafts");

    m_invoiceTab = new InvoiceTab();
    connect(DataBase::instance(), &DataBase::updated, m_invoiceTab, &InvoiceTab::onDatabaseUpdated);
    m_tabWidget->addTab(m_invoiceTab, "Invoices");

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabSelected);

    // Add the QTabWidget to the layout
    mainLayout->addWidget(m_tabWidget);

    createActions();
    createMenus();

    setWindowTitle(QString("Ordning - v%1").arg(BUILD_VERSION));
    setMinimumSize(160, 160);
    resize(1280, 800);

    {
        QString message = "Open an existing business, or create a new!";
        QSettings settings;
        qDebug() << "settings.fileName() = " << settings.fileName();
        QString dbfilename = settings.value("dbfilename", "").toString();
        qDebug() << "dbfilename" << dbfilename;
        if (!dbfilename.isEmpty())
        {
            if (DataBase::instance()->loadSqlite(dbfilename)) {
                message = "Business opened from " + dbfilename;
            } else {
                message = "Failed to open DB " + dbfilename;
            }
        }
        statusBar()->showMessage(message);
    }
}

//-----------------------------------------------------------------------------
MainWindow::~MainWindow() {}

//-----------------------------------------------------------------------------
void MainWindow::onTabSelected(int current)
{
    Q_UNUSED(current)
    if (m_tabWidget->currentWidget() == m_customerTab)
    {
        m_customerTab->load();
    }
    else if (m_tabWidget->currentWidget() == m_agreementTab)
    {
        m_agreementTab->load();
    }
    else if (m_tabWidget->currentWidget() == m_invoiceDraftTab)
    {
        m_invoiceDraftTab->load();
    }
    else if (m_tabWidget->currentWidget() == m_invoiceTab)
    {
        m_invoiceTab->load();
    }
}

//-----------------------------------------------------------------------------
void MainWindow::onTabEditing(bool editing)
{
    for (int i=0; i< m_tabWidget->count(); i++)
    {
        if (i == m_tabWidget->currentIndex())
        {
            continue;
        }
        m_tabWidget->setTabEnabled(i, !editing);
    }
}

//-----------------------------------------------------------------------------
void MainWindow::newFile()
{
    DataBase::instance()->clear();
    statusBar()->showMessage("Business cleared!");
}

//-----------------------------------------------------------------------------
void MainWindow::open()
{
    QString dbfileName = QFileDialog::getOpenFileName(this, "Open File", "", "SQLite Files (*.sqlite *.db);;All Files (*)");
    if (!dbfileName.isEmpty())
    {
        if (DataBase::instance()->loadSqlite(dbfileName))
        {
            QSettings settings;
            settings.setValue("dbfilename", dbfileName);
            settings.sync();
            statusBar()->showMessage("Business opened from " + dbfileName);
        } else {
            statusBar()->showMessage("Failed to open " + dbfileName);
        }
    }
}

//-----------------------------------------------------------------------------
void MainWindow::save()
{
    QSettings settings;
    QString dbfilename = settings.value("dbfilename", "").toString();
    if (!dbfilename.isEmpty())
    {
        DataBase::instance()->saveSqlite(dbfilename);
        statusBar()->showMessage("Business saved to " + dbfilename);
    }
}

//-----------------------------------------------------------------------------
void MainWindow::saveAs()
{
    QString dbfileName = QFileDialog::getSaveFileName(this, "Save File", "", "SQLite Files (*.sqlite *.db);;All Files (*)");
    if (!dbfileName.isEmpty())
    {
        if (DataBase::instance()->saveSqlite(dbfileName))
        {
            QSettings settings;
            settings.setValue("dbfilename", dbfileName);
            settings.sync();
            statusBar()->showMessage("Business saved to " + dbfileName);
        } else {
            statusBar()->showMessage("Failed to save " + dbfileName);
        }
    }
}

//-----------------------------------------------------------------------------
void MainWindow::about()
{
    AboutDialog dialog(this);
    dialog.exec();
}

//-----------------------------------------------------------------------------
void MainWindow::createActions()
{
    newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new business"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing business"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the business to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);

    saveAsAct = new QAction(tr("&Save as..."), this);
    saveAsAct->setShortcuts(QKeySequence::Save);
    saveAsAct->setStatusTip(tr("Save the business to disk"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAs);

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);
}

//-----------------------------------------------------------------------------
void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
}
