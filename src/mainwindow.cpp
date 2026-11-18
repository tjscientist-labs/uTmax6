/* utGui MainWindow Implementation
 *
 * Copyright (C) 2013 Nick Barton http:\\www.bmamps.com
 * The IaCalc() model is derived from the MatLab by (c) Norman Koren.
 * Used here with permission.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "cal_dialog.h"
#include "cal_wizard.h"
#include "options_dialog.h"
#include "debug_dialog.h"
#include "datasavedialog.h"
#include "grid_word.h"
#include "reply_format.h"
#include "voltage_limit.h"
#include <QFile>
#include <QString>
#include <QDebug>
#include <QFileDialog>
#include <QLineEdit>
#include <QtCore/qmath.h>
#include <qmessagebox.h>
#include <qcustomplot.h>
#include <qvector.h>
#include <QColorDialog>
#include <QAction>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <math.h>


#define VMIN 2
#define COMDEBUG (false)
#define DIGITCNT (4)
#define QDEBUG (false)
#define LOGGING (false)
#define VGLIM 3 //difference from max
#define Vdi 0.5  //drop across diode D11
#define Vdar 0.75  //drop across darlington
//#define DISTIME 45 //time allowed for caps to discharge after an abort
#define min(a,b) (a<b?a:b)
bool update_failed=false;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    if (COMDEBUG || LOGGING) {
        logFile.setFileName("utmax_log.txt");
        //logFile.open(QIODevice::Append | QIODevice::Text);
//        logFile.open(QIODevice::Append);
        logFile.open(QIODevice::WriteOnly );
        QTextStream(&logFile) << "New Session\n";
    }
    fwdebug=false;
    //LogFile(Qt::Checked); //enable logging at start up
    LogFile(Qt::Unchecked); //disable logging at start up
    do_fw_update=false;
    options.uTracer6=false;
    options.ExtHtr=false;
    calData.heaterscale=0;
    on_TubeType_currentIndexChanged_enable=true;
    activeStore=0;
    adc_data.Ia=0;
    adc_data.Ia_raw=0;
    adc_data.Is=0;
    adc_data.Is_raw=0;
    adc_data.Va=0;
    adc_data.Vs=0;
    adc_data.Vsu=0;
    adc_data.Vn=0;
    adc_data.Gain_a=9;
    adc_data.Gain_s=10;
    adc_scale.Ia=    1000.0*5.0/1023.0;
    adc_scale.Ia_raw=1000.0*5.0/1023.0;
    adc_scale.Is=    1000.0*5.0/1023.0;
    adc_scale.Is_raw=1000.0*5.0/1023.0;
    adc_scale.Gain_a=1.0;
    adc_scale.Gain_s=1.0;
    options.AvgNum=0;
    options.Delay=0;
    options.IaRange=0;
    options.IsRange=0;
    options.Ilimit=0;
    options.AbortOnLimit=true;
    options.VgScale=1.0;
    options.VaScale=1.0;
    portInUse = NULL;
    penList = new QList<QPen>;
    status=Idle;
    startSweep=0;
    stop=false;
    timer_on = false;
    timeout=PING_TIMEOUT;
    timer=NULL;
    newMessage=false;
    sendADC=false;
    sendPing=false;
    VaADC=0;
    VsADC=0;
    VgADC=0;
    VfADC=0;
    VaNow=0;
    VsNow=0;
    VgNow=0;
    VfNow=0;
    HtrHoldEnd=false;
    on_TubeSelector_currentIndexChanged_enable=false;
    dataStore = new QList<results_t>;
    refStore = new QList<results_t>;
    //tubeDataList = new QList<tubeData_t>;
    sweepList = new QList<test_vector_t>;
    options.TLSortOrder=0;
    calData.Firmware=100;
    ui->setupUi(this);
    //once ui is up...
    //Hack for OSX
    this->setWindowTitle("utMax 3.01");
    /* ****************************************************************************************
     * 3.01
     *  Vg underflow fixed. This caouslresult in getting large negative grid volatges
     *  when a value close to zero was wanted.
     * 3.00
     *  64bit Windows version
     *  Clarified some calibration wizard prompts.
     *  Fixed debug log enable not checking for Firmware version
     * 2.32
     *  Change: During plotting the power limit on the controls tab is used.
     *  Fixed: When using Vg on the x-axis, legend entires were duplicated.
     *  Change: The power limit checkbox is now implemented when Vg is on the x-axis
     * 2.31
     * Fixed incorrect text items on plot legend
     * 2.3
     *  No log on startup
     *  First release with utracer6 support
     * 2.21p , not released
     *  Open log on startup
     *  log now always in home directory
     *  Fixed: in calWIz the cal data was being written to the chip for any firmware version.
     * 2.21o , not released
     *  fwdebug = true on startup
     * 2.21n , not released
     *  Cal wizard: changed Va and Vs to 250V
     * 2.21m , not released
     *  Changed Vneg to -15V for uTracer 6
     * 2.21l , not released
     *  Increase range for Vneg in the cal wizard for utracer6
     * 2.21k not released, for testing ut6 only
     *  added 1meg resistor for ut6 voltage divider
     *  added 200k/2k divider for negative rail for ut6
     *  added 10k/1k divider for positive supply
     *  no longer any need to restart if raVal or ut6 options changed.
     * 2.21j not released, for testing ut6 only
     *  In cal dialog, for firmware<200 the writing of cal data to the chip is skipped.
     * 2.21i
     *  Add smooth plot for Vg on x axis. Note this requires many Vg steps to do anything or indeed anything useful.
     *  Fixed plots not working if x-axis set to  "Vg" with no dataset.
     * 2.21h
     *  Fixed legend as y2 axis showing when not using y2 axis
     *  Dual diode quick test LCD display for (b) section currents
     *  plot not being updated while sweeping
     *  Changed heater=  option behaviour:
     *       If heater==0 heater code in comms is for the uTracer.
     *       If heater>0 an Ext Heater check box appears on the options dialog 
     *       and the heater code is linear. It also has the top byte set to "e"
     *       is scaled 1023 * (requested heater voltage)/ heater
     *
     * 2.21g
     *  Tube sort order combo box was not updated when opening the option dialog
     *  Added test for nan in KVB estimate
     *  data.csv was being written with an invaled value for Va=Vs check box state.
     * 2.21f
     *  Added Va=Vs column to data.csv
     * 2.21e
     *  Modified way the last segment of a plot line it handled to avoid truncating plots on over current
     * 2.21d
     *  Cosmetic tweaks to options dialog
     *  Added a checkbox to plot tabs to allow the points over the power limit to be hidden
     *  Added a column to the results data file to hold the current and power limit status
     * 2.21c
     *  Fixed crash when loading a second tube data file which is different first data file's tube model
     *  Fixed crash in b-spline
     *  Added knots to start and end of b-spline range
     *  Add ignore flag to data pool for over current and over power
     * 2.21b
     *  Option "heater=" is now saved to cal file
     *  For uTracer3 compatibility, if Firmware=100 (or =1) in cal file then the chip is not interrogated for the firmware version
     * 2.21a
     *  User can add heater=n to cal file.
     *   If n=0 (or line is missing) then it utmax or ut3 heater firmware compatible. Otherwise the
     *   10 bit heater field is scaled by n.
     *   e.g. n =10 means that 0x3ff is output when a 10V heater is specified.
     * 2.21
     *  uTracer 6 checkbox was not updated on options dialog
     *  Fixed bug in grid voltage calculation introduced with uTracer6 changes
     *  Invalidate model status when creating optimizer.
     *  Added positive grid voltage handling for uTracer6
     * 2.20g1
     *  Added tests for invalid tube model index to prevent crash
     * 2.20g
     *  Added support for uTracer6
     *  Changed interpolation from cubic spline to B-spline (cubic) smoothing
     * 2.20f
     *  Removed firmware update command for fw 2.99 or lower
     *  Fixed QMessageBox styles as they  were not inheriting the parent style
     *  Changed interpolation from polynomial to cubic spline
     * 2.20e
     *  Changed main timer to 250mS to make measurements faster.
     *  Fixed bug in Vs value calculation
     *  Fixed divide by zero in Ra calculation
     *  Added polynomial based curve fitter
     *  Added polynomial order spinbox to plot tabs
     *  Modified labels and layout of plot tab control to make room for the new 'order' spinbox
     *  Modified options dialog layout for polymomial order value to be added (at a later date)
     * 2.20d
     *  Added yaxis2 mouse drag and zoom
     *  Changed LCD overflow from "-" to "OL"
     * 2.20c
     *  Fixed range check issue with Vsupp in cal wiz
     *  Fixed range check issue with Vs in cal wiz
     *  Set focus to measurement entry box in cal wiz
     * 2.20b
     *  Put next step flag for cal wiz into RxData() to avoid heatbeat ping conflict causing wiz lockout.
     * 2.20a
     *  Fixed: on dual triode quick test, gm(b) was shown incorrectly
     *  Changed low screen and anode voltage during cal wiz
     * 2.20
     *  12 bit grid for 2.03+: puts the two LSBs at top of 16 bit command so that new 12 bit firmware
     *  is still compatible with 10 bit commands.
     *  Heater not reset during ping
     *  LCDs increased to four digits
     * 2.19j
     *  Changes order of LCDs for dual triodes
     * 2.19i
     *  Changed: If a port could not be opened on start than processing would go no further.
     *  Made COM status label wider
     * 2.19h
     *  Fixed: LCD current color always red
     *  Cal Dialog: Changed calibration range to 1
     *  Cal Dialog: Changed non-grid cal sliders to have 1/10th range.
     *  Cal wizard: Widened valid range for Va and Vs on the low end
     *  Cal wizard: Fixed failure to retry if Va out of bounds
     *  At end of sweep an Apply command is used to lower the HV volatge. This removes the need for
     *  the HV discharge timeout.
     *  All calibration parameters now use floats
     *  Chip cal parameters are interger analogs of the cal parameters x1000
     *  Changed grid calibration to use offset and slope: A recalibration is required for this version
     *  Numerous enhancements to COM port handling to detect port closure and auto reopen.
     *  Added port status to front page
     *  Added port rediscovery to debug dialog
     * 2.19g
     *  Cal wizard: fixed bug in Vn and Vsupp
     *  Debug logging now ignores received "<" when expecting calibration data from chip
     * 2.19f
     * 2.19e
     *  Cal wizard: reset all cal corrections after prev or error
     *  Calibration now using round() rather than truncation
     * 2.19d
     *  Fixed missing "next" button after current cal fail
     *  Fixed reset grid corrections after fail
     * 2.19c
     *  Calibration range changed from 0.3 to 0.4.  This means a recalibration is required for this version
     *  Changed to 10K calibration resistor
     *  Changed valid ranges for calibration.
     *  Used timer for final discharge wait in cal wiz
     * 2.19
     *  Fixed: marker text position wrong when zooming and scrolling axes
     *  Fixed: marker text position wrong when frozen and curve is changed
     *  Tidied options dialog layout
     *  Fixed Cal wiz Va, Vs and sanity checking
     *  Fixed Cal wiz Ia, &  Is calibration
     *  Added Cal wiz previous button
     *  Changed debug enable/disable to suit new firmware
     *  Added firmware version to options dialog
     * 2.17a
     *  Added chip cal data erase
     *  Added outline calibration wizard
     *  Added axis scroll and zoom
     *  Fixed: missing Rx data from debug dialog
     *  Added calibration wizard
     *  Made all dialogs delete on close
     * 2.16a
     *  Fixed incorrect length when sending cal data
     *  Add checksum and checking on chip cal data r/w
     *  Changed scale on IaRsense and IsRsense from x 10 to x100
     * 2.15a
     * 2.14a
     *  Addressed visibily of markers when starting a new plot
     * 2.13a
     *  Added right click to freeze marker
     *  Reverted y axis range to an integer of max current
     *  Fixed tube type and model not updated when reading saved data file
     *  Fixed infinite loop in UpdateTitle()
     * 2.12a
     *  Va X axis range is a multiple of 10
     *  Ia & Is Y axis range is a multiple of 5
     *  Validity checking added to LCDs to prevent "A" and "F" being displayed
     *  Fixed incorrect label on marker text when y2 axis in use
     *  Added limit to curve number spinbox
     * 2.11a
     *  Fixed issue in modelling TriodeInit() where Ia=0 was not handled
     *  Changed marker text position offet to be fixed rather than propotional
     *  Changed Ra(a), Ra(b) and gm axi labels to place units first
     *  Fixed marker text position wrong after a plot setting change e.g. curve number
     * 2.10a
     *  Fixed when clicking "start" the wrong in the tube list was replaced do leading to a missing entry and a duplicate.
     *  Fixed incorrect sort order displayed on startup
     *  Fixed missing entries in legend
     *  Fixed plot not being marked invalid when hitting start after a completed sweep
     *  Fixed markers were still visible when plot changed from valid to invalid.
     * 2.09a
     *  Fixed auto file name on startup
     *  Fixed flickering doubling of length of legend while ploting
     *  Fixed no markers if no model is selected
     * 2.08a
     * Disabled markers if a plot is incomplete
     * Fixed tube list display synchronization with internal database and data.csv file
     * Fixed tube delete of first item in selector
     * 2.07a
     *  Fixed: issues with LCD not displaying when number would not fit specified number of digits
     *  Fixed: missing / incorrect labels on LCDs
     *  Increase discharge time to 45 to reduce timeout messages
     *  Change timeout message to give a better hint for resolution
     *  Added: pre fill tube name for file save for data or spice model
     *  Fixed: tube list sort order reverts to alpha after adding a tube
     *  Removed Vg2 on x axis plot as not very useful
     *  Tube list sort order on startup now matches setting cal.txt
     *  Smooth model lines when Vg on x axis
     *  Fixed: Heater voltage set to max if a positive grid voltage specified
     *  Added: OFF option to left yaxis
     *  Fixed Vg on x axis range
     *  Fixed incorrect marker positon when VG on xaxis
     * 2.06a
     *  fixed model results show for pentode section when triode plot selected
     *  fixed crash when model changed to none and yaxis showing model values
     *  fixed incorrect SPICE file output
     *  added auto path for SPICE file save dialog.
     *  crashes when plotting without full dataset
     *  enable all controls after a timeout
     * 2.05a
     *  Changed linearized parameters display for pentodes to show Va, Vs, Vg, gm and Ia.
     *  fixed infinite loop on adding a tube
     *  moved marker to upper layer
     *  tweaked positioning of marker text
     *  on plots with Vg on x-axis fixed limit of 16 colors
     *  on plots with Vg on x-axis fixed ledgend display show Yaxis2 info
     *  on plots with Vg on x-axis fixed marker text positon TBD: y scale is slighly incorrect
     *  Fix crash when switch xaxis type ( Va/Vg2/Vg) and then back again due to invalid showIndexValue
     *  estimates of alpha_s and beta incorrect: reduced size for convergenace and added overrides for DERK_E alpha_a and beta
     *  Data file not saved when editing parameters for existing type
     *  When attaching marker text to lines using yAaxis2, the y axis was scaled incorrectly
     *  X left limit marging for marker text changed to -0.05 from -0.1.
     *  Fixed flickering of legend when loading a theme.
     *  Added smooth curves for model
     *  Fixed incorrect marker text for y2axis
     *  Fixed yaxis2 missing color
     * 2.04a
     *  plotLineWidth added to qss
     *  added group by color feature
     *  LCD digits set to 3
     *  Quick test file format changed to prevent error when trying to load it.
     *  Add auto mu KV/V scale option for pentodes
     * 2.03a
     *  Vg display gets two decimal places
     *  Fix plot line 0 marker not working
     *  Added debug/ pretty print (Off hopefully) to debug log off checkbox
     * 2.02a
     *  fixed greying for dual triode and dual diodes
     *  crash when mouseover on empty plot
     *  I limit setting out by /100
     * 2.01a
     *  Modeller: Fixed typos in TriodeInit() math
     *  Modeller: Handle case where Is=0 for KG2 estimate
     *  moved plot value markers to an overlay layer
     * 2.00a 12 bit Vg?,
     *  calibration data on chip,
     *  plot line colors from qss file:
     *      plotLegendTextColor=#ccc
     *      plotSHowValColor=#ff0
     *      plotLineColor0=#00f up to plotLineColor49
     *  calibration reset,
     *  hardware settiNgs now in gui
     *  MRU, LU, Alpha tube sort options
     *  Read data file auto path
     *  More status messages
     * 1.58 More Front panel layout
     * 1.57 More Front panel layout
     * 1.56 More Front panel layout
     * 1.55 More Front panel layout
     * 1.54 Front panel layout
     * 1.53 Fixes to plotLegendColor= and plotLegendTextColor= to stylesheet keys
     * 1.52 Added plotLegendColor= and plotLegendTextColor= to stylesheet keys
     * 1.51 fix size of main window controls
     *      set color of ledgend background
     * 1.50 Center buttons at bottom of debug dialog
     * 1.49 made debug dialog wider
     * 1.48 Updated labels in Tracer Limits options dialog
     * 1.47 Remove size constriant from options dialog
     * 1.46 Move tracer info from debug to options
     * 1.45 Remove title from options dialog
     * 1.44 Added groupBox to options dialog
     * 1.43 Updated Debug Dialog object names
     * 1.42 Theme path uses last used path, fixed issues with filenames in cal.txt
     * 1.41 Added tick, subtick color -  same as axis color
     * 1.40 Increased size of plot controls box
     * 1.39 Fixed plot title color
     * 1.38 Add objectName plotCtl for plot controls GroupBox
     * 1.37: Added  axis color)
     * 1.36: Added  more plot color styles ( Text color)
     * 1.35: Added  plot color style
     * 1.34: Fixed typo, changed default path for themes
     * 1.33: Add groupboxes
     * 1.32: Remove column scaling
     * 1.31
     * Unsized - size from qss
     * 1.30
     * Removed top border style from QTabWidet
     * Removed unused menu
     * 1.29
     * Annoyingly picky cosmetic tweaks ;)
     * 1.28
     * Resizable plot area
     * 1.27
     *  Added style picker tool
     * 1.26
     *  Added savepath to cal.txt
     * 1.25
     *  Removed close button from default tabs
     * 1.24
     *  Removed all styles
     * 1.23
     *  Changed step limit from individial max 30 to total max of 50,000 data points
     * 1.22
     * added stylesheet
    * 1.21
    *   Was skipping reheat after a "STOP"
    *   Fixed input field disables ofr diode and triode quick test
    *   Fixed sweep for diode and triode
    * 1.20 debug
    *   Added log file for fw debug
    * 1.19
    *  Added Vg 0.5V calibration slider
    * 1.18
     *  Added disable of all sweep parameters whilst busy
     * 1.17
     *  Fixed clearing of heater progress on "stop"
     *  Fixed failure to send heater=0 command to hardware
     *  Checked operation of saving in different image formats:OK
     *  Check saving to specified path
     * 1.16
     *  Bug: Fixed incorect VaEnd in Data file read
     * 1.15
     * Add entry in cal file for firmware
     * 1.14
     * fixed rounding isue in cal sliders
     * 1.13
     *  Fix Vg1 slider saving incorrect value in cal file
     * 1.12:
     *  Fixed bug in RxData where partial recived commands were not handled correctly.
     * 1.11:
     *  Added heater hold function
     *  Tidied up ToolTips
     *  Added limit to numer of times the Start button can pressed in sucession
     *  Prevent the sweep parameters for being recalulated in mid sweep should start be pressed
    *********************************************************************************************/
    lcds[0] = ui->lcd1;
    lcds[1] = ui->lcd2;
    lcds[2] = ui->lcd3;
    lcds[3] = ui->lcd4;
    lcds[4] = ui->lcd5;
    lcds[5] = ui->lcd6;
    lcds[6] = ui->lcd7;
    lcds[7] = ui->lcd8;
    lcds[8] = ui->lcd9;
    ClearLCDs();

    ui->TubeType->addItem(NONE);
    ui->TubeType->addItem(DIODE);
    ui->TubeType->addItem(DUAL_DIODE);
    ui->TubeType->addItem(TRIODE);
    ui->TubeType->addItem(DUAL_TRIODE);
    ui->TubeType->addItem(KOREN_P);
    ui->TubeType->addItem(DERK_P);
    ui->TubeType->addItem(DERK_P2);
    ui->TubeType->addItem(DERK_PE);
    ui->TubeType->addItem(DERK_B);
    ui->checkAutoNumber->setChecked(false);
    on_checkQuickTest_clicked();

    //get paths and define file names
    //appDir = QCoreApplication.applicationDirPath()
    uTmaxDir = QDir::homePath();
    uTmaxDir.append("/uTmax_files/");
    dataFileName = uTmaxDir;
    dataFileName.append("data.csv");
    calFileName = uTmaxDir;
    calFileName.append("cal.txt");
    ReadCalibration();
    ReadDataFile();
    Update_adc_scales();
    //create plot tab
    ignoreIndexChange=false;
    on_Tabs_currentChanged(1); //add the first default plot tab
    ui->Tabs->setCurrentIndex(0);
    plotGridColor= QColor(0xff0000);
    plotAreaColor= QColor(0xf0f0f0);
    plotBackGroundColor=QColor(0xe0e0e0);
    plotAxisColor=QColor(0x000001);
    plotTextColor=QColor(0x000001);
    plotLegendTextColor=QColor(0x000001);
    plotLegendColor=QColor(0x808080);
    plotGridLine=2;
    SetStyleSheet();
    ui->Tabs->tabBar()->tabButton(0, QTabBar::RightSide)->deleteLater();
    ui->Tabs->tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    ui->Tabs->tabBar()->tabButton(1, QTabBar::RightSide)->deleteLater();
    ui->Tabs->tabBar()->setTabButton(1, QTabBar::RightSide, 0);
//    QList<QGroupBox *> widgets = this->findChildren<QGroupBox *>("plotCtl");
    //
    optimizer = new dr_optimize();
    readVersion=0;
    readChipCal=0;
    saveCalToChip=0;
    sendPing=0;
    calWizStep=false;
    enableDebugMsg=false;
    stopDebugMsg=false;
    SerialPortDiscovery();
}

MainWindow::~MainWindow()
{
    SortTubeData(0);
    SaveTubeDataFile();
    SaveCalFile();
    delete optimizer;
    //Close open port
    if (portInUse->isOpen()  ) {
        portInUse->close();
    }
    delete ui;
}

void MainWindow::Update_adc_scales(void) {
    float rv = float(calData.RaVal);
    float rvtop =  470000;
    if (options.uTracer6) rvtop=1000000;
    adc_scale.Va=(rvtop + rv)/rv;
    adc_scale.Vs=(rvtop + rv)/rv;
    adc_scale.Vn=(47 + 2)/2;
    if (options.uTracer6) adc_scale.Vn=(47+4.7)/4.7;
    adc_scale.Vsu=5.0/1023.0*(1.8+6.8)/1.8;
    if (options.uTracer6) adc_scale.Vsu=5.0/1023.0*(1+10)/1;

}
void MainWindow::SaveCalToChip(bool erase) {
    if (calData.Firmware<200) return;
    EraseChip=erase;
    saveCalToChip=true;
}

void MainWindow::SetStyleSheet(void) {
    int i,s,e;

    QFile uStyle;
    QString c;
    uStyle.setFileName(calData.Style);
    if (uStyle.open(QIODevice::ReadOnly)) {
        QString  StyleSheet = uStyle.readAll();
        this->setStyleSheet(StyleSheet);
        // Look for plotAreaColor
        i = StyleSheet.indexOf("/*plotAreaColor=");
        if(i>-1) {
            s = StyleSheet.indexOf("=",i) +1;
            e = StyleSheet.indexOf("*/",s);
            if ((s>-1) && (e>-1)) {
                c= StyleSheet.mid(s,e-s);
                plotAreaColor.setNamedColor(c);
            }
        }
        // Look for PlotBAckgroundColor
        i = StyleSheet.indexOf("/*plotBackGroundColor=");
        if(i>-1) {
            s = StyleSheet.indexOf("=",i) +1;
            e = StyleSheet.indexOf("*/",s);
            if ((s>-1) && (e>-1)) {
                c= StyleSheet.mid(s,e-s);
                plotBackGroundColor.setNamedColor(c);
            }
        }
        // Look for PlotAreaColor
        i = StyleSheet.indexOf("/*plotGridColor=");
        if(i>-1) {
            s = StyleSheet.indexOf("=",i) +1;
            e = StyleSheet.indexOf("*/",s);
            if ((s>-1) && (e>-1)) {
                c= StyleSheet.mid(s,e-s);
                plotGridColor.setNamedColor(c);
            }
        }
        // Look for PlotAxisColor
        i = StyleSheet.indexOf("/*plotAxisColor=");
        if(i>-1) {
            s = StyleSheet.indexOf("=",i) +1;
            e = StyleSheet.indexOf("*/",s);
            if ((s>-1) && (e>-1)) {
                c= StyleSheet.mid(s,e-s);
                plotAxisColor.setNamedColor(c);
            }
        }
        // Look for PlotTextColor
        i = StyleSheet.indexOf("/*plotTextColor=");
        if(i>-1) {
            s = StyleSheet.indexOf("=",i) +1;
            e = StyleSheet.indexOf("*/",s);
            if ((s>-1) && (e>-1)) {
                c= StyleSheet.mid(s,e-s);
                plotTextColor.setNamedColor(c);
            }
        }
        // Look for plotGridLine
        i = StyleSheet.indexOf("/*plotGridLine=");
        if(i>-1) {
            s = StyleSheet.indexOf("=",i) +1;
            e = StyleSheet.indexOf("*/",s);
            if ((s>-1) && (e>-1)) {
                c= StyleSheet.mid(s,e-s);
                plotGridLine=c.toInt();
            }
        }
        // Look for plotLegendColor
        i = StyleSheet.indexOf("/*plotLegendColor=");
        if(i>-1) {
            s = StyleSheet.indexOf("=",i) +1;
            e = StyleSheet.indexOf("*/",s);
            if ((s>-1) && (e>-1)) {
                c= StyleSheet.mid(s,e-s);
                plotLegendColor.setNamedColor(c);
            }
        }
        // Look for plotLegendTextColor
        i = StyleSheet.indexOf("/*plotLegendTextColor=");
        if(i>-1) {
            s = StyleSheet.indexOf("=",i) +1;
            e = StyleSheet.indexOf("*/",s);
            if ((s>-1) && (e>-1)) {
                c= StyleSheet.mid(s,e-s);
                plotLegendTextColor.setNamedColor(c);
            }
        }
        // Look for plotSHowValColor
        i = StyleSheet.indexOf("/*plotSHowValColor=");
        if(i>-1) {
            s = StyleSheet.indexOf("=",i) +1;
            e = StyleSheet.indexOf("*/",s);
            if ((s>-1) && (e>-1)) {
                c= StyleSheet.mid(s,e-s);
                plotSHowValColor.setNamedColor(c);
            }
        }
        //Initialize a Pen List-
        int r,g,b,h,s,v;
        int plots=16;
        QPen grPen;
        for(int i=0; i<50; i++) {
//            h = 360*(( (i + (i&1)*plots)>>1 ) % plots)/plots ;
            h = 360* (i  % plots)/plots ;
            //qDebug() << "h=" << h;
            s = 200;
            v = 230-100* floor(i / plots)/floor(50/plots);
            r = QColor::fromHsv(h,s,v,255).red() ;
            g = QColor::fromHsv(h,s,v,255).green() ;
            b = QColor::fromHsv(h,s,v,255).blue() ;
            grPen.setColor(QColor::fromRgb(r,g,b));
            grPen.setWidthF(2);
            if ( penList->length() == i) penList->append(grPen); else penList->replace(i, grPen);
        }
        //Create a black pen to fill unused entries
        QPen black;
        black.setColor(QColor::fromRgb(0,0,0,255));
        black.setWidthF(2);

        // Look for plotLineColor(#)
        QColor plcolor;
        QPen pen;
        QString plnum;
        for( int plc=0; plc<50; plc++) {
            plnum = "/*plotLineColor" + QObject::tr("%1").arg(plc) + "=";
            i = StyleSheet.indexOf(plnum);
            if(i>-1) {
                s = StyleSheet.indexOf("=",i) +1;
                e = StyleSheet.indexOf("*/",s);
                if ((s>-1) && (e>-1)) {
                    c= StyleSheet.mid(s,e-s);
                    plcolor.setNamedColor(c);
                    pen =  penList->at(plc);
                    pen.setColor(plcolor);
                    penList->replace(plc, pen);
                }
            }

            // Look for plotLineWidth
            i = StyleSheet.indexOf("/*plotLineWidth=");
            c="";
            if(i>-1) {
                s = StyleSheet.indexOf("=",i) +1;
                e = StyleSheet.indexOf("*/",s);
                if ((s>-1) && (e>-1)) {
                    c= StyleSheet.mid(s,e-s);
                }
            }
            if ( c!= "") {
                for( int plc=0; plc<50; plc++) {
                    pen =  penList->at(plc);
                    pen.setWidth(c.toUInt());
                    penList->replace(plc, pen);
                }
            }
        }
        uStyle.close();
        RePlot(activeStore);
    }
}

// Serial Port Management
void MainWindow::SerialPortDiscovery()
{
    //create a port object
    portInUse = new QSerialPort(this);
    portInUse->setBaudRate(QSerialPort::Baud9600);
    portInUse->setDataBits(QSerialPort::Data8);
    portInUse->setParity(QSerialPort::NoParity );
    portInUse->setStopBits(QSerialPort::OneStop);
    portInUse->setFlowControl(QSerialPort::NoFlowControl);
    //10 bits????
    connect(portInUse, SIGNAL(readyRead()), this, SLOT(readData()));

    //get a list of ports
    serPortInfo = QSerialPortInfo::availablePorts();
    bool found = false;
    foreach (QSerialPortInfo port, serPortInfo) {
        qDebug() << "SerialPortDiscovery: comport=" << comport <<  "available=" << port.portName();
        if (comport.contains(port.portName()) && (port.portName()!="")) {
            found = true;
            break;
        }
    }
//    if (found) {
        OpenComPort(&comport);
//    }
//    else  {
//        ui->statusBar->showMessage(tr("The requested COM port was not found, try a different one."));
//    }
}

void MainWindow::OpenComPort(const QString *portName)
{
    qDebug() <<"MainWindow::OpenComPort";
    //Close open port
    if (portInUse->isOpen()) {
        qDebug() << "Closing Port:" << portInUse->portName();
        portInUse->close();
    }
    //Open requested port
    comport = *portName;
    portInUse->setPortName(comport);
    qDebug() << "MainWindow::OpenComPort"  << portInUse->portName();
    QString msg;
    if ( portInUse->open(QIODevice::ReadWrite))
    {
        msg = QString("Port %1 opened").arg(comport);
        SaveCalFile(); //Update cal file with new port info
        ui->label_COM->setStyleSheet("QLabel { background-color : green; }");
//        ui->label_COM->setStyleSheet("QLabel { border-image: url(C:/Users/nick/QT/uTmax_2/release/Themes/img/GreenLed.png) 0 0 0 0 strech strech; }");
        rxLen=0;
        cmd.clear();
        sendADC=true;
    }
    else
    {
       msg = QString("Could not open %1").arg(comport);
       ui->label_COM->setStyleSheet("QLabel { background-color : red; }");
//       ui->label_COM->setStyleSheet("QLabel { border-image: url(C:/Users/nick/QT/uTmax_2/release/Themes/img/RedLed.png) 0 0 0 0 strech strech; }");
//       ui->label_COM->setStyleSheet("QLabel { border-image: url(./release/Themes/img/RedLed.png) 0 0 0 0 strech strech; }");
    }
    ui->statusBar->showMessage(msg);

    //Start RxData Timer
    if (timer==NULL) {
        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(RxData()));
        ui->statusBar->showMessage(tr("Starting up..."));
        timer_on=true;
        timeout=PING_TIMEOUT;
        rxLen=0;
        cmd.clear();
        sendADC=true;
        timer->start(TIMER_SET);
    }
}

void MainWindow::readData() {
    static bool comment=false;

    QByteArray d=portInUse->readAll();
    //qDebug()<< "readData d=" << d;
    if (fwdebug) {
        QTextStream(&logFile) << d;
        //logFile.write(d);
        //logFile.flush();
    }

    for(int i=0; i< d.length(); i++) {
        if (d.at(i) == '<' && status!=readcal) comment = true;
        if (comment) {
            if ((d.at(i)=='\r') || (d.at(i)=='\n')) {
                comment=false;
            }
        }
        else {
            if (d.at(i)=='\r' && status!=pgm_flash) d.remove(i,0);
            else if (d.at(i)=='\n'&& status!=pgm_flash) d.remove(i,0);
            else if (d.at(i)==' '&& status!=pgm_flash) d.remove(i,0);
            else RxString.append(d.at(i));
        }
    }
}


int MainWindow::RxPkt(int rxlen, QByteArray * cmd, QByteArray * response) {
    //if (portInUse->bytesAvailable()>0)   RxString.append(portInUse->readAll());
    if (rxlen==0)  return RXSUCCESS;
   //qDebug() << "RxPkt call: rxlen=" << rxlen << "cmd=" << cmd->constData() << "RxString=" <<  RxString << "length=" << RxString.length();
    timeout--;
    if (timeout ==0) {
        qDebug() << "RxPkt: TIMEOUT RxString=" << RxString <<  "rxlen=" << rxlen << "Got Length=" << RxString.length();
        RxString.clear();
        response->clear();
        return RXTIMEOUT;
    }
    //if (RxString.length()>0) qDebug() << "RxPkt: RxString:" << RxString << "length:" << RxString.length() << "cmd:" << ( * cmd);
    // uTmax6: a result string (commands 10 and 50) is 38 characters, or 42 with
    // Dekker's positive-grid extension board fitted (status bit 7 set, grid-current
    // word appended). Read the status byte first to know which, as Dekker's own GUI
    // does. uTmax 3.01 waited for exactly 38, so with the board every reply timed
    // out. See reply_format.h.
    int want = rxlen;
    if (cmd->length() > 0 && rxlen == cmd->length() + RESULT_LEN
            && RxString.length() >= cmd->length() + 2) {
        int st = replyStatus(RxString.at(cmd->length()), RxString.at(cmd->length() + 1));
        want = cmd->length() + resultLength(st);
    }
    if (RxString.length()==want) {
        if ( (cmd->length()==0) || RxString.startsWith(* cmd) ) {
            qDebug() << "RxPkt: reply OK, response:" << RxString;
            * response = RxString.mid(cmd->length(),want);
            update_failed=false;
            RxString.clear();
            if (rxlen==18) echoString = TxString;
            if (rxlen==38+18) {
                echoString = TxString;
                statusString = response->constData();
            }
            cmd->clear();
            return RXSUCCESS ;
        }
        else {
            qDebug() << "RxPkt: reply invalid. RxString:" << RxString << "TxString:" << TxString  ;
            RxString.clear();
            response->clear();
            return RXINVALID;
        }
    }
    return RXCONTINUE;
}

//---------------------------------------------
//The main control process
void MainWindow::RxData()
{   static uint openport_time=0;
    // ---------------------------------------------------
    // if port not open try to open it.
    if (!portInUse->isOpen())
    {
        if (openport_time==0) {
            openport_time=RETRYTIME;
            status = Idle;
            OpenComPort(&comport);
        } else openport_time--;
        return;
    }
    // ---------------------------------------------------
    // Check for the uTracer response
    int RxCode = RxPkt(rxLen, &cmd, &response);
    //if (cmd.length()>0) qDebug() << "RxData() cmd:" << cmd;
    // Check for timeout
    if (RxCode==RXTIMEOUT && timer_on)
    {
        ui->statusBar->showMessage(tr("No response from Tracer. A reconnect will be attempted. Try Debug->Read ADC or power cycle"));
        portInUse->close();
        response.clear();
        cmd.clear();
        RecallCtlState();
        rxLen=0;
        status = Idle;
        return;
    }
    //Check for invalid rsponse
    if (RxCode==RXINVALID)
    {
        ui->statusBar->showMessage(tr("Unexpected response from Tracer; power cycle and restart"));
        RecallCtlState();
        response.clear();
        cmd.clear();
        rxLen = 0;
        timer_on= false;
        status = Idle;
        return;
    }
    if (RxCode==RXCONTINUE) return;
    rxLen=0;
    cmd.clear();

    //serial comms heartbeat
    static uint heartBeatTime =HEARTBEAT;
    if (status==Idle) {
        if (heartBeatTime==0) {
            heartBeatTime=HEARTBEAT;
            sendPing=true;
        }
        else heartBeatTime--;
    } else heartBeatTime=HEARTBEAT;

    // ---------------------------------------------------
    //Main state machine
    //Check state
    switch (status) {
        case Idle:
            Idle_f();
            break;
        case readversion:
            ReadVersion();
            break;
        case readcal:
            ReadChipCalData();
            break;
        case  writeChipCal:
            if (response.at(0) == 0x31) ui->statusBar->showMessage("Sucessfully wrote calibration to chip");
            if (response.at(0) == 0x30) ui->statusBar->showMessage("Failed to write calibration data to chip");
            status= Idle;
            break;
        case WaitPing:
            Wait_Ping();
            break;
        case wait_adc:
            Wait_ADC();
            break;
        case Heating_wait00:
            Heating_Wait00();
            break;
        case Heating_wait_adc:
            Heating_Wait_ADC();
            break;
        case Heating:
            Heating_f();
            break;
        case heat_done:
            Heat_Done();
            break;
        case Sweep_set:
            Sweep_Set();
            break;
        case hold_ack:
            status=hold;
            timer_on=false;
            rxLen=0;
            cmd.clear();
            break;
        case hold :
            Hold();
            break;
        case Sweep_adc:
            Sweep_ADC();
            break;
        case HeatOff:
            HeatOff_f();
        break;
        case apply:
            Apply();
            break;
        case apply_measure:
            Apply_Measure();
            break;
        case apply_response:
            Apply_Response();
            break;
        case apply_end:
            Apply_End();
            break;
        case rxadc:
            RxADC();
            break;
        case erase_flash:
            Erase_Flash();
            break;
        case pgm_flash:
            Pgm_Flash();
            break;
    }
}
//------------------------------------------------------
// Read version from Chip
void MainWindow::ReadVersion() {

    calData.Firmware = response.toShort();
    status= Idle;
    ui->statusBar->showMessage("Firmware Version read");
    readChipCal=1;
}
// Read Calibration from Chip
void MainWindow::ReadChipCalData() {
    if (response.mid(0,4)!="\xff\xff\xff\xff") {
        uint16_t sum=0;
        for(int i=0 ; i<62; i++) {
            sum+=uint16_t(response.at(i)) & 0xff ;
        }
        char upper= toUpper14(sum);
        char lower= toLower14(sum);
        if ( (upper==response.at(62) && lower == response.at(63) ) ) {
            ui->statusBar->showMessage(tr("On chip calibration read OK"));
            calData.VaVal = from14bit(response.at(0), response.at(1))/1000;
            calData.VsVal = from14bit(response.at(2), response.at(3))/1000;
            calData.IaVal = from14bit(response.at(4), response.at(5))/1000;
            calData.IsVal = from14bit(response.at(6), response.at(7))/1000;
            calData.VsuVal = from14bit(response.at(8), response.at(9))/1000;
            calData.Vg0 = from14bit(response.at(10), response.at(11))/1000;
            calData.Vgm = from14bit(response.at(12), response.at(13))/1000;
            //calData.Vg05Val = from14bit(response.at(10), response.at(11));
            //calData.Vg1Val = from14bit(response.at(12), response.at(13));
            //calData.Vg4Val = from14bit(response.at(14), response.at(15));
            //calData.Vg40Val = from14bit(response.at(16), response.at(17));
            calData.VnVal = from14bit(response.at(14), response.at(15))/1000;
            calData.RaVal = from14bit(response.at(16), response.at(17));
            calData.VaMax = from14bit(response.at(18), response.at(19));
            calData.IaRsense = from14bit(response.at(20), response.at(21))/100;
            calData.IsRsense = from14bit(response.at(22), response.at(23))/100;
            calData.VgMax = from14bit(response.at(24), response.at(25));
        }
        else
             ui->statusBar->showMessage(tr("Error when reading on chip calibration data"));
    }
    status= Idle;
}

uint8_t MainWindow::toUpper14(float f) {
    short c = short(round(f));
    return(uint8_t(((c >> 7) & 0x7f )  + 0x30));
}
uint8_t MainWindow::toLower14(float f) {
    short c = short(round(f));
    return(uint8_t((c & 0x7f) + 0x30));
}

float MainWindow::from14bit(uint8_t upper, uint8_t lower) {
    short x=0;
    x =  short(lower - 0x30)  & 0x7f; //process lower byte
    x |=  (short(upper - 0x30) << 7) & 0x3fff;
    if (x & 0x2000) x|= 0xe000; //sign extend
    return(float(x));
}

//------------------------------------------------------
// Write Calibration to Chip
void MainWindow::WriteChipCalData()
{
    if (calData.Firmware==1) return;
    cmd = TxString = "51";
    if (EraseChip) ui->statusBar->showMessage("Erasing Calibration");
    else  {
        ui->statusBar->showMessage("Saving Calibration");
        // we can use just the lower 14 bits of the 16 bit short as the range is +/1000
        // upper is bits 13:7 + 0x20, lower is 6:0 +0x20;
        TxString+= toUpper14(calData.VaVal*1000);
        TxString+= toLower14(calData.VaVal*1000);
        TxString+= toUpper14(calData.VsVal*1000);
        TxString+= toLower14(calData.VsVal*1000);
        TxString+= toUpper14(calData.IaVal*1000);
        TxString+= toLower14(calData.IaVal*1000);
        TxString+= toUpper14(calData.IsVal*1000);
        TxString+= toLower14(calData.IsVal*1000);
        TxString+= toUpper14(calData.VsuVal*1000);
        TxString+= toLower14(calData.VsuVal*1000);
        TxString+= toUpper14(calData.Vg0*1000);
        TxString+= toLower14(calData.Vg0*1000);
        TxString+= toUpper14(calData.Vgm*1000);
        TxString+= toLower14(calData.Vgm*1000);
//        TxString+= toUpper14(calData.Vg05Val);
//        TxString+= toLower14(calData.Vg05Val);
//        TxString+= toUpper14(calData.Vg1Val);
//        TxString+= toLower14(calData.Vg1Val);
//        TxString+= toUpper14(calData.Vg4Val);
//        TxString+= toLower14(calData.Vg4Val);
//        TxString+= toUpper14(calData.Vg40Val);
//        TxString+= toLower14(calData.Vg40Val);
        TxString+= toUpper14(calData.VnVal*1000);
        TxString+= toLower14(calData.VnVal*1000);
        TxString+= toUpper14(calData.RaVal);
        TxString+= toLower14(calData.RaVal);
        TxString+= toUpper14(calData.VaMax);
        TxString+= toLower14(calData.VaMax);
        TxString+= toUpper14(calData.IaRsense*100);
        TxString+= toLower14(calData.IaRsense*100);
        TxString+= toUpper14(calData.IsRsense*100);
        TxString+= toLower14(calData.IsRsense*100);
        TxString+= toUpper14(calData.VgMax);
        TxString+= toLower14(calData.VgMax);
    }
    for(int i=TxString.length(); i<64; i++) TxString.append(0xff);
    uint16_t sum=0;
    for(int i=2 ; i<2+62; i++) {
        sum+=uint16_t(TxString.at(i)) & 0xff ;
    }
    TxString+= toUpper14(sum);
    TxString+= toLower14(sum);
    //qDebug() << "WriteChipCal sum=" << sum <<"upper=" << uint8_t(TxString.at(64)) << "lower=" << uint8_t(TxString.at(65));
    status=writeChipCal;
    timer_on=true;
    timeout = PING_TIMEOUT;
    cmd="51";
    rxLen=3; //cmd + 1 byte status
    sendSer();
}

void MainWindow::Idle_f() {
    rxLen = 0;
    timer_on=false;
    time=0;
    if (do_fw_update) {
        //enable bootloader and reset target
        do_fw_update=false;
        cmd=TxString="59";
        rxLen=2;
        if (update_failed) rxLen = 0; //don't expect a response
        sendSer();
        status=erase_flash;
        timeout = PING_TIMEOUT;
        timer_on=false;
        return;
    }
    if (enableDebugMsg) {
        //enable debug and pretty print
        enableDebugMsg=false;
        if ( calData.Firmware==1 || calData.Firmware==100) return;
        cmd=TxString="5556";
        rxLen = 4;
        sendSer();
        status=Idle;
        timeout = PING_TIMEOUT;
        timer_on=false;
        return;
    }
    if (stopDebugMsg) {
        //disable debug and pretty print
        stopDebugMsg=false;
        if ( calData.Firmware==1 || calData.Firmware==100) return;
        cmd=TxString="5758";
        rxLen = 4;
        sendSer();
        status=Idle;
        timeout = PING_TIMEOUT;
        timer_on=false;
        return;
    }
    if (sendADC)
    {
        ui->statusBar->showMessage(tr("ADC Read, setting up..."));
        cmd = TxString="500000000000000000";
        rxLen = TxString.length()+38;
        heat=0;
        sendSer();
        sendADC=false;
        status=wait_adc;
        timeout = PING_TIMEOUT;
        timer_on=true;
        return;
    }
    if (sendPing)
    {
        ui->statusBar->showMessage("Send Ping");
        cmd = TxString="300000000000000000";
        rxLen = TxString.length();
        sendPing=false;
        status=WaitPing;
        timeout = PING_TIMEOUT;
        timer_on=true;
        sendSer();
        return;
    }
    if (readVersion) {
        readVersion=0;
        cmd = TxString ="54";
        rxLen=6;
        status=readversion;
        timer_on=true;
        timeout = PING_TIMEOUT;
        sendSer();
        return;
    }
    if (readChipCal && calData.Firmware >= 200) {
        readChipCal=0;
        cmd = TxString ="52";
        rxLen=66;
        status=readcal;
        timer_on=true;
        timeout = PING_TIMEOUT;
        sendSer();
        return;
    }
    if (saveCalToChip) {
        saveCalToChip=0;
        WriteChipCalData();
        return;
    }
    if (HtrHoldEnd) {
        timer_on=true;
        timeout = PING_TIMEOUT;
        ui->HeaterProg->setValue(0);
        heat=0;
        HtrHoldEnd = false;
        cmd = TxString ="400000000000000000";
        rxLen=18;
        sendSer();
    }
    if (calWizStep) {
        calWizStep=0;
        TxString="0000000000";
        ::snprintf(buf, 3, "%02X", lim[options.Ilimit]);
        TxString+=buf;
        ::snprintf(buf, 3, "%02X", avg[0]);
        TxString+=buf;
        ::snprintf(buf, 3, "%02X", Ir[0]);
        TxString+=buf;
        ::snprintf(buf, 3, "%02X", Ir[0]);
        TxString+=buf;
        cmd =TxString;
        rxLen=18;
        status=apply;
        sendSer();
        timer_on=true;
        timeout = PING_TIMEOUT;
    }
    if (startSweep>0)
    {
        startSweep-=1;
        SaveCtlState();
        CreateTestVectors();
        StoreData(false); //Init data store
        VsStep=0;
        VgStep=0;
        VaStep=0;
        curve=0;
        if (heat<=HEAT_CNT_MAX) {
            ui->statusBar->showMessage(tr("Heating setup"));
            status = Heating_wait00;
            heat=0;
            ui->HeaterProg->setValue(1);
        }
        else {
            ui->statusBar->showMessage(tr("Sweep setup"));
            status = Sweep_set;
            delay=(options.Delay*100)/(TIMER_SET/10);
        }
        ui->CaptureProg->setValue(1);
        TxString="0000000000";
        ::snprintf(buf, 3, "%02X", lim[options.Ilimit]);
        TxString+=buf;
        ::snprintf(buf, 3, "%02X", avg[options.AvgNum]);
        TxString+=buf;
        ::snprintf(buf, 3, "%02X", Ir[options.IsRange]);
        TxString+=buf;
        ::snprintf(buf, 3, "%02X", Ir[options.IaRange]);
        TxString+=buf;
        cmd =TxString;
        rxLen=18;
        sendSer();
        timer_on=true;
        timeout = PING_TIMEOUT;
    }
}

void MainWindow::Wait_Ping() {
//qDebug()<<"WaitPing";
        status=Idle;
        ui->statusBar->showMessage(tr("Ping OK"));
        timer_on=false;
        timeout = PING_TIMEOUT;
}

void MainWindow::Wait_ADC() {
    //qDebug()<<"Wait_adc";
        saveADCInfo(&response);
        ui->statusBar->showMessage(tr("Ready"));
        timer_on=false;
        timeout = PING_TIMEOUT;
        status=Idle;
        if (calData.Firmware > 100) readVersion=1;
}

void MainWindow::Heating_Wait00() {
    //qDebug()<<"Heating_wait00";
        ui->statusBar->showMessage(tr("Heating, get ADC info"));
        status=Heating_wait_adc;
        cmd = TxString="500000000000000000";
        rxLen = TxString.length()+38;
        sendSer();
        timer_on=true;
        timeout=PING_TIMEOUT;
}

void MainWindow::Heating_Wait_ADC() {
    //qDebug()<<"Heating_wait_adc";
        VgNow=ui->VgStart->text().toFloat();
        saveADCInfo(&response);
        heat=0;
        status=Heating;
        ui->statusBar->showMessage(tr("Heating"));
        TxString="400000000000000000";
        rxLen = TxString.length();
        cmd = TxString;
        sendSer();
        timer_on=true;
        timeout = PING_TIMEOUT;
        heat=1;
}

void MainWindow::Heating_f() {
    //qDebug()<<"Heating";
    if (stop) {
        stop = false;
        End(1);
    }
    else if (heat<=HEAT_CNT_MAX) {
            if (startSweep > 0) {
                startSweep -=1;
                heat = HEAT_CNT_MAX;
            }
            ui->HeaterProg->setValue((100*heat)/HEAT_CNT_MAX);
            VfADC = GetVf((float)heat);
            ::snprintf(buf, 19, "40000000000000%04X", VfADC);
            TxString = buf;
            rxLen = TxString.length();
            cmd = TxString;
            timeout = PING_TIMEOUT;
            sendSer();
            heat++;
        }
        else {
            //qDebug()<<"Heating Done";
            ui->HeaterProg->setValue(100);
            timeout = PING_TIMEOUT;
            status=heat_done;
            timer_on=false;
            rxLen=0;
            cmd.clear();
        }
}

void MainWindow::Heat_Done() {
    //qDebug()<<"Heat_done";
    QString m = QString("Press 'start' when ready; heating for %1 secs").arg(time/1000);
    time+=TIMER_SET;
    ui->statusBar->showMessage(m);
    delay=(options.Delay*100)/(TIMER_SET/10);
    if (stop)
    {
        stop = false;
        End(1);
    }
    else if (startSweep>0 || time/1000==ui->HtrSecs->text().toInt()) {
        startSweep=0;
        if (dataStore->length()>0) dataStore->clear();
        rxLen=0;
        cmd.clear();
        curve=0;
        status=Sweep_set;
    }
}

void MainWindow::Sweep_Set() {
    if (stop) {
        //qDebug() << "Sweep set(stop)";
        stop = false;
        End(1);
    }
    else if (delay==0) {
        //qDebug() << "Sweep set";
        plot1.complete= false;
        status=hold_ack;
        ui->statusBar->showMessage(tr("Measuring"));
        VaNow = sweepList->at(curve).Va;
        VsNow = sweepList->at(curve).Vs;
        VgNow = sweepList->at(curve).Vg;
        // uTmax6: the same backstop as the hold path (3.01 had none here). voltage_limit.h
        if (!hvWithinLimit(VaNow, VsNow, options.uTracer6, calData.VaMax)) {
            ui->statusBar->showMessage(tr("Internal Error: Va or Vs excessive"));
            stop = false;
            End(1);
            return;
        }
        //qDebug() << "a=" << VaNow << "s=" << VsNow << "g=" << VgNow;
        //qDebug() << "VaStep=" << VaStep << "VsStep=" << VsStep << "VgStep=" << VgStep;
        VaADC = GetVa( VaNow );
        VsADC = GetVs( VsNow );
        VgADC = GetVg( VgNow );
        VfADC = GetVf( HEAT_CNT_MAX );
        status=Sweep_adc;
        timeout=ADC_READ_TIMEOUT;
        timer_on=true;
        ::snprintf(buf,19,"10%04X%04X%04X%04X",VaADC,VsADC,VgADC,VfADC );
        cmd = TxString=buf;
        rxLen=TxString.length()+38;
        sendSer();
    }
    else {
        ui->statusBar->showMessage(tr("Holding"));
        VaNow = sweepList->at(curve).Va;
        VsNow = sweepList->at(curve).Vs;
        VgNow = sweepList->at(curve).Vg;
        // uTmax6: was a fixed 425 V (the uTracer3's ceiling), which stopped every
        // delayed sweep above 425 V on a uTracer6. Now VaMax, capped per model. voltage_limit.h
        if (!hvWithinLimit(VaNow, VsNow, options.uTracer6, calData.VaMax)) {
            ui->statusBar->showMessage(tr("Internal Error: Va or Vs excessive"));
            stop = false;
            End(1);
            return;
        }
//                qDebug() << "a=" << VaNow << "s=" << VsNow << "g=" << VgNow;
//                qDebug() << "VaStep=" << VaStep << "VsStep=" << VsStep << "VgStep=" << VgStep;
        VaADC = GetVa( VaNow );
        VsADC = GetVs( VsNow );
        VgADC = GetVg( VgNow );
        VfADC = GetVf( HEAT_CNT_MAX );
        status=hold_ack;
        timeout=ADC_READ_TIMEOUT + (options.Delay*100)/(TIMER_SET/10);
        timer_on=true;
        sprintf(buf,"20%04X%04X%04X%04X",VaADC,VsADC,VgADC,VfADC );
        cmd = TxString=buf;
        rxLen=TxString.length();
        sendSer();
    }
}

void MainWindow::Hold() {
    if (stop) {
        stop = false;
        ui->statusBar->showMessage(tr("Abort:Heater off"));
        End(1);
        return;
    }
    if (delay==0) {
        status=Sweep_adc;
        ui->statusBar->showMessage(tr("Setting measurement parameters"));
        //qDebug() << "a=" << VaNow << "s=" << VsNow << "g=" << VgNow;
        //qDebug() << "VaStep=" << VaStep << "VsStep=" << VsStep << "VgStep=" << VgStep;
        VaNow = sweepList->at(curve).Va;
        VsNow = sweepList->at(curve).Vs;
        VgNow = sweepList->at(curve).Vg;
        // uTmax6: was a fixed 425 V (the uTracer3's ceiling), which stopped every
        // delayed sweep above 425 V on a uTracer6. Now VaMax, capped per model. voltage_limit.h
        if (!hvWithinLimit(VaNow, VsNow, options.uTracer6, calData.VaMax)) {
            ui->statusBar->showMessage(tr("Internal Error: Va or Vs excessive"));
            stop = false;
            //float v = VaNow > VsNow ? VaNow : VsNow;
            //HV_Discharge_Timer = distime = DWait(v);
            ui->statusBar->showMessage(tr("Abort:Heater off"));
            ui->checkHtrHold->setCheckState(Qt::Unchecked);
            End(1);
            return;
        }

        VaADC = GetVa( VaNow );
        VsADC = GetVs( VsNow );
        VgADC = GetVg( VgNow );
        VfADC = GetVf( HEAT_CNT_MAX );
        timeout=ADC_READ_TIMEOUT;
        timer_on=true;
        sprintf(buf,"10%04X%04X%04X%04X",VaADC,VsADC,VgADC,VfADC );
        cmd = TxString=buf;
        rxLen = TxString.length()+38;
        sendSer();
    }
    else {
        ui->statusBar->showMessage(tr("Holding"));
        //qDebug() << "delay=" << delay;
        delay--;
        status=hold;
        rxLen =0;
        cmd ="";
    }
}

void MainWindow::Sweep_ADC() {
    //check current limit
    results.ignore=0;
    saveADCInfo(&response);
    if (LOGGING) QTextStream(&logFile) << "Sweep_adc @"<< curve <<"\n";


    //check current limit
    ui->MsgLabel->setText("");
    // uTmax6: 11H, or 91H with the positive-grid board (3.01 compared with "11" only)
    if (response.size() >= 2 && statusIsCurrentLimit(replyStatus(response.at(0), response.at(1)))) {
        if (LOGGING) QTextStream(&logFile) << "+Current limit\n";
        ui->MsgLabel->setText("Current Limit!");
        results.ignore|=IGNORE_CURRENT;
        if (options.AbortOnLimit==true) {
            qDebug() << "Sweep_adc: Current Limit Abort";
            status=HeatOff;
            End(1);
            return;
        }
    }
    StoreData(true); //Add to data store
    curve++;

    if (curve < sweepList->length()) {
        if (LOGGING) QTextStream(&logFile) << "+progress\n";
        int progress=(100*curve)/sweepList->length();
        ui->CaptureProg->setValue(progress);
        QString msg = QString("Sweep %1 % done").arg(progress);
        ui->statusBar->showMessage(msg);
        delay=(options.Delay*100)/(TIMER_SET/10);

        //qDebug() << "From Sweep_adc to Sweep_set";
        status=Sweep_set;
        timeout=ADC_READ_TIMEOUT;
        rxLen=0;
        cmd.clear();
    }
    else { //sweep complete
        if (LOGGING) QTextStream(&logFile) << "+sweep complete, heat off\n";
        ui->CaptureProg->setValue(100);
        ui->statusBar->showMessage(tr("Sweep complete"));
        //Save data
        if (ui->checkAutoNumber->isChecked()) {
            if (!ui->checkQuickTest->isChecked()) on_actionSave_plot_triggered();
            on_actionSave_Data_triggered();
            int fn = ui->AutoNumber->text().toInt(&ok);
            ui->AutoNumber->setText(QString::number(fn+1));
        }
        //optimize
        if ( !ui->checkQuickTest->isChecked() && (dataStore->length()>5) && ui->TubeType->currentText()!=NONE) {
            optimizer->Optimize(dataStore, ui->TubeType->currentIndex(), 1, VaSteps, VsSteps, VgSteps);
            if (optimizer->Status())
                ui->statusBar->showMessage("Info: model fit is poor. Try a different model or change your sweep settings");
            else
                ui->statusBar->showMessage("Info: Model fit OK");
            updateLcdsWithModel();
            RePlot(dataStore);
        }
        End(0);
    }
}

void MainWindow::End(bool heat_off) {
    ui->statusBar->showMessage("Wait for voltage to be removed");
    status=HeatOff;
    ui->CaptureProg->setValue(100);
    //check for heater hold
    if (ui->checkHtrHold->isChecked() && !heat_off) {
        VfADC = GetVf( HEAT_CNT_MAX );
    } else {
        ui->HeaterProg->setValue(0);
        VfADC = GetVf( 0 );
        heat=0;
    }
    //measure cmd to set HV and heater then wait
    VaADC = GetVa( 4 );
    VsADC = GetVs( 4 );
    VgADC = GetVg( 0 );
    timeout=ADC_READ_TIMEOUT;
    timer_on=true;
    sprintf(buf,"10%04X%04X%04X%04X",VaADC,VsADC,VgADC,VfADC );
    cmd = TxString=buf;
    rxLen = TxString.length()+38;
    sendSer();
}

void MainWindow::HeatOff_f() {
    timer_on=false;
    RecallCtlState();
    ui->statusBar->showMessage(tr("Ready"));
    status=Idle;
    rxLen=0;
}

void MainWindow::on_actionCalibration_Wizard_triggered()
{
    if (portInUse->isOpen()) {
        Cal_Wizard *w =  new Cal_Wizard(this);
        connect(this, SIGNAL(Next()), w, SLOT(on_Next_pushButton_clicked()));
        w->show();
    } else ui->statusBar->showMessage("Cannot start wizard as no COM port is open.");
}

settings_t aset;
void MainWindow::GetADC() {
    cmd = TxString="500000000000000000";
    rxLen = TxString.length()+38;
    heat=0;
    sendSer();
    status=rxadc;
    timeout = PING_TIMEOUT;
    timer_on=true;
}

void MainWindow::RxADC() {
    status=Idle;
    saveADCInfo(&response);
    emit Next();
}

void MainWindow::Apply_Mode(settings_t s) {
    aset = s;
    calWizStep=true;
}
uint am_delay;

//set tracer volatges
void MainWindow::Apply() {
    uint x;
    if (aset.VaVsTrue) {
        VaADC= GetVa(aset.Va);
        VsADC= GetVs(aset.Vs);
    }
    else {
        x =(uint)(1023.0/5.0*aset.Va/adc_scale.Va);
        VaADC = (x>1023?1023:x);
        x =(uint)(1023.0/5.0*aset.Vs/adc_scale.Vs);
        VsADC = (x>1023?1023:x);
    }
    VgADC = GetVg( aset.Vg );
    VfADC = GetVf( 0 );
    timeout=ADC_READ_TIMEOUT;
    timer_on=true;
    ::snprintf(buf,19,"20%04X%04X%04X%04X",VaADC,VsADC,VgADC,VfADC );
    cmd = TxString=buf;
    rxLen=TxString.length();
    status=apply_measure ;
    am_delay=aset.delay*1000/TIMER_SET;
    sendSer();
}
//pause for delay time with set voltages then make a measurement
void MainWindow::Apply_Measure() {
    uint x;
    if (am_delay > 0) {
        am_delay--;
        return;
    }
    if (aset.VaVsTrue) {
        VaADC= GetVa(aset.Va);
        VsADC= GetVs(aset.Vs);
    }
    else {
        x =(uint)(1023.0/5.0*aset.Va/adc_scale.Va);
        VaADC = (x>1023?1023:x);
        x =(uint)(1023.0/5.0*aset.Vs/adc_scale.Vs);
        VsADC = (x>1023?1023:x);
    }
    VgADC = GetVg( aset.Vg );
    VfADC = GetVf( 0 );
    timeout=ADC_READ_TIMEOUT;
    timer_on=true;
    ::snprintf(buf,19,"10%04X%04X%04X%04X",VaADC,VsADC,VgADC,VfADC );
    cmd = TxString=buf;
    rxLen=TxString.length()+38;
    status=apply_response ;
    sendSer();
}
//send the measurement results
void MainWindow::Apply_Response() {
    saveADCInfo(&response);
    VgNow=aset.Vg;
    GetReal();
    //Apply safe voltages
    //uint x;
    if (aset.VaVsTrue) {
        VaADC= GetVa(4);
        VsADC= GetVs(4);
    }
    else {
        VaADC= GetVa(5);
        VsADC= GetVs(5);
    }
    VgADC = GetVg( 0 );
    VfADC = GetVf( 0 );
    timeout=ADC_READ_TIMEOUT;
    timer_on=true;
    ::snprintf(buf,19,"20%04X%04X%04X%04X",VaADC,VsADC,VgADC,VfADC );
    cmd = TxString=buf;
    rxLen=TxString.length();
    status=apply_end;
    sendSer();
}
void MainWindow::Apply_End() {
    status=Idle ;
    emit Next();
}

void MainWindow::CalcVnCal(float m) {
    calData.VnVal = m / adc_real.Vn ;
}

void MainWindow::CalcVpCal(float m) {
    calData.VsuVal = m / adc_real.Vsu;
}

void MainWindow::CalcVaCal(float m) {    
    calData.VaVal = m / (adc_real.Va+adc_real.Vsu-Vdi+Vdar);
}

void MainWindow::CalcVsCal(float m) {
    calData.VsVal =  m / (adc_real.Vs+adc_real.Vsu-Vdi+Vdar);
}
/*
void MainWindow::CalcVg05Cal(float m) {
    calData.Vg05Val = FromRealVal(0.5/m) ;
}

void MainWindow::CalcVg1Cal(float m) {
    calData.Vg1Val = FromRealVal(1/m) ;
}

void MainWindow::CalcVg4Cal(float m) {
    calData.Vg4Val = FromRealVal(4/m) ;
}

void MainWindow::CalcVg40Cal(float m) {
    calData.Vg40Val = FromRealVal(40/m) ;
}
*/
void MainWindow::CalcVg(float v1, float v2) {
    calData.Vgm=(v2-v1)/(40-0.5);
    calData.Vg0= calData.Vgm*40-v2;
    qDebug() << "Vgm="<< calData.Vgm << "Vg0=" << calData.Vg0;
}

void MainWindow::CalcIaCal(float m) {
    calData.IaVal = m/adc_real.Ia/10 ;
}

void MainWindow::CalcIsCal(float m) {
    calData.IsVal = m/adc_real.Is/10 ;
}

void MainWindow::sendSer()
{
    qDebug() << "sendSer Tx:" << TxString << "TxLen:" << TxString.length() << "cmd:" << cmd << "rxLen:" << rxLen;
    //RxString.clear();
    portInUse->write(TxString);
}

/*int MainWindow::DWait(float v) {
    if (calData.Firmware<2) return((int)(DISTIME * v/400));
    else return((int)(DISTIME * v/400/3));
}*/

// ======================
// Plot new point
// ======================

void MainWindow::RePlot(QList<results_t> * dataSet) {
    for(int i=0; i<plotTabs.length();i++) {
        plotTabs.at(i)->AxisColor(plotAxisColor);
        plotTabs.at(i)->RectColor(plotAreaColor);
        plotTabs.at(i)->BackGroundColor(plotBackGroundColor);
        plotTabs.at(i)->GridColor(plotGridColor);
        plotTabs.at(i)->TextColor(plotTextColor);
        plotTabs.at(i)->setGridLine(plotGridLine);
        plotTabs.at(i)->setLegendTextColor(plotLegendTextColor);
        plotTabs.at(i)->setLegendColor(plotLegendColor);
        plotTabs.at(i)->setShowValColor(plotSHowValColor);
        plotTabs.at(i)->updateColors();
    }
    if (dataSet==0) return;
    if (dataSet->length() <2) return;
    activeStore = dataSet;
    for(int i=0; i<plotTabs.length();i++) {
       plot1.penList = penList;
       plot1.penChange=false;
       plot1.dataSet = dataSet;
       plot1.VaSteps =VaSteps;
       plot1.VsSteps =VsSteps;
       plot1.VgSteps =VgSteps;
       plot1.VaStart =VaStart;
       plot1.VaEnd = VaEnd;
       plot1.VsStart =VsStart;
       plot1.VsEnd = VsEnd;
       plot1.VgStart =VgStart;
       plot1.VgEnd = VgEnd;
       plot1.tube =  ui->TubeSelector->currentText();
       plot1.type = ui->TubeType->currentText();
       plot1.VsEQVa = ui->checkVsEqVa->isChecked();
       plot1.optimizer= optimizer;
       plot1.Power=ui->PowLim->text().toFloat();
       plotTabs.at(i)->plotUpdate(&plot1);
    }
    if ( plot1.penChange ) SaveCalFile();

}

//---------------------------------------------------
// Manage results data pool
void MainWindow::StoreData(bool add)
{
    if(!add) {   //deletes data Store
        dataStore->clear();
        for (int i=0;i< plotTabs.length(); i++ ){
            plotTabs.at(i)->clearGraphs();
        }
        return;
    }
    ClearLCDs();
    results.Ia=adc_real.Ia;
    results.Is=adc_real.Is;
    results.Va=adc_real.Va;
    results.Vs=adc_real.Vs;
    results.Vg=adc_real.Vg;
    results.Vf=adc_real.Vf;
    results.gm_a=0;
    results.gm_b=0;
    results.ra_a=0;
    results.ra_b=0;
    results.mu_a=0;
    results.mu_b=0;
    //in lieu of model data, copy Ia & Is
    results.IaMod=0;
    results.IsMod=0;
//        results.IaMod=results.Ia;
//        results.IsMod=results.Is;
    if (LOGGING) QTextStream(&logFile) << "StoreData len"<< dataStore->length()<<"\n";
    ClearLCDs();
    ui->LCD1_label->setText("Va(a)");
    ui->LCD2_label->setText("Va(b)/g2");
    ui->LCD3_label->setText("-Vg1");
    ui->LCD4_label->setText("Ia(a)");
    ui->LCD5_label->setText("Ia(b)/g2");
    ui->LCD6_label->setText("Watts(a)");
    ui->LCD7_label->setText("Watts(b)");
    lcdVals[0] = adc_real.Va;
    lcdVals[1] = adc_real.Vs;
    lcdVals[2] = (-adc_real.Vg);
    lcdVals[3] = adc_real.Ia;
    lcdVals[4] = adc_real.Is;
    float Imax=  3600/calData.IaRsense;
    //power a
    float powera = adc_real.Ia * adc_real.Va/1000;
    if ( powera > ui->PowLim->text().toFloat() ) results.ignore|=IGNORE_POWER; //ignore this point
    lcdVals[5] = powera;
    if ( powera > ui->PowLim->text().toFloat() ) ui->lcd6->setPalette((Qt::red));
        else ui->lcd6->setPalette(Qt::green);

    //power b
    float powerb = adc_real.Is * adc_real.Vs/1000;
    lcdVals[6] = powerb;
    if ( powerb > ui->PowLim->text().toFloat() ) ui->lcd7->setPalette((Qt::red));
        else ui->lcd7->setPalette(Qt::green);
    if (adc_real.Ia<Imax*0.8) ui->lcd4->setPalette(Qt::green);
    else if (adc_real.Ia < Imax*0.9) ui->lcd4->setPalette(Qt::yellow);
    else ui->lcd4->setPalette(Qt::red);

    if (adc_real.Is<Imax*0.8) ui->lcd5->setPalette(Qt::green);
    else if (adc_real.Is<Imax*0.9) ui->lcd5->setPalette(Qt::yellow);
    else ui->lcd5->setPalette(Qt::red);
    writeLCDs();
    dataStore->append(results);
    //
    //check for end of quicktest
    if (ui->checkQuickTest->isChecked()) {
        if (QDEBUG)  qDebug() << "TubeType:" << ui->TubeType->currentText() << '\n';
        if (dataStore->length()==sweepList->length()) {
            QuickRes.clear();
            ClearLCDs();
            float Is_avg =0;
            float Ia_avg =0;
            int avgsiz= dataStore->length()-1;
            for (int i=0; i<avgsiz; i++) {
                Is_avg+=dataStore->at(i).Is;
                Ia_avg+=dataStore->at(i).Ia;
            }
            Ia_avg/=avgsiz;
            Is_avg/=avgsiz;
            //mu, gm, Ia
            if (ui->TubeType->currentText()==DIODE || ui->TubeType->currentText()==DUAL_DIODE) {
                ui->LCD1_label->setText("Ra(a)");
                ui->LCD3_label->setText("Ia(a)");
                ui->LCD5_label->setText("DC(a)");
                //Ra
                float Ra=0;
                if ((dataStore->at(1).Ia-dataStore->at(0).Ia)>0) {
                    Ra = (dataStore->at(1).Va-dataStore->at(0).Va)/(dataStore->at(1).Ia-dataStore->at(0).Ia);
                    lcdVals[0] = Ra;
                }
                //Ia avg
                lcdVals[2] = Ia_avg;
                //Bias
                lcdVals[4] = dataStore->at(4).Ia;
                float Rb=0;
                if (ui->TubeType->currentText()==DUAL_DIODE) {
                    //Rb
                    if ((dataStore->at(3).Is-dataStore->at(2).Is)>0) {
                        Rb= (dataStore->at(3).Vs-dataStore->at(2).Vs)/(dataStore->at(3).Is-dataStore->at(2).Is);
                        lcdVals[1] = Rb;
                    }
                    ui->LCD2_label->setText("Ra(b)");
                    ui->LCD4_label->setText("Ia(b)");
                    ui->LCD6_label->setText("DC(b)");
                    lcdVals[3] = Is_avg;
                    lcdVals[5] = dataStore->at(4).Is;
                }
                QTextStream(&QuickRes)  << "Ia(a)=" << Ia_avg
                                        << " Ia(b)=" << Is_avg
                                        << " Ra(a)=" << Ra
                                        << " Ra(b)=" << Rb
                                        << " Bias=" << dataStore->at(4).Ia;
            }
            else if (ui->TubeType->currentText()=="Koren Triode") {
                ui->LCD1_label->setText("RaK");
                ui->LCD2_label->setText("Gm-mS");
                ui->LCD3_label->setText("Mu");
                ui->LCD4_label->setText("Ia");
                float Ra = (dataStore->at(1).Va-dataStore->at(0).Va)/(dataStore->at(1).Ia-dataStore->at(0).Ia);
                float Gm = (dataStore->at(5).Ia-dataStore->at(4).Ia)/(dataStore->at(5).Vg-dataStore->at(4).Vg);
                lcdVals[0] = Ra;
                lcdVals[1] = Gm;
                float GmMin = ui->gm1Typ->text().toFloat() * (1- ui->gm1Delta->text().toFloat()/100);
                float GmMax = ui->gm1Typ->text().toFloat() * (1+ ui->gm1Delta->text().toFloat()/100);
                if ( Gm < GmMin || Gm>GmMax )
                                        ui->lcd2->setPalette(Qt::red);
                                    else
                                        ui->lcd2->setPalette(Qt::green);

                lcdVals[2] = Ra*Gm;
                lcdVals[3] = Ia_avg;

                //Emmission
                ui->LCD9_label->setText("DC");
                lcdVals[5] = dataStore->at(6).Ia;

                QTextStream(&QuickRes) << "RaK=" << Ra
                                       << " Gm=" << Gm*10
                                       << " Mu=" << Ra*Gm
                                       << " Ia(a)=" << Ia_avg
                                       << " Bias=" << dataStore->at(6).Ia;
            }
            else if (ui->TubeType->currentText()=="Koren Dual Triode") {
                float Ra = (dataStore->at(1).Va-dataStore->at(0).Va)/(dataStore->at(1).Ia-dataStore->at(0).Ia);
                float Gm = (dataStore->at(5).Ia-dataStore->at(4).Ia)/(dataStore->at(5).Vg-dataStore->at(4).Vg);
                float Gmb = (dataStore->at(5).Is-dataStore->at(4).Is)/(dataStore->at(5).Vg-dataStore->at(4).Vg);
                float GmMin = ui->gm1Typ->text().toFloat() * (1- ui->gm1Delta->text().toFloat()/100);
                float GmMax = ui->gm1Typ->text().toFloat() * (1+ ui->gm1Delta->text().toFloat()/100);
                float Rb = (dataStore->at(3).Vs-dataStore->at(2).Vs)/(dataStore->at(3).Is-dataStore->at(2).Is);
                lcdVals[0] = Ra;
                lcdVals[1] = Rb;
                lcdVals[2] = Gm;
                lcdVals[3] = Gmb;
                if ( Gm < GmMin || Gm > GmMax )
                    ui->lcd3->setPalette(Qt::red);
                else
                    ui->lcd3->setPalette(Qt::green);
                lcdVals[4] = Ra*Gm;
                GmMin = ui->gm2Typ->text().toFloat() * (1- ui->gm2Delta->text().toFloat()/100);
                GmMax = ui->gm2Typ->text().toFloat() * (1+ ui->gm2Delta->text().toFloat()/100);
                if ( Gmb < GmMin || Gmb > GmMax )
                    ui->lcd4->setPalette(Qt::red);
                else
                    ui->lcd4->setPalette(Qt::green);
                lcdVals[5] = Rb*Gmb;
                lcdVals[6] = dataStore->at(6).Ia;
                lcdVals[7] = dataStore->at(6).Is;
                ui->LCD1_label->setText("Ra K(a)");
                ui->LCD3_label->setText("Gm mS(a)");
                ui->LCD5_label->setText("Mu(a)");
                ui->LCD7_label->setText("Bias(a)");
                ui->LCD2_label->setText("Ra K(b)");
                ui->LCD4_label->setText("Gm mS(b)");
                ui->LCD6_label->setText("Mu(b)");
                ui->LCD8_label->setText("Bias(b)");
                ui->LCD9_label->setText("");

                QTextStream(&QuickRes)
                    << "Ra(a)K" << "\t"
                    << "gm(a)" <<  "\t"
                    << "Mu(a)" <<  "\t"
                    << "Ia(a)" <<  "\t"
                    << "Bias(a)" << "\t"
                    << "Ra(b)K" <<  "\t"
                    << "Gm(b)" <<  "\t"
                    << "Mu(b)" <<  "\t"
                    << "Ia(b)" <<  "\t"
                    << "Bias(b)" << "\n"
                    << round(Ra)  << "\t"
                    << round(Gm*10)/10  << "\t"
                    << round(Ra*Gm*10)/10  << "\t"
                    << round(10*Ia_avg)/10  << "\t"
                    << round(10*dataStore->at(6).Ia)/10 << "\t"
                    << round(Rb) << "\t"
                    << round(10*Gmb)/10 << "\t"
                    << round(10*Rb*Gmb)/10 << "\t"
                    << round(10*Is_avg)/10 << "\t"
                    << round(10*dataStore->at(6).Is)/10;
            }
            else if (ui->TubeType->currentText()!=NONE || ui->checkQuickTest->isChecked() ) {
                //pentodes
                ui->LCD1_label->setText("Ra K");
                float Ra = (dataStore->at(1).Va-dataStore->at(0).Va)/(dataStore->at(1).Ia-dataStore->at(0).Ia);
                lcdVals[0] = Ra;

                ui->LCD2_label->setText("Ia/Vg2 mS");
                float Gm2 = (dataStore->at(3).Ia-dataStore->at(2).Ia)/(dataStore->at(3).Vs-dataStore->at(2).Vs);
                lcdVals[1] = Gm2;
                float GmMin = ui->gm2Typ->text().toFloat() * (1- ui->gm2Delta->text().toFloat()/100);
                float GmMax = ui->gm2Typ->text().toFloat() * (1+ ui->gm2Delta->text().toFloat()/100);
                if ( Gm2 < GmMin || Gm2 > GmMax )
                    ui->lcd2->setPalette(Qt::red);
                else
                    ui->lcd2->setPalette(Qt::green);

                ui->LCD3_label->setText("Gm mS");
                float Gm1 = (dataStore->at(5).Ia-dataStore->at(4).Ia)/(dataStore->at(5).Vg-dataStore->at(4).Vg);
                lcdVals[2] = Gm1;
                GmMin = ui->gm1Typ->text().toFloat() * (1- ui->gm1Delta->text().toFloat()/100);
                GmMax = ui->gm1Typ->text().toFloat() * (1+ ui->gm1Delta->text().toFloat()/100);
                if ( Gm1 < GmMin || Gm1 > GmMax )
                    ui->lcd3->setPalette(Qt::red);
                else
                    ui->lcd3->setPalette(Qt::green);

                ui->LCD4_label->setText("Mu");
                lcdVals[3] = Ra*Gm1;

                ui->LCD5_label->setText("Ia");
                lcdVals[4] = Ia_avg;

                ui->LCD6_label->setText("Is");
                lcdVals[5] = Is_avg;

                //Emmission
                ui->LCD8_label->setText("Ia @ Bias");
                lcdVals[7] = dataStore->at(6).Ia;
                ui->LCD9_label->setText("Ig2 @ Bias");
                lcdVals[8] = dataStore->at(6).Is;

                QTextStream(&QuickRes)
                        << "RaK" << "\t"
                        << " gm2" << "\t"
                        << " gm1" << "\t"
                        << " Mu" << "\t"
                        << " Ia" << "\t"
                        << " Ig2" << "\t"
                        << " Bias" << "\n"
                        << round(Ra) << "\t"
                        << round(10*Gm2)/10 << "\t"
                        << round(10*Gm1)/10 << "\t"
                        << round(10*Ra*Gm1)/10 << "\t"
                        << round(10*Ia_avg)/10 << "\t"
                        << round(10*Is_avg)/10 << "\t"
                        << round(10*dataStore->at(6).Ia)/10;

            }
        }
        writeLCDs();
    }
    else if  ( ( dataStore->length()>5) &&  dataStore->length()== sweepList->length() &&
                !ui->checkQuickTest->isChecked() ) { //Full sweep
            if (ui->TubeType->currentText()!=NONE ) {
                if (LOGGING) QTextStream(&logFile) << "+optimise\n";
                optimizer->Optimize(dataStore, ui->TubeType->currentIndex(), 1, VaSteps, VsSteps, VgSteps);
                if (optimizer->Status())
                    ui->statusBar->showMessage("Info: model fit is poor. Try a different model or change your sweep settings");
                else
                    ui->statusBar->showMessage("Model fit OK");
                updateLcdsWithModel();
            }
            if (LOGGING) QTextStream(&logFile) << "+replot\n";
            plot1.complete= true;
    }
    RePlot(dataStore);
    if (LOGGING) QTextStream(&logFile) << "+end StoreData\n";

}

void MainWindow::writeLCDs() {
    for(uint i =0; i<9; i++) {
        if (fabs(lcdVals[i]) > 9999 || (lcdVals[i]!=lcdVals[i])) lcds[i]->display("OL");
        else {
            //if (lcdVals[i] <1) f = 100;
            //else if (lcdVals[i] <10) f = 10; else f=1;
            //if (lcds[i]) lcds[i]->display(round(lcdVals[i]*f)/f);
            lcds[i]->display(lcdVals[i]);
        }
    }
}

void MainWindow::ClearLCDs() {
    for(uint i =0; i<9; i++) {
        lcds[i]->setPalette(Qt::green);
        lcdVals[i] = 0;
        lcds[i]->setDigitCount(DIGITCNT);
    }
    ui->LCD1_label->setText("");
    ui->LCD2_label->setText("");
    ui->LCD3_label->setText("");
    ui->LCD4_label->setText("");
    ui->LCD5_label->setText("");
    ui->LCD6_label->setText("");
    ui->LCD7_label->setText("");
    ui->LCD8_label->setText("");
    ui->LCD9_label->setText("");
    writeLCDs();
}

void MainWindow::updateLcdsWithModel() {

    float Ra,Gm;
    float Vca = ui->Vca->text().toFloat();
    float Vcs = ui->Vcs->text().toFloat();
    float Vcg = ui->Vcg->text().toFloat();

    ClearLCDs();
    if (LOGGING) QTextStream(&logFile) << "updateLCDsWithModel\n";

    // TODO : calculate correct values
    if (ui->TubeType->currentText()==DIODE) {
        if (LOGGING) QTextStream(&logFile) << "+diode\n";
        ui->LCD1_label->setText("Ra(a) K");

        Ra = optimizer->RaCalc(Vca, Vcs, Vcg);
        lcdVals[0] = Ra;
    }
    else if (ui->TubeType->currentText()==DUAL_DIODE) {
        if (LOGGING) QTextStream(&logFile) << "+dual diode\n";
        ui->LCD1_label->setText("Ra(a) K");
        ui->LCD2_label->setText("Ra(b) K");
        Ra = optimizer->RaCalc(Vca, Vcs, Vcg);
        lcdVals[0] = Ra;
        Ra = optimizer->RaCalcB(Vca, Vcs, Vcg);
        lcdVals[1] = Ra;
    }
    else if (ui->TubeType->currentText()==TRIODE) {
        if (LOGGING) QTextStream(&logFile) << "+triode\n";
        ui->LCD1_label->setText("Ra K");
        ui->LCD2_label->setText("Gm mS");
        ui->LCD3_label->setText("Mu");

        Ra = optimizer->RaCalc(Vca, Vcs, Vcg);
        if (LOGGING) QTextStream(&logFile) << "+opt RA triode\n";
        Gm = optimizer->GmCalc(Vca, Vcs, Vcg);
        if (LOGGING) QTextStream(&logFile) << "+opt GM triode\n";
        lcdVals[0] = Ra;
        lcdVals[1] = Gm;
        lcdVals[2] = Ra*Gm;
    }
    else if (ui->TubeType->currentText()==DUAL_TRIODE) {
        if (LOGGING) QTextStream(&logFile) << "+dual triode\n";
        ui->LCD1_label->setText("Ra(a) K");
        ui->LCD2_label->setText("Gm(a) mS(a)");
        ui->LCD3_label->setText("Mu(a)");
        ui->LCD4_label->setText("Ra(b) K");
        ui->LCD5_label->setText("Gm(b) mS");
        ui->LCD6_label->setText("Mu(b)");

        Ra = optimizer->RaCalc(Vca, Vcs, Vcg);
        Gm = optimizer->GmCalc(Vca, Vcs, Vcg);
        lcdVals[0] = Ra;
        lcdVals[1] = Gm;
        lcdVals[2] = Ra*Gm;

        Ra = optimizer->RaCalcB(Vca, Vcs, Vcg);
        Gm = optimizer->GmCalcB(Vca, Vcs, Vcg);
        lcdVals[3] = Ra;
        lcdVals[4] = Gm;
        lcdVals[5] = Ra*Gm;
    }
    else if (ui->TubeType->currentText()!=NONE) {
        if (LOGGING) QTextStream(&logFile) << "+pentode\n";
        //pentodes
        ui->LCD1_label->setText("Va");
        ui->LCD2_label->setText("Vs");
        ui->LCD3_label->setText("Vg");
        ui->LCD4_label->setText("Ia");
        ui->LCD5_label->setText("Gm mS");
        Gm = optimizer->GmCalc(Vca, Vcs, Vcg);
        lcdVals[0] = ui->Vca->text().toFloat();
        lcdVals[1] = ui->Vcs->text().toFloat();
        lcdVals[2] = ui->Vcg->text().toFloat();
        lcdVals[3] = optimizer->IaCalc(Vca, Vcs, Vcg);
        lcdVals[4] = Gm;
    }
    writeLCDs();
}
//---------------------------------------------------
// Transform ADC values into real data
void MainWindow::saveADCInfo(QByteArray * adc_pkt)
{
    adc_data.Ia = adc_pkt->mid(2,4).toInt(&ok,16);
    adc_data.Ia_raw = adc_pkt->mid(6,4).toInt(&ok,16);
    adc_data.Is = adc_pkt->mid(10,4).toInt(&ok,16);
    adc_data.Is_raw = adc_pkt->mid(14,4).toInt(&ok,16);
    adc_data.Va = adc_pkt->mid(18,4).toInt(&ok,16)/options.VaScale;
    adc_data.Vs = adc_pkt->mid(22,4).toInt(&ok,16);
    adc_data.Vsu = adc_pkt->mid(26,4).toInt(&ok,16);
    adc_data.Vn = adc_pkt->mid(30,4).toInt(&ok,16);
    adc_data.Gain_a = adc_pkt->mid(34,2).toInt(&ok,16);
    adc_data.Gain_s = adc_pkt->mid(36,2).toInt(&ok,16);
    //fw reports silly values on current limit
    // uTmax6: 11H, or 91H with the positive-grid board (3.01 compared with "11" only)
    if (adc_pkt->size() >= 2 && statusIsCurrentLimit(replyStatus(adc_pkt->at(0), adc_pkt->at(1)))) {
        adc_data.Ia=0;
        adc_data.Is=0;
    }
    //qDebug() << "Ia_gain_pkt=" << adc_pkt->mid(34,2) << "Is_gain_pkt=" << adc_pkt->mid(36,2);
    GetReal();
}

// Applies calibration corrections
void MainWindow::GetReal()
{   float gain[8]={1, 2, 5, 10, 20, 50, 100, 200};
    float avg[8]={0,1,2,4,8,16,32,1}; //0 represents auto averaging

    //determine averaging used based on highest gain
    float auto_avg[8]={1,1,1,1,2,2,4,8};
    float av = avg[options.AvgNum];
    int a = adc_data.Gain_a;
    if ( a < adc_data.Gain_s) a = adc_data.Gain_s;
    if (options.AvgNum==0) av = auto_avg[a];
    //qDebug()<< "Avg="<<av;

    // calc  actual currents
    int i =adc_data.Gain_a;
    if (i>7) i=7;
    adc_real.Ia = (float) (adc_data.Ia) /gain[i] * adc_scale.Ia * calData.IaVal/av/float(calData.IaRsense) ;
    adc_real.Ia_raw = (float) adc_data.Ia_raw * adc_scale.Ia_raw/av/float(calData.IaRsense);

    i = adc_data.Gain_s;
    if (i>7) i=7;
    adc_real.Is = ((float) (adc_data.Is)) /gain[i]* adc_scale.Is * calData.IsVal/av/float(calData.IsRsense);
    adc_real.Is_raw = (float) adc_data.Is_raw * adc_scale.Is_raw/av/float(calData.IsRsense);
    //Calc actual voltages
    adc_real.Vsu = ((float) adc_data.Vsu) * adc_scale.Vsu * calData.VsuVal;
    adc_real.Vg = VgNow;
    //Iares=0;
    //Isres=0;
    adc_real.Vn = 5.0*(adc_scale.Vn *(adc_data.Vn/1023.0-1.0)+1.0) *calData.VnVal;
    adc_real.Va = (float) (adc_data.Va*5.0/1023.0)* adc_scale.Va * calData.VaVal -adc_real.Vsu - adc_real.Ia*float(calData.IaRsense)/1000.0 + Vdi - Vdar ;
    adc_real.Vs = (float) (adc_data.Vs*5.0/1023.0)* adc_scale.Vs * calData.VsVal -adc_real.Vsu - adc_real.Is*float(calData.IsRsense)/1000.0 + Vdi - Vdar ;
    if (adc_real.Ia<0) adc_real.Ia=0;
    if (adc_real.Is<0) adc_real.Is=0;
    if (adc_real.Va < 1) {
        adc_real.Va =0;
        adc_real.Ia =0;
    }
    if (adc_real.Vs < 1) {
        adc_real.Vs =0;
        adc_real.Is =0;
    }
}

//These calculate the values to be sent to the uTracer
uint MainWindow::GetVa(float v)
{   uint x;
    x= (uint)(1023.0/5.0*( (v + adc_real.Vsu -Vdi +Vdar)/adc_scale.Va / calData.VaVal));
    return (x>1023?1023:x);
}
uint MainWindow::GetVs(float v)
{
    uint x;
    x = (uint)(1023.0/5.0*((v + adc_real.Vsu -Vdi +Vdar)/ adc_scale.Vs / calData.VsVal));
    return (x>1023?1023:x);
}
uint MainWindow::GetVg(float v) {
    float Vg,V0; //,V1; //Vg1;
    int g;
    float VgScaled = v / options.VgScale;
    V0=(VgScaled - calData.Vg0) / calData.Vgm;
/*
    if (VgScaled >= -0.5) { // 0 > V > -0.5
        Vg=-1024 * VgScaled /float(calData.VgMax) * RealVal(calData.Vg05Val);
    } else if (VgScaled >= -1) { // -0.5 > V > -1
        V0 = 1024 * 0.5 / float(calData.VgMax) * RealVal(calData.Vg05Val);
        V1 = 1024 * 1 / float(calData.VgMax) * RealVal(calData.Vg1Val);
        Vg =(V1-V0)*(VgScaled+0.5)/(0.5-1) + V0;
    } else if (VgScaled >= -4) { // -1 > V > -4
        V0 = 1024 / float(calData.VgMax) * RealVal(calData.Vg1Val);
        V1 = 1024 * 4 / float(calData.VgMax) * RealVal(calData.Vg4Val);
        Vg =(V1-V0)*(VgScaled+1)/(1-4) + V0;
    } else { //-4 > V
        V0 = 1024 *4 / float(calData.VgMax) * RealVal(calData.Vg4Val);
        V1 = 1024 * 40 / float(calData.VgMax) * RealVal(calData.Vg40Val);
        Vg =(V1-V0)*(VgScaled+4)/(4-40) + V0;
    } */
    //qDebug() << "Grid Vgm=" << calData.Vgm << "Grid Vg0=" << calData.Vg0;
    if (options.uTracer6) {
        // uTmax6: Dekker's sign-and-magnitude word (see grid_word.h). uTmax 3.01
        // returned a negative int here for a positive grid, which went out as
        // FFFF (+100 V) with the heater word lost.
        return utracer6GridWord(V0, float(calData.VgMax));
    } else {
        if (Vg<0) Vg=0;
        Vg=(-4096) / float(calData.VgMax) *V0;
        g =int(round(Vg));
        if( g> 4095) g = 4095;
        if ( g < 0) g=0;
        g = ((g>>2) & 0x3ff ) | ((g & 0x3) << 14); //for 12 bit fw 203 and higher
        if (calData.Firmware < 203) g = (g & 0x3ff);
    }
    //qDebug() << "Grid Vin=" << V0 << "Grid PWM=" << g;
    return  (g);
}

uint MainWindow::GetVf(float v)
{
    VfNow = v*ui->VhVal->text().toFloat()/HEAT_CNT_MAX;
    adc_real.Vf=  VfNow;
    uint f;
    if (options.ExtHtr==0)
        f  = 1023.0*(VfNow/adc_real.Vsu)*(VfNow/adc_real.Vsu);  // note it's rms voltage that matters!!
    else {
        f = uint(VfNow / calData.heaterscale * 1023.0);
        if (f>1023) f=0x3ff;
        f|=0xe000;
    }
    return f;
}

//------------------------------------------------------
// Calibration File Management
void MainWindow::ReadCalibration()
{
    //Check to see if the cal file exists
    QFile datafile(calFileName);
    if (! datafile.exists())
    {
        //It doesn't exist so set up default values and create a file
        datafile.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream calFile(&datafile);
        calFile << "COM=COM1\n";
        calFile << "Va=1.0\n";
        calFile << "Vs=1.0\n";
        calFile << "Ia=1.0\n";
        calFile << "Is=1.0\n";
        calFile << "Vsu=1.0\n";
        calFile << "Vg05=1.0\n";
        calFile << "Vg1=1.0\n";
        calFile << "Vg4=1.0\n";
        calFile << "Vg40=1.0\n";
        calFile << "Vg40=1.0\n";
        calFile << "Vn=1.0\n";
        calFile << "RaVal=6800\n";
        calFile << "VaMax=300\n";
        calFile << "IaRsense=18\n";
        calFile << "IsRsense=18\n";
        calFile << "VgMax=50\n";
        calFile << "Firmware=1\n";
        calFile << "SavePath="<<QDir::homePath()<<'\n';
        calData.Style=QCoreApplication::applicationDirPath();
        calData.Style.append("/style.qss\n");
        calFile << "Style="<<calData.Style;
        calFile << "Sort=0" << "\n";
        calFile << "Index=0" << "\n";
        calFile << "uTracer6=0" << "\n";
        calFile << "heater=0" << "\n";
        calFile.flush();
        datafile.close();
        //qDebug() << "ReadCalibration: Created new cal file";
    }
    //Open the cal.txt file
    datafile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&datafile);
    calData.SavePath = QDir::homePath();
    while ( !in.atEnd() )
    {
        QString line = in.readLine();
        QStringList list = line.split("=");
        //qDebug() << "ReadCalibration:" << list;
        //Get Cal data and set sliders
        if (list.first()=="COM") comport=list.at(1);
        else if (list.first()=="Va") calData.VaVal = (list.at(1).toFloat());
        else if (list.first()=="Vs") calData.VsVal = (list.at(1).toFloat());
        else if (list.first()=="Ia") calData.IaVal = (list.at(1).toFloat());
        else if (list.first()=="Is") calData.IsVal = (list.at(1).toFloat());
        else if (list.first()=="Vsu") calData.VsuVal = (list.at(1).toFloat());
        else if (list.first()=="Vg0") calData.Vg0 = list.at(1).toFloat();
        else if (list.first()=="Vgm") calData.Vgm = (list.at(1).toFloat());
        //else if (list.first()=="Vg05") calData.Vg05Val = FromRealVal(list.at(1).toFloat());
        //else if (list.first()=="Vg1") calData.Vg1Val = FromRealVal(list.at(1).toFloat());
        //else if (list.first()=="Vg4") calData.Vg4Val = FromRealVal(list.at(1).toFloat());
        //else if (list.first()=="Vg40") calData.Vg40Val = FromRealVal(list.at(1).toFloat());
        else if (list.first()=="Vn") calData.VnVal = (list.at(1).toFloat());
        else if (list.first()=="RaVal") calData.RaVal = list.at(1).toFloat();
        else if (list.first()=="VaMax") calData.VaMax = list.at(1).toFloat();
        else if (list.first()=="IaRsense") calData.IaRsense = list.at(1).toFloat();
        else if (list.first()=="IsRsense") calData.IsRsense = list.at(1).toFloat();
        else if (list.first()=="VgMax") calData.VgMax = list.at(1).toFloat();
        else if (list.first()=="Firmware") calData.Firmware = list.at(1).toShort();
        else if (list.first()=="SavePath") calData.SavePath = list.at(1);
        else if (list.first()=="Style") calData.Style = list.at(1);
        else if (list.first()=="Sort") options.TLSortOrder = list.at(1).toShort();
        else if (list.first()=="Index") {
            on_TubeSelector_currentIndexChanged_enable=false;
            ui->TubeSelector->setCurrentIndex(list.at(1).toShort());
            on_TubeSelector_currentIndexChanged_enable=true;
        }
        else if (list.first()=="uTracer6") options.uTracer6 = list.at(1).toInt();
        else if (list.first()=="heater") calData.heaterscale = list.at(1).toFloat();
    }
    if (calData.Firmware==1 || calData.Firmware==2) calData.Firmware=calData.Firmware*100;
    datafile.close();
    ui->AutoPath->setText(calData.SavePath);
}

void MainWindow::SaveCalFile()
{
    qDebug () << "SaveCalFile...";
    QFile datafile(calFileName);
    if (datafile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream calFile(&datafile);
        if (portInUse!=NULL) calFile << "COM=" << portInUse->portName() << "\n";
        else calFile << "COM=COM1\n";
        calFile << "Va=" << calData.VaVal << "\n";
        calFile << "Vs=" << calData.VsVal << "\n";
        calFile << "Ia=" << calData.IaVal << "\n";
        calFile << "Is=" << calData.IsVal << "\n";
        calFile << "Vsu=" << calData.VsuVal << "\n";
        calFile << "Vg0=" << calData.Vg0 << "\n";
        calFile << "Vgm=" << calData.Vgm << "\n";
        //calFile << "Vg05=" << RealVal(calData.Vg05Val) << "\n";
        //calFile << "Vg1=" << RealVal(calData.Vg1Val) << "\n";
        //calFile << "Vg4=" << RealVal(calData.Vg4Val) << "\n";
        //calFile << "Vg40=" << RealVal(calData.Vg40Val) << "\n";
        calFile << "Vn=" << calData.VnVal << "\n";
        calFile << "RaVal=" << calData.RaVal << "\n";
        calFile << "VaMax=" << calData.VaMax << "\n";
        calFile << "IaRsense=" << calData.IaRsense << "\n";
        calFile << "IsRsense=" << calData.IsRsense << "\n";
        calFile << "VgMax=" << calData.VgMax << "\n";
        calFile << "Firmware=" << calData.Firmware << "\n";
        calFile << "SavePath=" << calData.SavePath << "\n";
        calFile << "Style=" << calData.Style << "\n";
        calFile << "Sort=" << options.TLSortOrder << "\n";
        calFile << "Index=" << ui->TubeSelector->currentIndex() << "\n";
        calFile << "uTracer6=" << options.uTracer6<< "\n";
        calFile << "heater=" << calData.heaterscale<< "\n";

        calFile.flush();
        datafile.close();
    } else qDebug() << "Save Cal file failed";
}

//------------------------------------------------------
// Tube Data file Management
//------------------------------------------------------
void MainWindow::ReadDataFile()
{
    //Check to see if usual data file exists
    QFile datafile(dataFileName);
    if (! datafile.exists(dataFileName)) {
        //It doesn't exist so ask for it
        dataFileName="";
        dataFileName = QFileDialog::getOpenFileName(this,tr("Read Tube Data file"),QDir::homePath(),"Text (*.csv)");
    }
    if (dataFileName=="") QCoreApplication::quit();
    datafile.setFileName(dataFileName);
    datafile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&datafile);
    QString line = in.readLine();
    QStringList dataline = line.split(",");
    //qDebug() << "MainWindow" << dataline;

    //read entire file into tubeDataList
    //The list is assumed to be sorted
    bool ok;
    while ( !in.atEnd() )
    {
        //Build list of tubes
        line = in.readLine();
        //qDebug() << "ReadDataFile:" << line;
        dataline = line.split(",");
        if (dataline.length()==20) {
            int i = 0;
            tubeData.ID=dataline.at(i++);
            tubeData.Anode =dataline.at(i++) ;
            tubeData.G2=dataline.at(i++);
            tubeData.G1=dataline.at(i++);
            tubeData.G1b=dataline.at(i++);
            tubeData.Cathode=dataline.at(i++);
            tubeData.G3=dataline.at(i++);
            tubeData.HtrA=dataline.at(i++);
            tubeData.HtrB=dataline.at(i++);
            tubeData.HtrVolts=dataline.at(i++).toFloat(&ok);
            tubeData.Model=dataline.at(i++);
            tubeData.VaStart=dataline.at(i++).toInt(&ok);
            tubeData.VaEnd=dataline.at(i++).toInt(&ok);
            tubeData.VaStep=dataline.at(i++).toInt(&ok);
            tubeData.VsStart=dataline.at(i++).toInt(&ok);
            tubeData.VsEnd=dataline.at(i++).toInt(&ok);
            tubeData.VsStep=dataline.at(i++).toInt(&ok);
            tubeData.VgStart=dataline.at(i++).toInt(&ok);
            tubeData.VgEnd=dataline.at(i++).toInt(&ok);
            tubeData.VgStep=dataline.at(i++).toInt(&ok);
            tubeData.Vca= 200;
            tubeData.Dva= 10;
            tubeData.Vcs= 200;
            tubeData.Dvs= 10;
            tubeData.Vcg= -5;
            tubeData.Dvg= 10;
            tubeData.powerLim=10;
            tubeData.EmmVa=200;
            tubeData.EmmVs=200;
            tubeData.EmmVg=0;
            tubeData.gm1Typ=5;
            tubeData.gm1Del=30;
            tubeData.gm2Typ=5;
            tubeData.gm2Del=30;
            tubeData.heatSecs=60;
            tubeData.last_time = QDateTime::currentSecsSinceEpoch();
            tubeData.times_used=0;
            tubeData.Va_eq_Vs=0;

            // add to library
            tubeDataVec.push_back(tubeData);
        }
        else if (dataline.length()>=26)
        {
            int i = 0;
            tubeData.ID=dataline.at(i++);
            tubeData.Anode =dataline.at(i++) ;
            tubeData.G2=dataline.at(i++);
            tubeData.G1=dataline.at(i++);
            tubeData.G1b=dataline.at(i++);
            tubeData.Cathode=dataline.at(i++);
            tubeData.G3=dataline.at(i++);
            tubeData.HtrA=dataline.at(i++);
            tubeData.HtrB=dataline.at(i++);
            tubeData.HtrVolts=dataline.at(i++).toFloat(&ok);
            tubeData.Model=dataline.at(i++);
            tubeData.VaStart=dataline.at(i++).toFloat(&ok);
            tubeData.VaEnd=dataline.at(i++).toFloat(&ok);
            tubeData.VaStep=dataline.at(i++).toInt(&ok);
            tubeData.VsStart=dataline.at(i++).toFloat(&ok);
            tubeData.VsEnd=dataline.at(i++).toFloat(&ok);
            tubeData.VsStep=dataline.at(i++).toInt(&ok);
            tubeData.VgStart=dataline.at(i++).toFloat(&ok);
            tubeData.VgEnd=dataline.at(i++).toFloat(&ok);
            tubeData.VgStep=dataline.at(i++).toInt(&ok);
            tubeData.Vca=dataline.at(i++).toFloat(&ok);
            tubeData.Dva=dataline.at(i++).toFloat(&ok);
            tubeData.Vcs=dataline.at(i++).toFloat(&ok);
            tubeData.Dvs=dataline.at(i++).toFloat(&ok);
            tubeData.Vcg=dataline.at(i++).toFloat(&ok);
            tubeData.Dvg=dataline.at(i++).toFloat(&ok);
            tubeData.powerLim=10;
            tubeData.EmmVa=200;
            tubeData.EmmVs=200;
            tubeData.EmmVg=0;
            tubeData.gm1Typ=5;
            tubeData.gm1Del=30;
            tubeData.gm2Typ=5;
            tubeData.gm2Del=30;
            tubeData.heatSecs=60;
            tubeData.last_time = QDateTime::currentSecsSinceEpoch();
            tubeData.times_used=0;
            tubeData.Va_eq_Vs=0;

            if (dataline.length()>=27)  tubeData.powerLim=dataline.at(i++).toFloat(&ok);
            if (dataline.length()>=31) {
                tubeData.EmmVa=dataline.at(i++).toFloat(&ok);
                tubeData.EmmVs=dataline.at(i++).toFloat(&ok);
                tubeData.EmmVg=dataline.at(i++).toFloat(&ok);
            }
            if (dataline.length()>=34) {
                tubeData.gm1Typ=dataline.at(i++).toFloat(&ok);
                tubeData.gm1Del=dataline.at(i++).toFloat(&ok);
                tubeData.gm2Typ=dataline.at(i++).toFloat(&ok);
                tubeData.gm2Del=dataline.at(i++).toFloat(&ok);
            }
            if (dataline.length()>=35) {
                tubeData.heatSecs=dataline.at(i++).toFloat(&ok);
            }
            if (dataline.length()>=37) {
                tubeData.last_time  =dataline.at(i++).toULongLong();
                tubeData.times_used =dataline.at(i++).toUInt();
            }
            if (dataline.length()>=38) {
                tubeData.Va_eq_Vs =dataline.at(i++).toUInt();
            }
            // add to library
                tubeDataVec.push_back(tubeData);
            }
        else ui->statusBar->showMessage(tr("Invalid line found in data.csv file"));
    }
    datafile.close();
    SortTubeData(options.TLSortOrder);
    updateTubeSelector(tubeDataVec.at(0).ID);
    TubeSelector_StringChanged(tubeDataVec.at(0).ID);
    //LabelPins(tubeDataList->at(0)); // should be triggered by change of index;
}

void MainWindow::LabelPins(tubeData_t tubeData) {

    ui->AnodePin->setText(tubeData.Anode );
    ui->G2Pin->setText(tubeData.G2);
    ui->G1Pin->setText(tubeData.G1 );
    ui->G1bPin->setText(tubeData.G1b );
    ui->CathodePin->setText(tubeData.Cathode );
    ui->G3Pin->setText(tubeData.G3 );
    ui->HtraPin->setText(tubeData.HtrA );
    ui->HtrbPin->setText(tubeData.HtrB );
    ui->VhVal->setText(QString::number(tubeData.HtrVolts)) ;
    ui->TubeType->setCurrentIndex( ui->TubeType->findText(tubeData.Model) ) ;
    ui->VaStart->setText(QString::number(tubeData.VaStart ));
    ui->VaEnd->setText(QString::number(tubeData.VaEnd));
    ui->VaSteps->setText(QString::number(tubeData.VaStep ));
    ui->VsStart->setText(QString::number(tubeData.VsStart ));
    ui->VsEnd->setText(QString::number(tubeData.VsEnd ));
    ui->VsSteps->setText(QString::number(tubeData.VsStep ));
    ui->VgStart->setText(QString::number(tubeData.VgStart ));
    ui->VgEnd->setText(QString::number(tubeData.VgEnd ));
    ui->VgSteps->setText(QString::number(tubeData.VgStep ));
    //quick sweep settings
    ui->Vca->setText(QString::number(tubeData.Vca ));
    ui->DeltaVa->setText(QString::number(tubeData.Dva ));
    ui->Vcs->setText(QString::number(tubeData.Vcs ));
    ui->DeltaVs->setText(QString::number(tubeData.Dvs ));
    ui->Vcg->setText(QString::number(tubeData.Vcg ));
    ui->DeltaVg->setText(QString::number(tubeData.Dvg ));
    ui->PowLim->setText(QString::number(tubeData.powerLim ));
    ui->EmmVa->setText(QString::number(tubeData.EmmVa ));
    ui->EmmVs->setText(QString::number(tubeData.EmmVs ));
    ui->EmmVg->setText(QString::number(tubeData.EmmVg ));
    ui->gm1Typ->setText(QString::number(tubeData.gm1Typ ));
    ui->gm1Delta->setText(QString::number(tubeData.gm1Del ));
    ui->gm2Typ->setText(QString::number(tubeData.gm2Typ ));
    ui->gm2Delta->setText(QString::number(tubeData.gm2Del ));
    ui->HtrSecs->setText(QString::number(tubeData.heatSecs ));
    ui->checkVsEqVa->setChecked(tubeData.Va_eq_Vs);
}
// added for Qt6 as "currentIndexChanged(const QString &arg1)" has been removed
// see https://doc.qt.io/qt-5/qcombobox-obsolete.html

void MainWindow::on_TubeSelector_currentIndexChanged(int index)
{
    QString tt = ui->TubeSelector->currentText();
    TubeSelector_StringChanged(tt);
}

//vector version
void MainWindow::TubeSelector_StringChanged(const QString &arg1) {
    if (! on_TubeSelector_currentIndexChanged_enable) return;
    //look for matching ID
    if (ui->TubeSelector->count()==0) return; //catch empty case
    uint count = 0;
    while (arg1 != tubeDataVec.at(count).ID && count < tubeDataVec.size()) count++;
    tubeData= tubeDataVec.at(count);
    ui->AutoFileName->setText( ui->TubeSelector->currentText());
    ui->AutoNumber->setText("1");
    LabelPins(tubeData);
    RePlot(activeStore);
}

void MainWindow::updateTubeSelector(QString ID) {
    //update tube selector drop down
    on_TubeSelector_currentIndexChanged_enable=false;
    ui->TubeSelector->clear();
    uint s = tubeDataVec.size();
    for(uint i = 0; i < s; i++ ) {
        ui->TubeSelector->addItem(tubeDataVec.at(i).ID);
    }
    //find current ID
     int i = ui->TubeSelector->findText(ID); //name
     if (i !=-1 ) ui->TubeSelector->setCurrentIndex(i); else ui->TubeSelector->setCurrentIndex(0);
     on_TubeSelector_currentIndexChanged_enable=true;
}
//#include <algorithm>

void MainWindow::on_TLsort_changed()
{
    QString ID = ui->TubeSelector->currentText();
    SortTubeData(options.TLSortOrder);
    updateTubeSelector(ID);
}

bool  MainWindow::sortbyTime(const tubeData_t & struct1, const tubeData_t & struct2)
{
    bool a = struct1.last_time > struct2.last_time;
    a |= (struct1.last_time == struct2.last_time) && (struct1.ID < struct2.ID);
    return a;
}

bool MainWindow::sortbyFreq(const tubeData_t & struct1, const tubeData_t & struct2)
{
    //inline bool operator() (const tubeData_t & struct1, const tubeData_t & struct2)
    //{
    bool a = struct1.times_used > struct2.times_used;
    a |= (struct1.times_used == struct2.times_used) && (struct1.ID < struct2.ID);
    return a;
    //}
}

bool MainWindow::sortbyAlpha(const tubeData_t & struct1, const tubeData_t & struct2)
{
    //inline bool operator() (const tubeData_t & struct1, const tubeData_t & struct2)
    //{
        return (struct1.ID < struct2.ID);
    //}
}

void MainWindow::SortTubeData(int type) {
    switch (type) {
    case 0:
        std::sort(tubeDataVec.begin(), tubeDataVec.end(), sortbyAlpha);
        break;
    case 1:
        std::sort(tubeDataVec.begin(), tubeDataVec.end(), sortbyFreq);
        break;
    case 2:
        std::sort(tubeDataVec.begin(), tubeDataVec.end(), sortbyTime);
        break;
    }
}

//Add Tube type
void MainWindow::on_AddType_clicked()
{
    DataSaveDialog *w = new DataSaveDialog(this, ui->TubeSelector->currentText());
    w->setModal(true);
    w->show();
}

void MainWindow::DataSaveDialog_clicked(const QString &Type) {
    //copy current settings into tubeData
    tubeData.ID = Type;
    tubeData.Anode = ui->AnodePin->text()  ;
    tubeData.G2 = ui->G2Pin->text();
    tubeData.G1 = ui->G1Pin->text();
    tubeData.G1b = ui->G1bPin->text();
    tubeData.Cathode = ui->CathodePin->text();
    tubeData.G3 = ui->G3Pin->text();
    tubeData.HtrA = ui->HtraPin->text();
    tubeData.HtrB = ui->HtrbPin->text();
    tubeData.HtrVolts = ui->VhVal->text().toFloat();
    tubeData.Model = ui->TubeType->currentText();
    tubeData.VaStart = ui->VaStart->text().toFloat();
    tubeData.VaEnd = ui->VaEnd->text().toFloat();
    tubeData.VaStep = ui->VaSteps->text().toInt();
    tubeData.VsStart = ui->VsStart->text().toFloat();
    tubeData.VsEnd = ui->VsEnd->text().toFloat();
    tubeData.VsStep = ui->VsSteps->text().toInt();
    tubeData.VgStart = ui->VgStart->text().toFloat();
    tubeData.VgEnd = ui->VgEnd->text().toFloat();
    tubeData.VgStep = ui->VgSteps->text().toInt();
    tubeData.Vca = ui->Vca->text().toFloat();
    tubeData.Dva = ui->DeltaVa->text().toFloat();
    tubeData.Vcs = ui->Vcs->text().toFloat();
    tubeData.Dvs = ui->DeltaVs->text().toFloat();
    tubeData.Vcg = ui->Vcg->text().toFloat();
    tubeData.Dvg = ui->DeltaVg->text().toFloat();
    tubeData.powerLim = ui->PowLim->text().toFloat();
    tubeData.EmmVa = ui->EmmVa->text().toFloat();
    tubeData.EmmVs = ui->EmmVs->text().toFloat();
    tubeData.EmmVg = ui->EmmVg->text().toFloat();
    tubeData.gm1Typ = ui->gm1Typ ->text().toFloat();
    tubeData.gm1Del = ui->gm1Delta->text().toFloat();
    tubeData.gm2Typ = ui->gm2Typ->text().toFloat();
    tubeData.gm2Del = ui->gm2Delta->text().toFloat();
    tubeData.heatSecs = ui->HtrSecs->text().toFloat();
    tubeData.Va_eq_Vs = ui->checkVsEqVa->isChecked();

    //decide what to do with the copied data
    //qDebug() << "Changing tube=" << Type;
    if (Type!="") {
        bool done = false;
        uint i = 0;
        //uint s = tubeDataVec.size();
        while ((i< tubeDataVec.size()) && !done ) {
            if ( (tubeDataVec.at(i).ID == Type) ){
                //found a match so replace
                if (tubeDataVec.at(i).ID != ui->TubeSelector->currentText()) qDebug() << "Change tube: Tube ID mismatch!";
                else tubeDataVec.at(i) = tubeData;
                done = true;
            }
            i++;
        }
        if (!done) {
            tubeData.last_time= QDateTime::currentSecsSinceEpoch();
            tubeData.times_used=0;
            tubeDataVec.push_back(tubeData); //a new type so add to end
        }
        SortTubeData(0); //Alpha sort
        SaveTubeDataFile();
        updateTubeSelector(Type);
        SortTubeData(options.TLSortOrder);
        on_TLsort_changed(); //update the display
    }
}

//Delete current Type type
void MainWindow::on_DeleteType_clicked()
{
   uint i = ui->TubeSelector->currentIndex();
   if (i>=tubeDataVec.size()) {
        qDebug() << "on_DeleteType: index out of range";
       return;
   }
    tubeDataVec.erase(tubeDataVec.begin() + ui->TubeSelector->currentIndex());
    on_TubeSelector_currentIndexChanged_enable=false;
    ui->TubeSelector->removeItem(i);
    on_TubeSelector_currentIndexChanged_enable=true;
    SortTubeData(0);
    SaveTubeDataFile();
    SortTubeData(options.TLSortOrder);
}

bool MainWindow::SaveTubeDataFile() {

    QFile datafile(dataFileName);
    if (datafile.open(QIODevice::WriteOnly  | QIODevice::Text)) {
        QTextStream out(&datafile);

        out << "ID,A,G2,G1,G1b,Cathode,G3,Heater,Heater,Htr Volts,SPICE model,VaStart,VaEnd,VaStep,VsStart," <<
               "VsEnd,VsStep,VgStart,VgEnd,VgStep,Vca,Dva,Vcg2,Dvg2,Vcg,Dvg,Power,EmmVa,EmmVs,EmmVg,gm1Typ," << ""
                "gm1Del,gm2Typ,gm1Del,HeatSecs, Time, Uses,Va=Vs\n";
        for (uint i=0; i< tubeDataVec.size(); i++) {
            out << tubeDataVec.at(i).ID << ","
                << tubeDataVec.at(i).Anode << ","
                << tubeDataVec.at(i).G2 << ","
                << tubeDataVec.at(i).G1 << ","
                << tubeDataVec.at(i).G1b << ","
                << tubeDataVec.at(i).Cathode << ","
                << tubeDataVec.at(i).G3 << ","
                << tubeDataVec.at(i).HtrA << ","
                << tubeDataVec.at(i).HtrB << ","
                << tubeDataVec.at(i).HtrVolts << ","
                << tubeDataVec.at(i).Model << ","
                << tubeDataVec.at(i).VaStart << ","
                << tubeDataVec.at(i).VaEnd << ","
                << tubeDataVec.at(i).VaStep << ","
                << tubeDataVec.at(i).VsStart << ","
                << tubeDataVec.at(i).VsEnd << ","
                << tubeDataVec.at(i).VsStep << ","
                << tubeDataVec.at(i).VgStart << ","
                << tubeDataVec.at(i).VgEnd << ","
                << tubeDataVec.at(i).VgStep << ","
                << tubeDataVec.at(i).Vca << ","
                << tubeDataVec.at(i).Dva << ","
                << tubeDataVec.at(i).Vcs << ","
                << tubeDataVec.at(i).Dvs << ","
                << tubeDataVec.at(i).Vcg << ","
                << tubeDataVec.at(i).Dvg << ","
                << tubeDataVec.at(i).powerLim << ","
                << tubeDataVec.at(i).EmmVa << ","
                << tubeDataVec.at(i).EmmVs << ","
                << tubeDataVec.at(i).EmmVg << ","
                << tubeDataVec.at(i).gm1Typ << ","
                << tubeDataVec.at(i).gm1Del << ","
                << tubeDataVec.at(i).gm2Typ << ","
                << tubeDataVec.at(i).gm2Del << ","
                << tubeDataVec.at(i).heatSecs << ","
                << tubeDataVec.at(i).last_time << ","
                << tubeDataVec.at(i).times_used << ","
                << tubeDataVec.at(i).Va_eq_Vs << "\n";
        }
        datafile.close();
        return(true);
    }
    return(true);
}

void MainWindow::on_TubeType_currentIndexChanged(int index)
{
    if (!on_TubeType_currentIndexChanged_enable) return;
    tubeData.Model=ui->TubeType->currentText();
    if (LOGGING) QTextStream(&logFile) << "+on_TubeType_currentIndexChanged\n";
    if (activeStore) {
        if (LOGGING) QTextStream(&logFile) << "+call optimizer\n";
        if (ui->TubeType->currentText()!=NONE) {
            optimizer->Optimize(activeStore, ui->TubeType->currentIndex(), 1, VaSteps, VsSteps, VgSteps);
            if (optimizer->Status())
                ui->statusBar->showMessage("Info: model fit is poor. Try a different model or change your sweep settings");
            else
                ui->statusBar->showMessage("Model fit OK");
        }
        if (LOGGING) QTextStream(&logFile) << "+updateLCDS\n";
        updateLcdsWithModel();
        if (LOGGING) QTextStream(&logFile) << "+RePlot\n";
        RePlot(activeStore);
        if (LOGGING) QTextStream(&logFile) << "+after Replot\n";
    }
    else {
        ClearLCDs();
        if (ui->TubeType->currentText()==DIODE || ui->TubeType->currentText()==DUAL_DIODE) {
            ui->gm_label->setText("Ra ohm");
        } else ui->gm_label->setText("Gm mS");

        if (ui->TubeType->currentText()==DUAL_TRIODE || ui->TubeType->currentText()==DUAL_DIODE) {
            ui->AnodeLabel->setText("A(a)");
            ui->Grid2Label->setText("A(b)");
            ui->Grid1Label->setText("G1(a)");
            ui->Grid1bLabel->setText("G1(b)");
            ui->CathodeLabel->setText("C(a)");
            ui->Grid3Label ->setText("C(b)");
        } else {
            ui->AnodeLabel->setText("A");
            ui->Grid2Label->setText("G2");
            ui->Grid1Label->setText("G1(a)");
            ui->Grid1bLabel->setText("G1(b)");
            ui->Grid3Label ->setText("G3");
            ui->CathodeLabel->setText("C");
        }
    }
    updateTubeGreying();
}

void MainWindow::updateTubeGreying() {
    //Enable sweep settings
    ui->VaStart->setEnabled(true);
    ui->VsStart->setEnabled(true);
    ui->VgStart->setEnabled(true);

    ui->VaEnd->setEnabled(true);
    ui->VsEnd->setEnabled(true);
    ui->VgEnd->setEnabled(true);

    ui->VaSteps->setEnabled(true);
    ui->VsSteps->setEnabled(true);
    ui->VgSteps->setEnabled(true);

    ui->Vca->setEnabled(true);
    ui->Vcs->setEnabled(true);
    ui->Vcg->setEnabled(true);

    ui->DeltaVa->setEnabled(true);
    ui->DeltaVs->setEnabled(true);
    ui->DeltaVg->setEnabled(true);

    ui->EmmVa->setEnabled(true);
    ui->EmmVs->setEnabled(true);
    ui->EmmVg->setEnabled(true);

    ui->gm1Typ->setEnabled(true);
    ui->gm1Delta->setEnabled(true);
    ui->gm2Typ->setEnabled(true);
    ui->gm2Delta->setEnabled(true);
    if (!ui->checkQuickTest->isChecked()) { //Normal Sweep
        ui->Vca->setDisabled(true);
        ui->DeltaVa ->setDisabled(true);
        ui->Vcs->setDisabled(true);
        ui->DeltaVs ->setDisabled(true);
        ui->Vcg->setDisabled(true);
        ui->DeltaVg->setDisabled(true);
        ui->EmmVa->setDisabled(true);
        ui->EmmVs->setDisabled(true);
        ui->EmmVg->setDisabled(true);
        ui->gm1Typ->setEnabled(false);
        ui->gm1Delta->setEnabled(false);
        ui->gm2Typ->setEnabled(false);
        ui->gm2Delta->setEnabled(false);

        if (ui->TubeType->currentText()==TRIODE) {
            ui->VsSteps->setDisabled(true);
            ui->VsStart->setDisabled(true);
            ui->VsEnd->setDisabled(true);
            ui->gm2Typ->setDisabled(true);
            ui->gm2Delta->setDisabled(true);
            ui->EmmVs->setDisabled(true);
            ui->DeltaVs->setDisabled(true);
            ui->Vcs->setDisabled(true);
        } else if (ui->TubeType->currentText()==DUAL_TRIODE) {
            if (ui->checkVsEqVa->isChecked()) {
                ui->VsSteps->setDisabled(true);
                ui->VsStart->setDisabled(true);
                ui->VsEnd->setDisabled(true);
            }
        } else if ( ui->TubeType->currentText()==DIODE) {
            ui->VgSteps->setDisabled(true);
            ui->VgStart->setDisabled(true);
            ui->VgEnd->setDisabled(true);
            ui->VsSteps->setDisabled(true);
            ui->VsStart->setDisabled(true);
            ui->VsEnd->setDisabled(true);
        } else if (ui->TubeType->currentText()==DUAL_DIODE) {
            ui->VgSteps->setDisabled(true);
            ui->VgStart->setDisabled(true);
            ui->VgEnd->setDisabled(true);
            if (ui->checkVsEqVa->isChecked()) {
                ui->VsSteps->setDisabled(true);
                ui->VsStart->setDisabled(true);
                ui->VsEnd->setDisabled(true);
            }
        } else if (ui->TubeType->currentText()==NONE) {
//            ui->VgSteps->setDisabled(true);
//            ui->VgStart->setDisabled(true);
//            ui->VgEnd->setDisabled(true);
//            ui->VsSteps->setDisabled(true);
//            ui->VsStart->setDisabled(true);
//            ui->VsEnd->setDisabled(true);
        } else ui->checkVsEqVa->setChecked(false);

    } else { //quick test
        ui->VaSteps->setDisabled(true);
        ui->VaStart->setDisabled(true);
        ui->VaEnd->setDisabled(true);
        ui->VsSteps->setDisabled(true);
        ui->VsStart->setDisabled(true);
        ui->VsEnd->setDisabled(true);
        ui->VgSteps->setDisabled(true);
        ui->VgStart->setDisabled(true);
        ui->VgEnd->setDisabled(true);
        if (ui->TubeType->currentText()==TRIODE) {
            ui->Vcs->setDisabled(true);
            ui->DeltaVs ->setDisabled(true);
            ui->gm2Typ->setDisabled(true);
            ui->gm2Delta->setDisabled(true);
            ui->EmmVs->setDisabled(true);
            ui->gm2Delta->setDisabled(true);

        }  else if (ui->TubeType->currentText()==DUAL_TRIODE) {
            ui->checkVsEqVa->setChecked(true);
        }  else if (ui->TubeType->currentText()==DIODE) {
            ui->Vcs->setDisabled(true);
            ui->DeltaVs ->setDisabled(true);
            ui->gm2Typ->setDisabled(true);
            ui->gm2Delta->setDisabled(true);
            ui->Vcg->setDisabled(true);
            ui->DeltaVg ->setDisabled(true);
            ui->EmmVs->setDisabled(true);
        } else if (ui->TubeType->currentText()==DUAL_DIODE) {
            ui->checkVsEqVa->setChecked(true);
            ui->Vcg->setDisabled(true);
            ui->DeltaVg ->setDisabled(true);
        }

    }
}

void MainWindow::updateSweepGreying()
{
    updateTubeGreying();
}


//Tube data file management end
//-----------------------------


//------------------------------------------------------
// UI Slots
void MainWindow::on_actionExit_triggered()
{
    if (LOGGING) QTextStream(&logFile) << "===Logging Ends===\n";
    if (LOGGING) logFile.close();
    exit(0);
}


void MainWindow::on_actionCal_triggered()
{   Cal_Dialog *w = new Cal_Dialog(this);

    w->show();
}

void MainWindow::on_actionOptions_triggered()
{
    options_dialog *w = new options_dialog(this);
    connect(w, SIGNAL(TSLSortChanged()), this, SLOT(on_TLsort_changed()));
    w->show();
}

void MainWindow::on_actionDebug_triggered()
{
    debug_dialog * w =  new debug_dialog(this);
    w->show();
}

void MainWindow::on_AutoPath_editingFinished()
{
    calData.SavePath=ui->AutoPath->text();
    SaveCalFile();
}

void MainWindow::on_actionSave_plot_triggered()
{
    QString dataFileName;
    if (ui->checkAutoNumber->isChecked() ) {
        dataFileName = ui->AutoPath->text()  ;
        dataFileName.append("/");
        dataFileName.append(ui->AutoFileName->text())  ;
        dataFileName.append("-");
        dataFileName.append(ui->AutoNumber->text());
        dataFileName.append(ui->AutoFileType->currentText());
    }
    else {
        dataFileName = QFileDialog::getSaveFileName(this,tr("Save plot file"),ui->AutoPath->text(),
                                                        "Pick jpg, png, bmp or pdf: *.jpg;; *.png;; *.bmp;; *.pdf");
    }
    if (dataFileName!="") {
        //qDebug() << "Save plot Filename:" << dataFileName;
        QList<QString> f =dataFileName.split(".");

        //save plot 1
        QString name;
        for (int i=0; i < plotTabs.length(); i++) {
            name.clear();
            QTextStream(&name) << f.first() << "_plot_" << (i+1) << "." << f.last();
            //(fileName, int  width = 0, int  height = 0, double  scale = 1.0, int  quality = -1  )
            if (f.last()=="jpg") plotTabs.at(i)->plotArea->saveJpg(name,0 , 0, 2.0,-1);
            else if (f.last()=="png") plotTabs.at(i)->plotArea->savePng(name, 0, 0, 2.0,-1);
            else if (f.last()=="pdf") plotTabs.at(i)->plotArea->savePdf(name);
            else if (f.last()=="bmp") plotTabs.at(i)->plotArea->saveBmp(name);
            else QMessageBox::information(this, "File Specification Error", "Only bmp, png, jpg and pdf format are supported.");
        }
    }
}


void MainWindow::on_actionSave_Data_triggered()
{
    static QString oldFileName;
    QString dataFileName;
    QString fn = ui->AutoFileName->text();
    fn = ui->AutoPath->text()  ;
    fn.append("/");
    fn.append(ui->AutoFileName->text())  ;
    fn.append("-");
    fn.append(ui->AutoNumber->text());
    fn.append(".txt");
    if (LOGGING) QTextStream(&logFile) << "Writing Data\n";

    if (ui->checkAutoNumber->isChecked() ) {
        dataFileName = fn;
    }
    else {
        dataFileName = QFileDialog::getSaveFileName(this,tr("Save Data file"),fn,"Data log file(*.txt)");
    }
    if (dataFileName!="") {
        if (LOGGING) QTextStream(&logFile) << dataFileName;
        //qDebug() << "Data file name:" << dataFileName << '\n';
        //Save the data
        QFile datafile(dataFileName);
        if (datafile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            //QTextStream out(&datafile);
            char m[256];
            if (ui->checkQuickTest->isChecked()) {
                datafile.write("Quick test results\n");
            }
            else datafile.write("Data 2\n");  //File format info

            datafile.write(ui->TubeSelector->currentText().toLatin1() );  //Type
            datafile.write("\n" );

            datafile.write(ui->TubeType->currentText().toLatin1() );  //Model
            datafile.write("\n" );
            if (ui->checkQuickTest->isChecked()) {
                sprintf(m,"Va Mid\t%d Delta Va\t%d\n",ui->Vca->text().toInt(), ui->DeltaVa->text().toInt());
                datafile.write(m);

                sprintf(m,"Vs/Va(b) Mid\t%d Delta Vs/Va(b)\t%d\n",ui->Vcs->text().toInt(), ui->DeltaVa->text().toInt());
                datafile.write(m);

                sprintf(m,"Vg Mid\t%f Delta Vg\t%d\n",ui->Vcg->text().toFloat(), ui->DeltaVg->text().toInt());
                datafile.write(m);
            }
            else {
                sprintf(m,"Va\t%d\t%d\t%d\n",(int)VaStart, (int)VaEnd, (int)VaSteps);
                datafile.write(m);

                sprintf(m,"Vs\t%d\t%d\t%d\n",(int)VsStart, (int)VsEnd, (int)VsSteps);
                datafile.write(m);

                sprintf(m,"Vg\t%d\t%d\t%d\n",(int)VgStart, (int)VgEnd, (int)VgSteps);
                datafile.write(m);
            }
            datafile.write("Point\tCurve\tIa\tIs\tVg\tVa\tVs\tVf\tLimit\n");
            if (LOGGING) QTextStream(&logFile) << "Header Written\n";

            if (dataStore->length()>0) {
                if (LOGGING) QTextStream(&logFile) << "Start of data section\n";
                //data
                results_t data_set;
                int curve=1, point=1, c=0;
                for(int i=0; i < dataStore->length(); i++) {
                    data_set = dataStore->at(i);
                    sprintf(m,"%d\t%d\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d\n",
                            point,curve,data_set.Ia,data_set.Is,data_set.Vg,data_set.Va,data_set.Vs,data_set.Vf,data_set.ignore);
                    point++;
                    if (c==VaSteps) {
                        curve++; c=0; point=1;
                    } else c++;

                    datafile.write(m);
                }
                if (LOGGING) QTextStream(&logFile) << "Data Written\n";
                if (ui->checkQuickTest->isChecked()) {

                    QTextStream(&datafile) << QuickRes;
                }
                datafile.close();
            }
        } else
            QMessageBox::information(this, "File Error", "Problem creating data file. Check the filename and/or path");

    }
}

bool MainWindow::SetUpSweepParams() {

    bool ok;
    float Vf;
    //check the entries
    VaSteps = ui->VaSteps->text().toFloat(&ok);
    if (!ok){
        QMessageBox::information(this, "Entry Error", "Invalid entry for number of Va steps");
        return false;
    }
    VsSteps = ui->VsSteps->text().toFloat(&ok);
    if (!ok){
        QMessageBox::information(this, "Entry Error", "Invalid entry for number of Vs steps");
    return false;
    }

    VgSteps = ui->VgSteps->text().toFloat(&ok);
    if (!ok) {
        QMessageBox::information(this, "Entry Error", "Invalid entry for number of Vg steps");
    return false;
    }
    VaStart = ui->VaStart->text().toFloat(&ok);
    if (!ok) {
        QMessageBox::information(this, "Entry Error", "Invalid entry for Va start");
    return false;
    }
    VsStart = ui->VsStart->text().toFloat(&ok);
    if (!ok) {
        QMessageBox::information(this, "Entry Error", "Invalid entry for Vs start");
    return false;
    }
    VgStart = ui->VgStart->text().toFloat(&ok);
    if (!ok) {
        QMessageBox::information(this, "Entry Error", "Invalid entry for Vg start");
    return false;
    }
    VaEnd = ui->VaEnd->text().toFloat(&ok);
    if (!ok) {
        QMessageBox::information(this, "Entry Error", "Invalid entry for Va stop");
    return false;
    }
    VsEnd = ui->VsEnd->text().toFloat(&ok);
    if (!ok) {
        QMessageBox::information(this, "Entry Error", "Invalid entry for Vs stop");
    return false;
    }
    VgEnd = ui->VgEnd->text().toFloat(&ok);
    if (!ok){
        QMessageBox::information(this, "Entry Error", "Invalid entry for Vg stop");
        return false;
    }
    //check steps range
    if (VaSteps<0) {
        QMessageBox::information(this, "Entry Error", "Va steps must be > 0 ");
        return false;
    }
    //check steps range
    if (VsSteps<0) {
        QMessageBox::information(this, "Entry Error", "Vs steps must be >0");
        return false;
    }
    //check steps range
    if (VgSteps<0) {
        QMessageBox::information(this, "Entry Error", "Vg steps must be >0");
        return false;
    }
    if ((VgSteps+1) * (VsSteps+1) * (VaSteps+1) > 50000) {
        QMessageBox::information(this, "Entry Error", "Total number of data points > 50000. Use less steps");
        return false;
    }
    //check Va, Vs, Vg range
    QString msg;
    QTextStream(&msg) << "Va start is " << VaStart << " but must be between " << VMIN << " and " << calData.VaMax;
    if (VaStart<VMIN || VaStart> calData.VaMax) {
        QMessageBox::information(this, "Entry Error", msg);
        return false;
    }
    msg.clear();
    QTextStream(&msg) << "Va End is " << VaEnd << " but must be between " << VaStart << " and " << calData.VaMax;
    if (VaEnd<VaStart || VaEnd > calData.VaMax) {
        QMessageBox::information(this, "Entry Error", msg);
        return false;
    }

    msg.clear();
    QTextStream(&msg) << "Vs start must be between " << VMIN << " and " << calData.VaMax;
    if (VsStart<VMIN || VsStart> calData.VaMax) {
        QMessageBox::information(this, "Entry Error ", msg);
        return false;
    }
    msg.clear();
    QTextStream(&msg) << "Vs End is " << VsEnd << " but must be between " << VsStart << " and " << calData.VaMax;
    if (VsEnd<VsStart || VsEnd> calData.VaMax) {
        QMessageBox::information(this, "Entry Error ", msg);
        return false;
    }
    //Check Vg range
    msg.clear();
    float VgMax= calData.VgMax;
    float VgPos = 0;
    if (options.uTracer6) VgPos=VgMax;
    QTextStream(&msg) << "Vg start is " << VgStart << " but must be between " << VgPos << " and " << (-VgMax);
    if (VgStart>VgPos || VgStart < (-VgMax)) {
        QMessageBox::information(this, "Entry Error", msg);
        return false;
    }
    msg.clear();
    QTextStream(&msg) << "Vg end is " << VgEnd << " but must be between " << VgPos << " and " << (-VgMax);
    if (VgEnd>VgPos || VgEnd < (-VgMax)) {
        QMessageBox::information(this, "Entry Error", msg);
        return false;
    }
    //End > Start?
    if (VsEnd < VsStart ) {
        QMessageBox::information(this, "Entry Error", "Vs end must be greater than start");
        return false;
    }
    if (VsEnd < VsStart) {
        QMessageBox::information(this, "Entry Error", "Vs end must be greater than start");
        return false;
    }
    if (VgEnd > VgStart ) {
        QMessageBox::information(this, "Entry Error", "Vg end must be less than start");
        return false;
    }
    //Heater
    Vf = ui->VhVal->text().toFloat(&ok);
    if (!ok) {
        QMessageBox::information(this, "Entry Error", "Heater voltage entry is not valid.");
        return false;
    }
    if ((Vf<0) || (Vf>18)) {
        QMessageBox::information(this, "Entry Error", "Heater voltage range must be between 0 and 18.");
        return false;
    }
    return true;
}


void MainWindow::on_Start_clicked() {
    if (startSweep==0) {
        if (!SetUpSweepParams()) return;
        plot1.complete= false;
        tubeData.last_time= QDateTime::currentSecsSinceEpoch();
        tubeData.times_used += 1;

        if (tubeDataVec.at(ui->TubeSelector->currentIndex()).ID != tubeData.ID) qDebug() << "StartClick: Error- ID mismatch";
        else tubeDataVec.at(ui->TubeSelector->currentIndex()) = tubeData;
        SortTubeData(0);
        SaveTubeDataFile();
        SortTubeData(options.TLSortOrder);
    }
    ui->MsgLabel->setText("");
    if (startSweep<2) {
        startSweep+=1;
        stop=false;
    }
}

void MainWindow::CreateTestVectors()
{
    //create test vectors
    sweepList->clear();
    tubeData.powerLim = ui->PowLim->text().toFloat();
    VaNow = ui->VaStart->text().toFloat();
    VsNow = ui->VsStart->text().toFloat();
    VgNow = ui->VgStart->text().toFloat();
    VaStart = ui->VaStart->text().toFloat();
    VsStart = ui->VsStart->text().toFloat();
    VgStart = ui->VgStart->text().toFloat();
    VaEnd = ui->VaEnd->text().toFloat();
    VsEnd = ui->VsEnd->text().toFloat();
    VgEnd = ui->VgEnd->text().toFloat();

    // ---  /Quick Test ---
    if (ui->checkQuickTest->isChecked()){
        if ((tubeData.Model==TRIODE) || (tubeData.Model==DUAL_TRIODE)) {
            ui->Vcs->setText( ui->Vca->text() );
        }
        test_vector.Va = ui->Vca->text().toFloat()*(1 - ui->DeltaVa->text().toFloat()/200);
        test_vector.Vs = ui->Vcs->text().toFloat();
        test_vector.Vg = ui->Vcg->text().toFloat();
        sweepList->append(test_vector);// Va

        test_vector.Va = ui->Vca->text().toFloat()*(1 + ui->DeltaVa->text().toFloat()/200);
        sweepList->append(test_vector); //Va

        test_vector.Va = ui->Vca->text().toFloat();
        test_vector.Vs = ui->Vcs->text().toFloat()*(1 - ui->DeltaVs->text().toFloat()/200);
        sweepList->append(test_vector);//Vs

        test_vector.Vs = ui->Vcs->text().toFloat()*(1 + ui->DeltaVs->text().toFloat()/200);
        sweepList->append(test_vector);//Vs
        // skip Vg changes for diodes
        if ( !((tubeData.Model==DIODE) || (tubeData.Model==DUAL_DIODE)))  {
         //   ui->Vcs->setText( ui->Vca->text() );

            test_vector.Vs = ui->Vcs->text().toFloat();
            test_vector.Vg = ui->Vcg->text().toFloat()*(1 - ui->DeltaVg->text().toFloat()/200);
            sweepList->append(test_vector);//Vg

            test_vector.Vg = ui->Vcg->text().toFloat()*(1 + ui->DeltaVg->text().toFloat()/200);
            sweepList->append(test_vector);//Vg
        }

        //-- Bias current---
        test_vector.Va = ui->EmmVa->text().toFloat();
        test_vector.Vs = ui->EmmVs->text().toFloat();
        test_vector.Vg = ui->EmmVg->text().toFloat();
        sweepList->append(test_vector);
    } else {
        //--pentodes---
        if (ui->checkVsEqVa->isChecked())
        {
            //make vs==Va
            for (int g = 0; g <= VgSteps; g ++)
            {
                test_vector.Vg = VgNow;
                VaNow = ui->VaStart->text().toFloat();
                if (VgSteps>0) VgNow += (VgEnd - VgStart)/(float)VgSteps;
                for (int a = 0; a <= VaSteps; a ++)
                {
                    test_vector.Vs = test_vector.Va = VaNow;
                    sweepList->append(test_vector);
                    if (VaSteps>0) VaNow += (VaEnd - VaStart)/(float)VaSteps;
                }
            }

        }
        else
        {
            //if using the pentode models, do a triode connected sweep first (for parameter estimation)
            //make vs==Va for a triode sweep
            if (ui->TubeType->currentText()!=NONE) { //skip if not modelling
            VgNow = ui->VgStart->text().toFloat();
            if (tubeData.Model==KOREN_P || tubeData.Model==DERK_P || tubeData.Model==DERK_P2 || tubeData.Model==DERK_PE || tubeData.Model==DERK_B)
                for (int g = 0; g <= VgSteps; g ++)
                {
                    test_vector.Vg = VgNow;
                    if (VgSteps>=1) VgNow += (VgEnd - VgStart)/(float)VgSteps;
                    VaNow = ui->VaStart->text().toFloat();
                    for (int a = 0; a <= VaSteps; a ++)
                    {
                        test_vector.Vs = test_vector.Va = VaNow;
                        sweepList->append(test_vector);
                        if (VaSteps>0) {
                            VaNow += (VaEnd - VaStart)/(float)VaSteps;
                            //VaNow = VaStart + (VaEnd-VaStart)*(pow(2,a/VaSteps)-1);
                        }
                    }
                }
            }
            if (tubeData.Model==DIODE || tubeData.Model==TRIODE) {
                //No Vs sweep
                VsNow = ui->VsStart->text().toFloat();
                test_vector.Vs = VsNow;
                VgNow = ui->VgStart->text().toFloat();
                for (int g = 0; g <= VgSteps; g++)
                {
                    test_vector.Vg = VgNow;
                    if (VgSteps>=1) VgNow += (VgEnd - VgStart)/(float)VgSteps;
                    VaNow = ui->VaStart->text().toFloat();
                    for (int a = 0; a <= VaSteps; a++)
                    {
                        if (VaNow>calData.VaMax) VaNow=calData.VaMax;
                        test_vector.Va = VaNow;
                        sweepList->append(test_vector);
                        if (VaSteps>=1) VaNow += (VaEnd - VaStart)/(float)VaSteps;
                    }
                }

            }
            else {
                //normal exhaustive sweep
                VsNow = ui->VsStart->text().toFloat();
                for (int s = 0; s <= VsSteps; s++)
                {
                    if (VsNow>calData.VaMax) VsNow=calData.VaMax;
                    test_vector.Vs = VsNow;
                    if (VsSteps>=1) VsNow += (VsEnd - VsStart)/(float)VsSteps;
                    VgNow = ui->VgStart->text().toFloat();
                    for (int g = 0; g <= VgSteps; g++)
                    {
                        test_vector.Vg = VgNow;
                        if (VgSteps>=1) VgNow += (VgEnd - VgStart)/(float)VgSteps;
                        VaNow = ui->VaStart->text().toFloat();
                        for (int a = 0; a <= VaSteps; a++)
                        {
                            if (VaNow>calData.VaMax) VaNow=calData.VaMax;
                            test_vector.Va = VaNow;
                            sweepList->append(test_vector);
                            if (VaSteps>=1) VaNow += (VaEnd - VaStart)/(float)VaSteps;
                        }
                    }
                }
            }
        }
    }

}

void MainWindow::on_VsStart_editingFinished() {
}

void MainWindow::on_Stop_clicked() {
    stop=true;
    startSweep=0;
}


/*
void MainWindow::on_plotDockWidget_dockLocationChanged(const Qt::DockWidgetArea &area)
{
//    ui->statusBar->showMessage("Dock Moved");
}


void MainWindow::on_plotDockWidget_topLevelChanged(bool topLevel)
{
    QRect winrect = QMainWindow::geometry();
    if (topLevel) {
        QMainWindow::setFixedSize(660,560);
    }
    else {
        QMainWindow::setFixedSize(1300,575);
    }
}
*/

void MainWindow::on_actionSave_Spice_Model_triggered()
{
    QString fn = ui->AutoFileName->text();
    fn = ui->AutoPath->text()  ;
    fn.append("/");
    fn.append(ui->AutoFileName->text())  ;
    fn.append("-");
    fn.append(ui->AutoNumber->text());
    optimizer->Save_Spice_Model(this, ui->TubeSelector->currentText().toLatin1(), fn) ;
}


//read a reference data set
void MainWindow::on_actionRead_Data_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Data File"),ui->AutoPath->text(), tr("Data Files (*.txt)"));
    QFile datafile(fileName);
    if (datafile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        refStore->clear();
        QTextStream in(&datafile);
        QString line = in.readLine();
        bool valid;
        if (valid = (line == "Data 2")) {
            //read type
            line =in.readLine();
            int i = ui->TubeSelector->findText(line); //name
            if (i !=-1 ) {
                //on_TubeSelector_currentIndexChanged_enable=false;
                ui->TubeSelector->setCurrentIndex(i);
                //on_TubeSelector_currentIndexChanged_enable=true;
            }
            else valid =false;
            //read model
            line =in.readLine();
            i= ui->TubeType->findText(line); //set model
            if ( i!=-1 ) {
                on_TubeType_currentIndexChanged_enable=false;
                ui->TubeType->setCurrentIndex(i);
                on_TubeType_currentIndexChanged_enable=true;
            } else valid =false;

            if (valid) {
                while ( !in.atEnd() && valid) {
                    line = in.readLine();
                    //Skip comments to get step and range info
                    //qDebug() << "Read Data Set" << line;
                    QStringList list = line.split('\t');
                    if (line.at(0).toLatin1()==';') {
                        //comment
                    }
                    else if (list.at(0)=="Va") {
                        if (list.length()==4) {
                            VaStart = list.at(1).toFloat();
                            VaEnd = list.at(2).toFloat();
                            VaStep = VaSteps = list.at(3).toFloat();
                        } else valid=false;
                    }
                    else if (list.at(0)=="Vs") {
                        if (list.length()==4) {
                            VsStart = list.at(1).toFloat();
                            VsEnd = list.at(2).toFloat();
                            VsStep = VsSteps = list.at(3).toFloat();
                        } else valid=false;
                    }
                    else if (list.at(0)=="Vg") {
                        if (list.length()==4) {
                            VgStart = list.at(1).toFloat();
                            VgEnd = list.at(2).toFloat();
                            VgStep = VgSteps = list.at(3).toFloat();
                        } else valid=false;
                    }
                    else if (list.at(0)=="Point") {
                    }
                    else {
                        results.Ia = list.at(2).toFloat();
                        results.Is = list.at(3).toFloat();
                        results.Vg = list.at(4).toFloat();
                        results.Va = list.at(5).toFloat();
                        results.Vs = list.at(6).toFloat();
                        results.Vf = list.at(7).toFloat();
                        results.IaMod=0;
                        results.IsMod=0;
                        results.gm_a=0;
                        results.gm_b=0;
                        results.ra_a=0;
                        results.ra_b=0;
                        results.mu_a=0;
                        results.mu_b=0;
                        results.ignore=0;
                        if (list.length()>=9) {
                            results.ignore|=list.at(8).toInt(); //current & power limit
                        }
                        refStore->append(results);
                    }
                }
            }
            datafile.close();
            if (valid)  {
                ui->statusBar->showMessage(tr("Sucessfully read data file"));
                optimizer->Optimize(refStore, ui->TubeType->currentIndex(), 1, VaSteps, VsSteps, VgSteps);
                if (optimizer->Status())
                    ui->statusBar->showMessage("Info: model fit is poor. Try a different model or change your sweep settings");
                else
                    ui->statusBar->showMessage("Model fit OK");
                ui->VaStart->setText(QString::number(VaStart));
                ui->VaEnd->setText(QString::number(VaEnd));
                ui->VaSteps->setText(QString::number(VaSteps));

                ui->VsStart->setText(QString::number(VsStart));
                ui->VsEnd->setText(QString::number(VsEnd));
                ui->VsSteps->setText(QString::number(VsSteps));

                ui->VgStart->setText(QString::number(VgStart));
                ui->VgEnd->setText(QString::number(VgEnd));
                ui->VgSteps->setText(QString::number(VgSteps));

                //ui->PlotArea->clearGraphs();
                updateLcdsWithModel();
                updateTubeGreying();
                plot1.complete= true;
                RePlot(refStore);
            } else ui->statusBar->showMessage(tr("Invalid data file: tube type missing from list??"));
        } else ui->statusBar->showMessage(tr("Invalid data file. Quick test files cannot be read."));
    }
}

void MainWindow::on_checkVsEqVa_clicked()
{
    updateSweepGreying();
    /*
     * if(ui->checkVsEqVa->isChecked()) {
        ui->VsStart->setDisabled(true);
        ui->VsEnd->setDisabled(true);
        ui->VsSteps->setDisabled(true);
    }
    else {
        ui->VsStart->setEnabled(true);
        ui->VsEnd->setEnabled(true);
        ui->VsSteps->setEnabled(true);
    } */
}

void MainWindow::on_checkQuickTest_clicked()
{
    updateSweepGreying();
}

/*------------------------------------------
 * Tab Management
 *------------------------------------------*/
void MainWindow::on_Tabs_currentChanged(int index)
{

    if (ignoreIndexChange==false) {
        if (index==ui->Tabs->count()-1  && index>0) {
            PlotTabWidget * plotTab  = new PlotTabWidget();
            ignoreIndexChange=true;
            ui->Tabs->insertTab(index,plotTab,QString::number(index));
            plotTab->BackGroundColor(plotBackGroundColor);
            plotTab->RectColor(plotAreaColor);
            plotTab->GridColor(plotGridColor);
            plotTab->TextColor(plotTextColor);
            plotTab->AxisColor(plotAxisColor);
            plotTabs.append(plotTab);
            ui->Tabs->setCurrentIndex(index);
            RePlot(activeStore);
            connect(plotTab, &PlotTabWidget::penChanged, this, &MainWindow::PenUpdate);
            ui->Tabs->tabBar()->tabButton(ui->Tabs->count()-1, QTabBar::RightSide)->deleteLater();
            ui->Tabs->tabBar()->setTabButton(ui->Tabs->count()-1, QTabBar::RightSide, 0);
        }
    }
    else {
        ignoreIndexChange =  false;
    }
}

void MainWindow::on_Tabs_tabCloseRequested(int index)
{
    if (index<(ui->Tabs->count()-1)  && index>1) { //prevent the default plot tab from being closed
        ignoreIndexChange=true;
        disconnect(plotTabs.at(index-1), &PlotTabWidget::penChanged, this, &MainWindow::SaveCalFile);
        delete plotTabs.at(index-1);
        plotTabs.removeAt(index-1);
        //ui->Tabs->removeTab(index);
        ui->Tabs->setCurrentIndex(index-1);
        //while (c==ui->Tabs->count()); //wait for tab to be removed
        for (int i=2 ; i < ui->Tabs->count()-1; i++) {
            ui->Tabs->setTabText(i,QString::number(i));
        }
        ui->Tabs->setTabText(ui->Tabs->count()-1,"New");
        ignoreIndexChange=false;
    }
}

void MainWindow::PenUpdate() {
    RePlot(activeStore);
    SaveCalFile();
}


void MainWindow::on_Browse_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                     QDir::homePath(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);
    if (path.length()>0) ui->AutoPath->setText(path);
    on_AutoPath_editingFinished();
}

void MainWindow::on_checkHtrHold_stateChanged(int arg1)
{
    if(!ui->checkHtrHold->isChecked()) HtrHoldEnd=true;
}

void MainWindow::SaveCtlState(void) {
    ui->VaSteps->setDisabled(true);
    ui->VsSteps->setDisabled(true);
    ui->VgSteps->setDisabled(true) ;
    ui->VaStart->setDisabled(true);
    ui->VsStart->setDisabled(true);
    ui->VgStart->setDisabled(true) ;
    ui->VaEnd->setDisabled(true) ;
    ui->VsEnd->setDisabled(true);
    ui->VgEnd->setDisabled(true);
    ui->Vca->setDisabled(true);
    ui->Vcs->setDisabled(true);
    ui->Vcg->setDisabled(true);
    ui->DeltaVa->setDisabled(true);
    ui->DeltaVs->setDisabled(true);
    ui->DeltaVg->setDisabled(true);
    ui->gm1Typ->setDisabled(true);
    ui->gm2Typ->setDisabled(true);
    ui->gm1Delta->setDisabled(true);
    ui->gm2Delta->setDisabled(true);
    ui->EmmVa->setDisabled(true);
    ui->EmmVs->setDisabled(true);
    ui->EmmVg->setDisabled(true);
    ui->TubeSelector->setDisabled(true);
    ui->TubeType->setDisabled(true);
    ui->checkQuickTest->setDisabled(true);
}

void MainWindow::RecallCtlState(void){
    updateTubeGreying();
    ui->TubeSelector->setDisabled(0);
    ui->TubeType->setDisabled(0);
    ui->checkQuickTest->setDisabled(0);
}

void MainWindow::LogFile(int arg1) {
    if (arg1==Qt::Unchecked) {
        if (logFile.isOpen()) logFile.close();
        fwdebug=false;
        stopDebugMsg=true;
    }
    else
    {
        fwdebug=true;
        enableDebugMsg=true;
        QString dataFileName = QDir::homePath();
        dataFileName.append("/log.txt");
        logFile.setFileName(dataFileName);
        //logFile.open(QIODevice::Append);
        logFile.open(QIODevice::WriteOnly );
        QTextStream(&logFile) << "New Session\n";

    }
}


void MainWindow::update_FW() {
    do_fw_update=true;
}
#define  READ_VERSION   '\x00'
#define  WRITE_FLASH    '\x02'
#define  ERASE_FLASH    '\x03'
#define  CALC_CHECKSUM  '\x08'
#define  RESET_DEVICE   '\x09'

void MainWindow::Erase_Flash() {
    //open the firmware file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select firmware file"),calData.SavePath, tr("Binary (*.bin)"));
    QFile datafile(fileName);
    if (datafile.open(QIODevice::ReadOnly)) {
        ui->statusBar->showMessage("Erasing flash");
        fw.clear();
        fw = datafile.readAll();
        datafile.close();
        TxString.clear();
        TxString.append(ERASE_FLASH);
        TxString.append('\x00'); //data length low byte
        TxString.append('\x00'); //data length high byte
        cmd=TxString ;
        rxLen = 4;
        status=pgm_flash;
        sendSer();
        timeout = ADC_READ_TIMEOUT;
        timer_on=true;
    } else {
        status=Idle;
        update_failed=true;
    }
}

void MainWindow::Pgm_Flash() {
    static uint pf=0;
    static uint fw_index=0;
    static uint checksum =0;
    uint c;
    QByteArray txbytes;

    switch (pf) {
    case 0:
        if (response.at(0) == 0) {
            ui->statusBar->showMessage("Error while erasing flash");
            update_failed=true;
            pf=0;
            timer_on=0;
            status=Idle;
            rxLen=0;
            cmd.clear();
            rxLen=0;
            status=Idle;
            return;
        }
        pf=1;
        checksum=0;
        fw_index=0;
        //if OK we fall through to case 1
    case 1: //pgm flash
        if (response.at(0) == 0) { //response.at(0) is always 1 after a fall thru from case 0
            ui->statusBar->showMessage("Error while programming flash");
            update_failed=true;
            rxLen=0;
            cmd="";
            status=Idle;
            pf=0;
            return;
        }
        if (fw_index< fw.length()) {
//        if (fw_index < 256*1) {
            //pgm flash
            QString msg = QString("Program flash block %1").arg(fw_index>>8);
            ui->statusBar->showMessage(msg);
            TxString.clear();
            TxString.append(WRITE_FLASH);
            TxString.append('\x00'); //data length low byte
            TxString.append('\x01'); //data length high byte
            cmd =  TxString;
            TxString.append(fw.mid(fw_index,256));
            for (int i = fw_index; i<(256+fw_index); i++) {
                if (i & 1) checksum += (uint)fw.at(i) & 0x3f;
                else checksum += (uint)fw.at(i) & 0xff;
            }
            fw_index+=256;
            rxLen = 4;
            sendSer();
            timeout = ADC_READ_TIMEOUT;
            pf=1;
        } else {
            //Done programming, verify checksum
            ui->statusBar->showMessage("Verifying update");
            TxString.clear();
            TxString.append(CALC_CHECKSUM);
            TxString.append('\x00'); //data length low byte
            TxString.append('\x00'); //data length high byte
            cmd=TxString ;
            rxLen = 5;
            sendSer();
            pf=2;
        }
        break;
    case 2: //verify checksum & reset
        c= response.at(0) & 0xff;
        c += (response.at(1)<<8);
        c &= 0xffff;
        checksum &= 0xffff;
        if (checksum == c) {
            ui->statusBar->showMessage("Verified update");
            update_failed=false;
            pf=3;
            TxString.clear();
            TxString.append(RESET_DEVICE);
            TxString.append('\x00'); //data length low byte
            TxString.append('\x00'); //data length high byte
            cmd=TxString;
            rxLen = 4;
            sendSer();
        }
        else {
            ui->statusBar->showMessage("Update failed, contact factory for help.");
            update_failed=true;
            pf=0;
            timer_on=0;
            status=Idle;
            rxLen=0;
            cmd.clear();
            rxLen=0;
            status=Idle;
        }
        break;
    case 3: //reset done
        pf=0;
        readVersion=true;
        timer_on=0;
        status=Idle;
        rxLen=0;
        cmd.clear();
        break;
    }
}



void MainWindow::on_PowLim_editingFinished()
{
    RePlot(activeStore);
}



