#include <QBrush>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>

#include "data/database.h"
#include "util/amount.h"

#include "salaryfactory.h"
#include "salaryslipprinter.h"
#include "salarysliptab.h"
#include "salarydrafttab.h"

#define LABEL_WIDTH 140

#define DATE_FORMAT_STRING  "yyyy-MM-dd"

// The inrikes traktamente, as it stands among the countries to pick from and as
// a line names the place it was paid for.
#define TRAKTAMENTE_DOMESTIC    "Inrikes"

// Columns of the line table on the edit page. The löneart carries the benämning
// and whether the line is skattefri, so neither is a column of its own.
// The second column and Kommentar are two columns and not one: what the line
// applies to — for a traktamente the place it was paid for — is a matter of
// record and is shown, while what it was for is the user's and is typed. That
// column carries no heading: what stands in it is plain from the cell, and a
// heading would have to name something general enough to cover whatever a later
// löneart puts there.
#define LINE_COLUMN_CODE        0
#define LINE_COLUMN_APPLIES_TO  1
#define LINE_COLUMN_COMMENT     2
#define LINE_COLUMN_QUANTITY    3
#define LINE_COLUMN_AMOUNT      4

//-----------------------------------------------------------------------------
SalaryDraftTab::SalaryDraftTab(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    m_stackedWidget = new QStackedWidget();

    // First stacked widget
    m_drafts = new QTableView();
    m_drafts->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_drafts->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_drafts, &QTableView::doubleClicked, this, &SalaryDraftTab::onDoubleClicked);
    connect(m_drafts, &QTableView::clicked, this, &SalaryDraftTab::onDraftSelected);
    m_draftModel = new SalaryDraftTableModel(this);
    m_drafts->setModel(m_draftModel);
    m_stackedWidget->addWidget(m_drafts);

    // Second stacked widget
    m_stackedWidget->addWidget(editWidgetCreate());
    m_stackedWidget->setCurrentIndex(0);
    connect(m_stackedWidget, &QStackedWidget::currentChanged, this, &SalaryDraftTab::onStackedWidgetChanged);

    layout->addWidget(m_stackedWidget);

    // Add the buttons at the bottom
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addStretch(1);

        m_approve = new QPushButton("Approve");
        connect(m_approve, &QPushButton::clicked, this, &SalaryDraftTab::onApprove);
        hbox->addWidget(m_approve);

        m_preview = new QPushButton("Preview");
        connect(m_preview, &QPushButton::clicked, this, &SalaryDraftTab::onPreview);
        hbox->addWidget(m_preview);

        m_new = new QPushButton("New");
        connect(m_new, &QPushButton::clicked, this, &SalaryDraftTab::onNew);
        hbox->addWidget(m_new);

        m_edit = new QPushButton("Edit");
        connect(m_edit, &QPushButton::clicked, this, &SalaryDraftTab::onEdit);
        hbox->addWidget(m_edit);

        m_delete = new QPushButton("Delete");
        connect(m_delete, &QPushButton::clicked, this, &SalaryDraftTab::onDelete);
        hbox->addWidget(m_delete);

        m_cancel = new QPushButton("Cancel");
        connect(m_cancel, &QPushButton::clicked, this, &SalaryDraftTab::onCancel);
        hbox->addWidget(m_cancel);

        m_ok = new QPushButton("Ok");
        connect(m_ok, &QPushButton::clicked, this, &SalaryDraftTab::onOk);
        hbox->addWidget(m_ok);

        updateButtons();
        layout->addLayout(hbox);
    }

    setLayout(layout);
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::load(void)
{
    m_draftModel->load();
    m_drafts->resizeColumnsToContents();
    updateButtons();
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onDatabaseUpdated(void)
{
    load();
}

//-----------------------------------------------------------------------------
QWidget* SalaryDraftTab::editWidgetCreate(void)
{
    QWidget* p_widget = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    auto labelled = [&](const QString& text, QWidget* p_field, Qt::Alignment alignment = Qt::AlignRight) {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel(text + ":");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(alignment);
        hbox->addWidget(p_label);
        hbox->addWidget(p_field);
        return hbox;
    };

    m_employee = new QComboBox();
    {
        QHBoxLayout *hbox = labelled("Employee", m_employee);
        hbox->addStretch(1);
        layout->addLayout(hbox);
    }

    m_payment_date = new QDateEdit();
    m_payment_date->setCalendarPopup(true);
    m_payment_date->setDisplayFormat(DATE_FORMAT_STRING);
    connect(m_payment_date, &QDateEdit::dateChanged, this, &SalaryDraftTab::onPaymentDateChanged);
    {
        QHBoxLayout *hbox = labelled("Utbetalningsdag", m_payment_date);
        hbox->addStretch(1);
        layout->addLayout(hbox);
    }

    // The salary lines, edited in place. Antal and Kronor take a decimal comma.
    {
        m_lines = new QTableWidget(0, 5);
        m_lines->setHorizontalHeaderLabels({ "Löneart", "", "Kommentar", "Antal", "Kronor" });
        // The comment is the one that runs long, so it gets whatever width is
        // left over.
        m_lines->horizontalHeader()->setSectionResizeMode(LINE_COLUMN_COMMENT, QHeaderView::Stretch);
        m_lines->horizontalHeader()->setSectionResizeMode(LINE_COLUMN_CODE, QHeaderView::ResizeToContents);
        m_lines->horizontalHeader()->setSectionResizeMode(LINE_COLUMN_APPLIES_TO, QHeaderView::ResizeToContents);
        m_lines->verticalHeader()->setVisible(false);
        m_lines->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_lines->setSelectionMode(QAbstractItemView::SingleSelection);
        m_lines->setMaximumHeight(240);
        connect(m_lines, &QTableWidget::itemChanged, this, &SalaryDraftTab::onLinesChanged);

        QHBoxLayout *hbox = labelled("Lines", m_lines, Qt::AlignRight | Qt::AlignTop);

        QVBoxLayout *vbox = new QVBoxLayout;
        QPushButton* p_add = new QPushButton("Add line");
        connect(p_add, &QPushButton::clicked, this, &SalaryDraftTab::onAddLine);
        vbox->addWidget(p_add);
        QPushButton* p_traktamente = new QPushButton("Add traktamente");
        p_traktamente->setToolTip("A skattefri line at the schablon of the payment year:\n"
                                 "the inrikes dagtraktamente, or a country's normalbelopp.\n"
                                 "Both are set up on the Company tab.");
        connect(p_traktamente, &QPushButton::clicked, this, &SalaryDraftTab::onAddTraktamenteLine);
        vbox->addWidget(p_traktamente);
        QPushButton* p_remove = new QPushButton("Remove line");
        connect(p_remove, &QPushButton::clicked, this, &SalaryDraftTab::onRemoveLine);
        vbox->addWidget(p_remove);
        vbox->addStretch(1);
        hbox->addLayout(vbox);

        layout->addLayout(hbox);
    }

    m_gross = new QLabel();
    {
        QHBoxLayout *hbox = labelled("Bruttolön", m_gross);
        hbox->addStretch(1);
        layout->addLayout(hbox);
    }

    // The skattefria ersättningar are paid out on top of the bruttolön rather
    // than being part of it, so they are counted up on their own.
    m_taxfree = new QLabel();
    {
        QHBoxLayout *hbox = labelled("Skattefritt", m_taxfree);
        hbox->addStretch(1);
        layout->addLayout(hbox);
    }

    layout->addStretch(1);
    p_widget->setLayout(layout);
    return p_widget;
}

//-----------------------------------------------------------------------------
// Offers the active employees, plus the one the draft already has should they
// have left since.
void SalaryDraftTab::fillEmployees(int selected_id)
{
    m_employee->clear();
    Employees& employees = DataBase::instance()->db()->m_employees;
    for (int i = 0; i < employees.size(); i++)
    {
        Employee* e = employees.at(i);
        if (e->m_active || e->id() == selected_id)
        {
            m_employee->addItem(QString("%1 %2").arg(e->employmentNumber(), e->name()), e->id());
        }
    }
    int index = m_employee->findData(selected_id);
    m_employee->setCurrentIndex(index >= 0 ? index : 0);
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::setLines(const QList<SalaryLine>& lines)
{
    // Filling the table fires itemChanged for every cell; the total is
    // updated once at the end instead.
    m_lines->blockSignals(true);
    m_lines->setRowCount(0);
    for (const SalaryLine& line : lines)
    {
        addLineRow(line);
    }
    m_lines->blockSignals(false);
    onLinesChanged();
}

//-----------------------------------------------------------------------------
// The schablon for `place` in `year`: the inrikes dagtraktamente, or the
// normalbelopp of a country imported for that year. False when the year holds
// nothing for that place, and then what a line already says is left alone: a
// stale amount is worth more than none, and it is the only record there is.
static bool traktamenteOreFor(const QString& place, int year, long long* ore)
{
    const Traktamente& t = DataBase::instance()->db()->m_traktamente;
    if (place == TRAKTAMENTE_DOMESTIC)
    {
        if (!t.hasDomestic(year))
        {
            return false;
        }
        *ore = t.domesticOre(year);
        return true;
    }

    bool found = false;
    const long long value = t.foreignOre(year, place, &found);
    if (!found)
    {
        return false;
    }
    *ore = value;
    return true;
}

//-----------------------------------------------------------------------------
// A cell of the line table, editable or not. One that cannot be edited while
// the cells above and below it can is shown in grey, so that a traktamente's
// schablon is plainly not to be typed over; the löneart and the resmål are
// never editable in any row and need no such telling.
static QTableWidgetItem* cellCreate(const QString& text, bool editable, bool grey)
{
    QTableWidgetItem* p_item = new QTableWidgetItem(text);
    if (!editable)
    {
        p_item->setFlags(p_item->flags() & ~Qt::ItemIsEditable);
        if (grey)
        {
            p_item->setForeground(QBrush(Qt::gray));
        }
    }
    return p_item;
}

//-----------------------------------------------------------------------------
// One row of the line table. What a line is is decided when it is added — Add
// line is a month's salary and Add traktamente is a trip — and not afterwards:
// a löneart written by hand can be neither reported on nor migrated later, and
// the tax treatment of a line is then a matter of opinion.
//
// A traktamente is the schablon of its year for the place it was paid for, both
// of them from what was imported on the Company tab, so neither the amount nor
// the place is typed: a schablon typed over is no longer a schablon, and the
// place is what it was looked up by. What the journey was for is the user's to
// write, and has to be: the employer has to be able to show its purpose as well
// as its place and its days. So the days and the comment are what a traktamente
// line leaves open.
void SalaryDraftTab::addLineRow(const SalaryLine& line)
{
    const SalaryType* p_type = salary_type(line.code);
    const bool taxfree = p_type != nullptr && p_type->kind == SALARY_LINE_TAX_FREE;

    const int row = m_lines->rowCount();
    m_lines->insertRow(row);
    m_lines->setItem(row, LINE_COLUMN_CODE, lineTypeItemCreate(line));
    // A line of ordinary lön applies to nothing in particular, and there is no
    // typing one in either.
    m_lines->setItem(row, LINE_COLUMN_APPLIES_TO, cellCreate(line.applies_to, false, false));
    m_lines->setItem(row, LINE_COLUMN_COMMENT, cellCreate(line.comment, true, true));
    m_lines->setItem(row, LINE_COLUMN_QUANTITY, cellCreate(amount_format_ore(line.quantity), true, true));
    m_lines->setItem(row, LINE_COLUMN_AMOUNT, cellCreate(amount_format_ore(line.amount), !taxfree, true));
}

//-----------------------------------------------------------------------------
// The löneart of a line: shown, since it is what the line is. The number is
// what the line is stored as, so it is kept on the cell rather than read back
// out of the text.
QTableWidgetItem* SalaryDraftTab::lineTypeItemCreate(const SalaryLine& line)
{
    const SalaryType* p_type = salary_type(line.code);
    // A line of an older draft carries a löneart that was typed by hand and
    // means nothing here. It cannot be given one of the fixed lönearter — the
    // benämning it was written with is all there is to go on — so it is shown
    // for what it is, and Ok refuses the draft until the line is removed.
    const QString text = p_type != nullptr
        ? QString("%1 %2").arg(p_type->number).arg(p_type->name)
        : QString("(okänd löneart) %1").arg(line.text).trimmed();

    QTableWidgetItem* p_item = cellCreate(text, false, false);
    p_item->setData(Qt::UserRole, p_type != nullptr ? p_type->number : SALARY_TYPE_NONE);
    return p_item;
}

//-----------------------------------------------------------------------------
// The löneart of `row`, or SALARY_TYPE_NONE when it has none.
int SalaryDraftTab::lineTypeOf(int row) const
{
    QTableWidgetItem* p_item = m_lines->item(row, LINE_COLUMN_CODE);
    return p_item != nullptr ? p_item->data(Qt::UserRole).toInt() : SALARY_TYPE_NONE;
}

//-----------------------------------------------------------------------------
// Reads the line table back. Tells the user what is wrong and returns false if
// the lines do not make sense.
bool SalaryDraftTab::readLines(QList<SalaryLine>* lines)
{
    QList<QStringList> rows;
    for (int row = 0; row < m_lines->rowCount(); row++)
    {
        // The cells go over in the order the parser expects, which is not the
        // order they are shown in: Avser and Kommentar stand beside the löneart
        // but are read after the figures.
        QStringList cells;
        cells.append(QString::number(lineTypeOf(row)));
        for (int column : { LINE_COLUMN_QUANTITY, LINE_COLUMN_AMOUNT, LINE_COLUMN_COMMENT, LINE_COLUMN_APPLIES_TO })
        {
            QTableWidgetItem* p_item = m_lines->item(row, column);
            cells.append(p_item ? p_item->text() : QString());
        }
        rows.append(cells);
    }

    QString error;
    if (!salary_parse_lines(rows, lines, &error))
    {
        QMessageBox::warning(this, "Salary draft", error);
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::openEditWidget(const SalaryDraft& d)
{
    m_draft_id = d.id();
    fillEmployees(d.m_employee_id);
    // Filling the date in is not the user changing the payment year: the lines
    // that follow are the draft's own, at the amounts it was stored with.
    m_payment_date->blockSignals(true);
    m_payment_date->setDate(d.m_payment_date.isValid() ? d.m_payment_date : QDate::currentDate());
    m_payment_date->blockSignals(false);
    setLines(d.m_lines);

    m_stackedWidget->setCurrentIndex(1);
    updateButtons();
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::updateButtons(void)
{
    const bool list = m_stackedWidget->currentIndex() == 0;
    const bool selected = m_drafts->currentIndex().row() >= 0;
    const QString disabled = "QPushButton:disabled { color: gray; background-color: lightgray; }";

    for (QPushButton* p : { m_approve, m_preview, m_edit, m_delete })
    {
        p->setVisible(list);
        p->setEnabled(selected);
        p->setStyleSheet(selected ? "" : disabled);
    }

    m_new->setVisible(list);
    m_cancel->setVisible(!list);
    m_ok->setVisible(!list);
}

//-----------------------------------------------------------------------------
bool SalaryDraftTab::confirmAction(QWidget* parent, const QString& title, const QString& message)
{
    QMessageBox messageBox(parent);
    messageBox.setWindowTitle(title);
    messageBox.setText(message);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    messageBox.setDefaultButton(QMessageBox::No);
    messageBox.setIcon(QMessageBox::Question);

    // This line makes the dialog modal, blocking the GUI until it's answered
    int result = messageBox.exec();

    return (result == QMessageBox::Yes);
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onStackedWidgetChanged(int index)
{
    emit editing(index > 0);
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onDoubleClicked(const QModelIndex &index)
{
    SalaryDraft* p_draft = m_draftModel->getSalaryDraft(index);
    if (p_draft != nullptr)
    {
        openEditWidget(*p_draft);
    }
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onDraftSelected(const QModelIndex &index)
{
    Q_UNUSED(index)
    updateButtons();
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onApprove(void)
{
    SalaryDraft* p_draft = m_draftModel->getSalaryDraft(m_drafts->currentIndex());
    if (p_draft == nullptr)
    {
        updateButtons();
        return;
    }

    QString error;
    std::unique_ptr<SalarySlip> p_slip = SalaryFactory::create(*p_draft, &error);
    if (p_slip == nullptr)
    {
        QMessageBox::warning(this, "Approve salary", error);
        return;
    }
    if (!SalarySlipPrinter::fits(*p_slip))
    {
        QMessageBox::warning(this, "Approve salary", "The lines do not fit on one page. Shorten the kommentarer, or use fewer lines.");
        return;
    }

    // A slip whose only lines are skattefria has no bruttolön and so no
    // skattetabell was applied; saying "tabell 0" would be claiming one was.
    const QString tabell = p_slip->m_tax_table > 0
        ? QString(" (tabell %1)").arg(p_slip->m_tax_table)
        : QString();
    QString message = QString("Approve the salary of %1, paid %2?\n\nBruttolön %3 kr\nSkatt %4 kr%5\n")
        .arg(p_slip->m_employee_name, p_slip->m_payment_date.toString(DATE_FORMAT_STRING),
             amount_format_ore(p_slip->m_gross), amount_format_ore(p_slip->m_tax), tabell);
    if (p_slip->m_tax_free != 0)
    {
        message += QString("Skattefritt %1 kr\n").arg(amount_format_ore(p_slip->m_tax_free));
    }
    message += QString("Utbetalt %1 kr").arg(amount_format_ore(p_slip->m_net));
    if (confirmAction(this, "Approve salary", message))
    {
        DataBase::instance()->db()->m_salaryslips.append(std::move(p_slip));
        DataBase::instance()->db()->m_salarydrafts.remove(p_draft);
        DataBase::instance()->setModified();
        load();
    }
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onPreview(void)
{
    SalaryDraft* p_draft = m_draftModel->getSalaryDraft(m_drafts->currentIndex());
    if (p_draft == nullptr)
    {
        updateButtons();
        return;
    }

    // The slip is built exactly as Approve would, but not stored.
    QString error;
    std::unique_ptr<SalarySlip> p_slip = SalaryFactory::create(*p_draft, &error);
    if (p_slip == nullptr)
    {
        QMessageBox::warning(this, "Preview salary", error);
        return;
    }
    if (!SalarySlipPrinter::fits(*p_slip))
    {
        QMessageBox::warning(this, "Preview salary", "The lines do not fit on one page. Shorten the kommentarer, or use fewer lines.");
        return;
    }
    SalarySlipTab::showPdf(*p_slip, true);
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onNew(void)
{
    if (DataBase::instance()->db()->m_employees.size() == 0)
    {
        QMessageBox::information(this, "Salary draft", "Add an employee on the Employees tab first.");
        return;
    }

    SalaryDraft d;
    SalaryLine line;
    line.code = SALARY_TYPE_MONTHLY_SALARY;
    d.m_lines.append(line);
    openEditWidget(d);
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onEdit(void)
{
    onDoubleClicked(m_drafts->currentIndex());
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onDelete(void)
{
    SalaryDraft* p_draft = m_draftModel->getSalaryDraft(m_drafts->currentIndex());
    if (p_draft == nullptr)
    {
        updateButtons();
        return;
    }

    if (confirmAction(this, "Delete salary draft", QString("Do you want to delete the salary draft paid %1?").arg(p_draft->m_payment_date.toString(DATE_FORMAT_STRING))))
    {
        DataBase::instance()->db()->m_salarydrafts.remove(p_draft);
        DataBase::instance()->setModified();
        load();
    }
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onCancel(void)
{
    m_stackedWidget->setCurrentIndex(0);
    updateButtons();
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onOk(void)
{
    SalaryDraft d;
    if (!readLines(&d.m_lines))
    {
        return;
    }
    if (m_employee->currentIndex() < 0)
    {
        QMessageBox::warning(this, "Salary draft", "Choose an employee.");
        return;
    }

    d.setId(m_draft_id);
    d.m_employee_id = m_employee->currentData().toInt();
    d.m_payment_date = m_payment_date->date();

    DataBase::instance()->db()->m_salarydrafts.update(d);
    DataBase::instance()->setModified();
    m_stackedWidget->setCurrentIndex(0);
    load();
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onAddLine(void)
{
    if (m_lines->rowCount() >= SALARY_MAX_LINES)
    {
        QMessageBox::information(this, "Salary draft", QString("A salary statement holds at most %1 lines.").arg(SALARY_MAX_LINES));
        return;
    }

    // A line is a month's salary: it is what Add line is for, and a traktamente
    // has a button of its own. What is left to say is the amount.
    SalaryLine line;
    line.code = SALARY_TYPE_MONTHLY_SALARY;

    const int row = m_lines->rowCount();
    m_lines->blockSignals(true);
    addLineRow(line);
    m_lines->blockSignals(false);
    m_lines->setCurrentCell(row, LINE_COLUMN_AMOUNT);
    m_lines->editItem(m_lines->item(row, LINE_COLUMN_AMOUNT));
    onLinesChanged();
}

//-----------------------------------------------------------------------------
// A traktamente line at the schablon of the payment year: the inrikes
// dagtraktamente, or the normalbelopp of one of the countries imported for that
// year. Antal is the number of days, which is the one thing the user has to
// fill in; a halvdag or a nattschablon is half of what stands here.
void SalaryDraftTab::onAddTraktamenteLine(void)
{
    if (m_lines->rowCount() >= SALARY_MAX_LINES)
    {
        QMessageBox::information(this, "Salary draft", QString("A salary statement holds at most %1 lines.").arg(SALARY_MAX_LINES));
        return;
    }

    const Traktamente& t = DataBase::instance()->db()->m_traktamente;
    const int year = m_payment_date->date().year();

    // What is set up for the year of the payment, since that is the year the
    // slip belongs to. Inrikes first: it is the common case.
    QStringList choices;
    if (t.hasDomestic(year))
    {
        choices.append(TRAKTAMENTE_DOMESTIC);
    }
    for (const TraktamenteCountry& c : t.countries(year))
    {
        choices.append(c.country);
    }

    if (choices.isEmpty())
    {
        QMessageBox::information(this, "Traktamente",
                                 QString("No traktamente is set up for %1. Enter the inrikes dagtraktamente or "
                                         "import Skatteverket's normalbelopp on the Company tab.").arg(year));
        return;
    }

    bool ok = false;
    const QString choice = QInputDialog::getItem(this, "Traktamente", "Vart gick resan?", choices, 0, false, &ok);
    if (!ok || choice.isEmpty())
    {
        return;
    }

    SalaryLine line;
    line.code = SALARY_TYPE_TRAKTAMENTE;
    line.kind = SALARY_LINE_TAX_FREE;
    line.quantity = 100; // one day, in hundredths
    traktamenteOreFor(choice, year, &line.amount);
    // The benämning is the löneart's and says only what kind of line it is, so
    // where the trip went is what the line applies to. It is what the schablon
    // was looked up by, and is not typed over for the same reason the amount is
    // not. What the journey was for is left for the user to write.
    line.applies_to = choice;

    const int row = m_lines->rowCount();
    m_lines->blockSignals(true);
    addLineRow(line);
    m_lines->blockSignals(false);
    // Antal is what is left to say, so the cursor starts there.
    m_lines->setCurrentCell(row, LINE_COLUMN_QUANTITY);
    m_lines->editItem(m_lines->item(row, LINE_COLUMN_QUANTITY));
    onLinesChanged();
}

//-----------------------------------------------------------------------------
// A traktamente is the schablon of the year it is paid in, and that is the
// payment date's year. The amount is not the user's to type, so it follows the
// date rather than standing at what another year's schablon happened to be.
void SalaryDraftTab::onPaymentDateChanged(void)
{
    const int year = m_payment_date->date().year();

    m_lines->blockSignals(true);
    for (int row = 0; row < m_lines->rowCount(); row++)
    {
        const SalaryType* p_type = salary_type(lineTypeOf(row));
        if (p_type == nullptr || p_type->kind != SALARY_LINE_TAX_FREE)
        {
            continue;
        }
        QTableWidgetItem* p_place = m_lines->item(row, LINE_COLUMN_APPLIES_TO);
        QTableWidgetItem* p_amount = m_lines->item(row, LINE_COLUMN_AMOUNT);
        long long ore = 0;
        if (p_place != nullptr && p_amount != nullptr
            && traktamenteOreFor(p_place->text(), year, &ore))
        {
            p_amount->setText(amount_format_ore(ore));
        }
    }
    m_lines->blockSignals(false);
    onLinesChanged();
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onRemoveLine(void)
{
    int row = m_lines->currentRow();
    if (row >= 0)
    {
        m_lines->removeRow(row);
        onLinesChanged();
    }
}

//-----------------------------------------------------------------------------
void SalaryDraftTab::onLinesChanged(void)
{
    // Keeps the bruttolön and the skattefria ersättningar in view while the
    // lines are edited. Lines that are not complete yet are skipped rather than
    // reported.
    long long gross = 0;
    long long tax_free = 0;
    for (int row = 0; row < m_lines->rowCount(); row++)
    {
        QTableWidgetItem* p_quantity = m_lines->item(row, LINE_COLUMN_QUANTITY);
        QTableWidgetItem* p_amount = m_lines->item(row, LINE_COLUMN_AMOUNT);
        const SalaryType* p_type = salary_type(lineTypeOf(row));
        bool ok_quantity = false, ok_amount = false;
        SalaryLine line;
        line.quantity = p_quantity ? (int)amount_parse_ore(p_quantity->text(), &ok_quantity) : 0;
        line.amount = p_amount ? amount_parse_ore(p_amount->text(), &ok_amount) : 0;
        if (ok_quantity && ok_amount && p_type != nullptr)
        {
            if (p_type->kind == SALARY_LINE_TAX_FREE)
            {
                tax_free += line.total_ore();
            }
            else
            {
                gross += line.total_ore();
            }
        }
    }
    m_gross->setText(amount_format_ore(gross) + " kr");
    m_taxfree->setText(amount_format_ore(tax_free) + " kr");
}
