#include "cal_dialog.h"
#include "ui_cal_dialog.h"
#include <QDebug>
#define RANGE (1.0)
#define STEPS (2000.0)
#define SliderPos(v) (int(round((v)*STEPS/2)))
#define FromSlider(v) float((v)*2/STEPS)

Cal_Dialog::Cal_Dialog(MainWindow *parent) :
    QDialog(parent),
    ui(new Ui::Cal_Dialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    this->setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    ui->IaSli->setMaximum(STEPS/10);
    ui->IaSli->setMinimum(-STEPS/10);
    ui->IsSli ->setMaximum(STEPS/10);
    ui->IsSli->setMinimum(-STEPS/10);
    ui->VaSli ->setMaximum(STEPS/10);
    ui->VaSli ->setMinimum(-STEPS/10);
    ui->VsSli ->setMaximum(STEPS/10);
    ui->VsSli ->setMinimum(-STEPS/10);
    ui->Vg05Sli ->setMinimum(-STEPS/5);
    ui->Vg05Sli ->setMaximum(STEPS/5);
    ui->Vg1Sli ->setMinimum(-STEPS/10);
    ui->Vg1Sli ->setMaximum(STEPS/10);
    ui->Vg4Sli ->setMaximum(STEPS/10);
    ui->Vg4Sli ->setMinimum(-STEPS/10);
    ui->Vg40Sli ->setMaximum(STEPS/10);
    ui->Vg40Sli ->setMinimum(-STEPS/10);
    ui->VsuSli ->setMaximum(STEPS/10);
    ui->VsuSli ->setMinimum(-STEPS/10);
    ui->VnSli ->setMaximum(STEPS/10);
    ui->VnSli ->setMinimum(-STEPS/10);
    // for new grid scheme:
    //hide unused itmes
    ui->Vg4Sli->hide();
    ui->Vg40Sli->hide();
    ui->Vg4Val->hide();
    ui->Vg40Val->hide();
    //change name of items
    ui->label_5->setText("Vg offset");
    ui->label_10->setText("Vg slope");
    ui->label_11->hide();
    ui->label_12->hide();
    mw = parent;
    UpdateDialog(parent);
}

Cal_Dialog::~Cal_Dialog()
{
    delete ui;
}


void Cal_Dialog::on_CancelButton_clicked()
{
    close();
}

void Cal_Dialog::on_SaveButton_clicked()
{
    mw->calData.RaVal= ui->lineEdit_RaVal->text().toFloat();
    mw->calData.VaMax= ui->lineEdit_VaMax->text().toFloat();
    mw->calData.IaRsense= ui->lineEdit_IaRsense->text().toFloat();
    mw->calData.IsRsense= ui->lineEdit_IsRsense->text().toFloat();
    mw->calData.VgMax= ui->lineEdit_VgMax->text().toFloat();
    mw->Update_adc_scales();
    mw->SaveCalFile();
    mw->SaveCalToChip(0);
    close();
}
void Cal_Dialog::UpdateDialog(MainWindow * p)
{
    UpdateSliders(p);
    UpdateLabels(p);
}

void Cal_Dialog::UpdateLabels(MainWindow * p)
{
    ui->VaVal->setText(QString::number(  p->calData.VaVal));
    ui->VsVal->setText(QString::number(  p->calData.VsVal));
    ui->IaVal->setText(QString::number(  p->calData.IaVal));
    ui->IsVal->setText(QString::number(  p->calData.IsVal));
    ui->VsuVal->setText(QString::number( p->calData.VsuVal));
    ui->Vg05Val->setText(QString::number(p->calData.Vg0));
    ui->Vg1Val->setText(QString::number( p->calData.Vgm));
   // ui->Vg05Val->setText(QString::number( p->calData.Vg05Val));
   // ui->Vg1Val->setText(QString::number(  p->calData.Vg1Val ));
   // ui->Vg4Val->setText(QString::number(  p->calData.Vg4Val ));
   // ui->Vg40Val->setText(QString::number( p->calData.Vg40Val));
    ui->VnVal->setText(QString::number( p->calData.VnVal));
    ui->lineEdit_RaVal->setText(QString::number(p->calData.RaVal));
    ui->lineEdit_VaMax->setText(QString::number(p->calData.VaMax));
    ui->lineEdit_VgMax->setText(QString::number(p->calData.VgMax));
    ui->lineEdit_IaRsense->setText(QString::number(p->calData.IaRsense));
    ui->lineEdit_IsRsense->setText(QString::number(p->calData.IsRsense));
}
void Cal_Dialog::UpdateSliders(MainWindow * p)
{
    ui->VaSli->setValue( SliderPos(p->calData.VaVal-1));
    ui->VsSli->setValue( SliderPos(p->calData.VsVal-1));
    ui->IaSli->setValue( SliderPos(p->calData.IaVal-1));
    ui->IsSli->setValue( SliderPos(p->calData.IsVal-1));
    ui->VsuSli->setValue( SliderPos(p->calData.VsuVal-1));
    ui->Vg05Sli->setValue(SliderPos(p->calData.Vg0));
    ui->Vg1Sli->setValue( SliderPos(p->calData.Vgm-1));
    //ui->Vg05Sli->setValue(p->calData.Vg05Val);
    //ui->Vg1Sli->setValue( p->calData.Vg1Val);
    //ui->Vg4Sli->setValue( p->calData.Vg4Val);
    //ui->Vg40Sli->setValue(p->calData.Vg40Val);
    ui->VnSli->setValue(  p->calData.VnVal-1);
}


void Cal_Dialog::on_VaSli_valueChanged(int value)
{
    ui->VaVal->setText(QString::number(FromSlider(value)+1));
    mw->calData.VaVal=FromSlider(value)+1;
}

void Cal_Dialog::on_VsSli_valueChanged(int value)
{
    ui->VsVal->setText(QString::number(FromSlider(value)+1));
    mw->calData.VsVal=(FromSlider(value)+1);
}

void Cal_Dialog::on_IaSli_valueChanged(int value)
{
    ui->IaVal->setText(QString::number(FromSlider(value)+1));
    mw->calData.IaVal=(FromSlider(value)+1);
}

void Cal_Dialog::on_IsSli_valueChanged(int value)
{
    ui->IsVal->setText(QString::number(FromSlider(value)+1));
    mw->calData.IsVal=(FromSlider(value)+1);
}

void Cal_Dialog::on_VsuSli_valueChanged(int value)
{
    ui->VsuVal->setText(QString::number(FromSlider(value)+1));
    mw->calData.VsuVal=(FromSlider(value)+1);
}

void Cal_Dialog::on_Vg05Sli_valueChanged(int value)
{
    //ui->Vg05Val->setText(QString::number(RealVal(value)));
    //mw->calData.Vg05Val=float(value);
    ui->Vg05Val->setText(QString::number(FromSlider(value)));
    mw->calData.Vg0=(FromSlider(value));

}

void Cal_Dialog::on_Vg1Sli_valueChanged(int value)
{
//    ui->Vg1Val->setText(QString::number(RealVal(value)));
//    mw->calData.Vg1Val=float(value);
    ui->Vg1Val->setText(QString::number(FromSlider(value)+1));
    mw->calData.Vgm=(FromSlider(value)+1);
}

void Cal_Dialog::on_Vg4Sli_valueChanged(int value)
{
    ui->Vg4Val->setText(QString::number(FromSlider(value)+1));
    mw->calData.Vg4Val=(FromSlider(value)+1);
}

void Cal_Dialog::on_Vg40Sli_valueChanged(int value)
{
    ui->Vg40Val->setText(QString::number(FromSlider(value)+1));
    mw->calData.Vg40Val=(FromSlider(value)+1);
}

void Cal_Dialog::on_VnSli_valueChanged(int value)
{
    ui->VnVal->setText(QString::number(FromSlider(value)+1));
    mw->calData.VnVal=(FromSlider(value)+1);
}

void Cal_Dialog::on_CalResetButton_clicked()
{
    mw->calData.VaVal=1;
    mw->calData.VsVal=1;
    mw->calData.IaVal=1;
    mw->calData.IsVal=1;
    mw->calData.VsuVal=1;
    mw->calData.Vg0=0;
    mw->calData.Vgm=1;
    //mw->calData.Vg05Val=0;
    //mw->calData.Vg1Val=0;
    //mw->calData.Vg4Val=0;
    //mw->calData.Vg40Val=0;
    mw->calData.VnVal=1;
    mw->calData.RaVal=5350;
    mw->calData.VaMax=400;
    mw->calData.VgMax=50;
    mw->calData.IaRsense=5.1;
    mw->calData.IsRsense=5.1;
    UpdateLabels(mw);
    UpdateSliders(mw);
}


void Cal_Dialog::on_EraseButton_clicked()
{
    mw->SaveCalToChip(1);
}



