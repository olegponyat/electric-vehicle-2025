# Electric Vehicle (2026)

Science Olympiad Electric Vehicle for the 2025-2026 season. Part files, PCB schematics, and Arduino C++ firmware for a vehicle that had to navigate a precise curved path and stop within millimeters of a target — with no remote control or autonomous correction.

## The Event

The 2025-2026 Electric Vehicle event required the car to:
- Travel a curved path with a specified radius
- Stop as close to a target distance marker as possible
- Be built as thin as possible to score a profile bonus

You set all parameters before the run. Whatever happened after that, happened. Scoring penalized every source of imprecision: miss your stop distance, miss your arc, miss your timing — all deducted separately.

## Design

### Chassis
- Low-profile chassis designed in Onshape and Inventor, optimized for minimum width
- 3D-printed structural components
- Driven by a single DC motor through a gear reduction stage

### Steering Mechanism
- Caliper-based turning control to set the curve angle before each run
- The caliper position determines the turning radius — set once, locked in
- Multiple iterations to achieve rigidity: the biggest challenge was preventing positional drift between sessions

### Electronics
- Custom PCB designed in KiCad
- ATmega-based microcontroller running PlatformIO/C++ firmware
- Hall effect sensors for distance measurement
- Power regulation circuitry to maintain consistent motor voltage

## The Hard Problem: Consistency

The technical challenge wasn't building the car — it was making it repeatable. The caliper steering would hold a set position during one session, then drift slightly before the next. Same parameters, different arc.

Since even a small deviation in the turning radius compounds over distance, this variability was fatal for scoring. The root cause: flex in the caliper mount under repeated load cycles. The fix required iterating the assembly geometry until the structure was rigid enough to hold position without sacrificing the caliper's range of motion.

Several reprints later, the mechanism held consistently across a full practice session for the first time.

## Results

- **4th Place Regional Competition**
- **9th Place State Competition**
- Science Olympiad 2026

## Reflection

The caliper problem was a useful lesson in what consistency actually requires. I had been treating it as a precision problem — tweak the value, compensate for drift — when it was a structural problem. The mechanism wasn't stiff enough to hold position under repeated load, and no amount of calibration would fix that.

What took me longer than it should have was recognizing that I was solving the wrong problem. Each time the car ran inconsistently, my instinct was to adjust the parameters rather than question the structure. Once I stopped trying to tune my way out of a physics problem and started treating rigidity as the actual design constraint, the path forward was clear.

4th at regionals and 9th at states isn't a win, but in an event where scores are separated by fractions of a second and millimeters, consistency is the whole game. The car that placed reliably finished above cars with faster top runs and more failures. That tradeoff — raw performance vs. repeatability — is one I think about a lot now.
