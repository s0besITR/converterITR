#include "mk_logic.h"
#include <QDebug>
#include <QMessageBox>

mk_logic::mk_logic()
{

}

int getNum (QString& l){
    const QRegExp rx(QLatin1String("[^0-9]+"));
    return l.split(rx, Qt::SkipEmptyParts).last().toInt();
};

QString replaceBrackets(const QString &l, int num)
{
    return QString(l.left(l.indexOf("[") + 1) + QString::number(num) +  l.right(l.length() - l.indexOf("]")));
};

void isagraf_csv_export(QString app_filename, QString csv_name)
{
    // Отркываем _application, как xml документ для парсинга переменных
    pugi::xml_document doc;
    openFileXml(doc, app_filename);
    pugi::xml_node g_vars = doc.child("project").child("addData");
    if(!g_vars)
        return;

    QFile csv_file(csv_name);
    if(!csv_file.open(QIODevice::ReadWrite | QIODevice::Text))
        return;
    QTextStream out(&csv_file);

    QString var_string = "%1,%2,%3,,%4,Var,ReadWrite,,,,,False,,";

    out << "Name,Data Type,Dimension,String Size,Initial Value,Direction,Attribute,Comment,Alias,Wiring,Address,Retained,Retained Flags,Groups,Version=2" << '\n';
    out << "(Name),(DataType),(Dimension),(StringSize),(InitialValue),(Direction),(Attribute),(Comment),(Alias),(Wiring),(UserAddress),(IsRetained),(RetainFlags),(Groups)" << '\n';

    for(pugi::xml_node & data_node : g_vars.children("data"))
        for(pugi::xml_node & g_var : data_node.children("globalVars"))
            for(pugi::xml_node & var : g_var.children("variable"))
            {
                pugi::xml_node type_node = var.child("type");
                if(!type_node)
                    continue;

                pugi::xml_node baseType_node = findNodeByName(type_node,"baseType");
                pugi::xml_node derivedType_node = findNodeByName(type_node,"derived");
                pugi::xml_node array_node = type_node.child("array");       // Если массив, то его размер

                QString name = var.attribute("name").value();
                QString type = derivedType_node ? derivedType_node.attribute("name").value() : baseType_node ? baseType_node.first_child().name() : type_node.first_child().name();
                QString dim = array_node ? QString("\"[%1..%2]\"").arg(array_node.child("dimension").attribute("lower").value()).arg(array_node.child("dimension").attribute("upper").value()) : "";
                QString init = "";

                if(var.child("initialValue"))
                    if(var.child("initialValue").child("simpleValue"))
                        init = var.child("initialValue").child("simpleValue").attribute("value").value();

                out << var_string.arg(name).arg(type).arg(dim).arg(init) << '\n';
            }
    csv_file.close();
}

/*void isagraf_xlsx_edit(xlnt::workbook & wb, QString filename)
{
    QStringList headers1, headers2;
    QStringList headers_width;
    headers1 << "Имя" << "Тип данных" << "Размерность" << "Размер строки" << "Начальное значение" << "Направление" << "Атрибут" << "Комментарий" << "Алиас" << "Монтаж" << "Адрес" << "Сохранение" << "Флаги сохранения" << "Группы" << "Version=2";
    headers2 << "(Name)" << "(DataType)" << "(Dimension)" << "(StringSize)" << "(InitialValue)" << "(Direction)" << "(Attribute)" << "(Comment)" << "(Alias)" << "(Wiring)" << "(UserAddress)" << "(IsRetained)" << "(RetainFlags)" << "(Groups)" << "";
    headers_width << "21" << "23" << "9" << "9" << "9" << "9" << "15" << "9" << "9" << "9" << "9" << "9" << "9" << "9" << "9";

    xlnt::worksheet sheet = wb.active_sheet();
    // Создаем шапку
    for(int i = 0; i != headers1.size(); ++i)
    {
        xlnt::cell cell1 = sheet.cell(i+1,1);
        xlnt::cell cell2 = sheet.cell(i+1,2);
        cell1.value(headers1.at(i).toStdString());
        cell2.value(headers2.at(i).toStdString());
        sheet.column_properties(cell1.column()).custom_width = true;
        sheet.column_properties(cell1.column()).width = headers_width[i].toUInt();
    }

    // Отркываем _application, как xml документ для парсинга переменных
    pugi::xml_document doc;
    openFileXml(doc, filename);
    pugi::xml_node g_vars = doc.child("project").child("addData");
    if(!g_vars)
        return;

    // Проходимся по глобальным переменным и переносим их в ячейки xlsx документа
    int r = 2;
    for(pugi::xml_node & data_node : g_vars.children("data"))
        for(pugi::xml_node & g_var : data_node.children("globalVars"))
            for(pugi::xml_node & var : g_var.children("variable"))
            {
                pugi::xml_node type_node = var.child("type");
                if(!type_node)
                    continue;
                 r++;

                pugi::xml_node baseType_node = findNodeByName(type_node,"baseType");
                pugi::xml_node derivedType_node = findNodeByName(type_node,"derived");
                QString type_str = derivedType_node ? derivedType_node.attribute("name").value() : baseType_node ? baseType_node.first_child().name() : type_node.first_child().name();

                sheet.cell(2,r).value(type_str.toStdString().c_str());      // Тип
                sheet.cell(1,r).value(var.attribute("name").value());       // Имя

                pugi::xml_node array_node = type_node.child("array");       // Если массив, то его размер
                if(array_node)
                    sheet.cell(3,r).value(QString("[%1..%2]").arg(array_node.child("dimension").attribute("lower").value()).arg(array_node.child("dimension").attribute("upper").value()).toStdString().c_str());

                if(var.child("initialValue"))
                    if(var.child("initialValue").child("simpleValue"))
                        sheet.cell(5,r).value(var.child("initialValue").child("simpleValue").attribute("value").value());

                 sheet.cell(6,r).value("Var");
                 sheet.cell(7,r).value("ReadWrite");
                 sheet.cell(12,r).value("False ");
            }
}
*/
void isagraf_ST_export(QString filename, QString folder, bool split, int max_lines)
{
    pugi::xml_document doc;
    openFileXml(doc,filename);

    pugi::xpath_node  xnode = doc.select_node("/project/types/pous");
    if(!xnode)
        return;

    //Для каждого pou
    for(pugi::xml_node & p_node : xnode.node().children("pou"))
    {
        if(QString(p_node.attribute("pouType").value()) != "program")                           // Нам нужны только с типом program
            continue;

        pugi::xml_node body_node = p_node.child("body").child("ST").child("xhtml");             // Если нет узла с ST кодом, едем дальше
        if(!body_node )
            continue;

        QStringList modified_data;
        modifyIsagrafData(body_node, modified_data, split, max_lines);                           // Модифицировали данные, положили в список

        // Сохраняем на диск
        QString out_file = folder +"/" + p_node.attribute("name").value();
        out_file.replace("_IEC_870_5", "_IecProgram");
        out_file.replace("_sFB", "_FB");

        for(int i = 0; i != modified_data.size(); ++i)
        {
            QString pref = modified_data.size() == 1 ? "" : QString::number(i + 1);
            QFile file(out_file + pref + ".txt");
            if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QMessageBox::critical(0, "Ошибка", "Невозможно создать файл " + out_file);
                continue;
            }
            file.write(modified_data.at(i).toStdString().c_str());
            file.close();
        }
    }
}

QString parse_indexIEC(QString & line, QTextStream & stream)
{
    static bool inside_index = false;
    static QMap<int, QVector<QString>> step_data;
    static int current_step = 0;
    static int index = 0;

    if(line.startsWith("indexIEC :=",Qt::CaseInsensitive))
    {
        index = getNum(line);
        inside_index = true;
        return "";
    }
    if(!inside_index)
        return line;

    if(line.startsWith("END_IF"))
    {
        // выгружаем нашу строчку
        int prev_key = 0;
        for(int key : step_data.keys())
        {
            if(index < key)
            {
                for(QString s : step_data[key])
                    stream << replaceBrackets(s, index - prev_key) << "\n";
                //for(auto p =  step_data[key].rbegin(); p !=  step_data[key].rend(); ++p)      // У баублиса строчки в обратном порядке выводились
                //    stream << replaceBrackets(*p, index - prev_key) << "\n";
                break;
            }
            prev_key = key;
        }
        step_data.clear();
        index = 0;
        current_step = 0;
        inside_index = false;
    }
    else if(line.startsWith("IF") || line.startsWith("ELSIF"))
        current_step = getNum(line);
    else if(line.startsWith("ELSE"))
        current_step =  999999;
    else
        step_data[current_step].push_back(line.replace("\t",""));
    return "";
}

QString parse_FOR_WHILE(QString & line, QString sfb_name)
{
    QString index_start = "";
    QString index_end = "";

    QString index_name = sfb_name.endsWith("sFBModbus") ? "indexFBModbus" : "indexFB";

    QString template_str = "%1 := %2;\n"
            "WHILE %1 <= %3 DO";

    if(line.startsWith("FOR Index :=",Qt::CaseInsensitive))
    {
        int _eq = line.indexOf(":=");
        int _to = line.indexOf(" TO ");
        int _do = line.length() - 3;

        index_start = line.mid(_eq + 2, _to - _eq - 2).replace(" ", "");
        index_end = line.mid(_to + 4, _do - _to - 4).replace(" ", "");

        return template_str.arg(index_name).arg(index_start).arg(index_end);
    }
    else if(line.contains("END_FOR"))
        return  QString("\t%1 := %1 + 1;\nEND_WHILE;").arg(index_name);
    return line;
}

 // Парсим аргументы FB(arg1 := arg2[arg3], arg1 := arg2[arg3]...)
void parseFbArguments(QString line, QVector<std::tuple<QString, QString, QString>> & args, QString index_name)
{
    line = line.replace(" ","").replace("\t","");               // Убираем лишние символы
    QString tmp = "";
    QString arg1, arg2, arg3 = "";
    for(QChar & ch : line)
    {
        if(ch == '=')    arg1 = tmp.replace(":","");
        if(ch == '[')    arg2 = tmp;
        if(ch == ']')    arg3 = tmp.replace("Index", index_name);
        if(ch == ',' || ch == ')')
        {
            args.push_back(std::make_tuple(arg1,arg2,arg3));
            arg1 = arg2 = arg3 = "";
            tmp = "";
        }
        if(ch == '(' || ch == ')' || ch =='=' || ch == ',' || ch == '[' || ch == ']')
        {
            tmp.clear();
            continue;
        }
        tmp.append(ch);
    }
}

QString getArgumentsString(QVector<std::tuple<QString, QString, QString>> & args, QString index_name)
{
    QString all_args = "";
    for(int i = 0; i != args.size(); ++i)
    {
        QString arg2 = std::get<1>(args.at(i));
        QString arg3 = std::get<2>(args.at(i));
        if(arg3 == "Index")
            arg3 = index_name;
        if(i != 0) all_args.append(",");

        if(arg3 == "")
            all_args.append(arg2);
        else
            all_args.append(QString("%1[%2]").arg(arg2).arg(arg3));
    }
    return all_args;
}

QString parse_FB(QString & line, QTextStream & stream, QString sfb_name)
{
    static QVector<std::tuple<QString, QString, QString>> args;
    QString index_name = sfb_name.endsWith("sFBModbus") ? "indexFBModbus" : "indexFB";
    static QString fb_name = "";
    static bool inside_fb = false;
    static bool inside_while = false;

    if(line.contains("WHILE ",Qt::CaseSensitive))
        inside_while = true;
    if(line.contains("END_WHILE",Qt::CaseSensitive))
        inside_while = false;

    if(line.contains("FB (") || line.contains("FB("))
    {
        fb_name = line.left(line.indexOf("FB")).replace(" ","").replace("\t","");
        inside_fb = true;
    }

    if(!inside_fb)
        return line;

    parseFbArguments(line, args, index_name);

    //Выгружаем аргументы, как надо
    if(line.endsWith(");"))
    {
        if(inside_while)   stream << "\t";
        inside_fb = false;
        if(args.size() == 0)
        {
            stream << QString("tmpBool := %1 ();").arg(fb_name) << "\n";
            fb_name = "";
            return "";
        }

        if(fb_name.contains("typeModbus"))
            stream << QString("tmpBool := %1 (%2);").arg(fb_name).arg(getArgumentsString(args,index_name)) << "\n";
        else
        {
            QString arg = std::get<2>(args.at(0));
            stream << QString("tmpBool := %1 (%2);").arg(fb_name).arg(arg) << "\n";
        }

        args.clear();
        fb_name = "";        
        return "";
    }
    return "";
}

void splitMaxLines(const QString & input, QStringList & list, int max)
{
    QString in = input;
    QTextStream _in (&in,     QIODevice::ReadOnly);

    QString out = "";
    int out_size = 0;

    QString buf = "";
    int buf_size = 0;

    bool inside = false;

    // Выгрузить
    auto push_input = [&out, & list, &out_size](){
        if(out.at(out.length()-1) == '\n')            // Удаляем последний перевод строки в конце, если он есть
            out.remove(out.length()-1,1);
        list << out;
        out = "";
        out_size = 0;
    };

    while (!_in.atEnd())
    {
        QString line = _in.readLine();
        if(line.startsWith("IF ") || line.startsWith("FOR ") || line.startsWith("WHILE "))
            inside = true;

        if(inside)
        {
            buf.append(line).append('\n');
            ++buf_size;
        }
        else
        {
            out_size++;
            out.append(line).append("\n");

            if(out_size >= max)
            {
                push_input();
            }
        }

        if(line.startsWith("END_"))                         // Если были внутри блока и нашли конец блока
        {
            inside = false;                                 // Снимаем флаг
            if(out_size + buf_size <= max)                  // Если длина текущего файла плюс длина буфера меньше макисмальной
            {
                out.append(buf);                            // Отправляем буффер в текущий файл
                out_size+=buf_size;                         // Увеличиваем длину файла на длину буфера
            }
            else                                            // Если длина текущего файла плюс длина буфера больше максимальной
            {
                push_input();
                out = buf;
                out_size = buf_size;
            }
            buf_size = 0;                                 // Очищаем буфер
            buf = "";
        }
    }

    if(out_size>0)
        list << out;

}

void modifyIsagrafData(pugi::xml_node body_node, QStringList & dataList, bool split, int max_lines)
{
    QString data = body_node.first_child().value();
    QString new_data;
    QTextStream data_stream     (&data,     QIODevice::ReadOnly);
    QTextStream new_data_stream (&new_data, QIODevice::WriteOnly);

    QString pou_name = body_node.parent().parent().parent().attribute("name").value();

    new_data_stream << "(* " << pou_name << " *)" << "\n";
    while (!data_stream.atEnd())
    {
        QString line = data_stream.readLine();                          // Читаем строчку
        line = parse_indexIEC(line,new_data_stream);                    // Проверяем строчку на наличие indexIEC                
        line = parse_FOR_WHILE(line, pou_name);
        line = parse_FB(line,new_data_stream, pou_name);

        if(line.isEmpty() || (line.size() == 1 && line.at(0) == '\t'))  // Пустую пропускаем
            continue;
        new_data_stream << line << "\n";            // Просто пишем строчку
    }
    new_data_stream << "(* end_section *)";
    if(!split || max_lines == 0 )
        dataList << new_data;
    else
        splitMaxLines(new_data, dataList, max_lines);

    qDebug() << new_data.count('\n');
}
