#ifndef CAL_WIZARD_H
#define CAL_WIZARD_H

#include <QDialog>
#include "typedefs.h"
#include "mainwindow.h"

namespace Ui {
class Cal_Wizard;
}

class Cal_Wizard : public QDialog
{
    Q_OBJECT

public:
    explicit Cal_Wizard(MainWindow *parent = nullptr);
    ~Cal_Wizard();
    void reset();

public slots:
    void on_Next_pushButton_clicked();

private slots:
    void on_prev_pushButton_clicked();

private:
    Ui::Cal_Wizard *ui;
    void Wizard();
};

#endif // CAL_WIZARD_H
