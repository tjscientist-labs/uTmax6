#ifndef PLOTTABWIDGET_H
#define PLOTTABWIDGET_H

#include <QObject>
#include <QGroupBox>
#include "qcustomplot.h"
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QPen>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>

#include "typedefs.h"
#include "dr_optimize.h"

struct plotInfo_t {
    int VaSteps;
    int VsSteps;
    int VgSteps;
    float VaStart;
    float VaEnd;
    float VsStart;
    float VsEnd;
    float VgStart;
    float VgEnd;
    float Power;
    QList<results_t> * dataSet;
    QString tube;
    QString type;
    QList<QPen> * penList;
    bool penChange;
    bool VsEQVa;
    dr_optimize * optimizer;
    bool complete;
};

class PlotTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlotTabWidget(QWidget * parent = 0);
    ~PlotTabWidget();
    void plotUpdate(plotInfo_t * plotinfo);
    void plot(plotInfo_t * plotInfo);
    void clearGraphs();
    QCustomPlot * plotArea;
    void RectColor(QColor);
    void BackGroundColor(QColor);
    void GridColor(QColor);
    void AxisColor(QColor);
    void TextColor(QColor);
    void setGridLine(int);
    void updateColors(void);
    void setLegendColor(QColor);
    void setLegendTextColor(QColor);
    void setShowValColor(QColor color);
    void setLineColorList(QList<QColor> colors);

    struct bspline_info_t {
        gsl_bspline_workspace *bw;
        gsl_vector *B;
        gsl_vector *c, *w;
        gsl_vector *x, *y;
        gsl_matrix *X, *cov;
        gsl_multifit_linear_workspace *mw;
    };

signals:
    void penChanged();

public slots:
    void plotSettingChanged();

private slots:
    void PlotInteractionUser();
    void plotLegendChanged();
    void showVals(QMouseEvent *);
    void Freeze(QMouseEvent *event);

private:
    QComboBox * x1cb;
    QComboBox * y1cb;
    QComboBox * y2cb;
    QCheckBox * checkLegend;
    QCheckBox * checkPlotTriode;
    QCheckBox * colorGroup;
    QCheckBox * powerCheck;
    QLabel * cgLabel;
    QSpinBox * CurveNum;
    QSpinBox * polySpin;
    QLabel * polyLabel;
    QWidget * plotWidget;
    QLabel * cnLabel;
    QHBoxLayout * hl;
    QGroupBox *plotSetup;
    QVBoxLayout * vl;
    plotInfo_t plot1;

    void UpdateTitle(QString tube);

    double calcPolyPoint( double x, QList<float> *);
    void CurveFit(int n, QList<QPointF> *, QList<float> *);
    void gaussEliminationLS(int m, int n, double * a, double x[]);

    void SplineFit(QList<QPointF> * points, gsl_interp_accel **acc, gsl_spline **spline );
    void SplineFree(gsl_interp_accel *acc, gsl_spline *spline);

    void bSplineFree(bspline_info_t *);
    void bSplineFit(bspline_info_t *, QList<QPointF> *);
    double bspline_eval(bspline_info_t *, double);

    QList<QPen> * ppenList=0;
    QCPTextElement * title = 0;
    //QCPPlotTitle * title = 0; //QCP v 1.3.1.
    QCPItemTracer * marker;
    QCPItemText * markerText;
    //QList<QCPItemTracer *> markerList;
    //QList<QCPItemText *> markerTextList;

    QColor plotAreaColor;
    QColor plotBackGroundColor;
    QColor plotGridColor;
    QColor plotAxisColor;
    QColor plotTextColor;
    int plotGridLine;
    QColor plotLegendColor;
    QColor plotLegendTextColor;
    QColor plotSHowValColor;
    //QList <QColor> plotLineColorList;
    int ShowValIndex;
};

#endif // PLOTTABWIDGET_H
