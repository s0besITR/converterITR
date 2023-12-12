#include "acs_xml/elesy_xml.h"
#include <stdexcept>
#include <QFileInfo>
#include <QDebug>
#include <QDir>
#include <QPixmap>

void openFileXml(pugi::xml_document& doc, const QString& filename)
{
    pugi::xml_parse_result result = doc.load_file(filename.toStdWString().data(), pugi::parse_default, pugi::encoding_utf8);
    if (result.status != pugi::status_ok)
    {
        QString msg = QString("Возникла ошибка при загрузке %1:\n%2").arg(filename, result.description());
        throw std::runtime_error(msg.toStdString().data());
    }
}

void parseACSFiles(QStringList files, uint plc, std::tuple<bool, bool> os_flags, QVector<Elesy_XML> & result_vector)
{
    // Формируем список пар. Если был выбран только Application, то догружаем файл крейта и наоборот
    while(files.isEmpty() != true)
    {
        QString file = files.at(0);
        QString app = file;
        QString crate = file;

        if(file.endsWith("_application.xml",Qt::CaseInsensitive))
            crate.replace("_application.xml", ".xml",Qt::CaseInsensitive);
        else
            app.replace(".xml", "_application.xml",Qt::CaseInsensitive);

        files.removeOne(app);
        files.removeOne(crate);
        if(QFile::exists(app) && QFile::exists(crate))                          // Добавляем в список объектов пару, только если оба файла существуют физически
            result_vector.push_back(Elesy_XML(std::make_pair(crate, app), plc, os_flags));
        else if(QFile::exists(app))
            result_vector.push_back(Elesy_XML(std::make_pair("", app), plc, os_flags));        // передаем application, если не нашли крейт
    }
}

void load_gvl_from_template(pugi::xml_document & doc,QString name, QVector<std::pair<QString,QString>> attributes)
{
    QFile file(":/gvl_template.xml");
    if(file.open(QIODevice::ReadOnly))
    {
        doc.load_string(QString(file.readAll()).toStdString().data(), pugi::parse_default);
        file.close();
    }

    pugi::xml_node node_vars =  findNodeByName(doc, "globalVars");
    pugi::xml_node node_obj =  findNodeByName(doc, "Object");
    pugi::xml_node node_attr =  findNodeByName(doc, "Attributes");

    if(!node_vars || !node_obj)
        return;
    node_vars.attribute("name").set_value(name.toStdString().data());
    node_obj.attribute("Name").set_value(name.toStdString().data());

    for(std::pair<QString,QString> attr_pair : attributes)
    {
        pugi::xml_node new_node = node_attr.append_child("Attribute");
        new_node.append_attribute("Name").set_value(std::get<0>(attr_pair).toStdString().data());
        new_node.append_attribute("Value").set_value(std::get<1>(attr_pair).toStdString().data());
    }
}

pugi::xml_node findNodeByAttribute(pugi::xml_node root, QString attr_name, QString attr_val)
{
    return root.find_node(
                [&attr_name, &attr_val](pugi::xml_node& node) {
                    return strcmp(node.attribute(attr_name.toStdString().data()).value(), attr_val.toStdString().data()) == 0;
                }
    );
}
pugi::xml_node findNodeByName(pugi::xml_node root, QString node_name)
{
    return root.find_node(
                [&node_name](pugi::xml_node& node) {
                    return strcmp(node.name(), node_name.toStdString().data()) == 0;
                }
    );
}

Elesy_XML::Elesy_XML(const Elesy_XML & copy)
    :_plc(copy._plc),  _regul_bus_os(copy._regul_bus_os), _iec104s_os(copy._iec104s_os), _usoName(copy._usoName), triggers(copy.triggers), slaves_104(copy.slaves_104), slaves_mbs(copy.slaves_mbs)
{
    // Все переопределение конструктора копирования ради вот этой строчки. Т.к. pugi запрещает копировать объекты
     this->app_doc.reset(copy.app_doc);
}

 Elesy_XML::Elesy_XML(std::pair<QString, QString> cfg_path, uint plc, std::tuple<bool, bool> os_flags)
    : _plc(plc)
{
   // this->app_doc = new pugi::xml_document;

     _regul_bus_os = std::get<0>(os_flags);
     _iec104s_os = std::get<1>(os_flags);    

    _usoName = QFileInfo(cfg_path.second).fileName().replace("_application.xml","");
    load_crate(cfg_path.first);
    openFileXml(this->app_doc,cfg_path.second);
    parseTriggers();
}

void Elesy_XML::load_crate(QString filename)
{
    if(filename.length()==0)
        return;

    //Открываем крейт
    pugi::xml_document doc_crate;
    openFileXml(doc_crate, filename);

    // Ищем узел SoftModules, в котором содержатся модули   
    pugi::xml_node soft_modules_node = findNodeByAttribute(doc_crate.root(),"name","SoftModules");
    if (!soft_modules_node)
    {
        QString msg = QString("Ошибка при чтении %1\nНе найден узел SoftModules").arg(filename);
        throw std::runtime_error(msg.toStdString());
    }

    pugi::xml_node configurations_node = findNodeByName(soft_modules_node,"configurations");
    if (!configurations_node)
        return;

    // Для каждого узла configuration внутри configurations
    for(const pugi::xml_node & module : configurations_node.children("configuration"))
    {
        pugi::xml_node module_id_node = findNodeByAttribute(module,"visiblename","ChName");
        QString module_id = module_id_node.first_child().value();

        if(module_id.toLower() == "'iec104s'")
           slaves_104.push_back(module);
        if(module_id.toLower() == "'mbtcps'")
           slaves_mbs.push_back(module);
    }
}

void Elesy_XML::parseTriggers()
{
    pugi::xpath_node  xnode = this->app_doc.select_node("/project/types/pous");
    if(!xnode)
        return;

    //Для каждого pou
    for(pugi::xml_node & p_node : xnode.node().children("pou"))
    {
        pugi::xml_node body_node = p_node.child("body").child("ST").child("xhtml");
        if(!body_node )
            continue;

        //Забрали текст из секции ST
        QByteArray st_data = body_node.first_child().value();
        QTextStream in_stream(&st_data, QIODevice::ReadOnly);

        // Проходимся по каждой строчке
        while (!in_stream.atEnd())
        {
            QString line = in_stream.readLine();
            //заполняем массив триггеров
            if(line.contains("_trigger_"))
            {
                QStringList tr_list = line.split(QRegExp("[( |,|\))]"));
                for(auto & tr : tr_list)
                    if(tr.contains("_trigger_"))
                        this->triggers.push_back(tr);
            }
        }
    }
}

void Elesy_XML::addMBS_GVL_United()
{
    int count  = 0;
    pugi::xml_node g_vars = this->app_doc.child("project").child("addData");
    if(!g_vars)
        return;

    for(mbsTCPSlave & mbs_slave : slaves_mbs)
    {
        if(!mbs_slave.checked)
            continue;

        QString gvl_name = "MBS_GVL";
        if(++count > 1)
            gvl_name = QString("MBS_GVL_%1").arg(count);

        // Загружамем xml узлы из файлов ресурсов
        pugi::xml_document mbs_doc;
        load_gvl_from_template(mbs_doc, gvl_name, {std::make_pair("subsequent",""), std::make_pair("pack_mode","1")});

        pugi::xml_node mbs_node = findNodeByAttribute(mbs_doc.root(), "name", gvl_name);

        // Создаем переменную  Slave_Dev : ARRAY [0 .. getLastByteAddress()] OF WORD
         pugi::xml_node var_node = mbs_node.insert_child_before("variable",mbs_node.child("documentation"));
         var_node.append_attribute("name").set_value("Slave_Dev");
         pugi::xml_node array_node = var_node.append_child("type").append_child("array");
         pugi::xml_node dim_node = array_node.append_child("dimension");
         dim_node.append_attribute("lower").set_value("0");
         dim_node.append_attribute("upper").set_value(mbs_slave.getLastByteAddress());
         array_node.append_child("baseType").append_child("WORD");

        for(modbus_tcp_channel & ch : mbs_slave.channels)
        {
            QString simp_val = QString("(ADR(Slave_Dev) + (%1* SIZEOF(WORD)))").arg(ch.Offset);

            pugi::xml_node var_node = mbs_node.insert_child_before("variable",mbs_node.child("documentation"));
            var_node.append_attribute("name").set_value(ch.VarName.toStdString().data());
            var_node.append_child("type").append_child("pointer").append_child("baseType").append_child(ch.getType().toStdString().data());
            var_node.append_child("initialValue").append_child("simpleValue").append_attribute("value").set_value(simp_val.toStdString().data());
            pugi::xml_node xhtml_node = var_node.append_child("documentation").append_child("xhtml");
            xhtml_node.append_attribute("xmlns").set_value("http://www.w3.org/1999/xhtml");
            xhtml_node.append_child(pugi::xml_node_type::node_pcdata);
            xhtml_node.first_child().set_value(ch.Descr.toStdString().data());
        }

        g_vars.append_copy(mbs_doc.first_child());
        g_vars.append_copy(mbs_doc.first_child().next_sibling());

    }
}

void Elesy_XML::addMBS_GVL()
{
     int count  = 0;
     pugi::xml_node g_vars = this->app_doc.child("project").child("addData");
     if(!g_vars)
         return;

    for(mbsTCPSlave & mbs_slave : slaves_mbs)
    {
        if(!mbs_slave.checked)
            continue;

        QString gvl_name = "MBS_GVL";
        if(++count > 1)
            gvl_name = QString("MBS_GVL_%1").arg(count);

        // Загружамем xml узлы из файлов ресурсов
        pugi::xml_document mbs_doc;
        load_gvl_from_template(mbs_doc, gvl_name, {std::make_pair("subsequent","")});

        pugi::xml_node mbs_node = findNodeByAttribute(mbs_doc.root(), "name", gvl_name);

        for(modbus_tcp_channel & ch : mbs_slave.channels)
        {
            pugi::xml_node var_node = mbs_node.insert_child_before("variable",mbs_node.child("documentation"));
            var_node.append_attribute("name").set_value(ch.VarName.toStdString().data());
            var_node.append_child("type").append_child(ch.getType().toStdString().data());
        }

        g_vars.append_copy(mbs_doc.first_child());
        g_vars.append_copy(mbs_doc.first_child().next_sibling());
    }
}

void Elesy_XML::replaceModbusVarsToPointers()
{
    pugi::xpath_node  xnode = this->app_doc.select_node("/project/types/pous");
    if(!xnode)
        return;

    for(pugi::xml_node & p_node : xnode.node().children("pou"))
    {

        QString name = p_node.attribute("name").value();
        if(name.contains("_Modbus") == false)
            continue;

        pugi::xml_node body_node =p_node.child("body").child("ST").child("xhtml") ;
        if(!body_node)
            continue;

        QString all_data = body_node.first_child().value();

        for(mbsTCPSlave & mbs_slave : slaves_mbs)
        {
            if(!mbs_slave.checked)
                continue;

            for(modbus_tcp_channel & ch : mbs_slave.channels)
            {
                all_data.replace( ch.VarName + " ",  ch.VarName + "^ ");
                all_data.replace( ch.VarName + ".",  ch.VarName + "^.");
                all_data.replace( ch.VarName + ";",  ch.VarName + "^;");
            }
        }

        body_node.first_child().set_value(all_data.toStdString().data());
    }
}

void Elesy_XML::modifyApplication()
{
    // Делаем обязательные текстовые замены
    standartTextReplaces();

    // dInitFlag retain->false
    set_dInitFlag(false);

    // Создаем глобальные переменные из массива найденных триггеров
    addTrigers();

    // Преобразовываем глобальные retain_vars в PersistentVars
    changeRatainToPersistent();

     // Вырезаем из секций FB все переменные variable settings, и вставляем их в конец field секции
    cutVarsFromFB_toField();

    // Добавляем глобальные переменные для резервирования
    switch(getRedundancyType())
    {
        case 0 :  add_Redundancy(); break;
        case 1 :  add_Redundancy_OS(); break;
        case 2 :  add_Redundancy_RT(); break;
        case 3 :  break;
    }   

    // Если есть каналы Modbus, то создаем MBS_GVL
    if(this->slaves_mbs.length() != 0)
    {
        if(this->slaves_mbs.at(0).united_memory)    // используем указатели на единую обл. памяти
        {
            addMBS_GVL_United();
            replaceModbusVarsToPointers();
        }
        else
            addMBS_GVL();
    }
}


void Elesy_XML::standartTextReplaces()
{
    QByteArray fb_vars;
    pugi::xpath_node  xnode = this->app_doc.select_node("/project/types/pous");
    if(!xnode)
        return;

    //Для каждого pou
    for(pugi::xml_node & p_node : xnode.node().children("pou"))
    {
        pugi::xml_node body_node = p_node.child("body").child("ST").child("xhtml");
        if(!body_node)
            continue;

        //Модификация текста
        QByteArray st_data = body_node.first_child().value();
        QByteArray new_data;
        QTextStream in_stream(&st_data, QIODevice::ReadOnly);
        QTextStream out_stream(&new_data, QIODevice::ReadWrite);
        while (!in_stream.atEnd())
        {
            QString line = in_stream.readLine();

             // Удаляем строчки типа A13_R500_DO_32.control := 2;
            if(line.contains(".control :="))    continue;

            // Замены
            line.replace(".Value__", ".");
            line.replace(" := _imit_;", ";");

            // Обращение к модулям
            if(this->_regul_bus_os && this->_plc == enums::plc::r500)
            {
                line.replace("_R500_DI_32.value", "_Discrets");
                line.replace("_R500_DO_32.value", "_Discrets");
            }
            else if(this->_plc == enums::plc::r200)
            {
                line.replace("_R500_DI_32.value", ".Inputs_v2^.Discrets");
                line.replace("_R500_DO_32.value", ".Outputs_v2^.Discrets");
            }
            else if(this->_plc == enums::plc::r500)
            {
                line.replace("_R500_DI_32.value", ".Inputs_v1^.Discrets");
                line.replace("_R500_DO_32.value", "_Discrets");
            }

            // Сохраняем преобразованную строчку во всех остальных случаях
            out_stream << line << "\n";
        }
        in_stream.flush();
        out_stream.flush();
        body_node.first_child().set_value(new_data);
    }
}

void Elesy_XML::set_dInitFlag(bool val)
{
    pugi::xpath_node  xnode = this->app_doc.select_node("/project/types/pous");
    if(!xnode)
        return;

    //Для каждого pou
    for(pugi::xml_node & p_node : xnode.node().children("pou"))
    {
        pugi::xml_node interface_node = p_node.child("interface");
        if(!interface_node)
            continue;

        // Модификация переменных
        for(pugi::xml_node & locals : interface_node.children("localVars"))
            for(pugi::xml_node & var : locals.children("variable"))
            {
                // dInit флаг снимаем retain
                QString name = var.attribute("name").value();
                if(name == "dInitFlag")
                    locals.attribute("retain").set_value(val ? "true" : "false");
            }
    }
}

void Elesy_XML::addTrigers()
{
    // Загружамем xml узлы из файлов ресурсов
    pugi::xml_document trig_doc;
    load_gvl_from_template(trig_doc, "triggers", {std::make_pair("subsequent","")});

    pugi::xml_node tr_node = findNodeByAttribute(trig_doc.root(), "name", "triggers");

    pugi::xml_node g_vars = this->app_doc.child("project").child("addData");
    if(!g_vars)
        return;

    for(QString& one_trig : triggers)
    {
        pugi::xml_node var_node = tr_node.insert_child_before("variable",tr_node.child("documentation"));
        var_node.append_attribute("name").set_value(one_trig.toStdString().c_str());
        var_node.append_child("type").append_child("BOOL");
        var_node.append_child("initialValue").append_child("simpleValue");
        var_node.append_child("documentation").append_child("xhtml").append_attribute("xmlns").set_value("http://www.w3.org/1999/xhtml");
    }

    g_vars.append_copy(trig_doc.first_child());
    g_vars.append_copy(trig_doc.first_child().next_sibling());
}

void Elesy_XML::changeRatainToPersistent()
{
    pugi::xml_node g_vars = this->app_doc.child("project").child("addData");
    if(!g_vars)
        return;

    for(pugi::xml_node & data_var : g_vars.children("data"))
        for(pugi::xml_node & g_var : data_var.children("globalVars"))
        {
            QString name = g_var.attribute("name").value();
            if(name == "retain_vars")
            {
                g_var.append_attribute("persistent").set_value("true");
                g_var.attribute("name").set_value("PersistentVars");
            }
        }
}

void Elesy_XML::cutVarsFromFB_toField()
{
    pugi::xpath_node  xnode = this->app_doc.select_node("/project/types/pous");
    if(!xnode)
        return;

    pugi::xml_node field_node;

    //Ищем _field блок
    for(pugi::xml_node & p_node : xnode.node().children("pou"))
    {
        QString name = p_node.attribute("name").value();
        if(name.endsWith("_field"))
        {
            field_node = p_node.child("body").child("ST").child("xhtml");
            if(!field_node)
                return;
            break;
        }
    }

    if(!field_node)
        return;

    QByteArray fb_vars;
    // Проходимся по всем и останавливаемся только на FB
    for(pugi::xml_node & p_node : xnode.node().children("pou"))
    {
        QString name = p_node.attribute("name").value();
        if(!name.contains("_FB"))
            continue;

        pugi::xml_node body_node = p_node.child("body").child("ST").child("xhtml");
        if(!body_node)
            continue;

        // Вырезали переменные
        fb_vars     = getFBVars(&body_node);

        // Переложили в конец field
        QByteArray field_data = field_node.first_child().value();
        field_data.append('\n');
        field_data.append(fb_vars);
        field_node.first_child().set_value(field_data);
    }
}

QByteArray Elesy_XML::getFBVars(pugi::xml_node * fb)
{
    QByteArray st_data = fb->first_child().value();         // Считываем данные с FB секции
    QTextStream st_stream(&st_data, QIODevice::ReadOnly);
    QByteArray new_data;                                    // Копируем нужные строчки сюда
    QTextStream new_stream(&new_data, QIODevice::ReadWrite);

    // Сюда кладем список переменных, которые нужно вырезать
    bool var_flag = false;
    QByteArray params;
    QTextStream params_stream(&params, QIODevice::ReadWrite);

    // Проходимся по секции FB
    while (!st_stream.atEnd())
    {
         QString line = st_stream.readLine();

         if(line.contains("(* variables setting *)"))
             var_flag = true;
         else if(line.contains("(* variables transitions *)"))
             var_flag = false;

         if(var_flag == true)
         {
             params_stream << line << "\n";
             continue;
         }
         new_stream << line << "\n";
    }
    st_stream.flush();
    new_stream.flush();
    params_stream.flush();
    fb->first_child().set_value(new_data);              // Перезаписываем секцию FB уже без данных variable settings

    return params;
}

void Elesy_XML::add_StartEndVariable(pugi::xml_document & doc, bool start)
{
    QString name = "StartAddress";
    QString description = "Начальный адрес резервируемой памяти";
    if(!start)
    {
        name = "EndAddress";
        description = "Конеченый адрес резервируемой памяти";
    }

    pugi::xml_node gVar_node =  findNodeByName(doc,"globalVars");
    pugi::xml_node var = gVar_node.insert_child_before("variable", gVar_node.child("documentation"));
    var.append_attribute("name").set_value(name.toStdString().data());
    var.append_child("type").append_child("BYTE");
    var.append_child("initialValue").append_child("simpleValue").append_attribute("value").set_value("0");
    pugi::xml_node xhtml_node = var.append_child("documentation").append_child("xhtml");
    xhtml_node.append_attribute("xmlns").set_value("http://www.w3.org/1999/xhtml");
    xhtml_node.append_child(pugi::xml_node_type::node_pcdata);
    xhtml_node.first_child().set_value(description.toStdString().data());
}

int Elesy_XML::getRedundancyType()
{
    pugi::xpath_node  xnode = this->app_doc.select_node("/project/types/pous");
    if(!xnode)
        return 3;

    //Для каждого pou
    for(pugi::xml_node & p_node : xnode.node().children("pou"))
    {
        pugi::xml_node interface_node = p_node.child("interface");
        if(!interface_node)
            continue;


        for(pugi::xml_node & locals : interface_node.children("localVars"))
            for(pugi::xml_node & var : locals.children("variable"))
            {
                QString name = var.attribute("name").value();
                if(name.contains("M_REGUL"))
                {
                    QString derived_name = var.child("type").child("derived").attribute("name").value();
                    if(derived_name == "M_REGUL")
                        return 0;
                    if(derived_name == "M_REGULOS")
                        return 1;
                    if(derived_name == "M_REGULRT")
                        return 2;
                }
            }
    }
    return 3;       // no redundancy
}

void Elesy_XML::addRedundancyFB_toSharedGVL(pugi::xml_node var)
{
    QString template_str = "<variable name=\"shared_data\">"
          "<type>"
            "<derived name=\"PSRedundancy.SharedMemory\" />"
          "</type>"
          "<addData>"
            "<data name=\"http://www.3s-software.com/plcopenxml/inputassignments\" handleUnknown=\"implementation\">"
              "<InputAssignments>"
                "<InputAssignment>"
                  "<Value>((ADR(EndAddress) - ADR(StartAddress)) - SIZEOF(StartAddress))</Value>"
                "</InputAssignment>"
                "<InputAssignment>"
                  "<Value>(ADR(StartAddress) + SIZEOF(StartAddress))</Value>"
                "</InputAssignment>"
              "</InputAssignments>"
            "</data>"
            "<data name=\"http://www.3s-software.com/plcopenxml/attributes\" handleUnknown=\"implementation\">"             
            "</data>"
          "</addData>"
        "</variable>";
    pugi::xml_document d;
    d.load_string(template_str.toStdString().c_str());
    var.append_copy(d.child("variable"));
    //var.parent().insert_copy_after(d.child("variable"), var);
}

void Elesy_XML::addRedundancyFB_toCrossedGVL(pugi::xml_node cr_node, QVector<QString> v)
{
    // var index = 1. no_rve = 2. rve  =3

    QString template_str = "<variable name=\"cross_data_%1\">"
            "<type>"
              "<derived name=\"PsRedundancy.CrossMemory\" />"
            "</type>"
            "<addData>"
              "<data name=\"http://www.3s-software.com/plcopenxml/inputassignments\" handleUnknown=\"implementation\">"
                "<InputAssignments>"
                  "<InputAssignment>"
                    "<Value>SIZEOF(%2)</Value>"
                  "</InputAssignment>"
                  "<InputAssignment>"
                    "<Value>ADR(%2)</Value>"
                  "</InputAssignment>"
                  "<InputAssignment>"
                    "<Value>ADR(%3)</Value>"
                  "</InputAssignment>"
                "</InputAssignments>"
              "</data>"
              "<data name=\"http://www.3s-software.com/plcopenxml/attributes\" handleUnknown=\"implementation\">"               
              "</data>"
            "</addData>"
          "</variable>";

    int cnt = 0;
    for(int i  = 0; i != v.size(); ++i)
    {
       QString var_name = v.at(i);
       if(!var_name.endsWith("_RVE"))
           continue;
       cnt++;
       var_name.replace("_RVE","");
       QString tmp =  template_str.arg(cnt).arg(var_name).arg(var_name + "_RVE");
       pugi::xml_document d;
       d.load_string(tmp.toStdString().c_str());
       //cr_node.insert_copy_before(d.child("variable"),cr_node.child("documentation"));
       cr_node.append_copy(d.child("variable"));
    }
}

void Elesy_XML::add_Redundancy()
{
    // Загружамем xml узлы из файлов ресурсов
    pugi::xml_document shared_GVL;
    load_gvl_from_template(shared_GVL, "shared_GVL", {std::make_pair("subsequent","")});

    // Добавляем переменные StartAdress и EndAdress
    add_StartEndVariable(shared_GVL, true);
    add_StartEndVariable(shared_GVL, false);

    pugi::xml_node sh_node =  findNodeByAttribute(shared_GVL.root(), "name", "EndAddress");
    pugi::xml_node g_vars = this->app_doc.child("project").child("addData");
    if(!g_vars)
        return;

    QVector<QString> delete_thease_node_names;                                    // Имена узлов для удаления

    // Проходимся и копируем узлы
    for(pugi::xml_node & data_var : g_vars.children("data"))
        for(pugi::xml_node & g_var : data_var.children("globalVars"))
            for(pugi::xml_node & var : g_var.children("variable"))
            {
                QString name = var.attribute("name").value();
                if(name.endsWith("_BROADCAST") || name.endsWith("_SHARED"))
                {
                    delete_thease_node_names.push_back(name);
                    sh_node.parent().insert_copy_before(var, sh_node);
                }
            }

    if(delete_thease_node_names.size() == 0)
        return;

    // Теперь удаляем скопированные узлы
    for(QString & name_attr : delete_thease_node_names)
    {
        pugi::xml_node del_node =  findNodeByAttribute(g_vars, "name", name_attr);
        if(del_node)    del_node.parent().remove_child(del_node);
    }

    // Копируем узлы из shared_GVL в Application.xml
    g_vars.append_copy(shared_GVL.first_child());
    g_vars.append_copy(shared_GVL.first_child().next_sibling());
}

void Elesy_XML::add_Redundancy_OS()
{
/*Добавляем глобальные переменные*/

    // Загружамем xml узлы из файлов ресурсов
    pugi::xml_document shared_GVL;
    pugi::xml_document crossed_GVL;
    load_gvl_from_template(shared_GVL, "shared_GVL", {std::make_pair("subsequent",""), std::make_pair("ps.add_redundancy","")});
    load_gvl_from_template(crossed_GVL, "crossed_GVL", {std::make_pair("subsequent","")});

    // Добавляем переменные StartAdress и EndAdress
    add_StartEndVariable(shared_GVL, true);
    add_StartEndVariable(shared_GVL, false);

    pugi::xml_node sh_node =  findNodeByAttribute(shared_GVL.root(), "name", "EndAddress");
    pugi::xml_node cr_node = findNodeByAttribute(crossed_GVL.root(), "name", "crossed_GVL");

    pugi::xml_node g_vars = this->app_doc.child("project").child("addData");
    if(!g_vars)
        return;

    QVector<QString> delete_thease_node_names;                                    // Имена узлов для удаления

    // Проходимся и копируем узлы
    for(pugi::xml_node & data_var : g_vars.children("data"))
        for(pugi::xml_node & g_var : data_var.children("globalVars"))
            for(pugi::xml_node & var : g_var.children("variable"))
            {
                QString name = var.attribute("name").value();
                if(name.endsWith("_SHARED"))
                {
                    delete_thease_node_names.push_back(name);
                    sh_node.parent().insert_copy_before(var, sh_node);
                }
                if(name.endsWith("_RVE"))
                {
                     cr_node.insert_copy_before(var,cr_node.child("documentation"));
                     delete_thease_node_names.push_back(name);
                     delete_thease_node_names.push_back(name.replace("_RVE",""));
                     pugi::xml_node _rve =  findNodeByAttribute(g_var, "name", name.replace("_RVE",""));
                     if(! _rve)
                         continue;
                     add_redundancy_pou(&_rve);
                     cr_node.insert_copy_before(_rve,cr_node.child("documentation"));
                }
            }

    if(delete_thease_node_names.size() == 0)
        return;

    // Теперь удаляем скопированные узлы
    for(QString & name_attr : delete_thease_node_names)
    {
        pugi::xml_node del_node =  findNodeByAttribute(g_vars, "name", name_attr);
        if(del_node)    del_node.parent().remove_child(del_node);
    }

    // Копируем узлы из shared_GVL в Application.xml
    g_vars.append_copy(shared_GVL.first_child());
    g_vars.append_copy(shared_GVL.first_child().next_sibling());

    // Копируем узлы из crossed_GVL в Application.xml
    g_vars.append_copy(crossed_GVL.first_child());
    g_vars.append_copy(crossed_GVL.first_child().next_sibling());


/*Добавляем атрибут ps.add_redundancy переменным pou*/

    pugi::xpath_node  xnode = this->app_doc.select_node("/project/types/pous");
    if(!xnode)
        return;

    //Для каждого pou
    for(pugi::xml_node & p_node : xnode.node().children("pou"))
    {
        pugi::xml_node interface_node = p_node.child("interface");
        if(!interface_node)
            continue;

        // Модификация переменных
        for(pugi::xml_node & locals : interface_node.children("localVars"))
            for(pugi::xml_node & var : locals.children("variable"))
            {
                // Добавляем атрибут редунданси CROSS переменным
                QString name = var.attribute("name").value();
                if(name.endsWith("_CROSS"))
                    add_redundancy_pou(&var);
            }
    }
}

void Elesy_XML::add_Redundancy_RT()
{
/*Добавляем глобальные переменные*/

    // Загружамем xml узлы из файлов ресурсов
    pugi::xml_document shared_GVL;
    pugi::xml_document crossed_GVL;
    load_gvl_from_template(shared_GVL, "shared_GVL", {std::make_pair("subsequent","")});
    load_gvl_from_template(crossed_GVL, "crossed_GVL", {std::make_pair("subsequent","")});

    // Добавляем переменные StartAdress и EndAdress
    add_StartEndVariable(shared_GVL, true);
    add_StartEndVariable(shared_GVL, false);

    pugi::xml_node sh_node =  findNodeByAttribute(shared_GVL.root(), "name", "EndAddress");
    pugi::xml_node cr_node = findNodeByAttribute(crossed_GVL.root(), "name", "crossed_GVL");

    pugi::xml_node g_vars = this->app_doc.child("project").child("addData");
    if(!g_vars)
        return;

    // В shared после EndAddress добавляем shared_data : PSRedundancy.SharedMemory(ADR(EndAddress) - ADR(StartAddress) - SIZEOF(StartAddress), ADR(StartAddress)+SIZEOF(StartAddress));
    addRedundancyFB_toSharedGVL(sh_node.parent());

    QVector<QString> delete_thease_node_names;                                    // Имена узлов для удаления

    // Проходимся и копируем узлы
    for(pugi::xml_node & data_var : g_vars.children("data"))
        for(pugi::xml_node & g_var : data_var.children("globalVars"))
            for(pugi::xml_node & var : g_var.children("variable"))
            {
                QString name = var.attribute("name").value();
                if(name.endsWith("_SHARED"))
                {
                    delete_thease_node_names.push_back(name);
                    sh_node.parent().insert_copy_before(var, sh_node);
                }
                if(name.endsWith("_RVE"))
                {
                     cr_node.insert_copy_before(var,cr_node.child("documentation"));
                     delete_thease_node_names.push_back(name);
                     delete_thease_node_names.push_back(name.replace("_RVE",""));
                     pugi::xml_node _rve =  findNodeByAttribute(g_var, "name", name.replace("_RVE",""));
                     if(! _rve)
                         continue;
                     cr_node.insert_copy_before(_rve,cr_node.child("documentation"));
                }
            }

    if(delete_thease_node_names.size() == 0)
        return;

    // Добавляем в crossed.gvl строчки для RVE переменных типа cross_data_N: PsRedundancy.CrossMemory(SIZEOF(COMP2_DI), ADR(COMP2_DI), ADR(COMP2_DI_RVE));
    addRedundancyFB_toCrossedGVL(cr_node, delete_thease_node_names);

    // Теперь удаляем скопированные узлы
    for(QString & name_attr : delete_thease_node_names)
    {
        pugi::xml_node del_node =  findNodeByAttribute(g_vars, "name", name_attr);
        if(del_node)    del_node.parent().remove_child(del_node);
    }

    // Копируем узлы из shared_GVL в Application.xml
    g_vars.append_copy(shared_GVL.first_child());
    g_vars.append_copy(shared_GVL.first_child().next_sibling());

    // Копируем узлы из crossed_GVL в Application.xml
    g_vars.append_copy(crossed_GVL.first_child());
    g_vars.append_copy(crossed_GVL.first_child().next_sibling());


    /*Добавляем PsRedundancy.CrossMemory переменным pou*/

    pugi::xpath_node  xnode = this->app_doc.select_node("/project/types/pous");
    if(!xnode)
        return;

    //Для этого заполним массив _RVE переменных
    QVector<QString> _rve_vars;

    //Для каждого pou
    for(pugi::xml_node & p_node : xnode.node().children("pou"))
    {
        pugi::xml_node interface_node = p_node.child("interface");
        if(!interface_node)
            continue;
        for(pugi::xml_node & locals : interface_node.children("localVars"))
        {
            for(pugi::xml_node & var : locals.children("variable"))
            {
                // Добавляем атрибут редунданси CROSS переменным
                QString name = var.attribute("name").value();
                if(name.endsWith("_RVE"))
                    _rve_vars.push_back(name);
            }
            addRedundancyFB_toCrossedGVL(locals, _rve_vars);
            _rve_vars.clear();
        }
    }

}

void Elesy_XML::add_redundancy_pou(pugi::xml_node * n)
{
    /*pugi::xml_node data_node = n->append_child("addData").append_child("data");
    data_node.append_attribute("name").set_value("http://www.3s-software.com/plcopenxml/attributes");
    data_node.append_attribute("handleUnknown").set_value("implementation");
    pugi::xml_node attr_node =  data_node.append_child("Attributes").append_child("Attribute");
    attr_node.append_attribute("Name").set_value("ps.add_redundancy");
    attr_node.append_attribute("Value").set_value("");*/

    pugi::xml_node xhtml_node = n->append_child("documentation").append_child("xhtml");
    xhtml_node.append_attribute("xmlns").set_value("http://www.w3.org/1999/xhtml");
    xhtml_node.text().set("{attribute 'ps.add_redundancy' := ''}");
};

void Elesy_XML::save(QString & folder)
{
    // Создаем папку с названием УСО (крейта)
    QDir dir(folder + "\\" + this->_usoName);
    if (!dir.exists())  dir.mkpath(".");

    // Сохраняем Application
    this->app_doc.save_file(dir.path().append("/REGUL_Application.xml").toStdWString().data(),"\t", pugi::format_indent | pugi::format_indent_attributes, pugi::encoding_utf8);

    // Сохраняем IEC104 каналы
    int edc_count = 0;
    int st_count = 0;
    QString filename_arg = "";
    for(iec104Slave & iec104 : this->slaves_104)
    {
        if(!iec104.checked)
            continue;

        if(iec104.module_name.contains("EDC",Qt::CaseInsensitive))
        {
            edc_count++;
            filename_arg = edc_count <= 1 ? "EDC" : QString("EDC_%1").arg(edc_count);
        }
        else
        {
            st_count++;
            filename_arg = st_count <= 1 ? "ST" : QString("ST_%1").arg(st_count);
        }

        QFile file_data(dir.path() + "/" + QString("REGUL_IEC104_%1_DATA.iec104data.xml").arg(filename_arg));
        QFile file_cmd(dir.path() + "/" + QString("REGUL_IEC104_%1_CMD.iec104cmd.xml").arg(filename_arg));

        file_data.open(QIODevice::WriteOnly | QIODevice::Text);
        file_cmd.open(QIODevice::WriteOnly | QIODevice::Text);

        QTextStream data_stream(&file_data);
        QTextStream cmd_stream(&file_cmd);
        data_stream.setCodec("UTF-8");
        cmd_stream.setCodec("UTF-8");

        data_stream << "<ListOfItem>" << "\n";
        cmd_stream << "<ListOfItem>" << "\n";


        for(iec104_channel_cmd & cmd : iec104.cmd_channels)
            cmd_stream << "  " << cmd.toString() << "\n";

        for(iec104_channel_data & data : iec104.data_channels)
             data_stream << "  " << data.toString() << "\n";

        data_stream << "</ListOfItem>";
        cmd_stream << "</ListOfItem>";

        file_data.close();
        file_cmd.close();
    }

    // Сохраняем Modbus tcp каналы
    for(mbsTCPSlave & mbs_tcp : this->slaves_mbs)
    {
        if(!mbs_tcp.checked)
            continue;

        QFile file_data(dir.path() + "/" + QString("REGUL_XML.mb_direct_channels.xml"));
        file_data.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream data_stream(&file_data);
        data_stream.setCodec("UTF-8");

        data_stream << "<ModbusSlaveDirectChannels>" << "\n";

        if(mbs_tcp.united_memory == false)
        {
            for(modbus_tcp_channel & data : mbs_tcp.channels)
                 data_stream << "  " << data.toString() << "\n";
        }
        else
        {
            QString data_united = QString("  <DirectSlaveCh Name=\"Data area 1\" Descr=\"United data area\" Type=\"HoldingRegisters\" Offset=\"0\" Length=\"%1\" VarName=\"MBS_GVL.Slave_Dev\" />").arg(mbs_tcp.getLastByteAddress() + 1);
            data_stream << "  " << data_united  << "\n";
        }
        data_stream << "</ModbusSlaveDirectChannels>";
        file_data.close();
    }
}


void modifyApplications(QVector<Elesy_XML>& xmls)
{
    for(Elesy_XML & xml : xmls)
    {
        xml.modifyApplication();
    }
}

void saveModified(QVector<Elesy_XML>& xmls, QString folder)
{
    //Для каждого крейта
    for(Elesy_XML & xml : xmls)
    {
       xml.save(folder);
    }
}
