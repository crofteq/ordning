# Ordning

`Ordning` is a desktop application for the paperwork of a small Swedish software consulting business: invoices, salaries and the reports that follow from them. It started as a way to bring order to the invoicing of a small but fast-growing consultancy, and has grown to cover the rest of the monthly routine.

It runs on Linux (AppImage) and Windows. Everything is stored locally in a single SQLite file — there is no account, no server and no registration, and the data can be read or moved elsewhere with any SQLite tool. The application is free to use.

The interface is in Swedish.

## Features

- **Företag** — company details, logo, räkenskapsår (calendar or broken), arbetsgivaravgifter per year, Skatteverket's skattetabeller and traktamente.
- **Kunder och avtal** — customers, their contact persons and the agreements (uppdrag) work is invoiced under.
- **Fakturor** — invoice drafts turned into numbered invoices and credit invoices (kreditfaktura), with VAT, rendered to PDF.
- **Anställda och löner** — employees, salary drafts and lönebesked as PDF, with preliminary tax from the imported skattetabell, arbetsgivaravgift and skattefritt traktamente (inrikes and utrikes).
- **Rapporter** — invoicing per customer, uppdrag and month or year, hours per consultant, the monthly underlag for the arbetsgivardeklaration, and an *Ekonomisk översikt*: a multi-year PDF report with key figures and charts.

Amounts are kept in öre, so what is printed adds up exactly. Databases from earlier versions are upgraded when opened, after a backup copy has been written next to the file.

## Limitations

- Invoices and lönebesked are Swedish only.
- Salaries cover monthly pay and skattefritt traktamente; benefits, deductions and holiday pay are not handled.
- Skatteverket's tables and traktamente are imported or typed in by the user, a year at a time.
- Nothing is sent to Skatteverket or a bank — the reports are underlag, not filings.

## Building

The build runs in a Docker container through [`just`](https://github.com/casey/just):

```
just build          # debug build
just build-linux    # release AppImage in build/artifacts/
just build-windows  # release Windows zip in build/artifacts/
```

## Changelog

See `CHANGELOG.md` for release notes and history.
