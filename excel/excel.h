#pragma once
#include <xlnt/xlnt.hpp>
#include <QString>
#include <QVector>
#include <QFile>


struct templateSokol_data {

    QString vent_name;
    QString ip;
    QString template_name;

    templateSokol_data() : vent_name(""), ip(""), template_name("") {}

    bool is_valid()
    {
        return (vent_name.length() > 0 && ip.length() > 0 && template_name.length() > 0);
    }
};

void readTemplateSokol_data(QString path, QVector<templateSokol_data> & v);
void wsriteNewSokol_Templates(QString folder, QString templates_path, QVector<templateSokol_data> & v);

class excel
{
public:
    excel();
};

