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

    Spaces and the case of the keys don't matter. Anything not understood is an
    error (ValueError), never silently dropped.
    """
    spec = spec.strip()
    kind, _, rest = spec.partition(":")
    kind = kind.strip().lower()
    if kind == "open" and not rest:
        return Open()
    if kind == "triode":
        tube = rest.strip().upper()
        if tube not in KorenTriode.PRESETS:
            raise ValueError(f"no triode preset {tube!r}; known: {', '.join(KorenTriode.PRESETS)}")
        return KorenTriode.preset(tube)
    if kind == "diode":
        fields = [f.strip() for f in rest.split(",")]
        if len(fields) != 2:
            raise ValueError(f"diode load needs 'diode:<drop V>,<ohms>': {spec!r}")
        try:
            vf = float(fields[0])
        except ValueError:
            raise ValueError(f"diode drop is not a number: {fields[0]!r}") from None
        if vf < 0:
            raise ValueError(f"diode drop must not be negative: {spec!r}")
        return Diode(vf, positive_ohms(fields[1]))
    if kind == "r":
        parts = {}
        for item in spec.split(","):
            key, sep, value = item.partition(":")
            key = key.strip().lower()
            if not sep or key not in ("r", "s") or key in parts:
                raise ValueError(f"resistor load is 'r:<ohms>' or 'r:<ohms>,s:<ohms>': {spec!r}")
            parts[key] = positive_ohms(value)
        return Resistor(parts["r"], parts.get("s"))
    raise ValueError(f"unknown load spec {spec!r}; use open, r:10k, r:10k,s:47k, "
                     "diode:0.7,1k or triode:12AX7")


def parse_ohms(text):
    """Ohms from '470', '4.7k', '4k7', '1M', '1meg', '2R2', '10 ohm'.

    'M' and 'meg' are megohms. A lone lower-case 'm' is refused: in resistor
    values it is ambiguous (milli or mega)."""
    t = text.strip()
    for unit in ("ohms", "ohm", "Ω", "ω"):
        if t.lower().endswith(unit.lower()):
            t = t[: -len(unit)].strip()
            break
    if t.lower().endswith("meg"):
        t = t[:-3] + "M"
    mults = {"R": 1.0, "r": 1.0, "k": 1e3, "K": 1e3, "M": 1e6}
    if "m" in t:
        raise ValueError(f"{text!r}: write M or meg for megohms ('m' is ambiguous)")
    for letter, mult in mults.items():
        if letter in t:
            whole, _, frac = t.partition(letter)
            if frac and not frac.isdigit():
                break
            try:
                return float((whole or "0") + ("." + frac if frac else "")) * mult
            except ValueError:
                break
    else:
        try:
            return float(t)
        except ValueError:
            pass
    raise ValueError(f"not a resistance: {text!r}")


def positive_ohms(text):
    ohms = parse_ohms(text)
    if not ohms > 0 or math.isinf(ohms):
        raise ValueError(f"resistance must be above 0: {text!r}")
    return ohms
