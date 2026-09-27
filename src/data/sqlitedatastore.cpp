#include "sqlitedatastore.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDate>
#include <QDateTime>
#include <QDebug>

#include <memory>

// Bumped whenever the stored shape or the meaning of a stored value changes.
// save() stamps it and load() migrates anything older up to it, so the two must
// be kept in step.
static const int DB_SCHEMA_VERSION = 3;

// SQLite schema for the datastore
static const char *DB_SCHEMA_SQL = R"SQL(
PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;

CREATE TABLE IF NOT EXISTS schema_migrations (
    version INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    applied_at TEXT NOT NULL DEFAULT (datetime('now')),
    checksum TEXT
);

CREATE TABLE IF NOT EXISTS companyinfo (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    uuid TEXT UNIQUE,
    legacy_id TEXT,
    name TEXT,
    adrsline1 TEXT,
    adrsline2 TEXT,
    org_number TEXT,
    bankgiro TEXT,
    swift_bic TEXT,
    phone TEXT,
    vat_number TEXT,
    iban TEXT,
    email TEXT,
    web TEXT,
    logo TEXT,
    fiscal_year_start_month INTEGER NOT NULL DEFAULT 1,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now')),
    CHECK(id = 1)
);

CREATE TABLE IF NOT EXISTS customers (
    id INTEGER PRIMARY KEY,
    uuid TEXT UNIQUE,
    legacy_id TEXT,
    name TEXT NOT NULL,
    org_number TEXT,
    adrsline1 TEXT,
    adrsline2 TEXT,
    email TEXT,
    email_invoice TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS agreements (
    id INTEGER PRIMARY KEY,
    uuid TEXT UNIQUE,
    legacy_id TEXT,
    customer_id INTEGER NOT NULL REFERENCES customers(id) ON DELETE CASCADE,
    active INTEGER NOT NULL DEFAULT 1,
    agreement_name TEXT,
    agreement_id TEXT,
    reference_name TEXT,
    reference_email TEXT,
    reference_phone TEXT,
    reference_name_ours TEXT,
    -- hourly rates in öre, see src/util/amount.h
    standard_hourly_rate INTEGER,
    overtime_hourly_rate INTEGER,
    qualified_hourly_rate INTEGER,
    traveltime_hourly_rate INTEGER,
    payment_terms_days INTEGER,
    late_payment_interest INTEGER,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS invoices (
    id INTEGER PRIMARY KEY,
    uuid TEXT UNIQUE,
    legacy_id TEXT,
    year INTEGER,
    count INTEGER,
    number TEXT,
    date TEXT,
    due_date TEXT,
    company_name TEXT,
    company_adrsline1 TEXT,
    company_adrsline2 TEXT,
    company_reference_name TEXT,
    company_org_number TEXT,
    company_bankgiro TEXT,
    company_phone TEXT,
    company_vat_number TEXT,
    company_swift_bic TEXT,
    company_email TEXT,
    company_f_skatt TEXT,
    company_iban TEXT,
    company_web TEXT,

    customer_id INTEGER REFERENCES customers(id),
    customer_name TEXT,
    customer_adrsline1 TEXT,
    customer_adrsline2 TEXT,
    customer_reference_name TEXT,
    customer_email_invoice TEXT,

    agreement_id TEXT,
    agreement_name TEXT,
    agreement_key INTEGER,
    agreement_payment_terms_days INTEGER,
    agreement_late_payment_interest INTEGER,

    employee_id INTEGER,
    consultant_name TEXT,
    description_consultant_period TEXT,

    hours_standard INTEGER,
    agreement_standard_hourly_rate INTEGER,
    agreement_standard_vat INTEGER,
    sum_standard INTEGER,
    description_standard TEXT,

    hours_overtime INTEGER,
    agreement_overtime_hourly_rate INTEGER,
    agreement_overtime_vat INTEGER,
    sum_overtime INTEGER,
    description_overtime TEXT,

    hours_qualified INTEGER,
    agreement_qualified_hourly_rate INTEGER,
    agreement_qualified_vat INTEGER,
    sum_qualified INTEGER,
    description_qualified TEXT,

    hours_traveltime INTEGER,
    agreement_traveltime_hourly_rate INTEGER,
    agreement_traveltime_vat INTEGER,
    sum_traveltime INTEGER,
    description_traveltime TEXT,

    comment TEXT,
    sum INTEGER,
    vat INTEGER,
    sum_including_vat INTEGER,
    is_credit INTEGER DEFAULT 0,
    credited_invoice_number TEXT DEFAULT '',
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS invoicedrafts (
    id INTEGER PRIMARY KEY,
    uuid TEXT UNIQUE,
    legacy_id TEXT,
    invoice_year INTEGER,
    invoice_number INTEGER,
    invoice_date TEXT,
    customer_id INTEGER,
    agreement_id INTEGER,
    employee_id INTEGER,
    month INTEGER,
    hours_standard INTEGER,
    hours_overtime INTEGER,
    hours_qualified INTEGER,
    hours_traveltime INTEGER,
    comment TEXT,
    is_credit INTEGER DEFAULT 0,
    credited_invoice_number TEXT DEFAULT '',
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS employees (
    id INTEGER PRIMARY KEY,
    first_name TEXT NOT NULL,
    last_name TEXT NOT NULL,
    adrsline1 TEXT,
    adrsline2 TEXT,
    personnummer TEXT,
    bank_account TEXT,
    active INTEGER NOT NULL DEFAULT 1,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS employee_tax_years (
    employee_id INTEGER NOT NULL REFERENCES employees(id) ON DELETE CASCADE,
    year INTEGER NOT NULL,
    table_number INTEGER NOT NULL,
    PRIMARY KEY (employee_id, year)
);

-- Skatteverket's monthly tax tables, Kolumn 1 only. income_to 0 is open-ended,
-- and percent 1 means tax is a percentage of the income rather than kronor.
CREATE TABLE IF NOT EXISTS taxtable_rows (
    year INTEGER NOT NULL,
    table_number INTEGER NOT NULL,
    income_from INTEGER NOT NULL,
    income_to INTEGER NOT NULL,
    percent INTEGER NOT NULL,
    tax INTEGER NOT NULL,
    PRIMARY KEY (year, table_number, income_from)
);

-- Salary drafts and the salary slips approved from them. Amounts in öre,
-- quantities in hundredths, see src/util/amount.h. A line's kind is 0 for
-- ordinary lön and 1 for a skattefri ersättning, which is paid out without
-- being taxed and is no part of the bruttolön. Its code is the number of a
-- löneart in the fixed table in src/data/salarytypes.h, which the text and the
-- kind both come from. The column was free text before the lönearter were
-- fixed and is left as TEXT rather than rebuilt for the sake of a type.
CREATE TABLE IF NOT EXISTS salarydrafts (
    id INTEGER PRIMARY KEY,
    employee_id INTEGER,
    payment_date TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS salarydraft_lines (
    draft_id INTEGER NOT NULL REFERENCES salarydrafts(id) ON DELETE CASCADE,
    position INTEGER NOT NULL,
    code TEXT,
    text TEXT,
    quantity INTEGER NOT NULL,
    amount INTEGER NOT NULL,
    kind INTEGER NOT NULL DEFAULT 0,
    comment TEXT,
    applies_to TEXT,
    PRIMARY KEY (draft_id, position)
);

CREATE TABLE IF NOT EXISTS employer_fee_rates (
    year INTEGER PRIMARY KEY,
    standard INTEGER NOT NULL,
    senior INTEGER NOT NULL
);

-- Traktamente, in öre. Inrikes is the helt dagtraktamente of a year, typed in
-- on the Company tab. Utrikes is Skatteverket's normalbelopp per country,
-- imported a year at a time, and position keeps the order the file listed them
-- in.
CREATE TABLE IF NOT EXISTS traktamente_domestic (
    year INTEGER PRIMARY KEY,
    full_day INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS traktamente_foreign (
    year INTEGER NOT NULL,
    country TEXT NOT NULL,
    amount INTEGER NOT NULL,
    position INTEGER NOT NULL,
    PRIMARY KEY (year, country)
);

CREATE TABLE IF NOT EXISTS salaryslips (
    id INTEGER PRIMARY KEY,
    payment_date TEXT,
    company_name TEXT,
    company_adrsline1 TEXT,
    company_adrsline2 TEXT,
    company_org_number TEXT,
    employee_id INTEGER,
    employee_name TEXT,
    employee_adrsline1 TEXT,
    employee_adrsline2 TEXT,
    employment_number TEXT,
    personnummer TEXT,
    bank_account TEXT,
    tax_table INTEGER,
    employer_fee_rate INTEGER,
    employer_fee INTEGER,
    gross INTEGER,
    tax INTEGER,
    tax_free INTEGER,
    net INTEGER,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS salaryslip_lines (
    slip_id INTEGER NOT NULL REFERENCES salaryslips(id) ON DELETE CASCADE,
    position INTEGER NOT NULL,
    code TEXT,
    text TEXT,
    quantity INTEGER NOT NULL,
    amount INTEGER NOT NULL,
    kind INTEGER NOT NULL DEFAULT 0,
    comment TEXT,
    applies_to TEXT,
    PRIMARY KEY (slip_id, position)
);

CREATE INDEX IF NOT EXISTS idx_invoices_customer_id ON invoices(customer_id);
CREATE INDEX IF NOT EXISTS idx_agreements_customer_id ON agreements(customer_id);
CREATE INDEX IF NOT EXISTS idx_customers_uuid ON customers(uuid);
CREATE INDEX IF NOT EXISTS idx_invoices_uuid ON invoices(uuid);

-- Triggers to automatically update the updated_at column on row changes
CREATE TRIGGER IF NOT EXISTS trg_customers_updated_at
AFTER UPDATE ON customers
FOR EACH ROW
WHEN NEW.updated_at = OLD.updated_at
BEGIN
    UPDATE customers SET updated_at = datetime('now') WHERE id = OLD.id;
END;

CREATE TRIGGER IF NOT EXISTS trg_agreements_updated_at
AFTER UPDATE ON agreements
FOR EACH ROW
WHEN NEW.updated_at = OLD.updated_at
BEGIN
    UPDATE agreements SET updated_at = datetime('now') WHERE id = OLD.id;
END;

CREATE TRIGGER IF NOT EXISTS trg_invoices_updated_at
AFTER UPDATE ON invoices
FOR EACH ROW
WHEN NEW.updated_at = OLD.updated_at
BEGIN
    UPDATE invoices SET updated_at = datetime('now') WHERE id = OLD.id;
END;

CREATE TRIGGER IF NOT EXISTS trg_invoicedrafts_updated_at
AFTER UPDATE ON invoicedrafts
FOR EACH ROW
WHEN NEW.updated_at = OLD.updated_at
BEGIN
    UPDATE invoicedrafts SET updated_at = datetime('now') WHERE id = OLD.id;
END;

CREATE TRIGGER IF NOT EXISTS trg_employees_updated_at
AFTER UPDATE ON employees
FOR EACH ROW
WHEN NEW.updated_at = OLD.updated_at
BEGIN
    UPDATE employees SET updated_at = datetime('now') WHERE id = OLD.id;
END;

CREATE TRIGGER IF NOT EXISTS trg_salarydrafts_updated_at
AFTER UPDATE ON salarydrafts
FOR EACH ROW
WHEN NEW.updated_at = OLD.updated_at
BEGIN
    UPDATE salarydrafts SET updated_at = datetime('now') WHERE id = OLD.id;
END;

CREATE TRIGGER IF NOT EXISTS trg_salaryslips_updated_at
AFTER UPDATE ON salaryslips
FOR EACH ROW
WHEN NEW.updated_at = OLD.updated_at
BEGIN
    UPDATE salaryslips SET updated_at = datetime('now') WHERE id = OLD.id;
END;
)SQL";

#include "business.h"

//-----------------------------------------------------------------------------
QString database_backup_path(const QString& filename, const QString& label)
{
    const QFileInfo fi(filename);
    const QString suffix = fi.suffix().isEmpty() ? QString() : "." + fi.suffix();
    const QString base = QString("%1-backup-%2-%3")
                             .arg(fi.completeBaseName(), label,
                                  QDate::currentDate().toString("yyyyMMdd"));

    QString candidate = fi.dir().filePath(base + suffix);
    for (int n = 2; QFile::exists(candidate); n++) {
        candidate = fi.dir().filePath(QString("%1-%2%3").arg(base).arg(n).arg(suffix));
    }
    return candidate;
}

//-----------------------------------------------------------------------------
// Runs DB_SCHEMA_SQL against `db`. Apart from the pragmas every statement in it
// is CREATE ... IF NOT EXISTS, so on an older file this creates exactly what
// the file lacks.
static bool execSchema(QSqlDatabase& db)
{
    QSqlQuery q(db);
    QString schema = QString::fromUtf8(DB_SCHEMA_SQL);
    // Execute statements but keep CREATE TRIGGER ... END; blocks intact
    QStringList parts = schema.split(';', Qt::SkipEmptyParts);
    QString acc;
    for (QString part : parts) {
        part = part.trimmed();
        if (part.isEmpty()) continue;

        bool containsTrigger = part.toUpper().contains("CREATE TRIGGER");
        if (!acc.isEmpty()) {
            // we are inside a trigger definition; append this part (restore the missing semicolon between statements)
            acc += ";\n" + part;
            if (part.toUpper().contains("END")) {
                QString toExec = acc.trimmed();
                if (!toExec.endsWith(';')) toExec += ';';
                if (!q.exec(toExec)) {
                    qWarning() << "Failed to execute schema statement:" << q.lastError().text() << "Statement:" << toExec;
                    return false;
                }
                acc.clear();
            }
        } else if (containsTrigger) {
            // start a new trigger accumulator; strip any leading comments before CREATE TRIGGER
            int idx = part.toUpper().indexOf("CREATE TRIGGER");
            QString triggerStart = part.mid(idx);
            acc = triggerStart;
            if (part.toUpper().contains("END")) {
                QString toExec = acc.trimmed();
                if (!toExec.endsWith(';')) toExec += ';';
                if (!q.exec(toExec)) {
                    qWarning() << "Failed to execute schema statement:" << q.lastError().text() << "Statement:" << toExec;
                    return false;
                }
                acc.clear();
            }
        } else {
            // normal statement
            if (!q.exec(part + ";")) {
                qWarning() << "Failed to execute schema statement:" << q.lastError().text() << "Statement:" << part;
                return false;
            }
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
// Writes the lines of a draft or a slip into `table`, whose owner column is
// `owner_column`.
static bool insertSalaryLines(QSqlDatabase& db, const QString& table, const QString& owner_column, int owner_id, const QList<SalaryLine>& lines)
{
    QSqlQuery ins(db);
    ins.prepare(QString("INSERT INTO %1 (%2, position, code, text, quantity, amount, kind, comment, applies_to) VALUES (:owner, :pos, :code, :text, :qty, :amount, :kind, :comment, :applies_to)").arg(table, owner_column));
    for (int i = 0; i < lines.size(); ++i) {
        ins.bindValue(":owner", owner_id);
        ins.bindValue(":pos", i);
        ins.bindValue(":code", lines.at(i).code);
        ins.bindValue(":text", lines.at(i).text);
        ins.bindValue(":qty", lines.at(i).quantity);
        ins.bindValue(":amount", lines.at(i).amount);
        ins.bindValue(":kind", lines.at(i).kind);
        ins.bindValue(":comment", lines.at(i).comment);
        ins.bindValue(":applies_to", lines.at(i).applies_to);
        if (!ins.exec()) {
            qWarning() << "Failed to insert into" << table << ins.lastError().text();
            return false;
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
// Reads back what insertSalaryLines() wrote, in their original order.
static QList<SalaryLine> selectSalaryLines(QSqlDatabase& db, const QString& table, const QString& owner_column, int owner_id)
{
    QList<SalaryLine> lines;
    QSqlQuery q(db);
    q.prepare(QString("SELECT code, text, quantity, amount, kind, comment, applies_to FROM %1 WHERE %2 = :owner ORDER BY position").arg(table, owner_column));
    q.bindValue(":owner", owner_id);
    if (q.exec()) {
        while (q.next()) {
            SalaryLine line;
            line.code = q.value(0).toInt();
            line.text = q.value(1).toString();
            line.quantity = q.value(2).toInt();
            line.amount = q.value(3).toLongLong();
            line.kind = q.value(4).toInt();
            line.comment = q.value(5).toString();
            line.applies_to = q.value(6).toString();
            lines.append(line);
        }
    }
    return lines;
}

bool SqliteDataStore::save(Business* b, const QString& filename)
{
    QFileInfo fi(filename);
    if (!fi.dir().exists()) {
        qDebug() << "Target directory does not exist:" << fi.dir().absolutePath();
        return false;
    }

    if (QFile::exists(filename)) {
        if (!QFile::remove(filename)) {
            qWarning() << "Failed to remove existing target DB:" << filename;
            return false;
        }
    }

    const QString connName = "datastore_save_conn";
    auto work = [&]() -> bool {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(filename);
        if (!db.open()) {
            qWarning() << "Failed to open target sqlite DB:" << db.lastError().text();
            return false;
        }

        if (!execSchema(db)) {
            db.close();
            return false;
        }

        if (!db.transaction()) {
            qWarning() << "Failed to start transaction:" << db.lastError().text();
        }

        QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        // Insert companyinfo
        {
            QSqlQuery ins(db);
            ins.prepare("INSERT INTO companyinfo (uuid, legacy_id, name, adrsline1, adrsline2, org_number, bankgiro, swift_bic, phone, vat_number, iban, email, web, logo, fiscal_year_start_month, created_at, updated_at) VALUES (:uuid, :legacy, :name, :a1, :a2, :org, :bank, :swift, :phone, :vat, :iban, :email, :web, :logo, :fiscal, :created_at, :updated_at)");
            ins.bindValue(":uuid", QUuid::createUuid().toString());
            ins.bindValue(":legacy", QString());
            ins.bindValue(":name", b->m_companyinfo.m_name);
            ins.bindValue(":a1", b->m_companyinfo.m_adrsline1);
            ins.bindValue(":a2", b->m_companyinfo.m_adrsline2);
            ins.bindValue(":org", b->m_companyinfo.m_org_number);
            ins.bindValue(":bank", b->m_companyinfo.m_bankgiro);
            ins.bindValue(":swift", b->m_companyinfo.m_swift_bic);
            ins.bindValue(":phone", b->m_companyinfo.m_phone);
            ins.bindValue(":vat", b->m_companyinfo.m_vat_number);
            ins.bindValue(":iban", b->m_companyinfo.m_iban);
            ins.bindValue(":email", b->m_companyinfo.m_email);
            ins.bindValue(":web", b->m_companyinfo.m_web);
            ins.bindValue(":logo", b->m_companyinfo.m_logo);
            ins.bindValue(":fiscal", b->m_companyinfo.m_fiscal_year_start_month);
            ins.bindValue(":created_at", now);
            ins.bindValue(":updated_at", now);
            if (!ins.exec()) qWarning() << "companyinfo insert failed:" << ins.lastError().text();
        }

        // Insert customers and agreements
        for (int i = 0; i < b->m_customers.size(); ++i) {
            Customer* c = b->m_customers.at(i);
            QSqlQuery insC(db);
            insC.prepare("INSERT INTO customers (id, uuid, legacy_id, name, org_number, adrsline1, adrsline2, email, email_invoice, created_at, updated_at) VALUES (:id, :uuid, :legacy, :name, :org, :a1, :a2, :email, :email_invoice, :created_at, :updated_at)");
            insC.bindValue(":id", c->id());
            insC.bindValue(":uuid", QUuid::createUuid().toString());
            insC.bindValue(":legacy", QString::number(c->id()));
            insC.bindValue(":name", c->m_name);
            insC.bindValue(":org", c->m_org_number);
            insC.bindValue(":a1", c->m_adrsline1);
            insC.bindValue(":a2", c->m_adrsline2);
            insC.bindValue(":email", c->m_email);
            insC.bindValue(":email_invoice", c->m_email_invoice);
            insC.bindValue(":created_at", now);
            insC.bindValue(":updated_at", now);
            if (!insC.exec()) {
                qWarning() << "Failed to insert customer" << c->m_name << insC.lastError().text();
            }

            for (int j = 0; j < c->m_agreements.size(); ++j) {
                Agreement* a = c->m_agreements.at(j);
                QSqlQuery insA(db);
                insA.prepare("INSERT INTO agreements (id, uuid, legacy_id, customer_id, active, agreement_name, agreement_id, reference_name, reference_email, reference_phone, reference_name_ours, standard_hourly_rate, overtime_hourly_rate, qualified_hourly_rate, traveltime_hourly_rate, payment_terms_days, late_payment_interest, created_at, updated_at) VALUES (:id, :uuid, :legacy, :cid, :active, :name, :aid, :ref_name, :ref_email, :ref_phone, :ref_ours, :std_rate, :ot_rate, :q_rate, :tt_rate, :pay_terms, :late, :created_at, :updated_at)");
                insA.bindValue(":id", a->id());
                insA.bindValue(":uuid", QUuid::createUuid().toString());
                insA.bindValue(":legacy", QString::number(a->id()));
                insA.bindValue(":cid", c->id());
                insA.bindValue(":active", a->m_active ? 1 : 0);
                insA.bindValue(":name", a->m_agreement_name);
                insA.bindValue(":aid", a->m_agreement_id);
                insA.bindValue(":ref_name", a->m_reference_name);
                insA.bindValue(":ref_email", a->m_reference_email);
                insA.bindValue(":ref_phone", a->m_reference_phone);
                insA.bindValue(":ref_ours", a->m_reference_name_ours);
                insA.bindValue(":std_rate", a->m_standard_hourly_rate);
                insA.bindValue(":ot_rate", a->m_overtime_hourly_rate);
                insA.bindValue(":q_rate", a->m_qualified_hourly_rate);
                insA.bindValue(":tt_rate", a->m_traveltime_hourly_rate);
                insA.bindValue(":pay_terms", a->m_payment_terms_days);
                insA.bindValue(":late", a->m_late_payment_interest);
                insA.bindValue(":created_at", now);
                insA.bindValue(":updated_at", now);
                if (!insA.exec()) qWarning() << "Failed to insert agreement for customer" << c->m_name << insA.lastError().text();
            }
        }

        // Employees and their tax years
        for (int i = 0; i < b->m_employees.size(); ++i) {
            Employee* e = b->m_employees.at(i);
            QSqlQuery insE(db);
            insE.prepare("INSERT INTO employees (id, first_name, last_name, adrsline1, adrsline2, personnummer, bank_account, active, created_at, updated_at) VALUES (:id, :first, :last, :a1, :a2, :pnr, :bank, :active, :created_at, :updated_at)");
            insE.bindValue(":id", e->id());
            insE.bindValue(":first", e->m_first_name);
            insE.bindValue(":last", e->m_last_name);
            insE.bindValue(":a1", e->m_adrsline1);
            insE.bindValue(":a2", e->m_adrsline2);
            insE.bindValue(":pnr", e->m_personnummer);
            insE.bindValue(":bank", e->m_bank_account);
            insE.bindValue(":active", e->m_active ? 1 : 0);
            insE.bindValue(":created_at", now);
            insE.bindValue(":updated_at", now);
            if (!insE.exec()) qWarning() << "Failed to insert employee" << e->name() << insE.lastError().text();

            for (auto it = e->m_tax_tables.constBegin(); it != e->m_tax_tables.constEnd(); ++it) {
                QSqlQuery insT(db);
                insT.prepare("INSERT INTO employee_tax_years (employee_id, year, table_number) VALUES (:eid, :year, :table)");
                insT.bindValue(":eid", e->id());
                insT.bindValue(":year", it.key());
                insT.bindValue(":table", it.value());
                if (!insT.exec()) qWarning() << "Failed to insert tax year for employee" << e->name() << insT.lastError().text();
            }
        }

        // Arbetsgivaravgifter
        for (int year : b->m_employerfees.years()) {
            QSqlQuery insF(db);
            insF.prepare("INSERT INTO employer_fee_rates (year, standard, senior) VALUES (:year, :standard, :senior)");
            insF.bindValue(":year", year);
            insF.bindValue(":standard", b->m_employerfees.rate(year).standard);
            insF.bindValue(":senior", b->m_employerfees.rate(year).senior);
            if (!insF.exec()) qWarning() << "Failed to insert employer fee rates for" << year << insF.lastError().text();
        }

        // Traktamente: the inrikes amount per year, and the utrikes
        // normalbelopp per country in the order they were imported.
        for (int year : b->m_traktamente.domesticYears()) {
            QSqlQuery insD(db);
            insD.prepare("INSERT INTO traktamente_domestic (year, full_day) VALUES (:year, :full_day)");
            insD.bindValue(":year", year);
            insD.bindValue(":full_day", b->m_traktamente.domesticOre(year));
            if (!insD.exec()) qWarning() << "Failed to insert traktamente for" << year << insD.lastError().text();
        }
        {
            QSqlQuery insC(db);
            insC.prepare("INSERT INTO traktamente_foreign (year, country, amount, position) VALUES (:year, :country, :amount, :position)");
            for (int year : b->m_traktamente.foreignYears()) {
                const QList<TraktamenteCountry> countries = b->m_traktamente.countries(year);
                for (int i = 0; i < countries.size(); i++) {
                    insC.bindValue(":year", year);
                    insC.bindValue(":country", countries.at(i).country);
                    insC.bindValue(":amount", countries.at(i).amount);
                    insC.bindValue(":position", i);
                    if (!insC.exec()) {
                        qWarning() << "Failed to insert normalbelopp for" << year << insC.lastError().text();
                        break;
                    }
                }
            }
        }

        // Tax tables. Several thousand rows a year, so the statement is
        // prepared once.
        {
            QSqlQuery insR(db);
            insR.prepare("INSERT INTO taxtable_rows (year, table_number, income_from, income_to, percent, tax) VALUES (:year, :table, :from, :to, :percent, :tax)");
            for (int year : b->m_taxtables.years()) {
                for (const TaxTableRow& r : b->m_taxtables.rows(year)) {
                    insR.bindValue(":year", year);
                    insR.bindValue(":table", r.table_number);
                    insR.bindValue(":from", r.income_from);
                    insR.bindValue(":to", r.income_to);
                    insR.bindValue(":percent", r.percent ? 1 : 0);
                    insR.bindValue(":tax", r.tax);
                    if (!insR.exec()) {
                        qWarning() << "Failed to insert tax table row for" << year << insR.lastError().text();
                        break;
                    }
                }
            }
        }

        // Salary drafts and slips
        for (int i = 0; i < b->m_salarydrafts.size(); ++i) {
            SalaryDraft* d = b->m_salarydrafts.at(i);
            QSqlQuery insD(db);
            insD.prepare("INSERT INTO salarydrafts (id, employee_id, payment_date, created_at, updated_at) VALUES (:id, :eid, :date, :created_at, :updated_at)");
            insD.bindValue(":id", d->id());
            insD.bindValue(":eid", d->m_employee_id);
            insD.bindValue(":date", d->m_payment_date.toString(Qt::ISODate));
            insD.bindValue(":created_at", now);
            insD.bindValue(":updated_at", now);
            if (!insD.exec()) qWarning() << "Failed to insert salary draft" << insD.lastError().text();
            insertSalaryLines(db, "salarydraft_lines", "draft_id", d->id(), d->m_lines);
        }

        for (int i = 0; i < b->m_salaryslips.size(); ++i) {
            SalarySlip* p = b->m_salaryslips.at(i);
            QSqlQuery insS(db);
            insS.prepare("INSERT INTO salaryslips (id, payment_date, company_name, company_adrsline1, company_adrsline2, company_org_number, employee_id, employee_name, employee_adrsline1, employee_adrsline2, employment_number, personnummer, bank_account, tax_table, employer_fee_rate, employer_fee, gross, tax, tax_free, net, created_at, updated_at) VALUES (:id, :date, :cname, :ca1, :ca2, :corg, :eid, :ename, :ea1, :ea2, :empno, :pnr, :bank, :table, :fee_rate, :fee, :gross, :tax, :tax_free, :net, :created_at, :updated_at)");
            insS.bindValue(":id", p->id());
            insS.bindValue(":date", p->m_payment_date.toString(Qt::ISODate));
            insS.bindValue(":cname", p->m_company_name);
            insS.bindValue(":ca1", p->m_company_adrsline1);
            insS.bindValue(":ca2", p->m_company_adrsline2);
            insS.bindValue(":corg", p->m_company_org_number);
            insS.bindValue(":eid", p->m_employee_id);
            insS.bindValue(":ename", p->m_employee_name);
            insS.bindValue(":ea1", p->m_employee_adrsline1);
            insS.bindValue(":ea2", p->m_employee_adrsline2);
            insS.bindValue(":empno", p->m_employment_number);
            insS.bindValue(":pnr", p->m_personnummer);
            insS.bindValue(":bank", p->m_bank_account);
            insS.bindValue(":table", p->m_tax_table);
            insS.bindValue(":fee_rate", p->m_employer_fee_rate);
            insS.bindValue(":fee", p->m_employer_fee);
            insS.bindValue(":gross", p->m_gross);
            insS.bindValue(":tax", p->m_tax);
            insS.bindValue(":tax_free", p->m_tax_free);
            insS.bindValue(":net", p->m_net);
            insS.bindValue(":created_at", now);
            insS.bindValue(":updated_at", now);
            if (!insS.exec()) qWarning() << "Failed to insert salary slip" << insS.lastError().text();
            insertSalaryLines(db, "salaryslip_lines", "slip_id", p->id(), p->m_lines);
        }

        // Invoice drafts
        for (int i = 0; i < b->m_invoicedrafts.size(); ++i) {
            InvoiceDraft* d = b->m_invoicedrafts.at(i);
            QSqlQuery insD(db);
            insD.prepare("INSERT INTO invoicedrafts (id, uuid, legacy_id, invoice_year, invoice_number, invoice_date, customer_id, agreement_id, employee_id, month, hours_standard, hours_overtime, hours_qualified, hours_traveltime, comment, is_credit, credited_invoice_number, created_at, updated_at) VALUES (:id, :uuid, :legacy, :year, :num, :date, :cid, :aid, :eid, :month, :hs, :ho, :hq, :ht, :comment, :is_credit, :credited_inv_num, :created_at, :updated_at)");
            insD.bindValue(":id", d->id());
            insD.bindValue(":uuid", QUuid::createUuid().toString());
            insD.bindValue(":legacy", QString::number(d->id()));
            insD.bindValue(":year", d->m_invoice_year);
            insD.bindValue(":num", d->m_invoice_number);
            insD.bindValue(":date", d->m_invoice_date.toString(Qt::ISODate));
            insD.bindValue(":cid", d->m_customer_id);
            insD.bindValue(":aid", d->m_agreement_id);
            insD.bindValue(":eid", d->m_employee_id);
            insD.bindValue(":month", d->m_month);
            insD.bindValue(":hs", d->m_hours_standard);
            insD.bindValue(":ho", d->m_hours_overtime);
            insD.bindValue(":hq", d->m_hours_qualified);
            insD.bindValue(":ht", d->m_hours_traveltime);
            insD.bindValue(":comment", d->m_comment);
            insD.bindValue(":is_credit", d->m_is_credit ? 1 : 0);
            insD.bindValue(":credited_inv_num", d->m_credited_invoice_number);
            insD.bindValue(":created_at", now);
            insD.bindValue(":updated_at", now);
            if (!insD.exec()) qWarning() << "Failed to insert invoicedraft" << insD.lastError().text();
        }

    // Invoices
    for (int i = 0; i < b->m_invoices.size(); ++i) {
        Invoice* inv = b->m_invoices.at(i);
        QSqlQuery insI(db);
        insI.prepare(R"SQL(INSERT INTO invoices (id, uuid, legacy_id, year, count, number, date, due_date, company_name, company_adrsline1, company_adrsline2, company_reference_name, company_org_number, company_bankgiro, company_phone, company_vat_number, company_swift_bic, company_email, company_f_skatt, company_iban, company_web, customer_id, customer_name, customer_adrsline1, customer_adrsline2, customer_reference_name, customer_email_invoice, agreement_id, agreement_name, agreement_key, agreement_payment_terms_days, agreement_late_payment_interest, employee_id, consultant_name, description_consultant_period, hours_standard, agreement_standard_hourly_rate, agreement_standard_vat, sum_standard, description_standard, hours_overtime, agreement_overtime_hourly_rate, agreement_overtime_vat, sum_overtime, description_overtime, hours_qualified, agreement_qualified_hourly_rate, agreement_qualified_vat, sum_qualified, description_qualified, hours_traveltime, agreement_traveltime_hourly_rate, agreement_traveltime_vat, sum_traveltime, description_traveltime, comment, sum, vat, sum_including_vat, is_credit, credited_invoice_number, created_at, updated_at) VALUES (:id, :uuid, :legacy, :year, :count, :number, :date, :due_date, :cname, :cad1, :cad2, :cref, :corg, :cbank, :cphone, :cvat, :cswift, :cemail, :cfskatt, :ciban, :cweb, :cid, :cname2, :cad12, :cad22, :cref2, :cemailinv, :agid, :agname, :agkey, :agpayterms, :aglate, :eid, :consultant, :descr, :hs, :hs_rate, :hs_vat, :sum_std, :desc_std, :ho, :ho_rate, :ho_vat, :sum_ot, :desc_ot, :hq, :hq_rate, :hq_vat, :sum_q, :desc_q, :ht, :ht_rate, :ht_vat, :sum_ht, :desc_ht, :comment, :sum, :vat, :sum_inc, :is_credit, :credited_inv_num, :created_at, :updated_at))SQL");

        insI.bindValue(":id", inv->id());
        insI.bindValue(":uuid", QUuid::createUuid().toString());
        insI.bindValue(":legacy", QString::number(inv->id()));
        insI.bindValue(":year", inv->m_year);
        insI.bindValue(":count", inv->m_count);
        insI.bindValue(":number", inv->m_number);
        insI.bindValue(":date", inv->m_date);
        insI.bindValue(":due_date", inv->m_due_date);
        insI.bindValue(":cname", inv->m_company_name);
        insI.bindValue(":cad1", inv->m_company_adrsline1);
        insI.bindValue(":cad2", inv->m_company_adrsline2);
        insI.bindValue(":cref", inv->m_company_reference_name);
        insI.bindValue(":corg", inv->m_company_org_number);
        insI.bindValue(":cbank", inv->m_company_bankgiro);
        insI.bindValue(":cphone", inv->m_company_phone);
        insI.bindValue(":cvat", inv->m_company_vat_number);
        insI.bindValue(":cswift", inv->m_company_swift_bic);
        insI.bindValue(":cemail", inv->m_company_email);
        insI.bindValue(":cfskatt", inv->m_company_f_skatt);
        insI.bindValue(":ciban", inv->m_company_iban);
        insI.bindValue(":cweb", inv->m_company_web);

        insI.bindValue(":cid", inv->m_customer_id);
        insI.bindValue(":cname2", inv->m_customer_name);
        insI.bindValue(":cad12", inv->m_customer_adrsline1);
        insI.bindValue(":cad22", inv->m_customer_adrsline2);
        insI.bindValue(":cref2", inv->m_customer_reference_name);
        insI.bindValue(":cemailinv", inv->m_customer_email_invoice);

        insI.bindValue(":agid", inv->m_agreement_id);
        insI.bindValue(":agname", inv->m_agreement_name);
        insI.bindValue(":agkey", inv->m_agreement_key);
        insI.bindValue(":agpayterms", inv->m_agreement_payment_terms_days);
        insI.bindValue(":aglate", inv->m_agreement_late_payment_interest);

        insI.bindValue(":eid", inv->m_employee_id);
        insI.bindValue(":consultant", inv->m_consultant_name);
        insI.bindValue(":descr", inv->m_description_consultant_period);

        insI.bindValue(":hs", inv->m_hours_standard);
        insI.bindValue(":hs_rate", inv->m_agreement_standard_hourly_rate);
        insI.bindValue(":hs_vat", inv->m_agreement_standard_vat);
        insI.bindValue(":sum_std", inv->m_sum_standard);
        insI.bindValue(":desc_std", inv->m_description_standard);

        insI.bindValue(":ho", inv->m_hours_overtime);
        insI.bindValue(":ho_rate", inv->m_agreement_overtime_hourly_rate);
        insI.bindValue(":ho_vat", inv->m_agreement_overtime_vat);
        insI.bindValue(":sum_ot", inv->m_sum_overtime);
        insI.bindValue(":desc_ot", inv->m_description_overtime);

        insI.bindValue(":hq", inv->m_hours_qualified);
        insI.bindValue(":hq_rate", inv->m_agreement_qualified_hourly_rate);
        insI.bindValue(":hq_vat", inv->m_agreement_qualified_vat);
        insI.bindValue(":sum_q", inv->m_sum_qualified);
        insI.bindValue(":desc_q", inv->m_description_qualified);

        insI.bindValue(":ht", inv->m_hours_traveltime);
        insI.bindValue(":ht_rate", inv->m_agreement_traveltime_hourly_rate);
        insI.bindValue(":ht_vat", inv->m_agreement_traveltime_vat);
        insI.bindValue(":sum_ht", inv->m_sum_traveltime);
        insI.bindValue(":desc_ht", inv->m_description_traveltime);

        insI.bindValue(":comment", inv->m_comment);
        insI.bindValue(":sum", inv->m_sum);
        insI.bindValue(":vat", inv->m_vat);
        insI.bindValue(":sum_inc", inv->m_sum_including_vat);
        insI.bindValue(":is_credit", inv->m_is_credit ? 1 : 0);
        insI.bindValue(":credited_inv_num", inv->m_credited_invoice_number);
        insI.bindValue(":created_at", now);
        insI.bindValue(":updated_at", now);

        if (!insI.exec()) qWarning() << "Failed to insert invoice" << insI.lastError().text();
    }

    // A file written here is at the current schema by construction, so stamp it
    // as such. Without this, load() would run the migrations over it again.
    {
        QSqlQuery mig(db);
        mig.prepare("INSERT OR REPLACE INTO schema_migrations (version, name) VALUES (:version, :name)");
        mig.bindValue(":version", DB_SCHEMA_VERSION);
        mig.bindValue(":name", QString("ore_and_fractional_hours_v3"));
        if (!mig.exec()) qWarning() << "Failed to record schema_migration:" << mig.lastError().text();

        QSqlQuery qv(db);
        if (!qv.exec(QString("PRAGMA user_version = %1").arg(DB_SCHEMA_VERSION))) {
            qWarning() << "Failed to set PRAGMA user_version:" << qv.lastError().text();
        }
    }

    if (!db.commit()) {
        qWarning() << "Failed to commit transaction:" << db.lastError().text();
    }

    db.close();
    return true;
    };

    bool ok = work();
    QSqlDatabase::removeDatabase(connName);

    if (ok)
        qInfo() << "Saved DB:" << filename;
    else
        qWarning() << "saveSqlite failed";

    return ok;
}

bool SqliteDataStore::load(Business* b, const QString& filename)
{
    if (!QFile::exists(filename)) {
        qWarning() << "SQLite file not found:" << filename;
        return false;
    }

    const QString connName = "datastore_load_conn";
    auto work = [&]() -> bool {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(filename);
        if (!db.open()) {
            qWarning() << "Failed to open sqlite DB:" << db.lastError().text();
            return false;
        }

        // Schema migration: upgrade existing DBs to current version
        {
            int userVersion = 0;
            QSqlQuery vq(db);
            if (vq.exec("PRAGMA user_version") && vq.next())
                userVersion = vq.value(0).toInt();
            // An unfinished statement keeps a transaction open, and the
            // schema step below cannot switch the journal mode inside one.
            vq.finish();
            if (userVersion < DB_SCHEMA_VERSION) {
                // The migrations below rewrite the file in place and cannot be
                // undone, so keep what the user had. Fold any write-ahead log
                // back into the main file first, otherwise the copy could be
                // missing the most recent commits.
                QSqlQuery cp(db);
                cp.exec("PRAGMA wal_checkpoint(TRUNCATE)");
                cp.finish();

                const QString backup = database_backup_path(filename, QString("v%1").arg(userVersion));
                if (!QFile::copy(filename, backup)) {
                    qWarning() << "Refusing to migrate" << filename
                               << "because the backup could not be written to" << backup;
                    db.close();
                    return false;
                }
                qInfo() << "Backed up the database before migrating:" << backup;
            }

            if (userVersion < 2) {
                // These fail silently on fresh DBs where columns already exist — that is fine
                QSqlQuery mq(db);
                mq.exec("ALTER TABLE invoices ADD COLUMN is_credit INTEGER DEFAULT 0");
                mq.exec("ALTER TABLE invoices ADD COLUMN credited_invoice_number TEXT DEFAULT ''");
                mq.exec("ALTER TABLE invoicedrafts ADD COLUMN is_credit INTEGER DEFAULT 0");
                mq.exec("ALTER TABLE invoicedrafts ADD COLUMN credited_invoice_number TEXT DEFAULT ''");
                mq.exec("PRAGMA user_version = 2");
            }
            if (userVersion < 3) {
                // Version 3 moved money from kronor to öre and hours from whole
                // hours to hundredths, so everything stored scales by 100.
                QSqlQuery mq(db);
                mq.exec("UPDATE agreements SET"
                        " standard_hourly_rate = standard_hourly_rate * 100,"
                        " overtime_hourly_rate = overtime_hourly_rate * 100,"
                        " qualified_hourly_rate = qualified_hourly_rate * 100,"
                        " traveltime_hourly_rate = traveltime_hourly_rate * 100");
                mq.exec("UPDATE invoices SET"
                        " hours_standard = hours_standard * 100,"
                        " hours_overtime = hours_overtime * 100,"
                        " hours_qualified = hours_qualified * 100,"
                        " hours_traveltime = hours_traveltime * 100,"
                        " agreement_standard_hourly_rate = agreement_standard_hourly_rate * 100,"
                        " agreement_overtime_hourly_rate = agreement_overtime_hourly_rate * 100,"
                        " agreement_qualified_hourly_rate = agreement_qualified_hourly_rate * 100,"
                        " agreement_traveltime_hourly_rate = agreement_traveltime_hourly_rate * 100,"
                        " sum_standard = sum_standard * 100,"
                        " sum_overtime = sum_overtime * 100,"
                        " sum_qualified = sum_qualified * 100,"
                        " sum_traveltime = sum_traveltime * 100,"
                        " sum = sum * 100,"
                        " vat = vat * 100,"
                        " sum_including_vat = sum_including_vat * 100");
                mq.exec("UPDATE invoicedrafts SET"
                        " hours_standard = hours_standard * 100,"
                        " hours_overtime = hours_overtime * 100,"
                        " hours_qualified = hours_qualified * 100,"
                        " hours_traveltime = hours_traveltime * 100");
                mq.exec(QString("PRAGMA user_version = %1").arg(DB_SCHEMA_VERSION));
            }

            // Whatever the file is stamped with, it can still be missing
            // something: a file written while the current version was being
            // built carries that version but not what was added to it
            // afterwards, and a SELECT naming a column the file does not have
            // returns nothing at all — a whole table would seem to have
            // disappeared. So the current schema is applied on every load.
            // Everything here either creates what is missing or fails
            // silently because it is already there.
            {
                QSqlQuery mq(db);
                // Columns added to version 3 while it was unreleased.
                mq.exec("ALTER TABLE invoices ADD COLUMN company_web TEXT DEFAULT ''");
                mq.exec("ALTER TABLE companyinfo ADD COLUMN fiscal_year_start_month INTEGER NOT NULL DEFAULT 1");
                mq.exec("ALTER TABLE invoices ADD COLUMN agreement_name TEXT DEFAULT ''");
                mq.exec("ALTER TABLE invoices ADD COLUMN agreement_key INTEGER DEFAULT 0");
                mq.exec("ALTER TABLE employees ADD COLUMN first_name TEXT DEFAULT ''");
                mq.exec("ALTER TABLE employees ADD COLUMN last_name TEXT DEFAULT ''");
                // An employee used to have one name field. Rather than guess
                // where a name splits, the old one becomes the first name and
                // the user moves the surname over.
                mq.exec("UPDATE employees SET first_name = name"
                        " WHERE (first_name IS NULL OR first_name = '') AND name IS NOT NULL AND name != ''");
                mq.exec("ALTER TABLE salaryslips ADD COLUMN employer_fee_rate INTEGER DEFAULT 0");
                mq.exec("ALTER TABLE salaryslips ADD COLUMN employer_fee INTEGER DEFAULT 0");
                // A salary line used to be lön and nothing else. Kind 0 is
                // what every line already in a file is.
                mq.exec("ALTER TABLE salarydraft_lines ADD COLUMN kind INTEGER NOT NULL DEFAULT 0");
                mq.exec("ALTER TABLE salaryslip_lines ADD COLUMN kind INTEGER NOT NULL DEFAULT 0");
                mq.exec("ALTER TABLE salaryslips ADD COLUMN tax_free INTEGER DEFAULT 0");
                // A line had nothing to say for itself beyond its benämning.
                mq.exec("ALTER TABLE salarydraft_lines ADD COLUMN comment TEXT");
                mq.exec("ALTER TABLE salaryslip_lines ADD COLUMN comment TEXT");
                // What a skattefri line applies to — the place it was paid for
                // — used to be part of the comment, or of the benämning before
                // that. It is the value the schablon was looked up by and not
                // free text, so it is kept on its own now.
                mq.exec("ALTER TABLE salarydraft_lines ADD COLUMN applies_to TEXT");
                mq.exec("ALTER TABLE salaryslip_lines ADD COLUMN applies_to TEXT");

                // A löneart used to be typed by hand, and the application
                // itself only ever wrote two: "010" for Månadslön and "310"
                // for a traktamente. They are the numbers of the fixed table
                // now. Anything else was typed by whoever wrote the line and
                // is left as it is: the SalaryDrafts tab shows such a line as
                // having no löneart until one is picked, which is better than
                // picking one for it. A slip keeps the benämning it printed
                // either way.
                for (const char* table : { "salarydraft_lines", "salaryslip_lines" })
                {
                    mq.exec(QString("UPDATE %1 SET code = '%2' WHERE code = '010'").arg(table).arg(SALARY_TYPE_MONTHLY_SALARY));
                    mq.exec(QString("UPDATE %1 SET code = '%2' WHERE code = '310'").arg(table).arg(SALARY_TYPE_TRAKTAMENTE));
                }

                // The consultant used to be the agreement's "vår referens",
                // written into the description and nowhere else; it is an
                // employee picked on the draft now. What an invoice was sent
                // with stays as it was printed, so the name is taken out of
                // the description it already carries and the employee looked
                // up by it. These run after the name split above, which is
                // what gives an older file its first and last name.
                mq.exec("ALTER TABLE invoices ADD COLUMN employee_id INTEGER DEFAULT 0");
                mq.exec("ALTER TABLE invoices ADD COLUMN consultant_name TEXT DEFAULT ''");
                mq.exec("ALTER TABLE invoicedrafts ADD COLUMN employee_id INTEGER DEFAULT 0");
                // "Konsult Namn Namnsson Period Januari" -> "Namn Namnsson".
                mq.exec("UPDATE invoices SET consultant_name = CASE"
                        "   WHEN instr(description_consultant_period, ' Period ') > 0"
                        "   THEN substr(description_consultant_period, 9, instr(description_consultant_period, ' Period ') - 9)"
                        "   ELSE substr(description_consultant_period, 9) END"
                        " WHERE (consultant_name IS NULL OR consultant_name = '')"
                        "   AND description_consultant_period LIKE 'Konsult %'");
                // Only a name that belongs to exactly one employee says who
                // the consultant was; anything else is left without one, as
                // the Konsult column on the invoice list shows.
                mq.exec("UPDATE invoices SET employee_id ="
                        " (SELECT e.id FROM employees e WHERE trim(e.first_name || ' ' || e.last_name) = trim(invoices.consultant_name))"
                        " WHERE (employee_id IS NULL OR employee_id <= 0)"
                        "   AND consultant_name IS NOT NULL AND consultant_name != ''"
                        "   AND (SELECT COUNT(*) FROM employees e WHERE trim(e.first_name || ' ' || e.last_name) = trim(invoices.consultant_name)) = 1");
                // A draft has no description to read; the agreement it is
                // written under still names the consultant the old way. An
                // agreement's id is only unique within its customer, so both
                // have to match.
                mq.exec("UPDATE invoicedrafts SET employee_id ="
                        " (SELECT e.id FROM employees e, agreements a"
                        "   WHERE a.id = invoicedrafts.agreement_id AND a.customer_id = invoicedrafts.customer_id"
                        "     AND trim(a.reference_name_ours) != ''"
                        "     AND trim(e.first_name || ' ' || e.last_name) = trim(a.reference_name_ours))"
                        " WHERE (employee_id IS NULL OR employee_id <= 0)"
                        "   AND (SELECT COUNT(*) FROM employees e, agreements a"
                        "         WHERE a.id = invoicedrafts.agreement_id AND a.customer_id = invoicedrafts.customer_id"
                        "           AND trim(a.reference_name_ours) != ''"
                        "           AND trim(e.first_name || ' ' || e.last_name) = trim(a.reference_name_ours)) = 1");
            }

            // Tables are simpler: every statement of the schema is
            // CREATE ... IF NOT EXISTS, so this adds the ones the file lacks
            // at their current shape.
            if (!execSchema(db)) {
                qWarning() << "Failed to bring" << filename << "up to the current schema";
                db.close();
                return false;
            }
        }

        // Clear current in-memory data
        b->clear();

        QSqlQuery q(db);

        // Company info
        if (!q.exec("SELECT name, adrsline1, adrsline2, org_number, bankgiro, swift_bic, phone, vat_number, iban, email, web, logo, fiscal_year_start_month FROM companyinfo LIMIT 1")) {
            qWarning() << "Failed to read" << filename << ":" << q.lastError().text();
            db.close();
            return false;
        }
        {
            if (q.next()) {
                b->m_companyinfo.m_name = q.value(0).toString();
                b->m_companyinfo.m_adrsline1 = q.value(1).toString();
                b->m_companyinfo.m_adrsline2 = q.value(2).toString();
                b->m_companyinfo.m_org_number = q.value(3).toString();
                b->m_companyinfo.m_bankgiro = q.value(4).toString();
                b->m_companyinfo.m_swift_bic = q.value(5).toString();
                b->m_companyinfo.m_phone = q.value(6).toString();
                b->m_companyinfo.m_vat_number = q.value(7).toString();
                b->m_companyinfo.m_iban = q.value(8).toString();
                b->m_companyinfo.m_email = q.value(9).toString();
                b->m_companyinfo.m_web = q.value(10).toString();
                b->m_companyinfo.m_logo = q.value(11).toString();
                // An older file has no räkenskapsår; the calendar year is the
                // one every company starts with.
                b->m_companyinfo.m_fiscal_year_start_month = qBound(1, q.value(12).toInt(), 12);
            }
        }

        // Customers and agreements
        if (!q.exec("SELECT id, name, org_number, adrsline1, adrsline2, email, email_invoice FROM customers ORDER BY id")) {
            qWarning() << "Failed to read" << filename << ":" << q.lastError().text();
            db.close();
            return false;
        }
        {
            while (q.next()) {
                // The stored ids are authoritative, so the row is loaded into
                // an empty customer which is then adopted with its own id.
                Customer loaded;
                int cid = q.value(0).toInt();
                loaded.m_name = q.value(1).toString();
                loaded.m_org_number = q.value(2).toString();
                loaded.m_adrsline1 = q.value(3).toString();
                loaded.m_adrsline2 = q.value(4).toString();
                loaded.m_email = q.value(5).toString();
                loaded.m_email_invoice = q.value(6).toString();

                Customer* c = b->m_customers.update(loaded);
                c->setId(cid);

                // load agreements for this customer
                QSqlQuery q2(db);
                q2.prepare("SELECT id, active, agreement_name, agreement_id, reference_name, reference_email, reference_phone, reference_name_ours, standard_hourly_rate, overtime_hourly_rate, qualified_hourly_rate, traveltime_hourly_rate, payment_terms_days, late_payment_interest FROM agreements WHERE customer_id = :cid ORDER BY id");
                q2.bindValue(":cid", cid);
                if (q2.exec()) {
                    while (q2.next()) {
                        Agreement loaded;
                        loaded.m_active = q2.value(1).toInt() ? true : false;
                        loaded.m_agreement_name = q2.value(2).toString();
                        loaded.m_agreement_id = q2.value(3).toString();
                        loaded.m_reference_name = q2.value(4).toString();
                        loaded.m_reference_email = q2.value(5).toString();
                        loaded.m_reference_phone = q2.value(6).toString();
                        loaded.m_reference_name_ours = q2.value(7).toString();
                        loaded.m_standard_hourly_rate = q2.value(8).toLongLong();
                        loaded.m_overtime_hourly_rate = q2.value(9).toLongLong();
                        loaded.m_qualified_hourly_rate = q2.value(10).toLongLong();
                        loaded.m_traveltime_hourly_rate = q2.value(11).toLongLong();
                        loaded.m_payment_terms_days = q2.value(12).toInt();
                        loaded.m_late_payment_interest = q2.value(13).toInt();

                        c->m_agreements.update(loaded)->setId(q2.value(0).toInt());
                    }
                }

            }
        }

        // Employees and their tax years
        if (!q.exec("SELECT id, first_name, last_name, adrsline1, adrsline2, personnummer, bank_account, active FROM employees ORDER BY id")) {
            qWarning() << "Failed to read" << filename << ":" << q.lastError().text();
            db.close();
            return false;
        }
        {
            while (q.next()) {
                Employee loaded;
                int eid = q.value(0).toInt();
                loaded.m_first_name = q.value(1).toString();
                loaded.m_last_name = q.value(2).toString();
                loaded.m_adrsline1 = q.value(3).toString();
                loaded.m_adrsline2 = q.value(4).toString();
                loaded.m_personnummer = q.value(5).toString();
                loaded.m_bank_account = q.value(6).toString();
                loaded.m_active = q.value(7).toInt() != 0;

                QSqlQuery q2(db);
                q2.prepare("SELECT year, table_number FROM employee_tax_years WHERE employee_id = :eid ORDER BY year");
                q2.bindValue(":eid", eid);
                if (q2.exec()) {
                    while (q2.next()) {
                        loaded.m_tax_tables.insert(q2.value(0).toInt(), q2.value(1).toInt());
                    }
                }

                b->m_employees.update(loaded)->setId(eid);
            }
        }

        // Arbetsgivaravgifter
        if (!q.exec("SELECT year, standard, senior FROM employer_fee_rates ORDER BY year")) {
            qWarning() << "Failed to read" << filename << ":" << q.lastError().text();
            db.close();
            return false;
        }
        {
            while (q.next()) {
                b->m_employerfees.setYear(q.value(0).toInt(), EmployerFeeRate{ q.value(1).toInt(), q.value(2).toInt() });
            }
        }

        // Traktamente
        if (!q.exec("SELECT year, full_day FROM traktamente_domestic ORDER BY year")) {
            qWarning() << "Failed to read" << filename << ":" << q.lastError().text();
            db.close();
            return false;
        }
        while (q.next()) {
            b->m_traktamente.setDomestic(q.value(0).toInt(), q.value(1).toLongLong());
        }

        if (!q.exec("SELECT year, country, amount FROM traktamente_foreign ORDER BY year, position")) {
            qWarning() << "Failed to read" << filename << ":" << q.lastError().text();
            db.close();
            return false;
        }
        {
            QMap<int, QList<TraktamenteCountry>> years;
            while (q.next()) {
                TraktamenteCountry c;
                c.country = q.value(1).toString();
                c.amount = q.value(2).toLongLong();
                years[q.value(0).toInt()].append(c);
            }
            for (auto it = years.constBegin(); it != years.constEnd(); ++it) {
                b->m_traktamente.setForeign(it.key(), it.value());
            }
        }

        // Tax tables
        if (!q.exec("SELECT year, table_number, income_from, income_to, percent, tax FROM taxtable_rows ORDER BY year, table_number, income_from")) {
            qWarning() << "Failed to read" << filename << ":" << q.lastError().text();
            db.close();
            return false;
        }
        {
            QMap<int, QList<TaxTableRow>> years;
            while (q.next()) {
                TaxTableRow r;
                r.table_number = q.value(1).toInt();
                r.income_from = q.value(2).toInt();
                r.income_to = q.value(3).toInt();
                r.percent = q.value(4).toInt() != 0;
                r.tax = q.value(5).toInt();
                years[q.value(0).toInt()].append(r);
            }
            for (auto it = years.constBegin(); it != years.constEnd(); ++it) {
                b->m_taxtables.setYear(it.key(), it.value());
            }
        }

        // Salary drafts and slips
        if (!q.exec("SELECT id, employee_id, payment_date FROM salarydrafts ORDER BY id")) {
            qWarning() << "Failed to read" << filename << ":" << q.lastError().text();
            db.close();
            return false;
        }
        {
            while (q.next()) {
                SalaryDraft d;
                int did = q.value(0).toInt();
                d.m_employee_id = q.value(1).toInt();
                d.m_payment_date = QDate::fromString(q.value(2).toString(), Qt::ISODate);
                d.m_lines = selectSalaryLines(db, "salarydraft_lines", "draft_id", did);
                b->m_salarydrafts.update(d)->setId(did);
            }
        }

        if (!q.exec("SELECT id, payment_date, company_name, company_adrsline1, company_adrsline2, company_org_number, employee_id, employee_name, employee_adrsline1, employee_adrsline2, employment_number, personnummer, bank_account, tax_table, employer_fee_rate, employer_fee, gross, tax, tax_free, net FROM salaryslips ORDER BY id")) {
            qWarning() << "Failed to read" << filename << ":" << q.lastError().text();
            db.close();
            return false;
        }
        {
            while (q.next()) {
                auto owned = std::make_unique<SalarySlip>();
                SalarySlip* p = owned.get();
                const int storedId = q.value(0).toInt();
                p->m_payment_date = QDate::fromString(q.value(1).toString(), Qt::ISODate);
                p->m_company_name = q.value(2).toString();
                p->m_company_adrsline1 = q.value(3).toString();
                p->m_company_adrsline2 = q.value(4).toString();
                p->m_company_org_number = q.value(5).toString();
                p->m_employee_id = q.value(6).toInt();
                p->m_employee_name = q.value(7).toString();
                p->m_employee_adrsline1 = q.value(8).toString();
                p->m_employee_adrsline2 = q.value(9).toString();
                p->m_employment_number = q.value(10).toString();
                p->m_personnummer = q.value(11).toString();
                p->m_bank_account = q.value(12).toString();
                p->m_tax_table = q.value(13).toInt();
                p->m_employer_fee_rate = q.value(14).toInt();
                p->m_employer_fee = q.value(15).toLongLong();
                p->m_gross = q.value(16).toLongLong();
                p->m_tax = q.value(17).toLongLong();
                p->m_tax_free = q.value(18).toLongLong();
                p->m_net = q.value(19).toLongLong();
                p->m_lines = selectSalaryLines(db, "salaryslip_lines", "slip_id", storedId);
                // The stored id is authoritative; append() would otherwise renumber.
                b->m_salaryslips.append(std::move(owned))->setId(storedId);
            }
        }

        // Invoice drafts
        if (!q.exec("SELECT id, invoice_year, invoice_number, invoice_date, customer_id, agreement_id, employee_id, month, hours_standard, hours_overtime, hours_qualified, hours_traveltime, comment, is_credit, credited_invoice_number FROM invoicedrafts ORDER BY id")) {
            qWarning() << "Failed to read" << filename << ":" << q.lastError().text();
            db.close();
            return false;
        }
        {
            while (q.next()) {
                InvoiceDraft d;
                d.m_invoice_year = q.value(1).toInt();
                d.m_invoice_number = q.value(2).toInt();
                d.m_invoice_date = QDate::fromString(q.value(3).toString(), Qt::ISODate);
                d.m_customer_id = q.value(4).toInt();
                d.m_agreement_id = q.value(5).toInt();
                // A draft from before a consultant was picked has none: the
                // column is then 0, which is no employee's id.
                d.m_employee_id = q.value(6).toInt();
                d.m_month = q.value(7).toInt();
                d.m_hours_standard = q.value(8).toInt();
                d.m_hours_overtime = q.value(9).toInt();
                d.m_hours_qualified = q.value(10).toInt();
                d.m_hours_traveltime = q.value(11).toInt();
                d.m_comment = q.value(12).toString();
                d.m_is_credit = q.value(13).toInt() != 0;
                d.m_credited_invoice_number = q.value(14).toString();
                b->m_invoicedrafts.update(d)->setId(q.value(0).toInt());
            }
        }

        // Invoices
        if (!q.exec("SELECT id, year, count, number, date, due_date, company_name, company_adrsline1, company_adrsline2, company_reference_name, company_org_number, company_bankgiro, company_phone, company_vat_number, company_swift_bic, company_email, company_f_skatt, company_iban, company_web, customer_id, customer_name, customer_adrsline1, customer_adrsline2, customer_reference_name, customer_email_invoice, agreement_id, agreement_name, agreement_key, agreement_payment_terms_days, agreement_late_payment_interest, employee_id, consultant_name, description_consultant_period, hours_standard, agreement_standard_hourly_rate, agreement_standard_vat, sum_standard, description_standard, hours_overtime, agreement_overtime_hourly_rate, agreement_overtime_vat, sum_overtime, description_overtime, hours_qualified, agreement_qualified_hourly_rate, agreement_qualified_vat, sum_qualified, description_qualified, hours_traveltime, agreement_traveltime_hourly_rate, agreement_traveltime_vat, sum_traveltime, description_traveltime, comment, sum, vat, sum_including_vat, is_credit, credited_invoice_number FROM invoices ORDER BY id")) {
            qWarning() << "Failed to read" << filename << ":" << q.lastError().text();
            db.close();
            return false;
        }
        {
            while (q.next()) {
                auto owned = std::make_unique<Invoice>();
                Invoice* inv = owned.get();
                inv->setId(q.value(0).toInt());
                inv->m_year = q.value(1).toInt();
                inv->m_count = q.value(2).toInt();
                inv->m_number = q.value(3).toString();
                inv->m_date = q.value(4).toString();
                inv->m_due_date = q.value(5).toString();
                inv->m_company_name = q.value(6).toString();
                inv->m_company_adrsline1 = q.value(7).toString();
                inv->m_company_adrsline2 = q.value(8).toString();
                inv->m_company_reference_name = q.value(9).toString();
                inv->m_company_org_number = q.value(10).toString();
                inv->m_company_bankgiro = q.value(11).toString();
                inv->m_company_phone = q.value(12).toString();
                inv->m_company_vat_number = q.value(13).toString();
                inv->m_company_swift_bic = q.value(14).toString();
                inv->m_company_email = q.value(15).toString();
                inv->m_company_f_skatt = q.value(16).toString();
                inv->m_company_iban = q.value(17).toString();
                inv->m_company_web = q.value(18).toString();

                inv->m_customer_id = q.value(19).toInt();
                inv->m_customer_name = q.value(20).toString();
                inv->m_customer_adrsline1 = q.value(21).toString();
                inv->m_customer_adrsline2 = q.value(22).toString();
                inv->m_customer_reference_name = q.value(23).toString();
                inv->m_customer_email_invoice = q.value(24).toString();

                inv->m_agreement_id = q.value(25).toString();
                inv->m_agreement_name = q.value(26).toString();
                inv->m_agreement_key = q.value(27).toInt();
                inv->m_agreement_payment_terms_days = q.value(28).toInt();
                inv->m_agreement_late_payment_interest = q.value(29).toInt();

                inv->m_employee_id = q.value(30).toInt();
                inv->m_consultant_name = q.value(31).toString();
                inv->m_description_consultant_period = q.value(32).toString();

                inv->m_hours_standard = q.value(33).toInt();
                inv->m_agreement_standard_hourly_rate = q.value(34).toLongLong();
                inv->m_agreement_standard_vat = q.value(35).toInt();
                inv->m_sum_standard = q.value(36).toLongLong();
                inv->m_description_standard = q.value(37).toString();

                inv->m_hours_overtime = q.value(38).toInt();
                inv->m_agreement_overtime_hourly_rate = q.value(39).toLongLong();
                inv->m_agreement_overtime_vat = q.value(40).toInt();
                inv->m_sum_overtime = q.value(41).toLongLong();
                inv->m_description_overtime = q.value(42).toString();

                inv->m_hours_qualified = q.value(43).toInt();
                inv->m_agreement_qualified_hourly_rate = q.value(44).toLongLong();
                inv->m_agreement_qualified_vat = q.value(45).toInt();
                inv->m_sum_qualified = q.value(46).toLongLong();
                inv->m_description_qualified = q.value(47).toString();

                inv->m_hours_traveltime = q.value(48).toInt();
                inv->m_agreement_traveltime_hourly_rate = q.value(49).toLongLong();
                inv->m_agreement_traveltime_vat = q.value(50).toInt();
                inv->m_sum_traveltime = q.value(51).toLongLong();
                inv->m_description_traveltime = q.value(52).toString();

                inv->m_comment = q.value(53).toString();
                inv->m_sum = q.value(54).toLongLong();
                inv->m_vat = q.value(55).toLongLong();
                inv->m_sum_including_vat = q.value(56).toLongLong();
                inv->m_is_credit = q.value(57).toInt() != 0;
                inv->m_credited_invoice_number = q.value(58).toString();

                // The stored id is authoritative; append() would otherwise renumber.
                const int storedId = inv->id();
                b->m_invoices.append(std::move(owned))->setId(storedId);
            }
        }

        db.close();
        return true;
    };

    bool ok = work();
    QSqlDatabase::removeDatabase(connName);

    return ok;
}
