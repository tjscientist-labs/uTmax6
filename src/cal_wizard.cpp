#include "cal_wizard.h"
#include "ui_cal_wizard.h"
#include <QThread>

uint state=0;
uint next=0;
uint prev=0;

MainWindow *mw;

QTimer * timer;
Cal_Wizard::Cal_Wizard(MainWindow *parent) :
    QDialog(parent),
    ui(new Ui::Cal_Wizard)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    this->setAttribute(Qt::WA_DeleteOnClose);
    mw = parent;
    ui->info_textBrowser->setPlainText("Welcome to the calibration wizard. You will need a voltmeter and two 10k 1% resistors. \
Enter all readings in volts, the sign is not important.\n\nWhen ready press 'Next'");
    ui->label->hide();
    ui->measurement_lineEdit->hide();
    timer = new QTimer(this);
    next=0;
    prev=0;
}

Cal_Wizard::~Cal_Wizard()
{
    delete ui;
}

void Cal_Wizard::reset() {
//    state=0;
}

void Cal_Wizard::on_prev_pushButton_clicked()
{
    state=prev;
    Wizard();
}


settings_t s;
void Cal_Wizard::on_Next_pushButton_clicked()
{
    state=next;
    Wizard();
}

void Cal_Wizard::Wizard()
{
    float m;
    static float v1=0;
    switch (state) {
    case 0:
        //reset calibration
        mw->calData.IaVal=1;
        mw->calData.IsVal=1;
        //mw->calData.Vg05Val=0;
        //mw->calData.Vg1Val=0;
        //mw->calData.Vg4Val=0;
        //mw->calData.Vg40Val=0;
        mw->calData.VnVal=1;
        mw->calData.VsuVal=1;
        ui->info_textBrowser->setPlainText("Setting up..");
        ui->label->hide();
        ui->measurement_lineEdit->hide();
        ui->Next_pushButton->setText("Next");
        next=1;
        prev=0;
        mw->GetADC();
        break;
    case 1:
        //Negative supply
        ui->info_textBrowser->setPlainText("Measure the negative supply (grid suppy for uTracer 3 and -15V for uTracer 6). Enter it below then press 'Next'");
        ui->label->show();
        ui->measurement_lineEdit->show();
        ui->label->setText(QString("Negative Supply is %1").arg(mw->adc_real.Vn,0,'g',4));
        ui->Next_pushButton->show();
        ui->measurement_lineEdit->setFocus();
        next=2;
        prev=0;
        break;
    case 2:
        m = fabs(ui->measurement_lineEdit->text().toFloat());
        if ( (mw->options.uTracer6==0)& ((m > 37) && (m < 44)) || (mw->options.uTracer6==1) & (((m > 13.5) && (m < 16.5))) ) {
            mw->CalcVnCal(-m);
            ui->info_textBrowser->setPlainText("Measure the positive supply and enter it below, then press 'Next'");
            ui->label->show();
            ui->measurement_lineEdit->clear();
            ui->measurement_lineEdit->show();
            ui->measurement_lineEdit->setFocus();
            ui->label->setText(QString("Positive Supply is %1").arg(mw->adc_real.Vsu,0,'g',4));
            next=3;
            prev=1;
        }  else {
            ui->info_textBrowser->setPlainText("That doesn't seem right, try again. \nIf the problem persists check your hardware.");
            next=1;
            prev=1;
        }
        break;
    case 102:
	    ui->info_textBrowser->setPlainText("Measure the positive supply and enter it below, then press 'Next'");
	    ui->label->show();
	    ui->measurement_lineEdit->clear();
	    ui->measurement_lineEdit->show();
        ui->label->setText(QString("Positive Supply is %1").arg(mw->adc_real.Vsu,0,'g',4));
	    next=3;
            prev=1;
        break;
    case 3:
        m = fabs(ui->measurement_lineEdit->text().toFloat());
        if ((m>17.5) && (m < 21)) {
            mw->CalcVpCal(m);
            ui->info_textBrowser->setPlainText("Connect a voltmeter across the anode boost capacitor. \
    The tracer will apply a high test voltage. When connected, press 'Next'");
            ui->label->hide();
            ui->measurement_lineEdit->hide();
            next=4;
            prev=102;
        } else {
            ui->info_textBrowser->setPlainText("That doesn't seem right, try again. \nIf the problem persists check your hardware.");
            next=102;
            prev=102;
        }
        break;
    case 103:
	    ui->info_textBrowser->setPlainText("Connect a voltmeter across the anode boost capacitor. \
	The tracer will apply a high test voltage. When connected, press 'Next'");
	    ui->label->hide();
	    ui->measurement_lineEdit->hide();
	    next=4;
	    prev=102;
        break;
    case 4:
        ui->info_textBrowser->setPlainText("Applying anode voltage. Allow a few seconds for the reading to stabilize before you note the reading.\
 After a few more seconds the voltage will be removed for safety.");
        s.Va=250;
        s.Vs=22;
        s.Vg=0;
        s.delay=5;
        s.VaVsTrue=false;
        ui->Next_pushButton->hide();
        ui->prev_pushButton->hide();
        next=5;
        mw->calData.VaVal=1;
        mw->Apply_Mode(s);
        break;
    case 5:
        ui->info_textBrowser->setPlainText(" Enter your reading in the box below then press 'Next'");
        ui->label->show();
        ui->measurement_lineEdit->clear();
        ui->measurement_lineEdit->show();
        ui->label->setText("Measure Va");
        ui->Next_pushButton->show();
        ui->prev_pushButton->show();
//        ui->measurement_lineEdit->selectedText();
        ui->measurement_lineEdit->setFocus();
        next=6;
        prev=103;
        break;
//Vs
    case 6:
        m = fabs(ui->measurement_lineEdit->text().toFloat());
        if (m> (225) && m<(275)) {
            mw->CalcVaCal(m);
            ui->info_textBrowser->setPlainText("Repeat this time with the voltmeter across the screen boost capacitor. When connected, press 'Next'.");
            ui->label->hide();
            ui->measurement_lineEdit->hide();
            next=7;
            prev=4;
        } else {
            prev=next=103;
            ui->info_textBrowser->setPlainText("That doesn't seem right, try again. \nIf the problem persists check your hardware.");
        }
        break;
    case 106:
            ui->info_textBrowser->setPlainText("Repeat this time with the voltmeter across the screen boost capacitor. When connected, press 'Next'.");
            ui->label->hide();
            ui->measurement_lineEdit->hide();
            next=7;
            prev=103;
        break;
    case 7:
        ui->info_textBrowser->setPlainText("Applying screen voltage. Watch the meter and allow a few seconds for the reading to stabilize.");
        s.Va=22;
        s.Vs=250;
        s.Vg=0;
        s.delay=5;
        s.VaVsTrue=false;
        ui->Next_pushButton->hide();
        ui->prev_pushButton->hide();
        next=8;
        mw->calData.VsVal=1;
        mw->Apply_Mode(s);
        break;
    case 8:
        ui->info_textBrowser->setPlainText(" Enter your reading in the box below then press 'Next'");
        ui->label->show();
        ui->measurement_lineEdit->clear();
        ui->measurement_lineEdit->show();
        ui->label->setText("Measure Vs");
        ui->Next_pushButton->show();
        ui->prev_pushButton->show();
        //ui->measurement_lineEdit->selectAll();
        ui->measurement_lineEdit->setFocus();
        next=9;
        prev=106;
        break;
//Vg 0.5V
    case 9:
        m = fabs(ui->measurement_lineEdit->text().toFloat());
        if ((m>225) && (m<275)) {
            mw->CalcVsCal(m);
            ui->info_textBrowser->setPlainText("Connect a voltmeter between grid and cathode, negative to cathode. For utracer6, remove J2.\n\nWhen connected, press 'Next'");
            ui->label->setText("Measure Vg");
            ui->label->hide();
            ui->measurement_lineEdit->hide();
            ui->Next_pushButton->show();
            ui->prev_pushButton->show();
            next=10;
            prev=106;
            break;
        } else {
            ui->info_textBrowser->setPlainText("That doesn't seem right, try again.\nIf the problem persists check your hardware.");
            next=prev=106;
            break;
        }
    case 109:
        ui->info_textBrowser->setPlainText("Connect a voltmeter between grid and cathode, negative to cathode. For utracer6, remove J2.\n\nWhen connected, press 'Next'");
	    ui->label->setText("Measure Vg");
	    ui->label->hide();
	    ui->measurement_lineEdit->hide();
	    ui->Next_pushButton->show();
	    ui->prev_pushButton->show();
	    next=10;
        prev=106;
	    break;
    case 10:
        ui->info_textBrowser->setPlainText("Applying grid voltage 0.5V. Watch the meter.");
        s.Va=4;
        s.Vs=4;
        s.Vg=-0.5;
        s.delay=5;
        s.VaVsTrue=true;
        ui->label->hide();
        ui->measurement_lineEdit->hide();
        ui->Next_pushButton->hide();
        ui->prev_pushButton->hide();
        next=11;
        //mw->calData.Vg05Val=0;
        //mw->calData.Vg1Val=0;
        //mw->calData.Vg4Val=0;
        //mw->calData.Vg40Val=0;
        mw->calData.Vg0=0;
        mw->calData.Vgm=1;
        mw->Apply_Mode(s);
        break;
    case 11:
        ui->info_textBrowser->setPlainText("Enter the reading, approx 0.5V, in the box below then press 'Next'");
        ui->label->show();
        ui->measurement_lineEdit->clear();
        ui->measurement_lineEdit->show();
        ui->Next_pushButton->show();
        ui->prev_pushButton->show();
        ui->measurement_lineEdit->setFocus();
        next=16;
        prev=109;
        break;
//Vg 1V
/*    case 12:
        m = fabs(ui->measurement_lineEdit->text().toFloat());
        if (m>=0.3 && m<0.7) {
            v1=m;
            //mw->CalcVg05Cal(m);
            ui->info_textBrowser->setPlainText("Applying grid voltage 1V. Watch the meter.");
            s.Va=4;
            s.Vs=4;
            s.Vg=-1;
            s.delay=5;
            ui->label->hide();
            ui->measurement_lineEdit->hide();
            ui->Next_pushButton->hide();
            ui->prev_pushButton->hide();
            next=13;
            prev=109;
            mw->Apply_Mode(s);
        } else {
            ui->info_textBrowser->setPlainText("That doesn't seem right, try again. \nIf the problem persists check your hardware.");
            next=prev=109;
        }
            break;
    case 13:
        ui->info_textBrowser->setPlainText("Enter the reading, approx 1V, in the box below then press 'Next'");
        ui->label->show();
        ui->measurement_lineEdit->clear();
        ui->measurement_lineEdit->show();
        ui->Next_pushButton->show();
        ui->prev_pushButton->show();
        ui->measurement_lineEdit->selectAll();
        next=14;
        prev=109;
        break;
    case 14:
        m = fabs(ui->measurement_lineEdit->text().toFloat());
        if (( m>0.7) && (m<1.3)) {
        //mw->CalcVg1Cal(m);
        ui->info_textBrowser->setPlainText("Applying grid voltage 4V. Watch the meter.");
        ui->label->hide();
        ui->measurement_lineEdit->hide();
        ui->Next_pushButton->hide();
        ui->prev_pushButton->hide();
        s.Va=4;
        s.Vs=4;
        s.Vg=-4;
        s.delay=5;
        next=15;
        prev=109;
        mw->Apply_Mode(s);
        } else {
            ui->info_textBrowser->setPlainText("That doesn't seem right, try again. \nIf the problem persists check your hardware.");
            next=prev=109;
    }
        break;
    case 15:
        ui->info_textBrowser->setPlainText("Enter the reading, approx 4V, in the box below then press 'Next'");
        ui->label->show();
        ui->measurement_lineEdit->clear();
        ui->measurement_lineEdit->show();
        ui->Next_pushButton->show();
        ui->prev_pushButton->show();
        ui->measurement_lineEdit->selectAll();
        next=16;
        prev=109;
        break; */
    case 16:
        m = fabs(ui->measurement_lineEdit->text().toFloat());
        if (m>=0.3 && m<0.7) {
            v1=m;
            //mw->CalcVg05Cal(m);
//        if ((m>3.5) && (m < 4.5)) {
//            mw->CalcVg4Cal(m);
            ui->info_textBrowser->setPlainText("Applying grid voltage 40V. Watch the meter.");
            ui->label->hide();
            ui->measurement_lineEdit->hide();
            ui->Next_pushButton->hide();
            ui->prev_pushButton->hide();
            s.Va=4;
            s.Vs=4;
            s.Vg=-40;
            s.delay=5;
            next=17;
            mw->Apply_Mode(s);
        } else {
            ui->info_textBrowser->setPlainText("That doesn't seem right, try again. \nIf the problem persists check your hardware.");
            next=prev=109;
        }
        break;
    case 17:
        ui->info_textBrowser->setPlainText("Enter the reading, approx 40V, in the box below then press 'Next'");
        ui->label->show();
        ui->measurement_lineEdit->clear();
        ui->measurement_lineEdit->show();
        ui->Next_pushButton->show();
        ui->prev_pushButton->show();
        ui->measurement_lineEdit->setFocus();
        next=18;
        prev=109;
        break;
//Ia, Is
    case 18:
        m = fabs(ui->measurement_lineEdit->text().toFloat());
        if ((m > 35) && (m < 45)) {
            //mw->CalcVg40Cal(m);
            mw->CalcVg(v1,m);
            ui->info_textBrowser->setPlainText("Connect a 10K 1% resistor between cathode and anode and another between screen and cathode. For utracer6, replace J2.\n\n When done press 'Next'. ");
            ui->measurement_lineEdit->hide();
            ui->label->hide();
            next=19;
            prev=109;
        } else {
            ui->info_textBrowser->setPlainText("That doesn't seem right, try again. \nIf the problem persists check your hardware.");
            next=prev=109;
        }
        break;
    case 118:
        ui->info_textBrowser->setPlainText("Connect a 10K 1% resistor between cathode and anode and another between screen and cathode. When done press 'Next'. ");
        ui->measurement_lineEdit->hide();
        ui->label->hide();
        next=19;
        prev=109;
        break;
    case 19:
        ui->info_textBrowser->setPlainText("Calibrating current. Please wait...");
        s.Va=250;
        s.Vs=250;
        s.Vg=0;
        s.VaVsTrue=true;
        ui->measurement_lineEdit->hide();
        ui->label->hide();
        ui->Next_pushButton->hide();
        ui->prev_pushButton->hide();
        next=20;
        mw->Apply_Mode(s);
        break;
    case 20:
        ui->label->setText(QString("Measured Ia=%1 Is=%2").arg(uint(mw->adc_real.Ia)).arg(uint(mw->adc_real.Is)));
        ui->Next_pushButton->hide();
        ui->prev_pushButton->show();
        ui->label->show();
        if (
            (mw->adc_real.Ia > 250*0.09) &&
            (mw->adc_real.Ia < 250*0.11) &&
            (mw->adc_real.Is > 250*0.09) &&
            (mw->adc_real.Is < 250*0.11)) {
            ui->info_textBrowser->setPlainText("Calibration complete.\n\nPlease wait 10 seconds for high voltage to be removed and the wizard to automatically close. ");
            mw->CalcIaCal(mw->adc_real.Va);
            mw->CalcIsCal(mw->adc_real.Vs);
            next=21;
            prev=118;
            QTimer::singleShot(10000, this, SLOT( on_Next_pushButton_clicked() ));
        }
        else {
            next=prev=118;
            ui->Next_pushButton->show();
            ui->info_textBrowser->setPlainText("That doesn't seem right, try again. \nIf the problem persists check your hardware.");
        }
        break;
    case 21:
        mw->SaveCalFile();
        mw->SaveCalToChip(false);
        close();
        break;
    default:
        ui->info_textBrowser->setPlainText("Something went wrong...");
    }
}


