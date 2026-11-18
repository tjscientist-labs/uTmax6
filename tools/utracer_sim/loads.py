"""What is connected to the simulated uTracer: the device under test.

Each load answers one question: given the voltages on the anode, screen and grid
(relative to the cathode), what currents flow into the anode and the screen?
All values in volts and amps.

The resistor and diode loads match the physical dummy loads used for the
first hardware checks, so a simulated run and a bench run can be compared
directly.
"""

import math


class Open:
    """Nothing connected."""
    name = "open"

    def currents(self, va, vs, vg):
        return 0.0, 0.0


class Resistor:
    """A resistor from anode to cathode, and optionally one from screen to cathode."""

    def __init__(self, r_anode, r_screen=None):
        self.ra = r_anode
        self.rs = r_screen
        self.name = f"resistor(anode={r_anode:g} ohm" + (
            f", screen={r_screen:g} ohm)" if r_screen else ")")

    def currents(self, va, vs, vg):
        ia = max(va, 0.0) / self.ra
        is_ = max(vs, 0.0) / self.rs if self.rs else 0.0
        return ia, is_


class Diode:
    """An ideal-ish diode in series with a resistor (forward drop vf, then R)."""

    def __init__(self, vf, r):
        self.vf, self.r = vf, r
        self.name = f"diode(vf={vf:g} V, r={r:g} ohm)"

    def currents(self, va, vs, vg):
        return max(va - self.vf, 0.0) / self.r, 0.0


class KorenTriode:
    """Norman Koren's triode model (the one uTmax fits to measured curves).

        E1 = (Va / Kp) * ln(1 + exp(Kp * (1/mu + Vg / sqrt(Kvb + Va^2))))
        Ia = E1^Ex / Kg1 * (1 + sign(E1))        [Ia in amps]

    Source: N. Koren, "Improved vacuum tube models for SPICE simulations",
    https://www.normankoren.com/Audio/Tubemodspice_article.html
    The screen line is unused (zero current) for a triode.
    """

    PRESETS = {
        # 12AX7 parameters as published in Koren's article. Add other tubes only
        # with their source; a wrong preset would make wrong "expected" curves.
        "12AX7": dict(mu=100.0, ex=1.4, kg1=1060.0, kp=600.0, kvb=300.0),
    }

    def __init__(self, mu, ex, kg1, kp, kvb, name="triode"):
        self.mu, self.ex, self.kg1, self.kp, self.kvb = mu, ex, kg1, kp, kvb
        self.name = f"koren-triode({name})"

    @classmethod
    def preset(cls, tube):
        return cls(name=tube, **cls.PRESETS[tube])

    def currents(self, va, vs, vg):
        if va <= 0:
            return 0.0, 0.0
        x = self.kp * (1.0 / self.mu + vg / math.sqrt(self.kvb + va * va))
        # ln(1+exp(x)) without overflow for large x
        soft = x + math.log1p(math.exp(-x)) if x > 30 else math.log1p(math.exp(x))
        e1 = va / self.kp * soft
        if e1 <= 0:
            return 0.0, 0.0
        return (e1 ** self.ex) / self.kg1 * 2.0, 0.0


def from_spec(spec):
    """Build a load from a short text spec, as given on the command line:

        open
        r:10k              resistor anode-cathode
        r:10k,s:47k        plus a screen resistor
        diode:0.7,1k       diode drop 0.7 V then 1 kOhm
        triode:12AX7       Koren model preset
    """
    spec = spec.strip()
    if spec == "open":
        return Open()
    if spec.startswith("triode:"):
        return KorenTriode.preset(spec.split(":", 1)[1].upper())
    if spec.startswith("diode:"):
        vf, r = spec.split(":", 1)[1].split(",")
        return Diode(float(vf), parse_ohms(r))
    if spec.startswith("r:"):
        parts = dict(p.split(":", 1) for p in spec.split(","))
        return Resistor(parse_ohms(parts["r"]),
                        parse_ohms(parts["s"]) if "s" in parts else None)
    raise ValueError(f"unknown load spec: {spec!r}")


def parse_ohms(text):
    text = text.strip().lower()
    for unit in ("ohms", "ohm", "ω"):
        if text.endswith(unit):
            text = text[: -len(unit)].strip()
    mult = 1.0
    if text.endswith("k"):
        mult, text = 1e3, text[:-1]
    elif text.endswith("m"):
        mult, text = 1e6, text[:-1]
    return float(text) * mult
