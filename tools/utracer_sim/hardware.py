"""The uTracer's hardware, as numbers: converting between real volts/amps and the
10-bit codes that travel over the serial line.

Everything that depends on the physical board (divider resistors, sense resistors,
the grid DAC range) comes from a *profile* file in ``profiles/``. Keeping those
numbers out of the code matters: the simulator is meant to model the real
hardware, so that when uTmax gets a conversion wrong the simulator disagrees with
it. Each value in a profile records where it came from.

Formulas follow Ronald Dekker's description of the uTracer protocol
(dos4ever.com, uTracer3 log section 15) and the reply layout that uTmax parses
(``MainWindow::saveADCInfo()`` in uTmax 3.01).
"""

import json
import math
import os

ADC_FULL = 1023      # 10-bit ADC and PWM
ADC_VREF = 5.0       # volts at full scale
PGA_GAINS = [1, 2, 5, 10, 20, 50, 100, 200]   # PGA113 gain codes 0..7; code 8 = auto


class Profile:
    """One uTracer model's hardware constants, loaded from a JSON profile."""

    def __init__(self, path):
        with open(path, encoding="utf-8") as f:
            raw = json.load(f)
        self.name = raw["name"]
        self.source_notes = raw.get("notes", "")
        # Each entry is {"value": ..., "source": "..."}; keep only the value here.
        v = {k: e["value"] for k, e in raw["constants"].items()}
        self.r_div_top = v["hv_divider_top_ohm"]          # anode/screen reservoir divider
        self.r_div_bottom = v["hv_divider_bottom_ohm"]
        self.r_sense_a = v["anode_sense_ohm"]             # current-sense resistors
        self.r_sense_s = v["screen_sense_ohm"]
        self.vsu_div_top = v["supply_divider_top_ohm"]    # supply-voltage divider
        self.vsu_div_bottom = v["supply_divider_bottom_ohm"]
        self.vn_div_top = v["neg_divider_top_ohm"]        # negative-rail divider
        self.vn_div_bottom = v["neg_divider_bottom_ohm"]
        self.supply_v = v["supply_volts"]                 # the laptop brick
        self.neg_rail_v = v["neg_rail_volts"]
        self.grid_bits = v["grid_dac_bits"]               # 16 (uTracer6) or 12/10 (uTracer3)
        self.grid_full_scale_v = v["grid_full_scale_volts"]
        self.v_drop_diode = v.get("cathode_diode_drop_volts", 0.0)
        self.v_drop_switch = v.get("hv_switch_drop_volts", 0.0)
        self.max_hv = v["max_hv_volts"]

    @staticmethod
    def find(name):
        """Load profiles/<name>.json next to this file."""
        here = os.path.dirname(os.path.abspath(__file__))
        return Profile(os.path.join(here, "profiles", name + ".json"))

    # ---- anode / screen reservoir voltage <-> code --------------------------------
    # The firmware charges each reservoir until its divided-down voltage reaches the
    # commanded ADC code, and reports the code it measured. The reservoir voltage is
    # measured to ground; the tube's cathode sits at the supply voltage (Dekker §15:
    # "the anode voltage is the voltage of the buffer capacitor minus 18.5 V").
    def hv_scale(self):
        return (self.r_div_top + self.r_div_bottom) / self.r_div_bottom

    def hv_code_to_volts(self, code):
        return code * ADC_VREF / ADC_FULL * self.hv_scale()

    def hv_volts_to_code(self, volts):
        return clamp_code(round(volts / self.hv_scale() * ADC_FULL / ADC_VREF))

    # ---- supply voltage ---------------------------------------------------------
    def vsu_code(self):
        scale = (self.vsu_div_top + self.vsu_div_bottom) / self.vsu_div_bottom
        return clamp_code(round(self.supply_v / scale * ADC_FULL / ADC_VREF))

    # ---- negative rail ----------------------------------------------------------
    # uTmax decodes: Vn = 5 * (k * (code/1023 - 1) + 1), k = (top+bottom)/bottom,
    # i.e. a divider from the negative rail up to the 5 V reference.
    def vn_code(self):
        k = (self.vn_div_top + self.vn_div_bottom) / self.vn_div_bottom
        code = ADC_FULL * ((self.neg_rail_v / ADC_VREF - 1.0) / k + 1.0)
        return clamp_code(round(code))

    # ---- grid -------------------------------------------------------------------
    def grid_code_to_volts(self, word):
        """Grid volts for the 16-bit word in a command (negative = more bias)."""
        if self.grid_bits == 16:
            signed = word - 0x10000 if word & 0x8000 else word
            return -signed * self.grid_full_scale_v / 32767.0
        # uTracer3: 12-bit PWM packed as uTmax does it (low 2 bits in bits 14-15).
        g = ((word & 0x3FF) << 2) | ((word >> 14) & 0x3)
        return -g * self.grid_full_scale_v / 4096.0

    # ---- heater -----------------------------------------------------------------
    def heater_code_to_volts(self, word):
        """RMS heater volts. The PWM duty is (Vrms/Vsupply)^2, scaled to 1023."""
        if word & 0xE000 == 0xE000:            # uTmax "external heater" encoding
            return None
        duty = min(word, ADC_FULL) / ADC_FULL
        return self.supply_v * math.sqrt(duty)

    # ---- currents ---------------------------------------------------------------
    @staticmethod
    def current_code(amps, r_sense, gain, n_avg):
        """ADC code for a current: volts across the sense resistor, times the PGA
        gain, summed over n_avg samples (uTmax divides by the averaging count)."""
        per_sample = amps * r_sense * gain / ADC_VREF * ADC_FULL
        return round(per_sample * n_avg), per_sample

    @staticmethod
    def auto_gain(amps, r_sense):
        """Highest PGA gain that keeps one sample inside the ADC range."""
        best = 0
        for i, g in enumerate(PGA_GAINS):
            if amps * r_sense * g / ADC_VREF * ADC_FULL <= ADC_FULL:
                best = i
        return best


def clamp_code(c):
    return max(0, min(ADC_FULL, int(c)))
