# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog.

## 1.1.2 - 260930

### Changed

- Grouped "Räkenskapsår" with "Företag" settings.
- Increased font (7pt to 8pt) in footer.

## 1.1.1 - 260928

### Changed

- The user interface is in Swedish throughout: tabs, buttons, labels, table headings and messages.
- The body text of invoices and salary slips is 8pt again, and the side margins are 20mm rather than 25mm to make room for it.
- The company report is now an "Ekonomisk översikt".

### Fixed

- Skattetabell, arbetsgivaravgifter and inrikes traktamente can be entered for previous years.
- The lower arbetsgivaravgift was applied a year too early: it belongs to those who had turned 66 when the year began, which is the cohort born 67 years before it, not 66.

## 1.1.0 - 260927

### Added

- Support for credit invoices
- Support for managing employees
- Support for generating salary slips
- Support for generating reports
- Support for generating a company report (first draft)

### Fixed

- Text wrapping in cells. This led to smaller font, from 8pt to 7pt.

## 1.0.0 - 260515

- First version.
