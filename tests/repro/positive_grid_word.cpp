// Reproduces uTmax 3.01 GetVg() (uTracer6 branch) and the sweep command snprintf,
// to show what is sent for a positive grid voltage. Build and run:
//   g++ -o /tmp/pgw tests/repro/positive_grid_word.cpp && /tmp/pgw
// See tests/test_known_bugs.py.
#include <cstdio>
#include <cmath>
typedef unsigned int uint;
uint GetVg(float v, float VgMax, float Vg0, float Vgm) {
    float V0 = (v - Vg0) / Vgm;            // mainwindow.cpp:2275
    float Vg = (-32767) / VgMax * V0;      // :2294
    int g = int(round(Vg));                // :2295
    if (g > 32767) g = 32767;
    if (g < -32768) g = -32768;
    return (g);                            // :2308, int -> uint
}
int main() {
    char buf[64];
    float tests[] = {-2.0f, 0.0f, 1.0f, 10.0f};
    for (float v : tests) {
        uint VgADC = GetVg(v, 100, 0, 1);
        uint VaADC = 0x00E3, VsADC = 0, VfADC = 107;
        int n = ::snprintf(buf, 19, "10%04X%04X%04X%04X", VaADC, VsADC, VgADC, VfADC); // :1415
        char full[64]; snprintf(full, 64, "10%04X%04X%04X%04X", VaADC, VsADC, VgADC, VfADC);
        printf("Vg=%+5.1f V  intended=%s (%d chars)  sent=%s\n", v, full, n, buf);
    }
}
