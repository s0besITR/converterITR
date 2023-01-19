#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include <QDialog>
#include "builddatetime.h"

namespace Ui {
class aboutwindow;
}

class aboutwindow : public QDialog
{
    Q_OBJECT

public:
    explicit aboutwindow(QWidget *parent = nullptr);
    ~aboutwindow();

private slots:
    void on_pushButton_Ok_clicked();

private:
    Ui::aboutwindow *ui;
};

#endif // ABOUTWINDOW_H
