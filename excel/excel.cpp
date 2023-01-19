#include "excel.h"

void readTemplateSokol_data(QString path, QVector<templateSokol_data> & v)
{
    xlnt::workbook wb;
    wb.load(path.toLocal8Bit().toStdString());

    xlnt::worksheet ws = wb.active_sheet();
    for (auto row = 1; row <= 1000; ++row)
    {
        templateSokol_data t;
        t.vent_name = ws.cell(xlnt::cell_reference(1, row)).to_string().c_str();
        t.ip = ws.cell(xlnt::cell_reference(2, row)).to_string().c_str();
        t.template_name = ws.cell(xlnt::cell_reference(3, row)).to_string().c_str();
        t.ip.replace(".", ",");
        if (t.is_valid())
            v.push_back(t);
       }
}

void wsriteNewSokol_Templates(QString folder, QString templates_path, QVector<templateSokol_data> & v)
{
    for(auto & t : v)
    {
        QFile file(templates_path + "\\" +  t.template_name + ".xml");
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        QString fileData;
        fileData = file.readAll();
        file.close();
        fileData.replace(t.template_name,t.vent_name);
        fileData.replace( "192,168,0,255",t.ip);

        QFile out_file(folder + "\\" + t.vent_name + ".xml");
        if(!out_file.open(QIODevice::WriteOnly | QIODevice::Text))
            continue;
        out_file.write(fileData.toStdString().c_str());
        out_file.close();
    }
}

excel::excel()
{

}
