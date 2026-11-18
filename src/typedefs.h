#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <QString>
#include <QList>
#include <QPen>

#define NONE "Do Not Model"
#define DIODE "Koren Diode"
#define DUAL_DIODE "Koren Dual Diode"
#define TRIODE "Koren Triode"
#define DUAL_TRIODE "Koren Dual Triode"
#define KOREN_P "Koren Pentode"
#define DERK_P "Derk Pentode"
#define DERK_P2 "Pentode II"
#define DERK_PE "Pentode + SE"
#define DERK_B "DerkE BTet+SE"

#define IGNORE_CURRENT (1)
#define IGNORE_POWER (2)

struct results_t
{
    float Ia;
    float Is;
    float Va;
    float Vs;
    float Vg;
    float Vf;
    float IaMod;
    float IsMod;
    float gm_a;
    float gm_b;
    float ra_a;
    float ra_b;
    float mu_a;
    float mu_b;
    uint ignore;
};

struct settings_t {
    float Va;
    float Vs;
    float Vg;
    uint delay;
    bool VaVsTrue;
} ;

#endif // TYPEDEFS_H
