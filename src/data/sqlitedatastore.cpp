#include "sqlitedatastore.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDateTime>
#include <QDebug>

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

    customer_id INTEGER REFERENCES customers(id),
    customer_name TEXT,
    customer_adrsline1 TEXT,
    customer_adrsline2 TEXT,
    customer_reference_name TEXT,
    customer_email_invoice TEXT,

    agreement_id TEXT,
    agreement_payment_terms_days INTEGER,
    agreement_late_payment_interest INTEGER,

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
    month INTEGER,
    hours_standard INTEGER,
    hours_overtime INTEGER,
    hours_qualified INTEGER,
    hours_traveltime INTEGER,
    comment TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
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
)SQL";

#include "business.h"

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

        // Create schema
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
                            db.close();
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
                            db.close();
                            return false;
                        }
                        acc.clear();
                    }
                } else {
                    // normal statement
                    if (!q.exec(part + ";")) {
                        qWarning() << "Failed to execute schema statement:" << q.lastError().text() << "Statement:" << part;
                        db.close();
                        return false;
                    }
                }
            }
        }

        if (!db.transaction()) {
            qWarning() << "Failed to start transaction:" << db.lastError().text();
        }

        QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        // Insert companyinfo
        {
            QSqlQuery ins(db);
            ins.prepare("INSERT INTO companyinfo (uuid, legacy_id, name, adrsline1, adrsline2, org_number, bankgiro, swift_bic, phone, vat_number, iban, email, web, logo, created_at, updated_at) VALUES (:uuid, :legacy, :name, :a1, :a2, :org, :bank, :swift, :phone, :vat, :iban, :email, :web, :logo, :created_at, :updated_at)");
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
            ins.bindValue(":created_at", now);
            ins.bindValue(":updated_at", now);
            if (!ins.exec()) qWarning() << "companyinfo insert failed:" << ins.lastError().text();
        }

        // Insert customers and agreements
        for (int i = 0; i < b->m_customers.m_list.size(); ++i) {
            Customer* c = b->m_customers.m_list.at(i);
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

        // Invoice drafts
        for (int i = 0; i < b->m_invoicedrafts.size(); ++i) {
            InvoiceDraft* d = b->m_invoicedrafts.at(i);
            QSqlQuery insD(db);
            insD.prepare("INSERT INTO invoicedrafts (id, uuid, legacy_id, invoice_year, invoice_number, invoice_date, customer_id, agreement_id, month, hours_standard, hours_overtime, hours_qualified, hours_traveltime, comment, created_at, updated_at) VALUES (:id, :uuid, :legacy, :year, :num, :date, :cid, :aid, :month, :hs, :ho, :hq, :ht, :comment, :created_at, :updated_at)");
            insD.bindValue(":id", d->id());
            insD.bindValue(":uuid", QUuid::createUuid().toString());
            insD.bindValue(":legacy", QString::number(d->id()));
            insD.bindValue(":year", d->m_invoice_year);
            insD.bindValue(":num", d->m_invoice_number);
            insD.bindValue(":date", d->m_invoice_date.toString(Qt::ISODate));
            insD.bindValue(":cid", d->m_customer_id);
            insD.bindValue(":aid", d->m_agreement_id);
            insD.bindValue(":month", d->m_month);
            insD.bindValue(":hs", d->m_hours_standard);
            insD.bindValue(":ho", d->m_hours_overtime);
            insD.bindValue(":hq", d->m_hours_qualified);
            insD.bindValue(":ht", d->m_hours_traveltime);
            insD.bindValue(":comment", d->m_comment);
            insD.bindValue(":created_at", now);
            insD.bindValue(":updated_at", now);
            if (!insD.exec()) qWarning() << "Failed to insert invoicedraft" << insD.lastError().text();
        }

    // Invoices
    for (int i = 0; i < b->m_invoices.size(); ++i) {
        Invoice* inv = b->m_invoices.at(i);
        QSqlQuery insI(db);
        insI.prepare(R"SQL(INSERT INTO invoices (id, uuid, legacy_id, year, count, number, date, due_date, company_name, company_adrsline1, company_adrsline2, company_reference_name, company_org_number, company_bankgiro, company_phone, company_vat_number, company_swift_bic, company_email, company_f_skatt, company_iban, customer_id, customer_name, customer_adrsline1, customer_adrsline2, customer_reference_name, customer_email_invoice, agreement_id, agreement_payment_terms_days, agreement_late_payment_interest, description_consultant_period, hours_standard, agreement_standard_hourly_rate, agreement_standard_vat, sum_standard, description_standard, hours_overtime, agreement_overtime_hourly_rate, agreement_overtime_vat, sum_overtime, description_overtime, hours_qualified, agreement_qualified_hourly_rate, agreement_qualified_vat, sum_qualified, description_qualified, hours_traveltime, agreement_traveltime_hourly_rate, agreement_traveltime_vat, sum_traveltime, description_traveltime, comment, sum, vat, sum_including_vat, created_at, updated_at) VALUES (:id, :uuid, :legacy, :year, :count, :number, :date, :due_date, :cname, :cad1, :cad2, :cref, :corg, :cbank, :cphone, :cvat, :cswift, :cemail, :cfskatt, :ciban, :cid, :cname2, :cad12, :cad22, :cref2, :cemailinv, :agid, :agpayterms, :aglate, :descr, :hs, :hs_rate, :hs_vat, :sum_std, :desc_std, :ho, :ho_rate, :ho_vat, :sum_ot, :desc_ot, :hq, :hq_rate, :hq_vat, :sum_q, :desc_q, :ht, :ht_rate, :ht_vat, :sum_ht, :desc_ht, :comment, :sum, :vat, :sum_inc, :created_at, :updated_at))SQL");

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

        insI.bindValue(":cid", inv->m_customer_id);
        insI.bindValue(":cname2", inv->m_customer_name);
        insI.bindValue(":cad12", inv->m_customer_adrsline1);
        insI.bindValue(":cad22", inv->m_customer_adrsline2);
        insI.bindValue(":cref2", inv->m_customer_reference_name);
        insI.bindValue(":cemailinv", inv->m_customer_email_invoice);

        insI.bindValue(":agid", inv->m_agreement_id);
        insI.bindValue(":agpayterms", inv->m_agreement_payment_terms_days);
        insI.bindValue(":aglate", inv->m_agreement_late_payment_interest);

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
        insI.bindValue(":created_at", now);
        insI.bindValue(":updated_at", now);

        if (!insI.exec()) qWarning() << "Failed to insert invoice" << insI.lastError().text();
    }

    // Record applied schema migration and set user_version
    {
        QSqlQuery mig(db);
        mig.prepare("INSERT OR REPLACE INTO schema_migrations (version, name) VALUES (:version, :name)");
        mig.bindValue(":version", 1);
        mig.bindValue(":name", QString("initial_schema_v1"));
        if (!mig.exec()) qWarning() << "Failed to record schema_migration:" << mig.lastError().text();

        QSqlQuery qv(db);
        if (!qv.exec("PRAGMA user_version = 1")) {
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

        // Clear current in-memory data
        b->clear();

        QSqlQuery q(db);

        // Company info
        if (q.exec("SELECT name, adrsline1, adrsline2, org_number, bankgiro, swift_bic, phone, vat_number, iban, email, logo FROM companyinfo LIMIT 1")) {
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
                b->m_companyinfo.m_logo = q.value(10).toString();
            }
        }

        // Customers and agreements
        if (q.exec("SELECT id, name, org_number, adrsline1, adrsline2, email, email_invoice FROM customers ORDER BY id")) {
            while (q.next()) {
                Customer* c = new Customer();
                int cid = q.value(0).toInt();
                c->setId(cid);
                c->m_name = q.value(1).toString();
                c->m_org_number = q.value(2).toString();
                c->m_adrsline1 = q.value(3).toString();
                c->m_adrsline2 = q.value(4).toString();
                c->m_email = q.value(5).toString();
                c->m_email_invoice = q.value(6).toString();

                // load agreements for this customer
                QSqlQuery q2(db);
                q2.prepare("SELECT id, active, agreement_name, agreement_id, reference_name, reference_email, reference_phone, reference_name_ours, standard_hourly_rate, overtime_hourly_rate, qualified_hourly_rate, traveltime_hourly_rate, payment_terms_days, late_payment_interest FROM agreements WHERE customer_id = :cid ORDER BY id");
                q2.bindValue(":cid", cid);
                if (q2.exec()) {
                    while (q2.next()) {
                        Agreement* a = new Agreement();
                        a->setId(q2.value(0).toInt());
                        a->m_active = q2.value(1).toInt() ? true : false;
                        a->m_agreement_name = q2.value(2).toString();
                        a->m_agreement_id = q2.value(3).toString();
                        a->m_reference_name = q2.value(4).toString();
                        a->m_reference_email = q2.value(5).toString();
                        a->m_reference_phone = q2.value(6).toString();
                        a->m_reference_name_ours = q2.value(7).toString();
                        a->m_standard_hourly_rate = q2.value(8).toInt();
                        a->m_overtime_hourly_rate = q2.value(9).toInt();
                        a->m_qualified_hourly_rate = q2.value(10).toInt();
                        a->m_traveltime_hourly_rate = q2.value(11).toInt();
                        a->m_payment_terms_days = q2.value(12).toInt();
                        a->m_late_payment_interest = q2.value(13).toInt();

                        c->m_agreements.update(a);
                    }
                }

                b->m_customers.m_list.append(c);
            }
        }

        // Invoice drafts
        if (q.exec("SELECT id, invoice_year, invoice_number, invoice_date, customer_id, agreement_id, month, hours_standard, hours_overtime, hours_qualified, hours_traveltime, comment FROM invoicedrafts ORDER BY id")) {
            while (q.next()) {
                InvoiceDraft* d = new InvoiceDraft();
                d->setId(q.value(0).toInt());
                d->m_invoice_year = q.value(1).toInt();
                d->m_invoice_number = q.value(2).toInt();
                d->m_invoice_date = QDate::fromString(q.value(3).toString(), Qt::ISODate);
                d->m_customer_id = q.value(4).toInt();
                d->m_agreement_id = q.value(5).toInt();
                d->m_month = q.value(6).toInt();
                d->m_hours_standard = q.value(7).toInt();
                d->m_hours_overtime = q.value(8).toInt();
                d->m_hours_qualified = q.value(9).toInt();
                d->m_hours_traveltime = q.value(10).toInt();
                d->m_comment = q.value(11).toString();
                b->m_invoicedrafts.update(d);
            }
        }

        // Invoices
        if (q.exec("SELECT id, year, count, number, date, due_date, company_name, company_adrsline1, company_adrsline2, company_reference_name, company_org_number, company_bankgiro, company_phone, company_vat_number, company_swift_bic, company_email, company_f_skatt, company_iban, customer_id, customer_name, customer_adrsline1, customer_adrsline2, customer_reference_name, customer_email_invoice, agreement_id, agreement_payment_terms_days, agreement_late_payment_interest, description_consultant_period, hours_standard, agreement_standard_hourly_rate, agreement_standard_vat, sum_standard, description_standard, hours_overtime, agreement_overtime_hourly_rate, agreement_overtime_vat, sum_overtime, description_overtime, hours_qualified, agreement_qualified_hourly_rate, agreement_qualified_vat, sum_qualified, description_qualified, hours_traveltime, agreement_traveltime_hourly_rate, agreement_traveltime_vat, sum_traveltime, description_traveltime, comment, sum, vat, sum_including_vat FROM invoices ORDER BY id")) {
            while (q.next()) {
                Invoice* inv = new Invoice();
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

                inv->m_customer_id = q.value(18).toInt();
                inv->m_customer_name = q.value(19).toString();
                inv->m_customer_adrsline1 = q.value(20).toString();
                inv->m_customer_adrsline2 = q.value(21).toString();
                inv->m_customer_reference_name = q.value(22).toString();
                inv->m_customer_email_invoice = q.value(23).toString();

                inv->m_agreement_id = q.value(24).toString();
                inv->m_agreement_payment_terms_days = q.value(25).toInt();
                inv->m_agreement_late_payment_interest = q.value(26).toInt();

                inv->m_description_consultant_period = q.value(27).toString();

                inv->m_hours_standard = q.value(28).toInt();
                inv->m_agreement_standard_hourly_rate = q.value(29).toInt();
                inv->m_agreement_standard_vat = q.value(30).toInt();
                inv->m_sum_standard = q.value(31).toInt();
                inv->m_description_standard = q.value(32).toString();

                inv->m_hours_overtime = q.value(33).toInt();
                inv->m_agreement_overtime_hourly_rate = q.value(34).toInt();
                inv->m_agreement_overtime_vat = q.value(35).toInt();
                inv->m_sum_overtime = q.value(36).toInt();
                inv->m_description_overtime = q.value(37).toString();

                inv->m_hours_qualified = q.value(38).toInt();
                inv->m_agreement_qualified_hourly_rate = q.value(39).toInt();
                inv->m_agreement_qualified_vat = q.value(40).toInt();
                inv->m_sum_qualified = q.value(41).toInt();
                inv->m_description_qualified = q.value(42).toString();

                inv->m_hours_traveltime = q.value(43).toInt();
                inv->m_agreement_traveltime_hourly_rate = q.value(44).toInt();
                inv->m_agreement_traveltime_vat = q.value(45).toInt();
                inv->m_sum_traveltime = q.value(46).toInt();
                inv->m_description_traveltime = q.value(47).toString();

                inv->m_comment = q.value(48).toString();
                inv->m_sum = q.value(49).toInt();
                inv->m_vat = q.value(50).toInt();
                inv->m_sum_including_vat = q.value(51).toInt();

                b->m_invoices.append(inv);
            }
        }

        db.close();
        return true;
    };

    bool ok = work();
    QSqlDatabase::removeDatabase(connName);

    return ok;
}
