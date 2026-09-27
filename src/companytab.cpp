#include <QDate>
#include <QDebug>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QPainter>
#include <QSvgRenderer>

#include "data/companyinfo.h"
#include "data/database.h"
#include "data/traktamente.h"
#include "util/amount.h"

#include "companytab.h"

// The logo is shown on its own button, at the size it is printed on an
// invoice.
#define LOGO_WIDTH  200
#define LOGO_HEIGHT 50

// A field is wide enough for its value but no wider: a line edit that
// stretches across the window only looks empty.
#define FIELD_WIDTH_MIN 240
#define FIELD_WIDTH_MAX 320

// The list of imported years holds nothing but a year.
#define TAXTABLE_WIDTH 320

//-----------------------------------------------------------------------------
// A titled group of labelled fields. Each field is created here and handed
// back through the pointer given with its label.
QGroupBox* CompanyTab::fieldGroup(const QString& title, const QList<QPair<QString, QLineEdit**>>& fields)
{
    QGroupBox* p_group = new QGroupBox(title);
    QFormLayout* p_form = new QFormLayout;

    for (const QPair<QString, QLineEdit**>& field : fields)
    {
        QLineEdit* p_edit = new QLineEdit();
        p_edit->setMinimumWidth(FIELD_WIDTH_MIN);
        p_edit->setMaximumWidth(FIELD_WIDTH_MAX);
        *field.second = p_edit;
        lineedits.append(p_edit);
        p_form->addRow(field.first + ":", p_edit);
    }

    p_group->setLayout(p_form);
    return p_group;
}

//-----------------------------------------------------------------------------
CompanyTab::CompanyTab(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    // Three groups side by side rather than one tall column of fields.
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addWidget(fieldGroup("Företag", {
            { "Company name", &m_name },
            { "Orgnummer", &m_orgnr },
            { "Momsnummer", &m_momsregnr },
        }));
        hbox->addWidget(fieldGroup("Kontakt", {
            { "Address line 1", &m_adrs1 },
            { "Address line 2", &m_adrs2 },
            { "Telefon", &m_phone },
            { "Email", &m_email },
            { "Web", &m_web },
        }));
        QGroupBox* p_bank = fieldGroup("Bank", {
            { "Bankgiro", &m_bankgiro },
            { "SWIFT/BIC", &m_swiftbic },
            { "IBAN", &m_iban },
        });

        // The räkenskapsår decides what a year means in the reports: the
        // calendar year, or a broken one beginning in some other month.
        m_fiscal_year = new QComboBox();
        m_fiscal_year->addItem("Kalenderår (januari)", 1);
        const QStringList months = { "januari", "februari", "mars", "april", "maj", "juni",
                                     "juli", "augusti", "september", "oktober", "november", "december" };
        for (int month = 2; month <= 12; month++)
        {
            m_fiscal_year->addItem(QString("Brutet, från %1").arg(months.at(month - 1)), month);
        }
        m_fiscal_year->setMaximumWidth(FIELD_WIDTH_MAX);
        qobject_cast<QFormLayout*>(p_bank->layout())->addRow("Räkenskapsår:", m_fiscal_year);

        hbox->addWidget(p_bank);
        hbox->addStretch(1);
        layout->addLayout(hbox);
    }

    // The logo is the button: once one is chosen it is shown there, and
    // clicking it again picks another.
    {
        QGroupBox* p_group = new QGroupBox("Logotyp");
        QHBoxLayout *hbox = new QHBoxLayout;
        m_logoselection = new QPushButton();
        m_logoselection->setEnabled(false);
        m_logoselection->setMinimumSize(LOGO_WIDTH + 20, LOGO_HEIGHT + 20);
        connect(m_logoselection, &QPushButton::clicked, this, &CompanyTab::onSelectLogo);
        hbox->addWidget(m_logoselection);
        hbox->addStretch(1);
        p_group->setLayout(hbox);
        layout->addWidget(p_group);
    }

    // The tax tables are imported and removed on their own, outside the edit
    // mode of the company fields. One row per imported year; a year holds all
    // of Skatteverket's tables, so there is room for as many years as wanted.
    {
        QGroupBox* p_group = new QGroupBox("Skattetabeller");
        QVBoxLayout *vbox = new QVBoxLayout;

        m_taxtables = new QTableView();
        m_taxtables->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_taxtables->setSelectionMode(QAbstractItemView::SingleSelection);
        m_taxtables->verticalHeader()->setVisible(false);
        m_taxtables->setMaximumHeight(140);
        m_taxtables->setMaximumWidth(TAXTABLE_WIDTH);
        m_taxtables->horizontalHeader()->setStretchLastSection(true);
        m_taxtableModel = new TaxTableYearsModel(this);
        m_taxtables->setModel(m_taxtableModel);
        connect(m_taxtables, &QTableView::clicked, this, &CompanyTab::onTaxTableSelected);
        vbox->addWidget(m_taxtables);

        // Under the list rather than beside it: buttons to the side line up
        // with whichever row happens to be next to them.
        QHBoxLayout *hbox = new QHBoxLayout;
        QPushButton* p_import = new QPushButton("Import CSV...");
        p_import->setToolTip("Skatteverket, \"Skattetabeller för månadslön\", one year per file:\n"
                             "https://www7.skatteverket.se/portal/apier-och-oppna-data/utvecklarportalen/oppetdata/Skattetabeller%20f%C3%B6r%20m%C3%A5nadsl%C3%B6n");
        connect(p_import, &QPushButton::clicked, this, &CompanyTab::onImportTaxTable);
        hbox->addWidget(p_import);

        m_taxtable_remove = new QPushButton("Remove year");
        connect(m_taxtable_remove, &QPushButton::clicked, this, &CompanyTab::onRemoveTaxTable);
        hbox->addWidget(m_taxtable_remove);
        hbox->addStretch(1);
        vbox->addLayout(hbox);

        QHBoxLayout *outer = new QHBoxLayout;
        outer->addLayout(vbox);
        outer->addStretch(1);
        p_group->setLayout(outer);

        // Beside it, the arbetsgivaravgifter: a percentage per year, which
        // Skatteverket publishes on their website rather than as open data.
        QGroupBox* p_fees = new QGroupBox("Arbetsgivaravgifter");
        QVBoxLayout* p_fee_box = new QVBoxLayout;

        m_fees = new QTableWidget(0, 3);
        m_fees->setHorizontalHeaderLabels({ "Year", "Standard %", "66+ år %" });
        m_fees->verticalHeader()->setVisible(false);
        m_fees->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_fees->setSelectionMode(QAbstractItemView::SingleSelection);
        m_fees->setMaximumHeight(140);
        m_fees->setMinimumWidth(340);
        m_fees->horizontalHeader()->setStretchLastSection(true);
        p_fee_box->addWidget(m_fees);

        QHBoxLayout* p_fee_buttons = new QHBoxLayout;
        m_fee_add = new QPushButton("Add year");
        connect(m_fee_add, &QPushButton::clicked, this, &CompanyTab::onAddFeeYear);
        p_fee_buttons->addWidget(m_fee_add);
        m_fee_remove = new QPushButton("Remove year");
        connect(m_fee_remove, &QPushButton::clicked, this, &CompanyTab::onRemoveFeeYear);
        p_fee_buttons->addWidget(m_fee_remove);
        p_fee_buttons->addStretch(1);
        p_fee_box->addLayout(p_fee_buttons);
        p_fees->setLayout(p_fee_box);

        QHBoxLayout* p_row = new QHBoxLayout;
        p_row->addWidget(p_group);
        p_row->addWidget(p_fees);
        p_row->addStretch(1);
        layout->addLayout(p_row);
    }

    // Traktamente. The inrikes amount is one figure a year, typed in like the
    // arbetsgivaravgifter; the utrikes normalbelopp are Skatteverket's hundred
    // and fifty countries, imported a year at a time like the skattetabeller.
    {
        QGroupBox* p_inrikes = new QGroupBox("Traktamente inrikes");
        QVBoxLayout* p_inrikes_box = new QVBoxLayout;

        m_trakt_inrikes = new QTableWidget(0, 2);
        m_trakt_inrikes->setHorizontalHeaderLabels({ "Year", "Helt dag (kr)" });
        m_trakt_inrikes->verticalHeader()->setVisible(false);
        m_trakt_inrikes->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_trakt_inrikes->setSelectionMode(QAbstractItemView::SingleSelection);
        m_trakt_inrikes->setMaximumHeight(140);
        m_trakt_inrikes->setMinimumWidth(340);
        m_trakt_inrikes->horizontalHeader()->setStretchLastSection(true);
        m_trakt_inrikes->setToolTip("Helt dagtraktamente inrikes: 0,5 % of prisbasbeloppet,\n"
                                    "rounded to the nearest tio kronor. Halvdag and nattschablon\n"
                                    "are half of it.");
        p_inrikes_box->addWidget(m_trakt_inrikes);

        QHBoxLayout* p_inrikes_buttons = new QHBoxLayout;
        m_trakt_inrikes_add = new QPushButton("Add year");
        connect(m_trakt_inrikes_add, &QPushButton::clicked, this, &CompanyTab::onAddTraktamenteYear);
        p_inrikes_buttons->addWidget(m_trakt_inrikes_add);
        m_trakt_inrikes_remove = new QPushButton("Remove year");
        connect(m_trakt_inrikes_remove, &QPushButton::clicked, this, &CompanyTab::onRemoveTraktamenteYear);
        p_inrikes_buttons->addWidget(m_trakt_inrikes_remove);
        p_inrikes_buttons->addStretch(1);
        p_inrikes_box->addLayout(p_inrikes_buttons);
        p_inrikes->setLayout(p_inrikes_box);

        QGroupBox* p_utrikes = new QGroupBox("Traktamente utrikes");
        QVBoxLayout* p_utrikes_box = new QVBoxLayout;

        // Read-only: a year comes in whole from a file, so there is nothing to
        // type here. The count is what tells a full import from a half one.
        m_trakt_utrikes = new QTableWidget(0, 2);
        m_trakt_utrikes->setHorizontalHeaderLabels({ "Year", "Länder" });
        m_trakt_utrikes->verticalHeader()->setVisible(false);
        m_trakt_utrikes->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_trakt_utrikes->setSelectionMode(QAbstractItemView::SingleSelection);
        m_trakt_utrikes->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_trakt_utrikes->setMaximumHeight(140);
        m_trakt_utrikes->setMinimumWidth(340);
        m_trakt_utrikes->horizontalHeader()->setStretchLastSection(true);
        connect(m_trakt_utrikes, &QTableWidget::itemSelectionChanged, this, &CompanyTab::onUtrikesTraktamenteSelected);
        p_utrikes_box->addWidget(m_trakt_utrikes);

        QHBoxLayout* p_utrikes_buttons = new QHBoxLayout;
        QPushButton* p_import = new QPushButton("Import CSV...");
        p_import->setToolTip("Skatteverket's normalbelopp för ökade levnadskostnader i utlandet,\n"
                             "one year per file. The first line has to name the columns:\n"
                             "Land;Normalbelopp, and År as well if the file says which year.");
        connect(p_import, &QPushButton::clicked, this, &CompanyTab::onImportTraktamente);
        p_utrikes_buttons->addWidget(p_import);
        m_trakt_utrikes_remove = new QPushButton("Remove year");
        connect(m_trakt_utrikes_remove, &QPushButton::clicked, this, &CompanyTab::onRemoveUtrikesTraktamente);
        p_utrikes_buttons->addWidget(m_trakt_utrikes_remove);
        p_utrikes_buttons->addStretch(1);
        p_utrikes_box->addLayout(p_utrikes_buttons);
        p_utrikes->setLayout(p_utrikes_box);

        QHBoxLayout* p_row = new QHBoxLayout;
        p_row->addWidget(p_inrikes);
        p_row->addWidget(p_utrikes);
        p_row->addStretch(1);
        layout->addLayout(p_row);
    }

    layout->addStretch(1);

    {
        QHBoxLayout *hbox = new QHBoxLayout;
        m_cancel = new QPushButton("Cancel");
        hbox->addStretch(1);
        hbox->addWidget(m_cancel);
        m_ok = new QPushButton("OK");
        hbox->addWidget(m_ok);
        m_edit = false;
        updateButtons();
        connect(m_ok, &QPushButton::clicked, this, &CompanyTab::onEditSave);
        connect(m_cancel, &QPushButton::clicked, this, &CompanyTab::onCancel);
        layout->addLayout(hbox);
    }

    setLayout(layout);

    onDatabaseUpdated();
}

//-----------------------------------------------------------------------------
void CompanyTab::onDatabaseUpdated(void)
{
    m_name->setText(DataBase::instance()->db()->m_companyinfo.m_name);
    m_adrs1->setText(DataBase::instance()->db()->m_companyinfo.m_adrsline1);
    m_adrs2->setText(DataBase::instance()->db()->m_companyinfo.m_adrsline2);
    m_orgnr->setText(DataBase::instance()->db()->m_companyinfo.m_org_number);
    m_momsregnr->setText(DataBase::instance()->db()->m_companyinfo.m_vat_number);
    m_bankgiro->setText(DataBase::instance()->db()->m_companyinfo.m_bankgiro);
    m_swiftbic->setText(DataBase::instance()->db()->m_companyinfo.m_swift_bic);
    m_iban->setText(DataBase::instance()->db()->m_companyinfo.m_iban);
    m_phone->setText(DataBase::instance()->db()->m_companyinfo.m_phone);
    m_email->setText(DataBase::instance()->db()->m_companyinfo.m_email);
    m_web->setText(DataBase::instance()->db()->m_companyinfo.m_web);
    {
        const int index = m_fiscal_year->findData(DataBase::instance()->db()->m_companyinfo.m_fiscal_year_start_month);
        m_fiscal_year->setCurrentIndex(index >= 0 ? index : 0);
    }
    setLogo(DataBase::instance()->db()->m_companyinfo.m_logo);
    updateTaxTableYears();
    setFees();
    setInrikesTraktamente();
    updateUtrikesTraktamente();
}

//-----------------------------------------------------------------------------
void CompanyTab::updateTaxTableYears(void)
{
    // Reloading resets the model, and with it the selection, so this is only
    // for when the years themselves have changed.
    m_taxtableModel->load();
    m_taxtables->resizeColumnsToContents();
    updateTaxTableButtons();
}

//-----------------------------------------------------------------------------
void CompanyTab::updateTaxTableButtons(void)
{
    const bool selected = m_taxtableModel->getYear(m_taxtables->currentIndex()) > 0;
    m_taxtable_remove->setEnabled(selected);
    m_taxtable_remove->setStyleSheet(selected ? "" : "QPushButton:disabled { color: gray; background-color: lightgray; }");
}

//-----------------------------------------------------------------------------
void CompanyTab::onTaxTableSelected(const QModelIndex &index)
{
    Q_UNUSED(index)
    updateTaxTableButtons();
}

//-----------------------------------------------------------------------------
void CompanyTab::onImportTaxTable(bool clicked)
{
    Q_UNUSED(clicked);

    QString filename = QFileDialog::getOpenFileName(this, "Import skattetabell...", "", "CSV Files (*.csv *.txt);;All Files (*)");
    if (filename.isEmpty())
    {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Import skattetabell", QString("Could not open %1.").arg(filename));
        return;
    }

    int year = 0;
    QList<TaxTableRow> rows;
    QString error;
    if (!taxtable_parse_csv(file.readAll(), &year, &rows, &error))
    {
        QMessageBox::warning(this, "Import skattetabell", error);
        return;
    }

    TaxTables& tables = DataBase::instance()->db()->m_taxtables;
    if (tables.hasYear(year)
        && QMessageBox::question(this, "Import skattetabell", QString("The tax tables for %1 are already imported. Replace them?").arg(year)) != QMessageBox::Yes)
    {
        return;
    }

    tables.setYear(year, rows);
    DataBase::instance()->setModified();
    updateTaxTableYears();
    QMessageBox::information(this, "Import skattetabell", QString("Imported %1 rows for %2.").arg(rows.size()).arg(year));
}

//-----------------------------------------------------------------------------
void CompanyTab::onRemoveTaxTable(bool clicked)
{
    Q_UNUSED(clicked);

    const int year = m_taxtableModel->getYear(m_taxtables->currentIndex());
    if (year <= 0)
    {
        updateTaxTableYears();
        return;
    }

    if (QMessageBox::question(this, "Remove skattetabell", QString("Remove the tax tables for %1?").arg(year)) != QMessageBox::Yes)
    {
        return;
    }

    DataBase::instance()->db()->m_taxtables.removeYear(year);
    DataBase::instance()->setModified();
    m_taxtables->setCurrentIndex(QModelIndex());
    updateTaxTableYears();
}

//-----------------------------------------------------------------------------
void CompanyTab::setLogo(QString svgString)
{
    QSvgRenderer renderer(svgString.toUtf8());
    if (!renderer.isValid())
    {
        m_logo = "";
        m_logoselection->setIcon(QIcon());
        m_logoselection->setText("Select logo...");
        m_logoselection->setToolTip("Pick an SVG file");
        return;
    }

    m_logo = svgString;

    QSize size = renderer.defaultSize();
    size.scale(LOGO_WIDTH, LOGO_HEIGHT, Qt::KeepAspectRatio);
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    {
        QPainter painter(&pixmap);
        renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
    }

    // The same picture whether the tab is being edited or not: a greyed-out
    // logo looks like something is wrong with it.
    QIcon icon;
    icon.addPixmap(pixmap, QIcon::Normal);
    icon.addPixmap(pixmap, QIcon::Disabled);

    m_logoselection->setText("");
    m_logoselection->setIcon(icon);
    m_logoselection->setIconSize(size);
    m_logoselection->setToolTip("Pick another SVG file");
}

//-----------------------------------------------------------------------------
void CompanyTab::onSelectLogo(bool clicked)
{
    Q_UNUSED(clicked);

    QString svgfile = QFileDialog::getOpenFileName(this, "Select logo...", "", "SVG Files (*.svg)");
    if (svgfile.isEmpty())
    {
        qDebug() << "No Svg file selected!";
        setLogo("");
        return;
    }

    {
        QSvgRenderer renderer(svgfile);
        if (!renderer.isValid())
        {
            qDebug() << "Svg file not valid!";
            setLogo("");
            return;
        }
    }

    QString svgString;
    {
        QFile file(svgfile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "Failed to open SVG file!";
            setLogo("");
            return;
        }
        QTextStream in(&file);
        svgString = in.readAll();
        file.close();
    }


    QSvgRenderer renderer(svgString.toUtf8());
    if (!renderer.isValid()) {
        qDebug() << "Invalid SVG string";
        setLogo("");
        return;
    }

    setLogo(svgString);
}

//-----------------------------------------------------------------------------
void CompanyTab::onEditSave(bool clicked)
{
    Q_UNUSED(clicked);

    if (m_edit)
    {
        // The rates are the one thing here that can be wrong, so they are
        // read back first: the tab stays in edit mode until they make sense.
        if (!readFees())
        {
            return;
        }
        if (!readInrikesTraktamente())
        {
            return;
        }

        // we was in edit mode, lets update database
        DataBase::instance()->db()->m_companyinfo.m_name = m_name->text();
        DataBase::instance()->db()->m_companyinfo.m_adrsline1 = m_adrs1->text();
        DataBase::instance()->db()->m_companyinfo.m_adrsline2 = m_adrs2->text();
        DataBase::instance()->db()->m_companyinfo.m_org_number = m_orgnr->text();
        DataBase::instance()->db()->m_companyinfo.m_vat_number = m_momsregnr->text();
        DataBase::instance()->db()->m_companyinfo.m_bankgiro = m_bankgiro->text();
        DataBase::instance()->db()->m_companyinfo.m_email = m_email->text();
        DataBase::instance()->db()->m_companyinfo.m_iban = m_iban->text();
        DataBase::instance()->db()->m_companyinfo.m_phone = m_phone->text();
        DataBase::instance()->db()->m_companyinfo.m_swift_bic = m_swiftbic->text();
        DataBase::instance()->db()->m_companyinfo.m_web = m_web->text();
        DataBase::instance()->db()->m_companyinfo.m_fiscal_year_start_month = m_fiscal_year->currentData().toInt();
        DataBase::instance()->db()->m_companyinfo.m_logo = m_logo;
        DataBase::instance()->setModified();
    }

    m_edit = !m_edit;
    updateButtons();
}

//-----------------------------------------------------------------------------
void CompanyTab::onCancel(bool clicked)
{
    Q_UNUSED(clicked);
    m_edit = !m_edit;
    setFees(); // throw away whatever was typed
    setInrikesTraktamente();
    updateButtons();
}

//-----------------------------------------------------------------------------
void CompanyTab::setFees(void)
{
    const EmployerFees& fees = DataBase::instance()->db()->m_employerfees;
    m_fees->setRowCount(0);
    for (int year : fees.years())
    {
        const int row = m_fees->rowCount();
        m_fees->insertRow(row);
        m_fees->setItem(row, 0, new QTableWidgetItem(QString::number(year)));
        // A percentage has two decimals, like an amount has öre.
        m_fees->setItem(row, 1, new QTableWidgetItem(amount_format_ore(fees.rate(year).standard)));
        m_fees->setItem(row, 2, new QTableWidgetItem(amount_format_ore(fees.rate(year).senior)));
    }
}

//-----------------------------------------------------------------------------
// Reads the rates back into the database. Tells the user what is wrong and
// returns false if a row does not make sense.
bool CompanyTab::readFees(void)
{
    QList<QStringList> rows;
    for (int row = 0; row < m_fees->rowCount(); row++)
    {
        QStringList cells;
        for (int column = 0; column < 3; column++)
        {
            QTableWidgetItem* p_item = m_fees->item(row, column);
            cells.append(p_item ? p_item->text() : QString());
        }
        rows.append(cells);
    }

    QMap<int, EmployerFeeRate> rates;
    QString error;
    if (!employer_fee_parse_rates(rows, &rates, &error))
    {
        QMessageBox::warning(this, "Arbetsgivaravgifter", error);
        return false;
    }

    EmployerFees& fees = DataBase::instance()->db()->m_employerfees;
    fees.clear();
    for (auto it = rates.constBegin(); it != rates.constEnd(); ++it)
    {
        fees.setYear(it.key(), it.value());
    }
    return true;
}

//-----------------------------------------------------------------------------
void CompanyTab::onAddFeeYear(void)
{
    // The year after the latest one listed, but not a past year, and the same
    // rates to start from since they rarely change by much.
    int latest = 0;
    QString standard, senior;
    for (int row = 0; row < m_fees->rowCount(); row++)
    {
        QTableWidgetItem* p_year = m_fees->item(row, 0);
        if (p_year && p_year->text().toInt() > latest)
        {
            latest = p_year->text().toInt();
            standard = m_fees->item(row, 1) ? m_fees->item(row, 1)->text() : QString();
            senior = m_fees->item(row, 2) ? m_fees->item(row, 2)->text() : QString();
        }
    }
    const int year = qMax(QDate::currentDate().year(), latest + 1);

    const int row = m_fees->rowCount();
    m_fees->insertRow(row);
    m_fees->setItem(row, 0, new QTableWidgetItem(QString::number(year)));
    m_fees->setItem(row, 1, new QTableWidgetItem(standard));
    m_fees->setItem(row, 2, new QTableWidgetItem(senior));
    m_fees->setCurrentCell(row, 1);
    m_fees->editItem(m_fees->item(row, 1));
}

//-----------------------------------------------------------------------------
void CompanyTab::onRemoveFeeYear(void)
{
    const int row = m_fees->currentRow();
    if (row >= 0)
    {
        m_fees->removeRow(row);
    }
}

//-----------------------------------------------------------------------------
void CompanyTab::setInrikesTraktamente(void)
{
    const Traktamente& t = DataBase::instance()->db()->m_traktamente;
    m_trakt_inrikes->setRowCount(0);
    for (int year : t.domesticYears())
    {
        const int row = m_trakt_inrikes->rowCount();
        m_trakt_inrikes->insertRow(row);
        m_trakt_inrikes->setItem(row, 0, new QTableWidgetItem(QString::number(year)));
        m_trakt_inrikes->setItem(row, 1, new QTableWidgetItem(amount_format_ore(t.domesticOre(year))));
    }
}

//-----------------------------------------------------------------------------
// Reads the inrikes amounts back into the database. Tells the user what is
// wrong and returns false if a row does not make sense.
bool CompanyTab::readInrikesTraktamente(void)
{
    QList<QStringList> rows;
    for (int row = 0; row < m_trakt_inrikes->rowCount(); row++)
    {
        QStringList cells;
        for (int column = 0; column < 2; column++)
        {
            QTableWidgetItem* p_item = m_trakt_inrikes->item(row, column);
            cells.append(p_item ? p_item->text() : QString());
        }
        rows.append(cells);
    }

    QMap<int, long long> amounts;
    QString error;
    if (!traktamente_parse_domestic(rows, &amounts, &error))
    {
        QMessageBox::warning(this, "Traktamente inrikes", error);
        return false;
    }

    // The imported utrikes years are left alone: only the typed-in amounts are
    // being read back here.
    Traktamente& t = DataBase::instance()->db()->m_traktamente;
    for (int year : t.domesticYears())
    {
        if (!amounts.contains(year))
        {
            t.removeDomestic(year);
        }
    }
    for (auto it = amounts.constBegin(); it != amounts.constEnd(); ++it)
    {
        t.setDomestic(it.key(), it.value());
    }
    return true;
}

//-----------------------------------------------------------------------------
void CompanyTab::onAddTraktamenteYear(void)
{
    // The year after the latest one listed, but not a past year, and the
    // amount of that year to start from: it changes by a tenner or two.
    int latest = 0;
    QString amount;
    for (int row = 0; row < m_trakt_inrikes->rowCount(); row++)
    {
        QTableWidgetItem* p_year = m_trakt_inrikes->item(row, 0);
        if (p_year && p_year->text().toInt() > latest)
        {
            latest = p_year->text().toInt();
            amount = m_trakt_inrikes->item(row, 1) ? m_trakt_inrikes->item(row, 1)->text() : QString();
        }
    }
    const int year = qMax(QDate::currentDate().year(), latest + 1);

    const int row = m_trakt_inrikes->rowCount();
    m_trakt_inrikes->insertRow(row);
    m_trakt_inrikes->setItem(row, 0, new QTableWidgetItem(QString::number(year)));
    m_trakt_inrikes->setItem(row, 1, new QTableWidgetItem(amount));
    m_trakt_inrikes->setCurrentCell(row, 1);
    m_trakt_inrikes->editItem(m_trakt_inrikes->item(row, 1));
}

//-----------------------------------------------------------------------------
void CompanyTab::onRemoveTraktamenteYear(void)
{
    const int row = m_trakt_inrikes->currentRow();
    if (row >= 0)
    {
        m_trakt_inrikes->removeRow(row);
    }
}

//-----------------------------------------------------------------------------
void CompanyTab::updateUtrikesTraktamente(void)
{
    // Rebuilding the list clears the selection, so this is only for when the
    // imported years themselves have changed.
    const Traktamente& t = DataBase::instance()->db()->m_traktamente;
    m_trakt_utrikes->setRowCount(0);
    for (int year : t.foreignYears())
    {
        const int row = m_trakt_utrikes->rowCount();
        m_trakt_utrikes->insertRow(row);
        m_trakt_utrikes->setItem(row, 0, new QTableWidgetItem(QString::number(year)));
        m_trakt_utrikes->setItem(row, 1, new QTableWidgetItem(QString::number(t.countries(year).size())));
    }
    m_trakt_utrikes->resizeColumnsToContents();
    updateUtrikesTraktamenteButtons();
}

//-----------------------------------------------------------------------------
void CompanyTab::updateUtrikesTraktamenteButtons(void)
{
    const bool selected = m_trakt_utrikes->currentRow() >= 0 && !m_trakt_utrikes->selectedItems().isEmpty();
    m_trakt_utrikes_remove->setEnabled(selected);
    m_trakt_utrikes_remove->setStyleSheet(selected ? "" : "QPushButton:disabled { color: gray; background-color: lightgray; }");
}

//-----------------------------------------------------------------------------
void CompanyTab::onUtrikesTraktamenteSelected(void)
{
    updateUtrikesTraktamenteButtons();
}

//-----------------------------------------------------------------------------
void CompanyTab::onImportTraktamente(bool clicked)
{
    Q_UNUSED(clicked);

    QString filename = QFileDialog::getOpenFileName(this, "Import normalbelopp...", "", "CSV Files (*.csv *.txt);;All Files (*)");
    if (filename.isEmpty())
    {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Import normalbelopp", QString("Could not open %1.").arg(filename));
        return;
    }

    int year = 0;
    QList<TraktamenteCountry> countries;
    QString error;
    if (!traktamente_parse_csv(file.readAll(), &year, &countries, &error))
    {
        QMessageBox::warning(this, "Import normalbelopp", error);
        return;
    }

    // Skatteverket's table is published a year at a time and the year is in
    // the title of the document rather than in the table, so a file that does
    // not carry it has to be placed by hand.
    if (year == 0)
    {
        bool ok = false;
        year = QInputDialog::getInt(this, "Import normalbelopp",
                                    "The file does not say which year it is for.\nWhich year do these normalbelopp apply to?",
                                    QDate::currentDate().year(), 2000, 2100, 1, &ok);
        if (!ok)
        {
            return;
        }
    }

    Traktamente& t = DataBase::instance()->db()->m_traktamente;
    if (t.hasForeign(year)
        && QMessageBox::question(this, "Import normalbelopp", QString("The normalbelopp for %1 are already imported. Replace them?").arg(year)) != QMessageBox::Yes)
    {
        return;
    }

    t.setForeign(year, countries);
    DataBase::instance()->setModified();
    updateUtrikesTraktamente();
    QMessageBox::information(this, "Import normalbelopp", QString("Imported %1 countries for %2.").arg(countries.size()).arg(year));
}

//-----------------------------------------------------------------------------
void CompanyTab::onRemoveUtrikesTraktamente(bool clicked)
{
    Q_UNUSED(clicked);

    const int row = m_trakt_utrikes->currentRow();
    QTableWidgetItem* p_year = row >= 0 ? m_trakt_utrikes->item(row, 0) : nullptr;
    const int year = p_year ? p_year->text().toInt() : 0;
    if (year <= 0)
    {
        updateUtrikesTraktamente();
        return;
    }

    if (QMessageBox::question(this, "Remove normalbelopp", QString("Remove the normalbelopp for %1?").arg(year)) != QMessageBox::Yes)
    {
        return;
    }

    DataBase::instance()->db()->m_traktamente.removeForeign(year);
    DataBase::instance()->setModified();
    updateUtrikesTraktamente();
}

//-----------------------------------------------------------------------------
void CompanyTab::updateButtons(void)
{
    for(int i=0; i<lineedits.size(); i++)
    {
        lineedits.at(i)->setReadOnly(!m_edit);
    }
    if (m_edit)
    {
        m_cancel->setEnabled(true);
        m_cancel->setHidden(false);
        m_ok->setText("Ok");
    }
    else
    {
        m_cancel->setEnabled(false);
        m_cancel->setHidden(true);
        m_ok->setText("Edit");
    }
    m_logoselection->setEnabled(m_edit);
    m_fiscal_year->setEnabled(m_edit);

    // The rates belong to the company fields, so they are edited along with
    // them rather than on their own like the tax tables.
    m_fees->setEditTriggers(m_edit ? QAbstractItemView::AllEditTriggers : QAbstractItemView::NoEditTriggers);
    m_fee_add->setEnabled(m_edit);
    m_fee_remove->setEnabled(m_edit);

    // The inrikes traktamente is typed in the same way. The utrikes list is
    // imported on its own, outside the edit mode, as the skattetabeller are.
    m_trakt_inrikes->setEditTriggers(m_edit ? QAbstractItemView::AllEditTriggers : QAbstractItemView::NoEditTriggers);
    m_trakt_inrikes_add->setEnabled(m_edit);
    m_trakt_inrikes_remove->setEnabled(m_edit);
}
