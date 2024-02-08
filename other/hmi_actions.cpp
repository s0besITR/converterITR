#include "hmi_actions.h"
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <qDebug>

hmi_actions::hmi_actions(){}
hmi_type::hmi_type(QString name) : name(name) {}
hmi_screen_info::hmi_screen_info(QString name, QString code) :name(name), code(code) {}
simple_walker::simple_walker(hmi_settings& setup, int index) : setup(setup), screen_index(index) {}

bool hmi_settings::isValid()
{
    return (types.size() > 0 && screens.size() > 0);
}


// Получить значение поля in_alias у node
QString simple_walker::getAlias(pugi::xml_node& node)
{
    QString alias;
    QString raw_alias;
    QString no_link("");

    for (auto tool = node.first_child(); tool; tool = tool.next_sibling())
    {
        if (QString(tool.attribute("target").value()).toLower() == "in_alias")
            raw_alias = QString(tool.first_child().first_child().value());			// Взяли значение
        if (QString(tool.attribute("target").value()) == "in_LinkTag")
            no_link = QString(tool.attribute("value").value());			// Взяли значение тега для NoLink (если есть)
    }

    //забираем только то, что между кавычек
    bool flag = false;
    for (auto it = raw_alias.begin(); it != raw_alias.end(); it++)
    {
        if (*it == '"') flag = true;
        else if (flag)	alias += *it;
    }
    return alias + no_link;
}

// Добавить val в tags, если его там нет
bool simple_walker::addIfNotDuplicate(QVector<QString>& tags, QString val)
{
    for (auto tag = tags.begin(); tag != tags.end(); tag++)
        if (*tag == val)
            return false;
    tags.push_back(val);
    return true;
}

// Проходимся по this->objs и выгружаем данные в wb документ excel
void hmi_settings::to_xlsx_fun(xlnt::workbook & wb)
{
    wb.remove_sheet(wb.active_sheet());
    wb.create_sheet(0).title("Функции");
    wb.create_sheet(1).title("Агрегаты");
    xlnt::worksheet sheet = wb.sheet_by_title("Функции");

    int i = 1;
    for(QString fun_name: this->getUniqFun())
    {
        sheet.cell(1,i++).value(fun_name.toStdString().c_str());
    }
    sheet = wb.sheet_by_title("Агрегаты");
    QStringList headers, headers_width;
    headers << "Имя" << "Тип" << "Тег" << "Функция" << "Экран";
    headers_width << "18" << "18" << "18" << "18" << "18";

    // Создаем шапку
    for(int i = 0; i != headers.size(); ++i)
    {
        xlnt::cell cell = sheet.cell(i+1,1);
        cell.value(headers.at(i).toStdString());
        sheet.column_properties(cell.column()).custom_width = true;
        sheet.column_properties(cell.column()).width = headers_width[i].toUInt();
    }
    // Выгружаем данные
    int r = 2;
    for(auto & data : this->objs)
    {
        sheet.cell(1,r).value(data.display_name.toStdString().c_str());
        sheet.cell(2,r).value(data.base_type.toStdString().c_str());
        sheet.cell(3,r).value(data.tag.toStdString().c_str());
        sheet.cell(4,r).value(data.function.toStdString().c_str());
        sheet.cell(5,r).value(data.screen_name.toStdString().c_str());
        r++;
    }
}

// Возвратить массив уникальных имен FUNCTION
QStringList hmi_settings::getUniqFun()
{
    QStringList tmp;
    for(auto & obj : this->objs)
    {
        if(!tmp.contains(obj.function))
            tmp << obj.function;
    }
    return tmp;
}

// Проходимся по всему xml документу и заполняем нужные нам вещи в объекте настроек
bool simple_walker::for_each(pugi::xml_node& node)
{
    if (QString(node.name()) == "object")
    {
        QString base_type = node.attribute("base-type").value();
        QString alias = getAlias(node);

        // Логика для формирования списка объектов и их функции попапов
        pugi::xml_node function_node = findNodeByAttribute(node, "target", "FUNCTION");
        if(function_node)
        {
            hmi_obj_type tmp_obj;
            tmp_obj.base_type = base_type;
            tmp_obj.display_name = node.attribute("display-name").value();
            tmp_obj.tag = alias;
            tmp_obj.function = function_node.attribute("value").value();
            tmp_obj.screen_name = setup.screens[screen_index].name;
            setup.objs.push_back(tmp_obj);
        }

        // Логика парсинга для формирования фукнции UpdateAlarm
         hmi_type * p_type(0);
        for(hmi_type & type : setup.types)
            if(type.name ==  base_type)
                p_type = &type;
        if(!p_type)
            return true;
        setup.screens[screen_index].elem_count++;

        for (int i = 0; i < p_type->tags.size(); i++)
        {
            if (addIfNotDuplicate(setup.screens[screen_index].tags, alias + p_type->tags[i]))
                setup.screens[screen_index].tags_count++;
        }
        if (p_type->name == "NoLink" && p_type->tags.size() == 0)
        {
            if(addIfNotDuplicate(setup.screens[screen_index].tags, alias))
                setup.screens[screen_index].tags_count++;
        }
    }
    return true;
}

// Открываем filename и парсим его в объект set
void parseHMISettins(QString filename, hmi_settings & set)
{
    pugi::xml_document doc;
    openFileXml(doc,filename);

    pugi::xml_node screens_node =    findNodeByName(doc.root(), "Screens");
    if(!screens_node)
        return;
    for(auto screen : screens_node.children("screen"))
    {
        hmi_screen_info my_screen_info(screen.attribute("name").value(), screen.attribute("code").value());
        for(auto tag : screen.children("tag"))
            my_screen_info.tags.push_back(tag.attribute("name").value());
        set.screens.push_back(my_screen_info);
    }

    for(auto obj_type : screens_node.parent().children("ObjectType"))
    {
        hmi_type my_type(obj_type.attribute("name").value());
        for(auto tag : obj_type.children("tag"))
            my_type.tags.push_back(tag.attribute("name").value());
        set.types.push_back(my_type);
    }
}

// Парсим отдельные экраны и достаем у них аварийные теги
bool parseHMIScreen(QString screen_path, hmi_settings & set, int screen_index)
{
    pugi::xml_document doc;
    try{
        openFileXml(doc,screen_path);
    }
    catch(const std::runtime_error& e){ return false;}
    simple_walker walker(set, screen_index);
    doc.traverse(walker);
    return true;
}

// Выгрузить настройки в txt файл в виде функции updateAlarm
QString hmi_settings::to_string()
{
    QString content;
    QTextStream out(&content);

    QString formattedTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");

    out << QString("/*%1*/").arg(formattedTime.toLocal8Bit().toStdString().c_str()) << Qt::endl;
    out << "/* Place this file to unit.Global.updateAlarm() function */" << Qt::endl << Qt::endl;

    // массивы
    for (int i = 0; i < screens.size(); i++)
    {
        out << "/* Tags count = " << screens[i].tags_count << " */" << Qt::endl;
        out << "/* Elements count = " << screens[i].elem_count << " */" << Qt::endl;
        out << "var " << screens[i].name << " = [" << Qt::endl;

        for (int j = 0; j < screens[i].tags.size(); j++)
        {
            out << "'" << screens[i].tags[j] << "'";
            if (j < screens[i].tags.size() - 1)
                out << ",";
            out << Qt::endl;
         }
         out << "];" << Qt::endl << Qt::endl;
    }

    //логика функции
    for (int i = 0; i < screens.size(); i++)
    {
        out << "if (button == " << screens[i].code << ")" << Qt::endl;
        out << "\t return JSON.stringify(" << screens[i].name << ");" << Qt::endl;
    }
    out << Qt::endl << "return JSON.stringify([]);";

    return content;
}


/* ГЕНЕРАЦИЯ HMI ОБЪЕКТОВ */

hmi_objects::hmi_objects(QString s)
{
    QStringList l = s.split(",");
    if(l.size() != 4)
         return;

    this->type = l.at(0);
    this->name = l.at(1);
    this->tag = l.at(2);

    // Расположение
    QString loc = l.at(3);

    if(loc.contains("(") && loc.contains(")"))
    {
        int start_pos = loc.indexOf("(");
        int end_pos = loc.indexOf(")");
        this->location = loc.mid(0, start_pos);
        this->room = loc.mid(start_pos + 1, end_pos - start_pos - 1);
    }
    else
    {
        this->location = loc;
        this->room = "SOME_ROOM";
    }
    valid = true;
}

void load_obj_map(QString & path, QMap<QString,QVector<hmi_objects>> & obj_map)
{
    QFile file(path);
    QVector<hmi_objects> obj_vector;
    bool has_room_number = false;

    if (file.open(QIODevice::ReadOnly))
    {
       QTextStream in(&file);
       while (!in.atEnd())
       {
           hmi_objects tmp(in.readLine());
           if(tmp.valid)
           {
                obj_vector.push_back(tmp);
                if (tmp.room != "SOME_ROOM")
                {
                    has_room_number = true;
                }
           }
       }
       file.close();
    }

    // Создаем карту
    for (hmi_objects & obj : obj_vector)
    {
        QString key = has_room_number ? obj.room : obj.location;
        obj_map[key].push_back(obj);
    }
}

void load_obj_vector(QString & path, QVector<hmi_objects> & obj_vec)
{
    QFile file(path);

    if (file.open(QIODevice::ReadOnly))
    {
       QTextStream in(&file);
       while (!in.atEnd())
       {
           hmi_objects tmp(in.readLine());
           if(tmp.valid)
           {
                obj_vec.push_back(tmp);
           }
       }
       file.close();
    }
}

void openFileXmlFromTemplate(pugi::xml_document& doc, const QString& filename)
{

    QFile file(filename);
    if(file.open(QIODevice::ReadOnly))
    {
        doc.load_string(QString(file.readAll()).toStdString().data(), pugi::parse_default);
        file.close();
    }
}


void change_mnemo(pugi::xml_node mnemo_obj, hmi_objects & obj, int & x_cor)
{
    QStringList tag_split = obj.tag.split(".");
    QString display_name = tag_split.last();

    // Имя
    mnemo_obj.attribute("name").set_value(display_name.toStdString().data());
    mnemo_obj.attribute("display-name").set_value(display_name.toStdString().data());

    // Расположение
    pugi::xml_node x_node = findNodeByAttribute(mnemo_obj,"target","X");
    pugi::xml_node y_node = findNodeByAttribute(mnemo_obj,"target","Y");
    x_node.attribute("value").set_value(QString::number(x_cor).toStdString().data());
    y_node.attribute("value").set_value("70");

    x_cor += 10 + QString(findNodeByAttribute(mnemo_obj,"target","Width").attribute("value").value()).toInt();

    // Alias
    pugi::xml_node alias_node = findNodeByAttribute(mnemo_obj,"target","in_Alias").child("expr");
    QString current_alias = alias_node.first_child().value();
    QStringList alias_split = current_alias.split("+");
    if (alias_split.length() == 1)
    {
        alias_node.first_child().set_value(obj.tag.toStdString().data());
    }
    else
    {
         QString alias_str = QString("%1 + \"%2\"").arg(alias_split.at(0), obj.tag);
         alias_node.first_child().set_value(alias_str.toStdString().data());
    }

    // Заголовок
    findNodeByAttribute(mnemo_obj,"value","Name").attribute("value").set_value(obj.name.toStdString().data());
    //или
    findNodeByAttribute(mnemo_obj,"target","Text").attribute("value").set_value(obj.name.toStdString().data());
}

void change_room(pugi::xml_node room_obj, hmi_objects obj, int & y_cor)
{
     // Расположение
    pugi::xml_node x_node = findNodeByAttribute(room_obj,"target","X");
    pugi::xml_node y_node = findNodeByAttribute(room_obj,"target","Y");
    x_node.attribute("value").set_value("0");
    y_node.attribute("value").set_value(QString::number(y_cor).toStdString().data());

    room_obj.attribute("name").set_value(QString("Room_%1").arg(obj.room).toStdString().data());
    room_obj.attribute("display-name").set_value(QString("Room_%1").arg(obj.room).toStdString().data());

    findNodeByAttribute(room_obj,"value","Венткамера").attribute("value").set_value(obj.location.toStdString().data());
    findNodeByAttribute(room_obj,"value","пом. 9.6").attribute("value").set_value(QString("пом. %1").arg(obj.room).toStdString().data());
}


void gen_hmi_form(QMap<QString,QVector<hmi_objects>> & obj_map,  pugi::xml_document & types_doc, QString save_name)
{
    if (obj_map.size() == 0)
    {
        return;
    }

    pugi::xml_document form_doc;
    pugi::xml_document room_doc;

    openFileXmlFromTemplate(form_doc,":/hmi_form.omobj");
    openFileXmlFromTemplate(room_doc,":/hmi_room.xml");

    int y_cor = 0;
    bool save = false;

    for(auto & key : obj_map.keys())
    {
        pugi::xml_node room =  findNodeByAttribute(room_doc,"name","Room_1");
        if(!room)
            continue;
        pugi::xml_node room_obj = form_doc.child("type").append_copy(room);
        change_room(room_obj,obj_map[key].at(0),y_cor);
        y_cor += 200;

        int obj_x_cor = 0;
        for(hmi_objects & obj : obj_map[key])
        {
            pugi::xml_node mnemoznak =  findNodeByAttribute(types_doc.child("types"),"base-type",obj.type);
            if(!mnemoznak)
                continue;

             pugi::xml_node mnemo_obj = room_obj.append_copy(mnemoznak);
             change_mnemo(mnemo_obj, obj, obj_x_cor);
             save = true;
        }
    }

    if(save)
        form_doc.save_file(save_name.append("/").append("FORM_BIG").append(".omobj").toStdWString().data(),"\t", pugi::format_indent, pugi::encoding_utf8);
}

void gen_hmi_nolink_form(QVector<hmi_objects> & obj_vec,  pugi::xml_document & types_doc, QString save_name)
{
    if (obj_vec.size() == 0)
    {
        return;
    }

    pugi::xml_document form_doc;
    openFileXmlFromTemplate(form_doc,":/hmi_form.omobj");

    pugi::xml_node link_node = findNodeByAttribute(types_doc,"base-type","NoLink");

    int y_cor = 0;
    int h = QString(findNodeByAttribute(link_node,"target","Height").attribute("value").value()).toInt();
    bool save = false;

    for(auto & obj : obj_vec)
    {
        pugi::xml_node link_obj = form_doc.child("type").append_copy(link_node);
        findNodeByAttribute(link_obj,"target","X").attribute("value").set_value("0");
        findNodeByAttribute(link_obj,"target","Y").attribute("value").set_value(y_cor);

        QStringList tag_split = obj.tag.split(".");
        QString display_name = "NoLink_" + tag_split.last();

        link_obj.attribute("name").set_value(display_name.toStdString().data());
        link_obj.attribute("display-name").set_value(display_name.toStdString().data());
        findNodeByAttribute(link_obj,"value","Text").attribute("value").set_value(obj.name.toStdString().data());

        //Alias
        pugi::xml_node alias_node = findNodeByAttribute(link_obj,"target","in_Alias").child("expr");
        QString current_alias = alias_node.first_child().value();
        QStringList alias_split = current_alias.split("+");
        if (alias_split.length() == 1)
        {
            alias_node.first_child().set_value(obj.tag.toStdString().data());
        }
        else
        {
             QString alias_str = QString("%1 + \"%2\"").arg(alias_split.at(0), obj.tag);
             alias_node.first_child().set_value(alias_str.toStdString().data());
        }

        y_cor += h;
        save = true;
    }

    if(save)
        form_doc.save_file(save_name.append("/").append("FORM_BIG_LINK").append(".omobj").toStdWString().data(),"\t", pugi::format_indent, pugi::encoding_utf8);
}
