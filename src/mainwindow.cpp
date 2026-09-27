#include <QCloseEvent>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QFileInfo>
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
#include "data/invoicematch.h"
#include "data/sqlitedatastore.h"
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

    m_employeeTab = new EmployeeTab();
    connect(DataBase::instance(), &DataBase::updated, m_employeeTab, &EmployeeTab::onDatabaseUpdated);
    connect(m_employeeTab, &EmployeeTab::editing, this, &MainWindow::onTabEditing);
    m_tabWidget->addTab(m_employeeTab, "Employees");

    m_salaryDraftTab = new SalaryDraftTab();
    connect(DataBase::instance(), &DataBase::updated, m_salaryDraftTab, &SalaryDraftTab::onDatabaseUpdated);
    connect(m_salaryDraftTab, &SalaryDraftTab::editing, this, &MainWindow::onTabEditing);
    m_tabWidget->addTab(m_salaryDraftTab, "SalaryDrafts");

    m_salarySlipTab = new SalarySlipTab();
    connect(DataBase::instance(), &DataBase::updated, m_salarySlipTab, &SalarySlipTab::onDatabaseUpdated);
    m_tabWidget->addTab(m_salarySlipTab, "SalarySlips");

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

    m_reportsTab = new ReportsTab();
    connect(DataBase::instance(), &DataBase::updated, m_reportsTab, &ReportsTab::onDatabaseUpdated);
    m_tabWidget->addTab(m_reportsTab, "Reports");

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabSelected);

    // Add the QTabWidget to the layout
    mainLayout->addWidget(m_tabWidget);

    createActions();
    createMenus();

    setMinimumSize(160, 160);
    resize(1280, 800);

    // Keep the title in sync with the file being edited, the company name and
    // whether there is unsaved work.
    connect(DataBase::instance(), &DataBase::modifiedChanged, this, &MainWindow::updateWindowTitle);
    connect(DataBase::instance(), &DataBase::updated, this, &MainWindow::updateWindowTitle);

    {
        QString message = "Open an existing business, or create a new!";
        QSettings settings;
        QString dbfilename = settings.value("dbfilename", "").toString();
        if (!dbfilename.isEmpty())
        {
            if (DataBase::instance()->loadSqlite(dbfilename)) {
                m_dbfilename = dbfilename;
                message = dbfilename;
                // Once the window is up: a question has nothing to stand on
                // before there is something to show it against.
                QTimer::singleShot(0, this, [this, dbfilename]() { offerToMatchAgreements(dbfilename); });
            } else {
                message = "Failed to open " + dbfilename;
            }
        }
        statusBar()->showMessage(message);

        // The settings file is not otherwise visible anywhere, and it is what
        // decides which database is opened at startup.
        QLabel* p_settings = new QLabel(settings.fileName());
        p_settings->setToolTip("Settings file");
        statusBar()->addPermanentWidget(p_settings);
    }

    updateWindowTitle();
}

//-----------------------------------------------------------------------------
void MainWindow::updateWindowTitle()
{
    QString title = QString("Ordning - v%1").arg(BUILD_VERSION);

    const QString company = DataBase::instance()->db()->m_companyinfo.m_name;
    if (!company.isEmpty())
    {
        title.append(" - " + company);
    }

    title.append(" - " + (m_dbfilename.isEmpty() ? QString("untitled")
                                                 : QFileInfo(m_dbfilename).fileName()));

    if (DataBase::instance()->isModified())
    {
        title.append('*');
    }

    setWindowTitle(title);
}

//-----------------------------------------------------------------------------
void MainWindow::setDatabaseFilename(const QString& filename)
{
    m_dbfilename = filename;

    QSettings settings;
    settings.setValue("dbfilename", filename);
    settings.sync();

    updateWindowTitle();
}

//-----------------------------------------------------------------------------
bool MainWindow::maybeSave()
{
    if (!DataBase::instance()->isModified())
    {
        return true;
    }

    const QMessageBox::StandardButton answer = QMessageBox::warning(
        this, "Ordning",
        "The business has unsaved changes.\nDo you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (answer == QMessageBox::Save)
    {
        return save();
    }
    return answer == QMessageBox::Discard;
}

//-----------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
    {
        event->accept();
    }
    else
    {
        event->ignore();
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
    else if (m_tabWidget->currentWidget() == m_employeeTab)
    {
        m_employeeTab->load();
    }
    else if (m_tabWidget->currentWidget() == m_salaryDraftTab)
    {
        m_salaryDraftTab->load();
    }
    else if (m_tabWidget->currentWidget() == m_salarySlipTab)
    {
        m_salarySlipTab->load();
    }
    else if (m_tabWidget->currentWidget() == m_invoiceDraftTab)
    {
        m_invoiceDraftTab->load();
    }
    else if (m_tabWidget->currentWidget() == m_invoiceTab)
    {
        m_invoiceTab->load();
    }
    else if (m_tabWidget->currentWidget() == m_reportsTab)
    {
        m_reportsTab->load();
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
    if (!maybeSave())
    {
        return;
    }

    DataBase::instance()->clear();

    // A new business has no file yet, so Save has to ask for one.
    setDatabaseFilename(QString());
    statusBar()->showMessage("Open an existing business, or create a new!");
}

//-----------------------------------------------------------------------------
void MainWindow::open()
{
    if (!maybeSave())
    {
        return;
    }

    QString dbfileName = QFileDialog::getOpenFileName(this, "Open File", "", "SQLite Files (*.sqlite *.db);;All Files (*)");
    if (!dbfileName.isEmpty())
    {
        if (DataBase::instance()->loadSqlite(dbfileName))
        {
            setDatabaseFilename(dbfileName);
            statusBar()->showMessage(dbfileName);
            offerToMatchAgreements(dbfileName);
        } else {
            statusBar()->showMessage("Failed to open " + dbfileName);
        }
    }
}

//-----------------------------------------------------------------------------
bool MainWindow::save()
{
    // No file chosen yet, so this is really a Save As.
    if (m_dbfilename.isEmpty())
    {
        return saveAs();
    }

    if (!DataBase::instance()->saveSqlite(m_dbfilename))
    {
        statusBar()->showMessage("Failed to save " + m_dbfilename);
        QMessageBox::critical(this, "Ordning", "Failed to save the business to " + m_dbfilename);
        return false;
    }

    updateWindowTitle();
    statusBar()->showMessage(m_dbfilename);
    return true;
}

//-----------------------------------------------------------------------------
bool MainWindow::saveAs()
{
    QString dbfileName = QFileDialog::getSaveFileName(this, "Save File", m_dbfilename, "SQLite Files (*.sqlite *.db);;All Files (*)");
    if (dbfileName.isEmpty())
    {
        return false;
    }

    if (!DataBase::instance()->saveSqlite(dbfileName))
    {
        statusBar()->showMessage("Failed to save " + dbfileName);
        QMessageBox::critical(this, "Ordning", "Failed to save the business to " + dbfileName);
        return false;
    }

    setDatabaseFilename(dbfileName);
    statusBar()->showMessage(dbfileName);
    return true;
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
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the business to a new file"));
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

//-----------------------------------------------------------------------------
void MainWindow::offerToMatchAgreements(const QString& filename)
{
    Business* p_business = DataBase::instance()->db();

    const QList<InvoiceAgreementMatch> matches = invoice_agreement_matches(p_business->m_invoices, p_business->m_customers);
    if (matches.isEmpty())
    {
        return;
    }

    // A no stays a no: the same business would otherwise ask again every time
    // it is opened.
    QSettings settings;
    QStringList declined = settings.value("uppdrag_match_declined").toStringList();
    if (declined.contains(filename))
    {
        return;
    }

    const QString backup = database_backup_path(filename, "uppdrag");
    if (QMessageBox::question(this, "Ordning",
                              QString("%1 invoice(s) in this business carry no uppdrag, which the reports need. "
                                      "They can be migrated now.\n\nA copy of the database as it is will be saved "
                                      "as %2 first.\n\nMigrate?")
                                  .arg(matches.size()).arg(QFileInfo(backup).fileName()),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
    {
        declined.append(filename);
        settings.setValue("uppdrag_match_declined", declined);
        settings.sync();
        return;
    }

    // The copy is the whole point of asking, so without one nothing is done.
    if (!QFile::copy(filename, backup))
    {
        QMessageBox::critical(this, "Ordning",
                              QString("The migration was not done: the copy could not be saved as %1.").arg(backup));
        return;
    }

    const int linked = invoice_link_agreements(matches);
    DataBase::instance()->notifyUpdated();

    QMessageBox::information(this, "Ordning",
                             QString("The migration is done: %1 invoice(s) were given an uppdrag, and the database "
                                     "as it was is saved as %2. Save to keep it.")
                                 .arg(linked).arg(QFileInfo(backup).fileName()));
}
