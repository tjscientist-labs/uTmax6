#include "options_dialog.h"
#include "ui_options_dialog.h"
#include <qmessagebox.h>
#include <QDebug>


options_dialog::options_dialog(MainWindow *parent) :
    QDialog(parent),
    ui(new Ui::options_dialog)
{
    char  buf[16];
    mw = parent;
    int gains[8]={1,2,5,10,20,50,100,200};
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    //initalize widgets
    if (mw->calData.heaterscale==0)
        ui->checkBox_ExtHtr->hide();
    else
        ui->checkBox_ExtHtr->show();
    ui->checkBox_ExtHtr->setChecked(mw->options.ExtHtr);
    ui->comboBox_TSL->setCurrentIndex(mw->options.TLSortOrder);
    ui->checkBox_uTracer6->setChecked(mw->options.uTracer6);
    ui->AvgSel->setCurrentIndex(mw->options.AvgNum);
    ui->IlimitSel->setCurrentIndex(mw->options.Ilimit);
    sprintf(buf,"%d",mw->options.Delay);
    ui->Delay->setText(buf) ;
    ui->AbortOnLimit->setChecked(mw->options.AbortOnLimit);
    ui->IsRangeSel->setCurrentIndex(mw->options.IsRange);
    ui->VgScaleText->setText(QString::number(mw->options.VgScale));
    ui->VaScaleText->setText(QString::number(mw->options.VaScale));
    ui->lineEdit_Style->setText(mw->calData.Style);
    setIlimit();
    QString m;
    m.clear();
    QTextStream(&m) << (int)(mw->calData.VaMax) << "V";
    ui->label_TracerV->setText(m);
    m.clear();
    QTextStream(&m) << (int)(3600/(float(mw->calData.IaRsense))) << "mA";
    ui->label_TracerI->setText(m);
    m.clear();
    QTextStream(&m) << float(mw->calData.Firmware)/100;
    if (mw->calData.Firmware<300) {
        ui->pushButton_update->hide();
        if (mw->calData.Firmware>=200) ui->checkBox_uTracer6->hide();
    }
    ui->FWVer->setText(m);
    //set up current ranges drop down text for sense resistor
    float range;
    for (int i=1; i<9; i++) {
        range = 5000/(float(mw->calData.IaRsense));
        range = round(10*range/gains[i-1])/10;
        ui->IaRangeSel->setItemText(i,QString::number(range));
        range = 5000/(float(mw->calData.IsRsense));
        range = round(10*range/gains[i-1])/10;
        ui->IsRangeSel->setItemText(i,QString::number(range));
    }
}

void options_dialog::setIlimit() {
    for (int i=0; i< ui->IlimitSel->count(); i++ ) {
        ui->IlimitSel->setItemText(i, QString::number(round(Ilist[i]*3600/float(mw->calData.IaRsense))) );
        //qDebug() << "ILim text=" <<  Ilist.at(i);
    }
    ui->IsLimit->setText( ui->IlimitSel->currentText()) ;
    ui->IaRangeSel->setCurrentIndex(mw->options.IaRange);
    mw->options.Ilimit = ui->IlimitSel->currentIndex();
}

options_dialog::~options_dialog()
{
    delete ui;
}

void options_dialog::on_buttonBox_accepted()
{
    close();
}

void options_dialog::on_IaRangeSel_currentIndexChanged(int index)
{
    mw->options.IaRange= index;
}

void options_dialog::on_IsRangeSel_currentIndexChanged(int index)
{
     mw->options.IsRange= index;
}

void options_dialog::on_AvgSel_currentIndexChanged(int index)
{
    mw->options.AvgNum = index;
}

void options_dialog::on_IlimitSel_currentIndexChanged(int index)
{
    mw->options.Ilimit= index;
    ui->IsLimit->setText( ui->IlimitSel->currentText() );
}

void options_dialog::on_Delay_editingFinished()
{
    bool ok;
    int a = ui->Delay->text().toInt(&ok,10);
    if (ok) mw->options.Delay = a;
    else QMessageBox::information(NULL, tr("Entry Error"), tr("Invalid entry for delay"));
}


void options_dialog::on_AbortOnLimit_clicked(bool checked)
{
    mw->options.AbortOnLimit = checked;
}



void options_dialog::on_VgScaleText_returnPressed()
{
    bool ok;
    float a = ui->VgScaleText->text().toFloat(&ok);
    if (ok) mw->options.VgScale = a;
    else QMessageBox::information(NULL, tr("Entry Error"), tr("Invalid entry for Vg Scale"));
}

void options_dialog::on_VgScaleText_editingFinished()
{
    on_VgScaleText_returnPressed();
}


void options_dialog::on_VaScaleText_returnPressed()
{
    bool ok;
    float a = ui->VaScaleText->text().toFloat(&ok);
    if (ok) mw->options.VaScale = a;
    else QMessageBox::information(NULL, tr("Entry Error"), tr("Invalid entry for Va Scale"));
}

void options_dialog::on_VaScaleText_editingFinished()
{
    on_VaScaleText_returnPressed();

}

void options_dialog::on_pushButton_Style_clicked()
{
    int i = mw->calData.Style.indexOf('/',-1);
    QString currentDir=  mw->calData.Style.left(i);
    mw->calData.Style = QFileDialog::getOpenFileName(this, tr("Select style file"),currentDir, tr("Style(*.qss)"));
//    mw->calData.Style = QFileDialog::getOpenFileName(this, tr("Select style file"),QCoreApplication::applicationDirPath(), tr("Style(*.qss)"));
    ui->lineEdit_Style->setText(mw->calData.Style);
    mw->SaveCalFile();
    mw->SetStyleSheet();
}


void options_dialog::on_pushButton_update_clicked()
{

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Firmware Update",
        "This will erase and update your firmware and cannot be undone. Do you want to continue?",
        QMessageBox::Yes|QMessageBox::No
    );
    if (reply==QMessageBox::No) return;
    mw->update_FW();
    close();
}


void options_dialog::on_checkBox_uTracer6_stateChanged(int arg1)
{
    mw->calData.VgMax=100;
    mw->options.uTracer6=(bool)arg1;
    mw->Update_adc_scales();
}


void options_dialog::on_comboBox_TSL_currentIndexChanged(int index)
{
    mw->options.TLSortOrder=index; //0= Alpha, 1==Freq, 2==Last used,
    emit TSLSortChanged();
}


void options_dialog::on_checkBox_ExtHtr_stateChanged(int arg1)
{
    mw->options.ExtHtr = (arg1==2);
}

