#include "plotTabWidget.h"
#include <QColor>
#include <QColorDialog>

PlotTabWidget::PlotTabWidget(QWidget *parent) :
    QWidget(parent)
{
    plot1.dataSet=0;
    x1cb = new QComboBox;
    x1cb->addItem("Va");
//    x1cb->addItem("Vg2");
    x1cb->addItem("Vg");
    x1cb->setToolTip("<html><head/><body><p>Select the independant variable for the X axis</p></body></html>");
    connect(x1cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PlotTabWidget::plotSettingChanged);

    y1cb = new QComboBox;
    y1cb->addItem("Ia(a)");
    y1cb->addItem("Ia(a) Smooth");
    y1cb->addItem("Ia(a) Model");
    y1cb->addItem("Ra(a)");
    y1cb->addItem("gm(a)");
    y1cb->addItem("mu(a)");
    y1cb->addItem("Ia(b)/g2");
    y1cb->addItem("Ia(b)/g2 Smooth");
    y1cb->addItem("Ia(b)/g2 Model");
    y1cb->addItem("Ra(b)");
    y1cb->addItem("gm(b)");
    y1cb->addItem("mu(b)");
    y1cb->addItem("Off");
    y1cb->setToolTip("<html><head/><body><p>Select the variable for the left Y axis</p></body></html>");
    connect(y1cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PlotTabWidget::plotSettingChanged);

    y2cb = new QComboBox;
    y2cb->addItem("Off");
    y2cb->addItem("Ia(a) Smooth");
    y2cb->addItem("Ia(a) Model");
    //y2cb->addItem("Power(a)");
    y2cb->addItem("Ra(a)");
    y2cb->addItem("gm(a)");
    y2cb->addItem("mu(a)");
    y2cb->addItem("Ia(b)/g2");
    y2cb->addItem("Ia(b)/g2 Smooth");
    y2cb->addItem("Ia(b)/g2 Model");
    y2cb->addItem("Ra(b)");
    y2cb->addItem("gm(b)");
    y2cb->addItem("mu(b)");
    y2cb->setToolTip("<html><head/><body><p>Select the variable for the right Y axis</p></body></html>");
    connect(y2cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PlotTabWidget::plotSettingChanged);

    checkLegend = new QCheckBox;
    checkLegend->setText("Legend");
    checkLegend->setToolTip("<html><head/><body><p>Display the plot legend</p></body></html>");

    connect(checkLegend, &QCheckBox::stateChanged,
            this, &PlotTabWidget::plotLegendChanged);

    checkPlotTriode = new QCheckBox;
    checkPlotTriode->setText("Triode Data");
    checkPlotTriode->setToolTip("<html><head/><body><p>Display only the data where Vg2=Va, Used when modelling pentodes.</p></body></html>");
    connect(checkPlotTriode, &QCheckBox::stateChanged,
            this, &PlotTabWidget::plotSettingChanged);

    CurveNum = new QSpinBox;
    CurveNum->setToolTip("<html><head/><body><p>Select the same Vs section for display. Zero selects all Vs Steps.</p></body></html>");
    CurveNum->setMinimum(0);
    CurveNum->setValue(0);
    connect(CurveNum, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &PlotTabWidget::plotSettingChanged);

    cnLabel = new QLabel("Vs Group");

    colorGroup = new QCheckBox;
    colorGroup->setText("Group");
    colorGroup->setToolTip("<html><head/><body><p>Groups plotline of same Vg2 by color</p></body></html>");
    connect(colorGroup, &QCheckBox::stateChanged, this, &PlotTabWidget::plotSettingChanged);

    powerCheck = new QCheckBox;
    powerCheck->setText("<Power");
    powerCheck->setToolTip("<html><head/><body><p>Hide points the exceed the power limit</p></body></html>");
    connect(powerCheck, &QCheckBox::stateChanged, this, &PlotTabWidget::plotSettingChanged);

    ShowValIndex = 0;

    hl = new QHBoxLayout(parent);
    hl->setContentsMargins(5,5,5,5);
    hl->addWidget(x1cb);
    hl->addWidget(y1cb);
    hl->addWidget(y2cb);
    hl->addWidget(checkLegend);
    hl->addWidget(checkPlotTriode);
    hl->addWidget(colorGroup);
    hl->addWidget(CurveNum);
    hl->addWidget(cnLabel);
    hl->addWidget(powerCheck);

    plotSetup = new QGroupBox(parent);
    plotSetup->setObjectName(QStringLiteral("plotCtl"));
    plotSetup->setTitle("Plot Setup");
    //plotSetup->setMaximumHeight(65);
    QSizePolicy hsp = plotSetup->sizePolicy();
    hsp.setHorizontalPolicy(QSizePolicy::Expanding);
    plotSetup->setSizePolicy(hsp);
    plotSetup->setLayout(hl);

    plotWidget = new QWidget(parent);
    plotArea = new QCustomPlot(plotWidget);
    //plotArea->setMaximumSize(641,421);
    //plotArea->setMinimumSize(641,421);
    vl = new QVBoxLayout(parent);
    vl->setContentsMargins(0,0,0,0);
    vl->setAlignment(Qt::AlignHCenter);
    vl->addWidget(plotSetup);
    vl->addWidget(plotArea);
    //vl->setGeometry(QRect(1,1,641,421));
    this->setLayout(vl);


    //set the defaults
    y2cb->setCurrentIndex(0); //Off
    y1cb->setCurrentIndex(0);//default Ia
    x1cb->setCurrentIndex(0); //default Va
    checkLegend->setChecked(true);
    checkPlotTriode->setChecked(false);

    // Initialise plot Area
    //X axis title,scale
    plotArea->xAxis->setLabel("Va");
    plotArea->xAxis->setRange(0,100);
    //Y1 axis title, scale
    plotArea->yAxis->setLabel("Ia mA");
    plotArea->yAxis->setRange(0,1);
    //Y2 axis title, scale
    plotArea->yAxis2->setLabel("Ia/g2 mA");
    plotArea->yAxis2->setRange(0,1);
    plotArea->yAxis2->setVisible(false);
    plotArea->legend->setVisible(checkLegend->checkState());
    //Interactions
    QList< QCPAxis *> axisList;
    axisList.append(plotArea->xAxis);
    axisList.append(plotArea->yAxis);
    axisList.append(plotArea->yAxis2);
    plotArea->setInteraction(QCP::iSelectPlottables,true);
    plotArea->setInteraction(QCP::iRangeDrag,true);
    plotArea->setInteraction(QCP::iRangeZoom,true);
    plotArea->axisRect()->setRangeDragAxes(axisList);
    plotArea->axisRect()->setRangeZoomAxes(axisList);
    //https://www.qcustomplot.com/documentation/classQCustomPlot.html#a5ee1e2f6ae27419deca53e75907c27e5
    //https://www.qcustomplot.com/documentation/classQCPAxisRect.html#a648cce336bd99daac4a5ca3e5743775d
    connect(plotArea, SIGNAL(selectionChangedByUser()), this, SLOT(PlotInteractionUser()));
    connect(plotArea, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(showVals(QMouseEvent*)));
    connect(plotArea, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(Freeze(QMouseEvent*)));
    //Title
    UpdateTitle("New");
    marker = new QCPItemTracer(plotArea);
    markerText = new QCPItemText(plotArea);
    plotArea->addLayer("markers");
    //markerList.clear();
    //markerTextList.clear();
//    plotArea->setRangeDrag(Qt::Vertical);
//    plotArea->setRangeZoom(Qt::Vertical);
}
QMetaObject::Connection mm;

PlotTabWidget::~PlotTabWidget()
{
    delete x1cb;
    delete y1cb;
    delete y2cb;
    delete checkLegend;
    delete checkPlotTriode;
    delete CurveNum;
    delete cnLabel;
    delete hl;
    delete plotSetup;
    delete plotWidget;
    delete plotArea;
    delete vl;
}

void PlotTabWidget::RectColor(QColor color) {
    plotAreaColor = color;
}

void PlotTabWidget::BackGroundColor(QColor color) {
    plotBackGroundColor = color;
}

void PlotTabWidget::GridColor(QColor color) {

    plotGridColor = color;
}

void PlotTabWidget::AxisColor(QColor color) {
    plotAxisColor = color;
}

void PlotTabWidget::TextColor(QColor color) {

    plotTextColor = color;
}

void PlotTabWidget::setGridLine(int line) {

    plotGridLine = line;
}
void PlotTabWidget::setLegendColor(QColor color) {

    plotLegendColor = color;
}
void PlotTabWidget::setLegendTextColor(QColor color) {

    plotLegendTextColor = color;
}

void PlotTabWidget::setShowValColor(QColor color){
    plotSHowValColor = color;
}

/*void PlotTabWidget::setLineColorList(QList<QColor> colors){
    plotLineColorList = colors;
}*/


void PlotTabWidget::updateColors(void) {
    plotArea->axisRect()->setBackground(QBrush(plotAreaColor,Qt::SolidPattern));
    plotArea->setBackground(QBrush(plotBackGroundColor,Qt::SolidPattern));
    plotArea->legend->setBrush(QBrush(plotBackGroundColor,Qt::SolidPattern));
    //QPen GridPen(plotGridColor, 1, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
    QPen GridPen(plotGridColor, 1, (Qt::PenStyle) plotGridLine, Qt::RoundCap, Qt::RoundJoin);
    plotArea->xAxis->grid()->setPen(GridPen);
    plotArea->yAxis->grid()->setPen(GridPen);
    plotArea->yAxis2->grid()->setPen(GridPen);
    plotArea->xAxis->setTickLabelColor(plotTextColor);
    plotArea->yAxis->setTickLabelColor(plotTextColor);
    plotArea->yAxis2->setTickLabelColor(plotTextColor);
    plotArea->xAxis->setLabelColor(plotTextColor);
    plotArea->yAxis->setLabelColor(plotTextColor);
    plotArea->yAxis2->setLabelColor(plotTextColor);
    title->setTextColor(plotTextColor);
    GridPen.setColor(plotAxisColor);
    GridPen.setStyle(Qt::SolidLine);
    plotArea->xAxis->setBasePen(GridPen);
    plotArea->yAxis->setBasePen(GridPen);
    plotArea->yAxis2->setBasePen(GridPen);
    plotArea->xAxis->setTickPen(GridPen);
    plotArea->yAxis->setTickPen(GridPen);
    plotArea->yAxis2->setTickPen(GridPen);
    plotArea->xAxis->setSubTickPen(GridPen);
    plotArea->yAxis->setSubTickPen(GridPen);
    plotArea->yAxis2->setSubTickPen(GridPen);
    plotArea->legend->setBrush(QBrush(plotLegendColor,Qt::SolidPattern));
    plotArea->legend->setTextColor(plotLegendTextColor);
    if (marker!=NULL) marker->setBrush(plotSHowValColor);
    if (marker!=NULL) markerText->setColor(plotSHowValColor);

    plotArea->replot();
}

void PlotTabWidget::plotSettingChanged() {
    ShowValIndex=0;
    if (plot1.dataSet!=0) plotUpdate(&plot1);
    showVals(0);
}

void PlotTabWidget::plotUpdate(plotInfo_t *plotinfo) {
    plot1 = *plotinfo;
    float VaStart = plotinfo->VaStart;
    float VsStart = plotinfo->VsStart;
    float VgStart = plotinfo->VgStart;
    float VaEnd = plotinfo->VaEnd;
    float VsEnd = plotinfo->VsEnd;
    float VgEnd = plotinfo->VgEnd;
    int VaSteps = plotinfo->VaSteps;
    int VsSteps = plotinfo->VsSteps;
    int VgSteps = plotinfo->VgSteps;
    bool bspline_fitted=false;


    CurveNum->setMaximum(VsSteps+1);
    int curve = CurveNum->value();

    ppenList = plotinfo->penList;
    QList<results_t> * activeStore = plotinfo->dataSet;
    if (activeStore==0) return;
    if (activeStore->length()==0) return;
    int i;//The index of a data set in dataStore is Va_step + (Va_Steps+1)*VgStep + (Va_Steps+1)*(Vs_Steps+1)*VsStep
    int gr;
    QVector<double> x,y1,y2, y1f, y2f;
    QPen grPen;

    //adjust start position of data set for allow for triode section
    int first = 0, last = activeStore->length() > (VaSteps+1)*(VgSteps+1) ? (VaSteps+1)*(VgSteps+1) :activeStore->length();
    bool plotTri = true;

    if (plotinfo->type==NONE && !plotinfo->VsEQVa) {
        plotTri = false;
        last = activeStore->length();
    } else if ( (plotinfo->type==KOREN_P || plotinfo->type==DERK_P  || plotinfo->type==DERK_P2 || plotinfo->type==DERK_PE || plotinfo->type==DERK_B)  && !checkPlotTriode->isChecked())
    {
        first = (VaSteps+1)*(VgSteps+1);
        last = activeStore->length();
        plotTri = false;
    }
    //limits
    float VaMax=0;
    float VsMax=0;
    float VgMin=0;
    float VgMax=-100;
    float IaMax=0;
    float IsMax=0;
    float gmMax=0;
    float raMax=0;
    float muMax=0;
    for(int i=first; i < last; i++) {
        results_t dataPoint = activeStore->at(i);
        if (VaMax < dataPoint.Va) VaMax=dataPoint.Va;
        if (VsMax < dataPoint.Vs) VsMax=dataPoint.Vs;
        if (VgMin > dataPoint.Vg) VgMin=dataPoint.Vg;
        if (VgMax < dataPoint.Vg) VgMax=dataPoint.Vg;
        if (IaMax < dataPoint.Ia) IaMax=dataPoint.Ia;
        if (IsMax < dataPoint.Is) IsMax=dataPoint.Is;
        if (gmMax < dataPoint.gm_a) gmMax=dataPoint.gm_a;
        if (gmMax < dataPoint.gm_b) gmMax=dataPoint.gm_b;
        if (raMax < dataPoint.ra_a) raMax=dataPoint.ra_a;
        if (raMax < dataPoint.ra_b) raMax=dataPoint.ra_b;
        if (muMax < dataPoint.mu_a) muMax=dataPoint.mu_a;
        if (muMax < dataPoint.mu_b) muMax=dataPoint.mu_b;
    }

    //plot
    plotArea->clearGraphs();
    plotArea->legend->setVisible(false);
    //plotArea->clearPlottables();
    //results_t r;
    results_t nextDataPoint;
    results_t dataPoint;
    QList<QPointF> iapoints;
    QList<QPointF> ispoints;
    QPointF point;
    bspline_info_t ia_bspline_info;
    bspline_info_t is_bspline_info;
    //int order;

    if (activeStore->length()-first<2) {plotArea->replot(); return;}
    if (x1cb-> currentText()=="Va") {
        //X axis title,scale
        plotArea->xAxis->setLabel("Va");
        plotArea->xAxis->setRange(0, 10*ceil(VaMax/10));
        //Build vectors
        float Vs = VsStart;
        float Vg = VgStart;
        for(int s=0,gr=0,gpnum=0; (s <= VsSteps) && (!plotTri || (s < 1)); s++) {
            if (VsSteps>=1) Vs = VsStart + (float)s*(VsEnd - VsStart)/(float)VsSteps;
            if (s == (curve-1) || curve==0 || plotTri) {                
                for(int g=0; (g <= VgSteps) ; g++) {
                    if (VgSteps>=1) Vg = VgStart + (float)g*(VgEnd - VgStart)/VgSteps;
                    x.clear();y1.clear();y2.clear();
                    //Fit a curve
                    iapoints.clear();
                    ispoints.clear();
                    bspline_fitted=false;
                    for(int a=0; (a <= VaSteps); a++) {
                        i = (VaSteps+1)*g + (VaSteps+1)*(VgSteps+1)*s + a + first;
                        //build a list of points for this curve
                        if ((i) < activeStore->length()) {
                            dataPoint = plotinfo->dataSet->at(i);
                            point.setX(dataPoint.Va);
                            point.setY(dataPoint.Ia);
                            iapoints.append(point);
                            point.setY(dataPoint.Is);
                            ispoints.append(point);
                            if (a==VaSteps && VaSteps>5) {
                                bSplineFit(&ia_bspline_info, &iapoints);
                                bSplineFit(&is_bspline_info, &ispoints);
                                bspline_fitted=true;
                            }
                        }
                    }
                    //plot the curves
                    for(int a=0; (a <= VaSteps); a++) {
                        i = (VaSteps+1)*g + (VaSteps+1)*(VgSteps+1)*s + a + first;
                        if ((i) < activeStore->length()) {
                            dataPoint = plotinfo->dataSet->at(i);
                            //Gap fill...
                            for( float f=0; f<4; f++) {
//                                if (!(dataPoint.ignore & IGNORE_CURRENT) &&
//                                     !((dataPoint.ignore & IGNORE_POWER) &&
//                                       powerCheck->isChecked())) {
                                bool plim = (dataPoint.Va * dataPoint.Ia > 1000*plotinfo->Power);
                                if (!(dataPoint.ignore & IGNORE_CURRENT) && !(plim && powerCheck->isChecked()) ) {
                                    //Linear interpolation
                                    nextDataPoint = plotinfo->dataSet->at(i);
                                    if (a!=VaSteps  && (i+1) < plotinfo->dataSet->length()) nextDataPoint = plotinfo->dataSet->at(i+1);
                                    float Ia=0;
                                    float Is=0;
//                                    if (f==0 ||
//                                         (!(nextDataPoint.ignore & IGNORE_CURRENT) &&
//                                         !((nextDataPoint.ignore & IGNORE_POWER) &&
//                                           powerCheck->isChecked()))) {
                                    plim = (nextDataPoint.Va * nextDataPoint.Ia > 1000*plotinfo->Power);
                                    if (f==0 ||
                                             (!(nextDataPoint.ignore & IGNORE_CURRENT) &&
                                             !(plim && powerCheck->isChecked()))) {
                                        float dx= f/4;
                                        float Va= dataPoint.Va + dx*(nextDataPoint.Va - dataPoint.Va);
                                        Ia = dataPoint.Ia + dx*(nextDataPoint.Ia - dataPoint.Ia);
                                        Is = dataPoint.Is + dx*(nextDataPoint.Is - dataPoint.Is);
                                        if (plotTri) Vs=Va;
                                        float IaSmooth=Ia;
                                        float IsSmooth=Is;
                                        float IaMod = 0;
                                        float IsMod = 0;
                                        float ra_a = 0;
                                        float ra_b = 0;
                                        float gm_a = 0;
                                        float gm_b = 0;
                                        float mu_a = 0;
                                        float mu_b = 0;
                                        float Vx;
                                        if (bspline_fitted) {
                                            Vx = Va-0.001;
                                            if (Vx>iapoints.last().x()) Vx= iapoints.last().x();
                                            IaSmooth =  bspline_eval(&ia_bspline_info, Vx);
                                            IsSmooth =  bspline_eval(&is_bspline_info, Vx);
                                        }
                                        if (plot1.optimizer->isValid() && plot1.type!=NONE) {
                                            IaMod = plot1.optimizer->IaCalc(Va, Vs, Vg);
                                            IsMod = plot1.optimizer->IsCalc(Va, Vs, Vg);
                                            ra_a = plot1.optimizer->RaCalc(Va, Vs, Vg);
                                            ra_b = plot1.optimizer->RaCalcB(Va, Vs, Vg);
                                            gm_a = plot1.optimizer->GmCalc(Va, Vs, Vg);
                                            gm_b = plot1.optimizer->GmCalcB(Va, Vs, Vg);
                                            mu_a = ra_a  *gm_a;
                                            mu_b = ra_b  *gm_b;
                                        }
                                        x.append(Va);
                                        //select data for y1 axis
                                        if (y1cb->currentText()=="Ia(a)") y1.append(Ia);
                                        else if (y1cb->currentText()=="Ia(a) Model") y1.append(IaMod);
                                        else if (y1cb->currentText()=="Ia(a) Smooth") y1.append(IaSmooth);
                                        else if (y1cb->currentText()=="Ia(b)/g2 Smooth") y1.append(IsSmooth);
                                        else if (y1cb->currentText()=="Ia(b)/g2") y1.append(Is);
                                        else if (y2cb->currentText()=="Ia(b)/g2 Model") y1.append(IsMod);
                                        else if (y1cb->currentText()=="Ra(a)") y1.append(ra_a);
                                        else if (y1cb->currentText()=="Ra(b)") y1.append(ra_b);
                                        else if (y1cb->currentText()=="gm(a)") y1.append(gm_a);
                                        else if (y1cb->currentText()=="gm(b)") y1.append(gm_b);
                                        else if (y1cb->currentText()=="mu(a)") y1.append(mu_a);
                                        else if (y1cb->currentText()=="mu(b)") y1.append(mu_b);
                                        //select data for y2 axis
                                        if (y2cb->currentText()=="Ia(b)/g2") y2.append(Is);
                                        else if (y2cb->currentText()=="Ia(a) Smooth") y2.append(IaSmooth);
                                        else if (y2cb->currentText()=="Ia(b)/g2 Smooth") y2.append(IsSmooth);
                                        else if (y2cb->currentText()=="Ia(a) Model") y2.append(IaMod);
                                        else if (y2cb->currentText()=="Ia(b)/g2 Model") y2.append(IsMod);
                                        else if (y2cb->currentText()=="Ra(a)") y2.append(ra_a);
                                        else if (y2cb->currentText()=="Ra(b)") y2.append(ra_b);
                                        else if (y2cb->currentText()=="gm(a)") y2.append(gm_a);
                                        else if (y2cb->currentText()=="gm(b)") y2.append(gm_b);
                                        else if (y2cb->currentText()=="mu(a)") y2.append(mu_a);
                                        else if (y2cb->currentText()=="mu(b)") y2.append(mu_b);
                                        //else if (y2cb->currentText()=="Power(a)") y2.append(1000*plotinfo->Power/Va);
                                    }
                                }
                                if (a==VaSteps) break;
                            }
                        }
                    }
                    Vs = round(10*Vs)/10;
                    Vg = round(10*Vg)/10;
                    plotArea->addGraph(plotArea->xAxis,plotArea->yAxis);
                    plotArea->graph(gr)->setData(x,y1)
                            ;
                    //----------- Set plotline pens--------------
                    //Are enough pens defined?
                    if (gpnum > (plotinfo->penList->length()-1)) {
                        //no pen so add one
                        plotinfo->penList->append(plotinfo->penList->at(gpnum % plotinfo->penList->length()));
                    }
                    if (colorGroup->isChecked()) plotArea->graph(gr)->setPen(plotinfo->penList->at(s));
                    else plotArea->graph(gr)->setPen(plotinfo->penList->at(gpnum));
                    //---------------------------------------------
                    QString grlabel;
                    //TODO: check - used to use Iaparams.penfun
                    if ( plotinfo->type==DIODE || plotinfo->type==TRIODE || plotinfo->type==DUAL_TRIODE || plotTri )
                        QTextStream(&grlabel)  << "Vg1:" << Vg;
                    else
                        QTextStream(&grlabel)  << "Vg1:" << Vg << " Vg2:" << Vs;
                    plotArea->graph(gr++)->setName(grlabel);
                    if (y2cb->currentText()!="Off") {
                        plotArea->addGraph(plotArea->xAxis,plotArea->yAxis2);
                        plotArea->graph(gr)->setData(x,y2);
                        if (colorGroup->isChecked()) grPen= plotinfo->penList->at(s); else grPen = plotinfo->penList->at(gpnum);
                        grPen.setStyle(Qt::DotLine);
                        plotArea->graph(gr)->setPen(grPen);
                        plotArea->graph(gr++)->setName(grlabel);
                    }
                    gpnum++;
                }
            }
        }
        if (bspline_fitted) {
            bSplineFree(&ia_bspline_info);
            bSplineFree(&is_bspline_info);
        }
    }

  /*  if (x1cb-> currentText()=="Vg2") {
        //X axis title,scale
        plotArea->xAxis->setLabel("Va/g2");
        plotArea->xAxis->setRange(0,VsMax);
        //Build vectors
        for(int a=0,gr=0,gpnum=0; (a <= VaSteps) ; a++) {
            if (a == (curve-1) || 0==curve  || plotTri) {
                for(int g=0; (g <= VgSteps) ; g++) {
                    x.clear();y1.clear();y2.clear();
                    for(int s=0; (s <= VsSteps)  && (!plotTri || (s < 1)); s++) {
                        i = a + (VaSteps+1)*g + (VaSteps+1)*(VgSteps+1)*s + first;
                        if (i < activeStore->length()) {
                            results_t dataPoint = activeStore->at(i);
                            if (dataPoint.Ia!=-1) {
                                x.append(dataPoint.Vs);
                                if (y1cb->currentText()=="Ia(a)") y1.append(dataPoint.Ia);
                                else if (y1cb->currentText()=="Ia(b)/g2") y1.append(dataPoint.Is);
                                else if (y1cb->currentText()=="Ra(a)") y1.append(dataPoint.ra_a);
                                else if (y1cb->currentText()=="Ra(b)") y1.append(dataPoint.ra_b);
                                else if (y1cb->currentText()=="gm(a)") y1.append(dataPoint.gm_a);
                                else if (y1cb->currentText()=="gm(b)") y1.append(dataPoint.gm_b);
                                else if (y1cb->currentText()=="mu(a)") y1.append(dataPoint.mu_a);
                                else if (y1cb->currentText()=="mu(b)") y1.append(dataPoint.mu_b);
                                if (y2cb->currentText()=="Ia(b)/g2") y2.append(dataPoint.Is);
                                else if (y2cb->currentText()=="Ia(a) Model") y2.append(dataPoint.IaMod);
                                else if (y2cb->currentText()=="Ia(b)/g2 Model") y2.append(dataPoint.IsMod);
                                else if (y2cb->currentText()=="Ra(a)") y2.append(dataPoint.ra_a);
                                else if (y2cb->currentText()=="Ra(b)") y2.append(dataPoint.ra_b);
                                else if (y2cb->currentText()=="gm(a)") y2.append(dataPoint.gm_a);
                                else if (y2cb->currentText()=="gm(b)") y2.append(dataPoint.gm_b);
                                else if (y2cb->currentText()=="mu(a)") y2.append(dataPoint.mu_a);
                                else if (y2cb->currentText()=="mu(b)") y2.append(dataPoint.mu_b);
                            }
                        }
                    }
                    plotArea->addGraph(plotArea->xAxis,plotArea->yAxis);
                    plotArea->graph(gr)->setData(x,y1);
                    //Are enough pens defined?
                    if (gpnum > (plotinfo->penList->length()-1)) {
                        //no pen so add one
                        plotinfo->penList->append(plotinfo->penList->at(gpnum % plotinfo->penList->length()));
                    }
                    plotArea->graph(gr)->setPen(plotinfo->penList->at(gpnum));
                    QString grlabel;
                    float Va = VaStart;
                    if (VaSteps>=1) Va += (float)a*(float)(VaEnd - VaStart)/(float)VaSteps;
                    float Vg = VgStart;
                    if (VgSteps>=1) Vg += (float)g*(float)(VgEnd - VgStart)/(float)VgSteps;
                    Vg = floor(0.5+10*Vg)/10;
                    Va = floor(0.5+10*Va)/10;
                    QTextStream(&grlabel)  << "Va:" << Va << " Vg:" << Vg;
                    plotArea->graph(gr++)->setName(grlabel);
                    if (y2cb->currentText()!="Off") {
                        plotArea->addGraph(plotArea->xAxis,plotArea->yAxis2);
                        plotArea->graph(gr)->setData(x,y2);
                        grPen = plotinfo->penList->at(gpnum);
                        grPen.setStyle(Qt::DotLine);
                        plotArea->graph(gr)->setPen(grPen);
                        plotArea->graph(gr++)->setName(grlabel);
                    }
                    gpnum++;
                }
            }
        }
    } */
    if (x1cb-> currentText()=="Vg") {
        //X axis title,scale
        plotArea->xAxis->setLabel("Vg");
        plotArea->xAxis->setRange(VgMin,VgMax);

        for(int s=0,gr=0,gpnum=0; (s <= VsSteps)  && (!plotTri || (s < 1)); s++) {
            if (s == (curve-1) || 0==curve || plotTri) {
                int astep = VaSteps/5;
                for(int a=0; (a <= VaSteps); a+=astep) {
                    x.clear();y1.clear();y2.clear();
                    //Fit a curve
                    iapoints.clear();
                    ispoints.clear();
                    bspline_fitted=false;
                    for(int g=0; (g <= VgSteps); g++) {
                        i = a + (VaSteps+1)*g + (VaSteps+1)*(VgSteps+1)*s + first;
                        //build a list of points for this curve
                        if ((i) < activeStore->length()) {
                            dataPoint = plotinfo->dataSet->at(i);
                            point.setX(-dataPoint.Vg); //b spline fit requires increasing x
                             point.setY(dataPoint.Ia);
                            iapoints.append(point);
                            point.setY(dataPoint.Is);
                            ispoints.append(point);
                            if (g==VgSteps && VgSteps>5) {
                                //qDebug() << "VG Spline fit, i=" <<i;
                                bSplineFit(&ia_bspline_info, &iapoints);
                                bSplineFit(&is_bspline_info, &ispoints);
                                bspline_fitted=true;
                            }
                        }
                    }
                    for(int g=0; (g <= VgSteps); g++) {
                        i = a + (VaSteps+1)*g + (VaSteps+1)*(VgSteps+1)*s + first;
                        if (i <activeStore->length()) {
                            //--
                            dataPoint = plotinfo->dataSet->at(i);
                            nextDataPoint = plotinfo->dataSet->at(i);
                            if (g!=VgSteps  && (i+2+VaSteps) < plotinfo->dataSet->length()) nextDataPoint = plotinfo->dataSet->at(i +1 +VaSteps);
                            float Va =  dataPoint.Va;
                            float Vs =  dataPoint.Vs;
                            float Vg =  dataPoint.Vg;
                            //qDebug() << "Vg C/N:" << Va << nextDataPoint.Va << Vs << nextDataPoint.Vs << Vg << nextDataPoint.Vg;

                            for( float f=0; f<4; f++) {
                                bool plim = (dataPoint.Va * dataPoint.Ia > 1000*plotinfo->Power);
                                if (!(dataPoint.ignore & IGNORE_CURRENT) && !(plim && powerCheck->isChecked()) ) {
                                    //Interpolate
                                    plim = (nextDataPoint.Va * nextDataPoint.Ia > 1000*plotinfo->Power);
                                    if (f==0 ||
                                             (!(nextDataPoint.ignore & IGNORE_CURRENT) &&
                                             !(plim && powerCheck->isChecked()))) {
                                        float dx= f/4;
                                        //linear interpolation
                                        float Vg= dataPoint.Vg + dx*(nextDataPoint.Vg - dataPoint.Vg);
                                        float Ia = dataPoint.Ia + dx*(nextDataPoint.Ia - dataPoint.Ia);
                                        float Is = dataPoint.Is + dx*(nextDataPoint.Is - dataPoint.Is);
                                        float IaMod = 0;
                                        float IsMod = 0;
                                        float ra_a = 0;
                                        float ra_b = 0;
                                        float gm_a = 0;
                                        float gm_b = 0;
                                        float mu_a = 0;
                                        float mu_b = 0;
                                        float IaSmooth=Ia;
                                        float IsSmooth=Is;
                                        if (bspline_fitted) {
                                            double Vx =Vg;
                                            if (Vx > iapoints.last().x()) Vx= iapoints.last().x();
                                            qDebug() << "VG Spline:Vx="  <<Vx << "First=" << iapoints.first().x() << "Last=" << iapoints.last().x();
                                            IaSmooth =  bspline_eval(&ia_bspline_info, -Vx);
                                            IsSmooth =  bspline_eval(&is_bspline_info, -Vx);
                                        }
                                        if (plot1.optimizer->isValid() && plot1.type!=NONE) {
                                            IaMod = plot1.optimizer->IaCalc(Va, Vs, Vg);
                                            IsMod = plot1.optimizer->IsCalc(Va, Vs, Vg);
                                            ra_a = plot1.optimizer->RaCalc(Va, Vs, Vg);
                                            ra_b = plot1.optimizer->RaCalcB(Va, Vs, Vg);
                                            gm_a = plot1.optimizer->GmCalc(Va, Vs, Vg);
                                            gm_b = plot1.optimizer->GmCalcB(Va, Vs, Vg);
                                            mu_a = ra_a  *gm_a;
                                            mu_b = ra_b  *gm_b;
                                        }
                                        x.append(Vg);
                                        if (y1cb->currentText()=="Ia(a)") y1.append(Ia);
                                        else if (y1cb->currentText()=="Ia(b)/g2") y1.append(Is);
                                        else if (y1cb->currentText()=="Ra(a)") y1.append(ra_a);
                                        else if (y1cb->currentText()=="Ra(b)") y1.append(ra_b);
                                        else if (y1cb->currentText()=="gm(a)") y1.append(gm_a);
                                        else if (y1cb->currentText()=="gm(b)") y1.append(gm_b);
                                        else if (y1cb->currentText()=="mu(a)") y1.append(mu_a);
                                        else if (y1cb->currentText()=="mu(b)") y1.append(mu_b);
                                        else if (y1cb->currentText()=="Ia(a) Smooth") y1.append(IaSmooth);
                                        else if (y1cb->currentText()=="Ia(b)/g2 Smooth") y1.append(IsSmooth);
                                        if (y2cb->currentText()=="Ia(b)/g2") y2.append(Is);
                                        else if (y2cb->currentText()=="Ia(a) Model") y2.append(IaMod);
                                        else if (y2cb->currentText()=="Ia(b)/g2 Model") y2.append(IsMod);
                                        else if (y2cb->currentText()=="Ra(a)") y2.append(ra_a);
                                        else if (y2cb->currentText()=="Ra(b)") y2.append(ra_b);
                                        else if (y2cb->currentText()=="gm(a)") y2.append(gm_a);
                                        else if (y2cb->currentText()=="gm(b)") y2.append(gm_b);
                                        else if (y2cb->currentText()=="mu(a)") y2.append(mu_a);
                                        else if (y2cb->currentText()=="mu(b)") y2.append(mu_b);
                                        else if (y2cb->currentText()=="Ia(a) Smooth") y2.append(IaSmooth);
                                        else if (y2cb->currentText()=="Ia(b)/g2 Smooth") y2.append(IsSmooth);
                                    }
                                }
                                if (g==VgSteps) break;
                            }
                        }
                    }
                    plotArea->addGraph(plotArea->xAxis,plotArea->yAxis);
                    plotArea->graph(gr)->setData(x,y1);
                    //----------- Set plotline pens--------------
                    //Are enough pens defined?
                    if (gpnum > (plotinfo->penList->length()-1)) {
                        //no pen so add one
                        plotinfo->penList->append(plotinfo->penList->at(gpnum % plotinfo->penList->length()));
                    }
                    if (colorGroup->isChecked()) plotArea->graph(gr)->setPen(plotinfo->penList->at(s));
                    else plotArea->graph(gr)->setPen(plotinfo->penList->at(gpnum));

                    QString grlabel;
                    float Vs = VsStart;
                    if (VsSteps>=1) Vs += (float)s*(float)(VsEnd - VsStart)/(float)VsSteps;
                    float Va = VaStart;
                    if (VaSteps>=1) Va += (float)a*(float)(VaEnd - VaStart)/(float)VaSteps;
                    Va = floor(0.5+10*Va)/10;
                    Vs = floor(0.5+10*Vs)/10;
                    if ( plotinfo->type==DIODE || plotinfo->type==TRIODE || plotinfo->type==DUAL_TRIODE || plotTri)
                        QTextStream(&grlabel)  << "Va:" << Va;
                    else
                        QTextStream(&grlabel)  << "Va:" << Va << " Vg2:" << Vs;
                    plotArea->graph(gr++)->setName(grlabel);
                    if (y2cb->currentText()!="Off") {
                        plotArea->addGraph(plotArea->xAxis,plotArea->yAxis2);
                        plotArea->graph(gr)->setData(x,y2);
                        //select pen
                        if (colorGroup->isChecked()) grPen= plotinfo->penList->at(s); else grPen = plotinfo->penList->at(gpnum);
                        grPen.setStyle(Qt::DotLine);
                        plotArea->graph(gr)->setPen(grPen);
                        plotArea->graph(gr++)->setName(grlabel);
                    }
                    gpnum++;
                }
            }
        }
    }
    //Axis titles, scale
    plotArea->yAxis->setLabel("n/a");
    plotArea->yAxis2->setLabel("n/a");
    if (plotinfo->type==DIODE) {
        if (y1cb->currentText()=="Ia(a)") plotArea->yAxis->setLabel("mA Ia");
        if (y1cb->currentText()=="Ra(a)") plotArea->yAxis->setLabel("Ra Kohm");
        if (y2cb->currentText()=="Ia(a)") plotArea->yAxis2->setLabel("mA Ia");
        if (y1cb->currentText()=="Ia(a) Smooth") plotArea->yAxis->setLabel("mA Ia(a) Smooth");
        if (y2cb->currentText()=="Ia(a) Smooth") plotArea->yAxis2->setLabel("mA Ia(a) Smooth");
        if (y2cb->currentText()=="Ia(a) Model") plotArea->yAxis2->setLabel("mA Ia Model");
        if (y2cb->currentText()=="Ra(a)") plotArea->yAxis2->setLabel("Ra Kohm");
    } else if (plotinfo->type==DUAL_DIODE) {
        if (y1cb->currentText()=="Ia(a) Smooth") plotArea->yAxis->setLabel("mA Ia(a) Smooth");
        if (y2cb->currentText()=="Ia(a) Smooth") plotArea->yAxis2->setLabel("mA Ia(a) Smooth");
        if (y1cb->currentText()=="Ia(b)/g2 Smooth") plotArea->yAxis->setLabel("mA Ia(b) Smooth");
        if (y2cb->currentText()=="Ia(b)/g2 Smooth") plotArea->yAxis2->setLabel("mA Ia(b) Smooth");
        if (y1cb->currentText()=="Ia(a)") plotArea->yAxis->setLabel("mA Ia(a)");
        if (y1cb->currentText()=="Ra(a)") plotArea->yAxis->setLabel("Ra(a) Kohm");
        if (y1cb->currentText()=="Ra(b)") plotArea->yAxis->setLabel("Ra(b) Kohm");
        if (y2cb->currentText()=="Ia(a)") plotArea->yAxis2->setLabel("mA Ia(a)");
        if (y2cb->currentText()=="Ia(b)/g2") plotArea->yAxis2->setLabel("mA Ia(b)");
        if (y2cb->currentText()=="Ia(a) Model") plotArea->yAxis2->setLabel("mA Ia(a) Model");
        if (y2cb->currentText()=="Ia(b)/g2 Model") plotArea->yAxis2->setLabel("mA Ia(b) Model");
        if (y2cb->currentText()=="Ra(a)") plotArea->yAxis2->setLabel("Ra(a) Kohm");
        if (y2cb->currentText()=="Ra(b)") plotArea->yAxis2->setLabel("Ra(b) Kohm");
    } else if (plotinfo->type==TRIODE ) {
        if (y1cb->currentText()=="Ia(a) Smooth") plotArea->yAxis->setLabel("mA Ia(a) Smooth");
        if (y2cb->currentText()=="Ia(a) Smooth") plotArea->yAxis2->setLabel("mA Ia(a) Smooth");
        if (y1cb->currentText()=="Ia(a)") plotArea->yAxis->setLabel("mA Ia");
        if (y1cb->currentText()=="Ra(a)") plotArea->yAxis->setLabel("Ra Kohm");
        if (y1cb->currentText()=="gm(a)") plotArea->yAxis->setLabel("-Gm mS");
        if (y1cb->currentText()=="mu(a)") plotArea->yAxis->setLabel("-mu");
        if (y2cb->currentText()=="Ia(a)") plotArea->yAxis2->setLabel("mA Ia");
        if (y2cb->currentText()=="Ia(a) Model") plotArea->yAxis2->setLabel("mA Ia Model");
        if (y2cb->currentText()=="Ra(a)") plotArea->yAxis2->setLabel("Ra Kohm");
        if (y2cb->currentText()=="gm(a)") plotArea->yAxis2->setLabel("-Gm mS");
        if (y2cb->currentText()=="mu(a)") plotArea->yAxis2->setLabel("-mu");
    } else if (plotinfo->type==DUAL_TRIODE ) {
        //a
        if (y1cb->currentText()=="Ia(a) Smooth") plotArea->yAxis->setLabel("mA Ia(a) Smooth");
        if (y2cb->currentText()=="Ia(a) Smooth") plotArea->yAxis2->setLabel("mA Ia(a) Smooth");
        if (y1cb->currentText()=="Ia(a)") plotArea->yAxis->setLabel("mA Ia");
        if (y1cb->currentText()=="Ra(a)") plotArea->yAxis->setLabel("Kohm Ra(a)");
        if (y1cb->currentText()=="gm(a)") plotArea->yAxis->setLabel("-mS Gm");
        if (y1cb->currentText()=="mu(a)") plotArea->yAxis->setLabel("-mu");
        if (y1cb->currentText()=="Ia(b)/g2") plotArea->yAxis->setLabel("mA Ia(b)");
        if (y2cb->currentText()=="Ia(a)") plotArea->yAxis2->setLabel("mA Ia");
        if (y2cb->currentText()=="Ia(a) Model") plotArea->yAxis2->setLabel("mA Ia Model");
        if (y2cb->currentText()=="Ra(a)") plotArea->yAxis2->setLabel("Kohm Ra");
        if (y2cb->currentText()=="gm(a)") plotArea->yAxis2->setLabel("-mS Gm");
        if (y2cb->currentText()=="mu(a)") plotArea->yAxis2->setLabel("-mu");
        //b
        if (y1cb->currentText()=="Ia(b)/g2 Smooth") plotArea->yAxis->setLabel("mA Ia(b) Smooth");
        if (y2cb->currentText()=="Ia(b)/g2 Smooth") plotArea->yAxis2->setLabel("mA Ia(b) Smooth");
        if (y1cb->currentText()=="Ia(b)/g2") plotArea->yAxis->setLabel("mA Ia(b)");
        if (y1cb->currentText()=="Ra(b)") plotArea->yAxis->setLabel("Kohm Ra(b)");
        if (y1cb->currentText()=="gm(b)") plotArea->yAxis->setLabel("-mS Gm(b)");
        if (y1cb->currentText()=="mu(b)") plotArea->yAxis->setLabel("-mu(b)");
        if (y2cb->currentText()=="Ia(b)/g2") plotArea->yAxis2->setLabel("mA Ia(b)");
        if (y2cb->currentText()=="Ia(b)/g2 Model") plotArea->yAxis2->setLabel("mA Ia(b) Model");
        if (y2cb->currentText()=="Ra(b)") plotArea->yAxis2->setLabel("Kohm Ra(b)");
        if (y2cb->currentText()=="gm(b)") plotArea->yAxis2->setLabel("-mS Gm(b)");
        if (y2cb->currentText()=="mu(b)") plotArea->yAxis2->setLabel("-mu(b)");
    } else { //pentodes
        if (y1cb->currentText()=="Ia(a) Smooth") plotArea->yAxis->setLabel("mA Ia(a) Smooth");
        if (y2cb->currentText()=="Ia(a) Smooth") plotArea->yAxis2->setLabel("mA Ia(a) Smooth");
        if (y1cb->currentText()=="Ia(b)/g2 Smooth") plotArea->yAxis->setLabel("mA Ia(b) Smooth");
        if (y2cb->currentText()=="Ia(b)/g2 Smooth") plotArea->yAxis2->setLabel("mA Ia(b) Smooth");
        if (y1cb->currentText()=="Ia(a)") plotArea->yAxis->setLabel("mA Ia");
        if (y1cb->currentText()=="Ia(b)/g2") plotArea->yAxis->setLabel("mA Ig2");
        if (y1cb->currentText()=="Ra(a)") plotArea->yAxis->setLabel("Kohm Ra(a)");
        if (y1cb->currentText()=="gm(a)") plotArea->yAxis->setLabel("-mS Gm");
        if (y1cb->currentText()=="mu(a)") plotArea->yAxis->setLabel("-mu");
        if (y2cb->currentText()=="Ia(a)") plotArea->yAxis2->setLabel("mA Ia");
        if (y2cb->currentText()=="Ia(b)/g2") plotArea->yAxis2->setLabel("mA g2");
        if (y2cb->currentText()=="Ia(a) Model") plotArea->yAxis2->setLabel("mA Ia Model");
        if (y2cb->currentText()=="Ia(b)/g2 Model") plotArea->yAxis2->setLabel("mA Ig2 Model");
        if (y2cb->currentText()=="Ra(a)") plotArea->yAxis2->setLabel("Kohm Ra(a)");
        if (y2cb->currentText()=="gm(a)") plotArea->yAxis2->setLabel("-mS Gm");
        if (y2cb->currentText()=="mu(a)") plotArea->yAxis2->setLabel("-mu");
    }

    if (y2cb->currentText()=="Off") plotArea->yAxis2->setVisible(false); else plotArea->yAxis2->setVisible(true);
    UpdateTitle(plotinfo->tube);
    //Remove text items from legend that are duplicated
   if (y2cb->currentText()!="Off" ) {
//    if (1) {
        int c =  plotArea->legend->itemCount()/2;
        for (int i=1; i <= c; i++) plotArea->legend->removeItem(i);
    }
    plotArea->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignLeft|Qt::AlignTop);
//    plotArea->legend->setPositionStyle(QCPLegend::psTopLeft);
    plotArea->legend->setLayer("grid");
    plotArea->legend->setVisible(checkLegend->checkState());
//    plotArea->legend->autoSize();
    //plotArea->axisRect()->insetLayout()->;
    //Rescale axis to match
    //qDebug() << "RePlot: IaMax=" << IaMax << "IsMax=" << IaMax;
//    IaMax= 5*ceil(IaMax/5);
//    IsMax= 5*ceil(IsMax/5);
    IaMax= ceil(IaMax);
    IsMax= ceil(IsMax);
/*
    if (IaMax<=1) IaMax=1;
    else if (IaMax<=2) IaMax=2;
    else if (IaMax<=5) IaMax=5;
    else if (IaMax<=10) IaMax=10;
    else if (IaMax<=20) IaMax=20;
    else if (IaMax<=50) IaMax=50;
    else if (IaMax<=100) IaMax=100;
    else if (IaMax<=200) IaMax=200;
    else if (IaMax<=500) IaMax=500;
    else if (IaMax<=1000) IaMax=1000;
    if (IsMax<=1) IsMax=1;
    else if (IsMax<=2) IsMax=2;
    else if (IsMax<=5) IsMax=5;
    else if (IsMax<=10) IsMax=10;
    else if (IsMax<=20) IsMax=20;
    else if (IsMax<=50) IsMax=50;
    else if (IsMax<=100) IsMax=100;
    else if (IsMax<=200) IsMax=200;
    else if (IsMax<=500) IsMax=500;
    else if (IsMax<=1000) IsMax=1000;*/

    //Make axes the same for dual triodes/ rects
    if ( (plotinfo->type==DUAL_TRIODE ) || (plotinfo->type==DUAL_DIODE) )  {
        if (IaMax > IsMax) IsMax= IaMax; else IaMax =  IsMax;
    }
    if (y1cb->currentText()=="Ia(a)" || y1cb->currentText()=="Ia(a) Smooth" || y1cb->currentText()=="Ia(a) Model") {
        plotArea->yAxis->setRange(0,IaMax);
        plotArea->yAxis2->setRange(0,IaMax);
        if (y2cb->currentText()=="Ia(b)/g2") plotArea->yAxis2->setRange(0,IsMax);
        if (y2cb->currentText()=="gm(a)") plotArea->yAxis2->setRange(0,gmMax);
        if (y2cb->currentText()=="Ra(a)") plotArea->yAxis2->setRange(0,raMax);
        if (y2cb->currentText()=="mu(a)") plotArea->yAxis2->setRange(0,muMax);
        if (y2cb->currentText()=="gm(b)") plotArea->yAxis2->setRange(0,gmMax);
        if (y2cb->currentText()=="Ra(b)") plotArea->yAxis2->setRange(0,raMax);
        if (y2cb->currentText()=="mu(b)") plotArea->yAxis2->setRange(0,muMax);
    }
    else if (y1cb->currentText()=="Ia(b)/g2" || y1cb->currentText()=="Ia(b)/g2 Smooth") {
        plotArea->yAxis->setRange(0,IsMax);
        plotArea->yAxis2->setRange(0,IsMax);
        if (y2cb->currentText()=="Ia(b)/g2 Model" || y1cb->currentText()=="Ib(a) Smooth") {
            plotArea->yAxis->setRange(0,IaMax);
            plotArea->yAxis2->setRange(0,IaMax);
        }
        if (y2cb->currentText()=="gm(a)") plotArea->yAxis2->setRange(0,gmMax);
        if (y2cb->currentText()=="Ra(a)") plotArea->yAxis2->setRange(0,raMax);
        if (y2cb->currentText()=="mu(a)") plotArea->yAxis2->setRange(0,muMax);
        if (y2cb->currentText()=="gm(b)") plotArea->yAxis2->setRange(0,gmMax);
        if (y2cb->currentText()=="Ra(b)") plotArea->yAxis2->setRange(0,raMax);
        if (y2cb->currentText()=="mu(b)") plotArea->yAxis2->setRange(0,muMax);
    }
    else if ( (y1cb->currentText()=="gm(a)") || (y1cb->currentText()=="gm(b)") ) {
        plotArea->yAxis->setRange(0,gmMax);
        if (y2cb->currentText()=="Ia(b)/g2") plotArea->yAxis2->setRange(0,IsMax);
        if (y2cb->currentText()=="Ia(a) Model") plotArea->yAxis2->setRange(0,IaMax);
        if (y2cb->currentText()=="Ia(b)/g2 Model") plotArea->yAxis2->setRange(0,IsMax);
        if (y2cb->currentText()=="gm(a)") plotArea->yAxis2->setRange(0,gmMax);
        if (y2cb->currentText()=="Ra(a)") plotArea->yAxis2->setRange(0,raMax);
        if (y2cb->currentText()=="mu(a)") plotArea->yAxis2->setRange(0,muMax);
        if (y2cb->currentText()=="gm(b)") plotArea->yAxis2->setRange(0,gmMax);
        if (y2cb->currentText()=="Ra(b)") plotArea->yAxis2->setRange(0,raMax);
        if (y2cb->currentText()=="mu(b)") plotArea->yAxis2->setRange(0,muMax);
    }
    else if ( (y1cb->currentText()=="Ra(a)") || (y1cb->currentText()=="Ra(b)") ) {
        plotArea->yAxis->setRange(0,raMax);
        if (y2cb->currentText()=="Ia(b)/g2") plotArea->yAxis2->setRange(0,IsMax);
        if (y2cb->currentText()=="Ia(a) Model") plotArea->yAxis2->setRange(0,IaMax);
        if (y2cb->currentText()=="Ia(b)/g2 Model") plotArea->yAxis2->setRange(0,IsMax);
        if (y2cb->currentText()=="gm(a)") plotArea->yAxis2->setRange(0,gmMax);
        if (y2cb->currentText()=="Ra(a)") plotArea->yAxis2->setRange(0,raMax);
        if (y2cb->currentText()=="mu(a)") plotArea->yAxis2->setRange(0,muMax);
        if (y2cb->currentText()=="gm(b)") plotArea->yAxis2->setRange(0,gmMax);
        if (y2cb->currentText()=="Ra(b)") plotArea->yAxis2->setRange(0,raMax);
        if (y2cb->currentText()=="mu(b)") plotArea->yAxis2->setRange(0,muMax);
    }
    else if ( (y1cb->currentText()=="mu(a)") || (y1cb->currentText()=="mu(b)") ) {
        plotArea->yAxis->setRange(0,muMax);
        if (y2cb->currentText()=="Ia(b)/g2") plotArea->yAxis2->setRange(0,IsMax);
        if (y2cb->currentText()=="Ia(a) Model") plotArea->yAxis2->setRange(0,IaMax);
        if (y2cb->currentText()=="Ia(b)/g2 Model") plotArea->yAxis2->setRange(0,IsMax);
        if (y2cb->currentText()=="gm(a)") plotArea->yAxis2->setRange(0,gmMax);
        if (y2cb->currentText()=="Ra(a)") plotArea->yAxis2->setRange(0,raMax);
        if (y2cb->currentText()=="mu(a)") plotArea->yAxis2->setRange(0,muMax);
        if (y2cb->currentText()=="gm(b)") plotArea->yAxis2->setRange(0,gmMax);
        if (y2cb->currentText()=="Ra(b)") plotArea->yAxis2->setRange(0,raMax);
        if (y2cb->currentText()=="mu(b)") plotArea->yAxis2->setRange(0,muMax);
    }

    //plotArea->rescaleAxes();
    plotArea->replot();
}


void PlotTabWidget::UpdateTitle(QString tube) {
    QString line;
    line.append(tube);
    /*line +=" ";
    if (y1cb->currentText()=="Ia(a)") line += "Ia";
    if (y1cb->currentText()=="Ia(b)/g2") line += "Ia/g2";
    if (y1cb->currentText()=="gm(a)") line += "gm(a)";
    if (y1cb->currentText()=="gm(b)") line += "gm(b)";
    if (y2cb->currentText()=="Ia(a) Model") line += ", Ia(a) Model";
    if (y2cb->currentText()=="Ia(b)/g2") line += ", Ia/g2";
    if (y2cb->currentText()=="Ia(b)/g2 Model") line += ", Ia(b)/g2 Model";
    if (y2cb->currentText()=="gm(a)") line += ", gm(a)";
    if (y2cb->currentText()=="gm(b)") line += ", gm(b)";
    if (x1cb->currentText()=="Va(a)") line.append("/Va1");
    if (x1cb->currentText()=="Vg2") line.append("/Vg2");
    if (x1cb->currentText()=="Vg") line.append("/Vg1");*/
    //plotArea->setTitle(line);
    if (title == 0) {
        //title = new QCPPlotTitle(plotArea, line); //QCP 1.3.1
        title = new QCPTextElement(plotArea, line);
        title->setTextColor(plotTextColor);
        plotArea->plotLayout()->insertRow(0);
        plotArea->plotLayout()->addElement(0, 0, title);
    }
    else {
      title->setText(line);
      title->setTextColor(plotTextColor);
    }
    //plotArea->replot();
}

//------------------------------
// Handle Plot Interactions
//------------------------------
void PlotTabWidget::PlotInteractionUser() {

    QList<QCPGraph *> line = plotArea->selectedGraphs();
    if (line.length()>0) {
        //Determine the index of the selected graph
        ShowValIndex=0;
        while ( plotArea->graph(ShowValIndex) != line.at(0) && ShowValIndex < plotArea->graphCount() ) {
            ShowValIndex++;
        }
        plotArea->deselectAll();
    }
    showVals(0);
}


//plotline color change by click:
/*void PlotTabWidget::PlotInteractionUser() {

    if (ppenList==0) return;
    QList<QCPGraph *> line = plotArea->selectedGraphs();
    if (line.length()>0) {
        //Determine the index of the selected graph
        int index=0;
        while ( plotArea->graph(index) != line.at(0) && index < plotArea->graphCount() ) {
            index++;
        }
        QColor lineColor = line.at(0)->pen().color();
        QColor c = QColorDialog::getColor(lineColor,this,"Select New Line Color");
        if (c.isValid()) {
            QPen grPen;
            grPen.setColor(c);
            grPen.setWidthF(2);
            if (y2cb->currentText() != "Off") {
                //plotArea->graph(index)->setPen(grPen);
                index >>= 1;
                ppenList->replace(index,grPen);
            } else {
                //line.at(0)->setPen(grPen);
                ppenList->replace(index,grPen);
            }
        }
        plotArea->deselectAll();
        plotUpdate(&plot1);
        //TODO Save the new scheme
        emit penChanged();
    }
}
*/

bool freeze=false;

void PlotTabWidget::clearGraphs() {
    plotArea->clearGraphs();
    ShowValIndex=0;
    if (markerText) markerText->setVisible(false);
    if (marker) marker->setVisible(false);
    freeze=false;
}


void PlotTabWidget::plotLegendChanged() {
    plotArea->legend->setVisible( checkLegend->isChecked() );
    plotArea->replot();
}


void PlotTabWidget::Freeze(QMouseEvent *event) {
    if (event) {
        if (event->button()==Qt::RightButton) {
            freeze= !freeze;
        }
    }

}

void PlotTabWidget::showVals(QMouseEvent *event)
{
    static double mouseX=0;
    static int xpixel=0;
    if(plotArea->graphCount()==0 || !plot1.complete) {
        if (markerText) markerText->setVisible(false);
        if (marker) marker->setVisible(false);
        return;
    }
    if (event && !freeze) xpixel= event->pos().x();
    mouseX = plotArea->xAxis->pixelToCoord(xpixel); //to axis value
    QString xText = plotArea->xAxis->label();
    QString yText = plotArea->yAxis->label();
    QString y2Text = plotArea->yAxis2->label();

    //for the  graph values #####
    int i = ShowValIndex;
    if ( i > plotArea->graphCount()) i = plotArea->graphCount();

    if(plotArea->xAxis->range().contains(mouseX))

    {
        // add the  circle marker:
        marker->setGraph(plotArea->graph(i));
        marker->setGraphKey(mouseX);
        marker->setInterpolating(true);
        marker->setStyle(QCPItemTracer::tsCircle);
        marker->setPen(QPen(plotSHowValColor));
        marker->setBrush(plotSHowValColor);
        marker->setSize(7);
        marker->setLayer("markers");
        QCPItemPosition * marker_posn = marker->position;
        double x = marker_posn->key();
        double y = marker_posn->value();
        QPointF marker_xypix = marker_posn->pixelPosition();

        // add label to marker:
        markerText->position->setType(QCPItemPosition::ptAxisRectRatio);
        QString infoLabel = QObject::tr("%1 ").arg(round(x*2)/2) + xText;
        if ( (y2cb->currentText()!="Off") && (i & 1)) infoLabel += QObject::tr(", %2 ").arg(round(y*100)/100) + y2Text;
        else infoLabel += QObject::tr(", %2 ").arg(round(y*100)/100) + yText;
        markerText->setText(infoLabel);
        markerText->setTextAlignment(Qt::AlignLeft);
        markerText->setFont(QFont(font().family(), 9));
        markerText->setColor(plotSHowValColor);

        //markerText->setBrush(plotBackGroundColor);
        markerText->setBrush(Qt::NoBrush);
        markerText->setLayer("markers");
        //set position of marker text
        markerText->position->setPixelPosition(marker_xypix); // trial position
        markerText->setPadding(QMargins(10, 10, 10, 10));
        markerText->setPositionAlignment(Qt::AlignRight| Qt::AlignBottom);
        //adjust if it hits an edge
        qreal top = markerText->top->pixelPosition().y();
        qreal right = markerText->right->pixelPosition().x();
        qreal left = markerText->left->pixelPosition().x();
        qreal bottom = markerText->bottom->pixelPosition().y();

        double xp_left=plotArea->rect().left();
        double yp_top=plotArea->rect().top();
        double xp_right=plotArea->rect().right();
        double yp_bottom=plotArea->rect().bottom();
        if (left < xp_left) marker_xypix.setX(right-left+xp_left);
        if (right > xp_right) marker_xypix.setX(xp_right);
        if (top < yp_top) marker_xypix.setY(bottom-top +yp_top);
        if (bottom > yp_bottom) marker_xypix.setY(yp_bottom);
        markerText->position->setPixelPosition(marker_xypix); // trial position

        markerText->setVisible(true);
        marker->setVisible(true);
        markerText->setClipToAxisRect(false);
    }
    else
    {
        markerText->setVisible(false);
        marker->setVisible(false);
    }
    plotArea->replot();
}


// Cubic Spline fitting
//gsl 1.8 doc Chapter 26: Interpolation 298
/*
void PlotTabWidget::SplineFit(QList<QPointF> * points, gsl_interp_accel **acc, gsl_spline **spline ) {
    int i;
    double x[500], y[500];

    *acc = gsl_interp_accel_alloc ();
    *spline = gsl_spline_alloc (gsl_interp_cspline, n);
    for(int i =0; (i < n)  && (i<500); i++) {
        x[i] = points->at(i).x();
        y[i] = points->at(i).y();
    }
    gsl_spline_init (*spline, x, y, n);
}

void PlotTabWidget::SplineFree(gsl_interp_accel *acc, gsl_spline *spline) {
    gsl_spline_free (spline);
    gsl_interp_accel_free (acc);
}

*/
// B spline fitting
void PlotTabWidget::bSplineFree(bspline_info_t *  bspline_info) {
  gsl_bspline_free(bspline_info->bw);
  gsl_vector_free(bspline_info->B);
  gsl_vector_free(bspline_info->x);
  gsl_vector_free(bspline_info->y);
  gsl_matrix_free(bspline_info->X);
  gsl_vector_free(bspline_info->c);
  gsl_vector_free(bspline_info->w);
  gsl_matrix_free(bspline_info->cov);
  gsl_multifit_linear_free(bspline_info->mw);
}

//#define ORDER  5 //quartic bspline
#define ORDER  4 //cubic bspline
#define NCOEFFS  14
#define NBREAK   (NCOEFFS +2 -ORDER)

void PlotTabWidget::bSplineFit(bspline_info_t * bspline_info, QList<QPointF> * points ) {
  const size_t n = points->length()+8; //extra 8 due to appends and prepends below
  const size_t ncoeffs = NCOEFFS;
  const size_t nbreak = NBREAK;
  size_t i, j;
  double dy;
  double chisq;
  //add point to start and end
  QPointF p;
    p = points->first();
    points->prepend(p);
    points->prepend(p);
    points->prepend(p);
    points->prepend(p);
    p = points->last();
    points->prepend(p);
    points->append(p);
    points->append(p);
    points->append(p);
  bspline_info->bw = gsl_bspline_alloc(ORDER, nbreak);
  bspline_info->B = gsl_vector_alloc(ncoeffs);
  bspline_info->x = gsl_vector_alloc(n);
  bspline_info->y = gsl_vector_alloc(n);
  bspline_info->X = gsl_matrix_alloc(n, ncoeffs);
  bspline_info->c = gsl_vector_alloc(ncoeffs);
  bspline_info->w = gsl_vector_alloc(n);
  bspline_info->cov = gsl_matrix_alloc(ncoeffs, ncoeffs);
  bspline_info->mw = gsl_multifit_linear_alloc(n, ncoeffs);

  /* this is the data to be fitted */
  for (i = 0; i < n; ++i) {
      gsl_vector_set(bspline_info->x, i, points->at(i).x());
      gsl_vector_set(bspline_info->y, i, points->at(i).y());
      gsl_vector_set(bspline_info->w, i, 1.0);
    }

  /* use uniform breakpoints on [0, 15] */
  gsl_bspline_knots_uniform(0.0, points->last().x(), bspline_info->bw);

  /* construct the fit matrix X */
  for (i = 0; i < n; ++i)
    {
      double xi = gsl_vector_get(bspline_info->x, i);
      /* compute B_j(xi) for all j */
      gsl_bspline_eval(xi, bspline_info->B, bspline_info->bw);
      /* fill in row i of X */
      for (j = 0; j < ncoeffs; ++j)
        {
          double Bj = gsl_vector_get(bspline_info->B, j);
          gsl_matrix_set(bspline_info->X, i, j, Bj);
        }
    }
  /* do the fit */
  gsl_multifit_wlinear(
              bspline_info->X, bspline_info->w, bspline_info->y,
              bspline_info->c, bspline_info->cov, &chisq, bspline_info->mw);
}

/* evaluate the smoothed curve at xi*/
double PlotTabWidget::bspline_eval(bspline_info_t * bspline_info, double xi){
      double yi, yerr;

      gsl_bspline_eval(xi, bspline_info->B, bspline_info->bw);
      gsl_multifit_linear_est(bspline_info->B, bspline_info->c, bspline_info->cov, &yi, &yerr);
      return(yi);
}
