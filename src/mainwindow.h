#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define TIMER_SET 250
#define ADC_READ_TIMEOUT (30000/TIMER_SET)
#define PING_TIMEOUT (2000/TIMER_SET)
#define HEAT_CNT_MAX (10000/TIMER_SET)
#define HEARTBEAT (20000/TIMER_SET)
#define RETRYTIME (10000/TIMER_SET)
//#define HEAT_WAIT_SECS 60


#include <QMainWindow>
//#include "../3rdparty/qextserialport-1.2rc/src/qextserialport.h"
//#include "../3rdparty/qextserialport-1.2rc/src/qextserialenumerator.h"
#include <QQueue>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include "typedefs.h"
#include "kb_optimize.h"
#include "dr_optimize.h"
#include <QComboBox>
#include <qcustomplot.h>
#include "plotTabWidget.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    struct calData_t
    {
        float VaVal;
        float VsVal;
        float IaVal;
        float IsVal;
        float VsuVal;
        float Vg0;
        float Vgm;
        float Vg05Val;
        float Vg1Val;
        float Vg4Val;
        float Vg40Val;
        float VnVal;
        float RaVal;
        float VaMax;
        float IaRsense;
        float IsRsense;
        float VgMax;
        float heaterscale; //Max heater voltage, only use if external heater supply enabled
        short Firmware; // == 1 for uTracer
        QString SavePath;
        QString Style;
    } calData;

    QString dataFileName;
    QString calFileName;

    void ReadCalibration();
    void ReadDataFile();
    void SaveCalToChip(bool);
    void update_FW();
    ~MainWindow();

    void SaveCalFile();
    void GetReal();
    void DataSaveDialog_clicked(const QString &);
    void Apply_Mode(settings_t);
    void CalcVnCal(float m);
    void CalcVpCal(float m);
    void CalcVaCal(float m);
    void CalcVsCal(float m);
    void CalcIaCal(float m);
    void CalcIsCal(float m);
    void CalcVg05Cal(float m);
    void CalcVg1Cal(float m);
    void CalcVg4Cal(float m);
    void CalcVg40Cal(float m);
    void CalcVg(float v1, float v2);
    void Update_adc_scales(void);
    void GetADC();

    struct adc_data_t
    {
        int Ia;
        int Ia_raw;
        int Is;
        int Is_raw;
        int Va;
        int Vs;
        int Vsu;
        int Vn;
        int Gain_a;
        int Gain_s;
    };
    // This holds the ADC data from uTracer
    adc_data_t adc_data;

    struct adc_scale_t
    {
        float Ia;
        float Ia_raw;
        float Is;
        float Is_raw;
        float Va;
        float Vs;
        float Vf;
        //float Ia400;
        //float Ia400_raw;
        //float Is400;
        //float Is400_raw;
        float Vg;
        float Vsu;
        float Vn;
        float Gain_a;
        float Gain_s;
    };
    adc_scale_t adc_scale;  //the ADC scale factors
    adc_scale_t adc_real;   //the real version of the ADC readings
    struct options_t {
        int IaRange;
        int IsRange;
        int Delay;
        int Ilimit;
        int AvgNum;
        bool AbortOnLimit;
        float VgScale;
        float VaScale;
        int TLSortOrder;
        bool uTracer6;
        bool ExtHtr;
    };
    options_t options;

    QString uTmaxDir;
    QByteArray TxString;
    QByteArray statusString;
    QByteArray echoString;
    bool sendADC;
    bool sendPing;
    float VaSteps;
    float VsSteps;
    float VgSteps;
    float VaStart;
    float VsStart;
    float VgStart;
    float VaEnd;
    float VsEnd;
    float VgEnd;
    int VaADC;
    int VsADC;
    int VgADC;
    int VfADC;
    float VaNow;
    float VsNow;
    float VgNow;
    float VfNow;
    bool fwdebug;
    //QextSerialPort * portInUse;
    //QList<QextPortInfo> serPortInfo;
    QSerialPort * portInUse;
    QList<QSerialPortInfo> serPortInfo;
    QString comport;
    void OpenComPort(const QString *);
    void LogFile(int arg1);
    void SetStyleSheet(void);
    bool calWizStep;

public slots:
    //void onDeviceDiscovered(const QextPortInfo & );
    //void onDeviceRemoved(const QextPortInfo & );
    //TODO Update plot when this happens:
    //void ErrorWeightClicked();
    void on_TLsort_changed();

private slots:
    void readData();
    void on_actionExit_triggered();
    void on_actionCal_triggered();
    void on_actionOptions_triggered();
    void on_actionSave_plot_triggered();
    void on_actionSave_Data_triggered();
    void on_actionDebug_triggered();
    void on_Start_clicked();
    void on_VsStart_editingFinished();
    void RxData();
    void on_Stop_clicked();
    void on_TubeType_currentIndexChanged(int index);
    void on_actionSave_Spice_Model_triggered();
    //void on_plotDockWidget_dockLocationChanged(const Qt::DockWidgetArea &area);
    //void on_plotDockWidget_topLevelChanged(bool topLevel);
    void on_actionRead_Data_triggered();
    //void on_actionPreferences_triggered();
    void on_AutoPath_editingFinished();
    void on_AddType_clicked();
    void on_DeleteType_clicked();
    void on_checkVsEqVa_clicked();
    void on_checkQuickTest_clicked();
    void on_Tabs_currentChanged(int index);
    void on_Tabs_tabCloseRequested(int index);
    void on_Browse_clicked();
    void on_checkHtrHold_stateChanged(int arg1);

    void on_actionCalibration_Wizard_triggered();

    void on_PowLim_editingFinished();

    void on_TubeSelector_currentIndexChanged(int index);

signals:
    void Next();

private:

    Ui::MainWindow *ui;
    void End(bool);
    void PenUpdate();
    void updateLcdsWithModel();
    void SerialPortDiscovery();
    void updateSweepGreying();
    void updateTubeGreying();
    void CreateTestVectors();
    void ClearLCDs();
    int  DWait(float);
    void SaveCtlState(void);
    void RecallCtlState(void);
    void Idle_f();
    void Wait_Ping();
    void Wait_ADC();
    void Heating_Wait00();
    void Heating_Wait_ADC();
    void Heating_f();
    void Heat_Done();
    void Sweep_Set();
    void Hold();
    void Sweep_ADC();
    void HeatOff_f();
    void sendSer();
    void StoreData(bool);
    void SetUpPlot();
    bool SetUpSweepParams();
    void UpdateTitle();
    void writeLCDs(void);
    void DoPlot(plotInfo_t *  );
    void RePlot(QList<results_t> * );
    bool SaveTubeDataFile();
    void ReadChipCalData();
    void WriteChipCalData();
    void ReadVersion();
    void SortTubeData(int type);
    void updateTubeSelector(QString ID);
    uint8_t toUpper14(float c);
    uint8_t toLower14(float c);
    float from14bit(uint8_t upper, uint8_t lower);
    bool EraseChip;
    void Apply();
    void Apply_Response();
    void Apply_Measure();
    void Apply_End();
    void RxADC();
    bool do_fw_update;
    void Erase_Flash();
    void Pgm_Flash();
    void TubeSelector_StringChanged(const QString &arg1);


    enum RxCode_t {RXSUCCESS, RXTIMEOUT, RXCONTINUE, RXINVALID };
    RxCode_t RxCode;

    enum Status_t { WaitPing, Heating, Heating_wait00, Heating_wait_adc,
                    Sweep_set, Sweep_adc, Idle, wait_adc,
                    hold_ack, hold, heat_done, HeatOff, readcal, readversion,
                    writeChipCal, apply, apply_measure, apply_response, apply_end, rxadc,
                  erase_flash, pgm_flash};
    Status_t status;

/*
    enum ControlEn {VaStepsEn, VsStepsEn,VgStepsEn,VaStartEn,VsStartEn,VdStartEn,VaEndEn,VsEndEn,VgEndEn,VcaEn, VcsEn, VcgEn,
                  DeltaVaEn, DeltaVsEn, DeltaVgEn, gm1TypEn, gm2TypEn, gm1Delta, gm2Delta, EmmVaEn, EmmVsEn, EmmVgEn, TubeSelEn,
                   TubeTypeEn, lastEn, QuickTest };
    bool CtlEnState[lastEn]; */
    int delay;
    //I Limits         200,  175,  150,  125, 100,    75,   50,   25,  12mA,  Off
    const int lim[10]={0x8f, 0x8d, 0xad, 0xab, 0x84, 0xa4, 0xa2, 0xa1, 0x80, 0x00};
    const int avg[8]={0x40, 1, 2, 4, 8, 16, 32, 0x40};
    const int Ir[9]={8,0,1,2,3,4,5,6,7};
    QByteArray response;
    bool readChipCal;
    bool readVersion;
    bool saveCalToChip;
    int distime;
    char buf[30];
    int HV_Discharge_Timer;
    int time;
    int rxLen;
    QByteArray cmd;
    int startSweep;
    int VsStep;
    int VgStep;
    int VaStep;
    int curve;
    int heat;
    bool stop;
    bool timer_on;
    int timeout;
    QTimer * timer;
    void saveADCInfo(QByteArray *);
    uint GetVa(float);
    uint GetVs(float);
    uint GetVg(float);
    uint GetVf(float);
    bool ok;
    bool newMessage;
    bool HtrHoldEnd;
    QByteArray RxString;
    float Vdi;
    QByteArray fw;
    //test vector store
    struct test_vector_t {
        float Va, Vs, Vg;
    };

    test_vector_t test_vector;
    QList<test_vector_t> *sweepList;

    //results store
    results_t results;
    QList<results_t> *dataStore;
    QList<results_t> *refStore;
    QList<results_t> *activeStore;

    QString QuickRes;

    struct tubeData_t
    {
        QString ID;
        QString Anode;
        QString G2;
        QString G1;
        QString G1b;
        QString Cathode;
        QString G3;
        QString HtrA;
        QString HtrB;
        float HtrVolts;
        QString Model;
        float VaStart;
        float VaEnd;
        int VaStep;
        float VsStart;
        float VsEnd;
        int VsStep;
        float VgStart;
        float VgEnd;
        int VgStep;
        float Vca;
        float Dva;
        float Vcs;
        float Dvs;
        float Vcg;
        float Dvg;
        float powerLim;
        float EmmVa;
        float EmmVs;
        float EmmVg;
        float gm1Typ;
        float gm1Del;
        float gm2Typ;
        float gm2Del;
        int heatSecs;
        quint64 last_time;
        uint times_used;
        uint Va_eq_Vs;
    };
    tubeData_t tubeData;
    //typedef QList<tubeData_t> tubeDataList_t;
    //tubeDataList_t * tubeDataList;
    std::vector<tubeData_t> tubeDataVec;
    static bool sortbyTime(const tubeData_t & struct1, const tubeData_t & struct2);
    static bool sortbyFreq(const tubeData_t & struct1, const tubeData_t & struct2);
    static bool sortbyAlpha(const tubeData_t & struct1, const tubeData_t & struct2);
    bool on_TubeSelector_currentIndexChanged_enable;
    bool on_TubeType_currentIndexChanged_enable;
    void LabelPins(tubeData_t tubeData);
    bool enableDebugMsg;
    bool stopDebugMsg;
    plotInfo_t plot1;

    QList<QPen> * penList;
    int RxPkt(int len, QByteArray * pCmd, QByteArray * pResponse);
    QFile logFile;
    dr_optimize * optimizer;

    QList<PlotTabWidget*> plotTabs;
    bool ignoreIndexChange;
    QColor plotAreaColor;
    QColor plotBackGroundColor;
    QColor plotGridColor;
    QColor plotAxisColor;
    QColor plotTextColor;
    int plotGridLine;
    QColor plotLegendColor;
    QColor plotLegendTextColor;
    QColor plotSHowValColor;
    QLCDNumber * lcds[9];
    float lcdVals[9];
};


#endif // MAINWINDOW_H
