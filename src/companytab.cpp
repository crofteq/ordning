#include <QDebug>
#include <QFileDialog>
#include <QSvgRenderer>

#include "data/companyinfo.h"
#include "data/database.h"

#include "companytab.h"

#define LABEL_WIDTH 120

//-----------------------------------------------------------------------------
CompanyTab::CompanyTab(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    QList<QString> labels;

    labels.append("Company name");
    m_name = new QLineEdit();
    lineedits.append(m_name);

    labels.append("Address line 1");
    m_adrs1 = new QLineEdit();
    lineedits.append(m_adrs1);

    labels.append("Address line 2");
    m_adrs2 = new QLineEdit();
    lineedits.append(m_adrs2);

    labels.append("Orgnummer");
    m_orgnr = new QLineEdit();
    lineedits.append(m_orgnr);

    labels.append("Momsnummer");
    m_momsregnr = new QLineEdit();
    lineedits.append(m_momsregnr);

    labels.append("Bankgiro");
    m_bankgiro = new QLineEdit();
    lineedits.append(m_bankgiro);

    labels.append("SWIFT/BIC");
    m_swiftbic = new QLineEdit();
    lineedits.append(m_swiftbic);

    labels.append("IBAN");
    m_iban = new QLineEdit();
    lineedits.append(m_iban);

    labels.append("Telefon");
    m_phone = new QLineEdit();
    lineedits.append(m_phone);

    labels.append("Email");
    m_email = new QLineEdit();
    lineedits.append(m_email);

    labels.append("Web");
    m_web = new QLineEdit();
    lineedits.append(m_web);

    for(int i=0; i<labels.size(); i++)
    {
        QHBoxLayout *hbox = new QHBoxLayout;
        QLabel *p_label = new QLabel(labels.at(i) + ":");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);
        hbox->addWidget(lineedits.at(i));
        layout->addLayout(hbox);
    }

    {
        QHBoxLayout *hbox = new QHBoxLayout;

        QLabel *p_label = new QLabel("Logo:");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);

        m_logoselection = new QPushButton("Select...");
        m_logoselection->setEnabled(false);
        connect(m_logoselection, &QPushButton::clicked, this, &CompanyTab::onSelectLogo);
        hbox->addWidget(m_logoselection);

        hbox->addStretch(1);
        layout->addLayout(hbox);
    }

    {
        QHBoxLayout *hbox = new QHBoxLayout;

        QLabel *p_label = new QLabel("");
        p_label->setMinimumWidth(LABEL_WIDTH);
        p_label->setAlignment(Qt::AlignRight);
        hbox->addWidget(p_label);

        m_logowidget = new QSvgWidget(this);
        m_logowidget->setVisible(false);
        hbox->addWidget(m_logowidget);

        hbox->addStretch(1);
        layout->addLayout(hbox);
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
    setLogo(DataBase::instance()->db()->m_companyinfo.m_logo);
}

//-----------------------------------------------------------------------------
void CompanyTab::setLogo(QString svgString)
{
    QSvgRenderer r(svgString.toUtf8());
    if (r.isValid())
    {
        const int max_width = 200;
        const int max_height = 50;
        m_logo = svgString;
        QSize image_size = r.defaultSize();
        image_size.scale(max_width, max_height, Qt::KeepAspectRatio);
        m_logowidget->setFixedWidth(image_size.width());
        m_logowidget->setFixedHeight(image_size.height());
        m_logowidget->load(m_logo.toUtf8());
        m_logowidget->setVisible(true);
    }
    else
    {
        m_logo = "";
        m_logowidget->setVisible(false);
    }
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
        DataBase::instance()->db()->m_companyinfo.m_logo = m_logo;
    }

    m_edit = !m_edit;
    updateButtons();
}

//-----------------------------------------------------------------------------
void CompanyTab::onCancel(bool clicked)
{
    Q_UNUSED(clicked);
    m_edit = !m_edit;
    updateButtons();
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
}
