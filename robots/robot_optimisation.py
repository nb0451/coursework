"""
Task 3 - Bot Pizza Delivery Optimisation
Implements charging, pizza allocation, and KPI reporting optimisation strategies for the bot ecosystem.
Strategies implemented:
- Per-bot-type charge threshold optimisation
- Multiple distributed chargers with nearest-charger routing
- Opportunistic charging during delivery/return journeys
- Distance-aware pizza allocation replacing first-come-first-served
- Heavier pizza weight allocation with bot capacity validation
- Counter-factual baseline comparison and formatted KPI tabulation (f-strings)

Name: Zaid Haris
ID: F533590
"""

import sys
import os
import math
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from robots.ecosystem.factory import ecofactory
from robots.ecosystem.bots import Bot, Robot, Droid, Drone
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

plt.close('all')

# Charge thresholds per bot type - tuned lower than 0.20 baseline since chargers are closer
CHARGE_THRESHOLDS = {'Robot': 0.18, 'Droid': 0.15, 'Drone': 0.12}
OPPORTUNISTIC_THRESHOLD = 0.40  # Only charge opportunistically if SOC is below this
OPPORTUNISTIC_DISTANCE = 5.0    # How close a bot needs to be to a charger to trigger opportunistic stop


def find_nearest_charger(bot, chargers):
    """Returns the closest charger to the bot using straight-line distance."""
    min_dist = float('inf')
    nearest = None
    for charger in chargers:
        dist = math.hypot(bot.coordinates[0] - charger.coordinates[0],
                          bot.coordinates[1] - charger.coordinates[1])
        if dist < min_dist:
            min_dist = dist
            nearest = charger
    return nearest


def find_nearest_pizza(bot, deliverables):
    """Returns the closest ready pizza the bot can carry.
    Checks weight limit so the ecosystem doesn't reject the contract."""
    nearest_pizza = None
    min_dist = float('inf')
    for pizza in deliverables:
        if pizza.status == 'ready' and pizza.weight <= bot.max_payload:  # Weight filter prevents contract rejections
            dist = math.hypot(bot.coordinates[0] - pizza.coordinates[0],
                              bot.coordinates[1] - pizza.coordinates[1])
            if dist < min_dist:
                min_dist = dist
                nearest_pizza = pizza
    return nearest_pizza


def run_simulation(mode='full'):
    """Runs the ecosystem under one of three modes for staged KPI comparison.
    baseline  - replicates the unoptimised demo exactly
    charging  - adds distributed chargers, per-bot thresholds, opportunistic charging
    full      - everything above plus nearest pizza and heavier pizza support"""
    if mode == 'baseline':
        es = ecofactory(robots=3, droids=3, drones=3, chargers=[55, 20], pizzas=9)
        threshold = 0.20  # Fixed threshold matching the original demo
    elif mode == 'charging':
        es = ecofactory(robots=3, droids=3, drones=3, chargers=[[20, 10], [60, 30], [40, 20]], pizzas=9)
        threshold = None
    else:
        es = ecofactory(robots=3, droids=3, drones=3, chargers=[[20, 10], [60, 30], [40, 20]], pizzas=9)
        es._max_weight = 30  # Raises max pizza weight to increase delivery payload
        threshold = None

    chargers = es.chargers()
    home = [40, 20, 0]

    es.display(show=0, pause=10)
    es.duration = "52 weeks"
    es.messages_on = False
    es.debug = False

    while es.active:
        for bot in es.bots():

            # Use per-bot threshold in optimised modes, fixed threshold in baseline
            t = CHARGE_THRESHOLDS.get(bot.kind, 0.20) if mode != 'baseline' else threshold
            if bot.soc / bot.max_soc < t and bot.station is None:
                bot.charge(find_nearest_charger(bot, chargers))

            # Opportunistic charging - stops to charge if passing near a charger with low SOC
            if mode != 'baseline' and bot.station is None and bot.activity != 'charging':
                nearest = find_nearest_charger(bot, chargers)
                dist = math.hypot(bot.coordinates[0] - nearest.coordinates[0],
                                  bot.coordinates[1] - nearest.coordinates[1])
                if dist < OPPORTUNISTIC_DISTANCE and bot.soc / bot.max_soc < OPPORTUNISTIC_THRESHOLD:
                    bot.charge(nearest)

            # Full mode uses nearest pizza, other modes use first-come-first-served
            if bot.activity == 'idle':
                if mode == 'full':
                    pizza = find_nearest_pizza(bot, es.deliverables())
                    if pizza:
                        bot.deliver(pizza)
                else:
                    for pizza in es.deliverables():
                        if pizza.status == 'ready':
                            bot.deliver(pizza)
                            break

            if not bot.destination and bot.coordinates != home:
                bot.target_destination = home  # Return home when idle with no delivery
            if bot.target_destination:
                bot.move()

        es.update()

    # Sum up fleet-wide totals after the run
    total_units = total_weight = total_distance = total_energy = total_damage = 0
    for bot in es.bots():
        total_units += bot.units_delivered
        total_weight += bot.weight_delivered
        total_distance += bot.distance
        total_energy += bot.energy
        total_damage += bot.damage

    return {
        'units': total_units,
        'weight': total_weight,
        'distance': total_distance,
        'energy': total_energy,
        'damage': total_damage
    }


def print_staged_report(baseline, charging, full):
    """Prints a formatted comparison table showing KPI improvement across all three stages."""
    print("\n" + "="*85)
    print("STAGED FLEET-WIDE KPI PERFORMANCE COMPARISON (52 WEEKS)")
    print("="*85)
    print(f"{'Metric':<20} {'Baseline':<12} {'Charging Opt':<12} {'Full Opt':<12} {'Δ vs Base':<12}")
    print("-"*85)

    metrics = ['units', 'weight', 'distance', 'energy', 'damage']
    for m in metrics:
        base_val = baseline[m]
        chg_val = charging[m]
        full_val = full[m]
        delta = full_val - base_val
        pct = (delta / base_val) * 100 if base_val != 0 else 0.0

        sign = "+" if delta > 0 else ""
        print(f"{m.capitalize():<20} {base_val:<12} {chg_val:<12} {full_val:<12} {sign}{delta:<11.2f} ({sign}{pct:.2f}%)")
    print("="*85)


if __name__ == "__main__":
    baseline_kpis = run_simulation(mode='baseline')   # Unoptimised counterfactual
    charging_kpis = run_simulation(mode='charging')   # Charging optimisation only
    full_kpis = run_simulation(mode='full')           # All optimisations active
    print_staged_report(baseline_kpis, charging_kpis, full_kpis)