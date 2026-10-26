#ifndef SALARYDRAFT_H
#define SALARYDRAFT_H

#include <QDate>
#include <QList>
#include <QString>
#include <QStringList>

#include "salarytypes.h"

// One line of a salary statement: Löneart, Benämning, Antal, Kronor, Totalt.
// The löneart is picked from the fixed table in salarytypes.h, and the benämning
// and the kind are the ones it carries. They are kept on the line all the same:
// a slip is a frozen copy of what it printed, so a löneart worded differently
// later does not reword a lönebesked already paid out.
struct SalaryLine
{
    int code = SALARY_TYPE_NONE;    // löneart, 10
    QString text;                   // benämning, "Månadslön"
    int quantity = 100;             // antal, in hundredths: 1,00 is 100
    long long amount = 0;           // kronor per unit, in öre
    int kind = SALARY_LINE_TAXABLE; // SalaryLineKind, the löneart's
    // What the line was for, printed under the benämning in the small type. A
    // skattefritt traktamente is the reason it is here: the employer has to be
    // able to show the journey's purpose, place and days, and that is a
    // different thing from what the line is called. Free text, and empty on
    // most lines.
    QString comment;
    // What the line applies to, as far as that is a matter of record rather
    // than of writing: for a skattefritt traktamente the place it was paid for,
    // "Inrikes" or the country whose normalbelopp it is. It is not free text
    // but the value the schablon was looked up by, which is why it is kept
    // apart from the comment — this is the reference data's and the comment is
    // the user's. A löneart that is neither lön nor a journey would put what it
    // applies to here as well: a förmån what the benefit was, a line for a
    // period which period. Printed ahead of the comment, and empty on a line of
    // ordinary lön.
    QString applies_to;

    // Totalt: antal * kronor, rounded to the öre. See util/amount.h.
    long long total_ore(void) const;

    bool operator==(const SalaryLine& o) const
    {
        return code == o.code && text == o.text && quantity == o.quantity
            && amount == o.amount && kind == o.kind && comment == o.comment
            && applies_to == o.applies_to;
    }
};

// The most lines a salary statement can carry and still fit its one page.
static const int SALARY_MAX_LINES = 15;

// Reads the line table of the edit page: per row the löneart, the antal and the
// kronor as typed, the comment, if any, and what the line applies to, which the
// page shows rather than takes. Antal
// and kronor take a decimal comma. The benämning and whether the line is
// skattefri come from the löneart and are not read, which is the point of the
// lönearter being fixed: a line cannot be given a tax treatment that does not
// belong to what it is. There has to be at least one line and at most
// SALARY_MAX_LINES, and every line needs a löneart the table holds. A row may
// stop short of the later cells, which is what a table with fewer columns hands
// over. On the first problem, returns false and says why in `error`.
bool salary_parse_lines(const QList<QStringList>& rows, QList<SalaryLine>* lines, QString* error);

class SalaryDraft
{
public:
    SalaryDraft();

    int m_employee_id = -1;
    QDate m_payment_date;       // utbetalningsdag; its year picks the skattetabell
    QList<SalaryLine> m_lines;

    // Copies the fields of `d`. The id is left alone.
    void update(const SalaryDraft& d);

    // Bruttolön: the sum of the lines of ordinary lön. A skattefri ersättning
    // is not part of it — it is paid out without being taxed.
    long long gross_ore(void) const;

    // The skattefria ersättningar: the sum of the lines that are not lön.
    long long taxfree_ore(void) const;

    int id(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;
};

#endif // SALARYDRAFT_H
