#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QLabel>

class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr);

private:
    void setupUI();
    static QString getOrdningLogo();
    static QString getCrofteqLogo();
};

#endif // ABOUTDIALOG_H
