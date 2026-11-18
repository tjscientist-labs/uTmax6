#ifndef OPTIONS_DIALOG_H
#define OPTIONS_DIALOG_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class options_dialog;
}

class options_dialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit options_dialog(MainWindow *parent = 0);
    ~options_dialog();
signals:
    void TSLSortChanged();

private slots:
    void on_buttonBox_accepted();
    void on_IaRangeSel_currentIndexChanged(int index);
    void on_IsRangeSel_currentIndexChanged(int index);
    void on_AvgSel_currentIndexChanged(int index);
    void on_IlimitSel_currentIndexChanged(int index);
    void on_Delay_editingFinished();
    void on_AbortOnLimit_clicked(bool checked);
    void on_VgScaleText_returnPressed();
    void on_VgScaleText_editingFinished();
    void on_VaScaleText_editingFinished();
    void on_VaScaleText_returnPressed();
    void on_pushButton_Style_clicked();
    void on_pushButton_update_clicked();
    void on_checkBox_uTracer6_stateChanged(int arg1);
    void on_comboBox_TSL_currentIndexChanged(int index);

    void on_checkBox_ExtHtr_stateChanged(int arg1);

private:
    Ui::options_dialog *ui;
    MainWindow * mw;
    void setIlimit();
    float Ilist[10]= {1, 0.875, 0.75, 0.625, 0.5, 0.25, 0.125, 0.06, 0.035, 0.0};
};

#endif // OPTIONS_DIALOG_H
