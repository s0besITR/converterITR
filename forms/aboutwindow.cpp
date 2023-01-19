#include "aboutwindow.h"
#include "ui_aboutwindow.h"

aboutwindow::aboutwindow(QWidget *parent) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new Ui::aboutwindow)
{
    ui->setupUi(this);

    ui->icon->setPixmap(QPixmap(":/icon.ico").scaled(80,80));

    QString content = "<b>Продукт: </b>ИТР Конфигуратор<br>"
                      "<b>Разработал: </b>Собецкий Александр<br>"
                      "<b>Дата сборки: </b>" + BUILDV + "<br><br>"
                      "(c) 2023 ООО ИТР<br>"
                      "<a href=\"mailto:sobetsky.alexander@gmail.com\">sobetsky.alexander@gmail.com</a>";

    ui->label_build_dt->setText(content);
}

aboutwindow::~aboutwindow()
{
    delete ui;
}

void aboutwindow::on_pushButton_Ok_clicked()
{
    this->close();
}

