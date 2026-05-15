#ifndef COMPANYTAB_H
#define COMPANYTAB_H

#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QtSvgWidgets/QSvgWidget>

class CompanyTab : public QWidget
{
    Q_OBJECT
public:
    explicit CompanyTab(QWidget *parent = nullptr);

public slots:
    void onDatabaseUpdated(void);

signals:

private:
    bool m_edit = false;

    QLineEdit* m_name;
    QLineEdit* m_adrs1;
    QLineEdit* m_adrs2;
    QLineEdit* m_orgnr;
    QLineEdit* m_momsregnr;
    QLineEdit* m_bankgiro;
    QLineEdit* m_swiftbic;
    QLineEdit* m_iban;
    QLineEdit* m_phone;
    QLineEdit* m_email;
    QLineEdit* m_web;
    QString m_logo;

    QList<QLineEdit*> lineedits;

    QPushButton* m_logoselection;
    QSvgWidget* m_logowidget = nullptr;

    QPushButton* m_cancel;
    QPushButton* m_ok;

    void setLogo(QString svgString);
    void updateButtons(void);

private slots:
    void onSelectLogo(bool clicked);
    void onEditSave(bool clicked);
    void onCancel(bool clicked);
};

#endif // COMPANYTAB_H
