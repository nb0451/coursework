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

CHARGE_THRESHOLDS = {
    'Robot': 0.18,
    'Droid': 0.15,
    'Drone': 0.12
}
OPPORTUNISTIC_THRESHOLD = 0.40
OPPORTUNISTIC_DISTANCE = 5.0

def find_nearest_charger(bot, chargers):
    min_dist = float('inf')
    nearest = None
    for charger in chargers:
        dist = math.sqrt((bot.coordinates[0] - charger.coordinates[0])**2 + (bot.coordinates[1] - charger.coordinates[1])**2)
        if dist < min_dist:
            min_dist = dist
            nearest = charger
    return nearest

def find_nearest_pizza(bot, deliverables):
    nearest_pizza = None
    min_dist = float('inf')
    for pizza in deliverables:
        if pizza.status == 'ready' and pizza.weight <= bot.max_payload:
            dist = math.sqrt((bot.coordinates[0] - pizza.coordinates[0])**2 + (bot.coordinates[1] - pizza.coordinates[1])**2)
            if dist < min_dist:
                min_dist = dist
                nearest_pizza = pizza
    return nearest_pizza

def run_simulation(mode='optimized'):
    if mode == 'baseline':
        es = ecofactory(robots=3, droids=3, drones=3, chargers=[55, 20], pizzas=9)
        threshold = 0.20
    else:
        es = ecofactory(robots=3, droids=3, drones=3, chargers=[[20, 10], [60, 30], [40, 20]], pizzas=9)
        threshold = None

    chargers = es.chargers()
    home = [40, 20, 0]

    es.display(show=0, pause=10)
    es.duration = "2 weeks"
    es.messages_on = False
    es.debug = False

    while es.active:
        for bot in es.bots():
            t = CHARGE_THRESHOLDS.get(bot.kind, 0.20) if mode == 'optimized' else threshold
            if bot.soc / bot.max_soc < t and bot.station is None:
                charger = find_nearest_charger(bot, chargers)
                bot.charge(charger)

            if mode == 'optimized' and bot.station is None and bot.activity != 'charging':
                nearest = find_nearest_charger(bot, chargers)
                dist = math.sqrt((bot.coordinates[0] - nearest.coordinates[0])**2 + (bot.coordinates[1] - nearest.coordinates[1])**2)
                if dist < OPPORTUNISTIC_DISTANCE and bot.soc / bot.max_soc < OPPORTUNISTIC_THRESHOLD:
                    bot.charge(nearest)

            if bot.activity == 'idle':
                pizza = find_nearest_pizza(bot, es.deliverables())
                if pizza:
                    bot.deliver(pizza)

            if not bot.destination and bot.coordinates != home:
                bot.target_destination = home

            if bot.target_destination:
                bot.move()

        es.update()

    kpi_data = {}
    total_units = total_weight = total_distance = total_energy = total_damage = 0
    for bot in es.bots():
        kpi_data[bot.name] = {
            'units': bot.units_delivered,
            'weight': bot.weight_delivered,
            'distance': bot.distance,
            'energy': bot.energy,
            'damage': bot.damage
        }
        total_units += bot.units_delivered
        total_weight += bot.weight_delivered
        total_distance += bot.distance
        total_energy += bot.energy
        total_damage += bot.damage

    kpi_data['fleet'] = {
        'units': total_units,
        'weight': total_weight,
        'distance': total_distance,
        'energy': total_energy,
        'damage': total_damage
    }
    return kpi_data

def print_comparison(baseline, optimized):
    base_units = baseline['fleet']['units']
    opt_units = optimized['fleet']['units']
    delta_units = opt_units - base_units
    pct_units = (delta_units / base_units) * 100

    base_distance = baseline['fleet']['distance']
    opt_distance = optimized['fleet']['distance']
    delta_distance = opt_distance - base_distance
    pct_distance = (delta_distance / base_distance) * 100

    base_energy = baseline['fleet']['energy']
    opt_energy = optimized['fleet']['energy']
    delta_energy = opt_energy - base_energy
    pct_energy = (delta_energy / base_energy) * 100

    print("\n=== PIZZA ALLOCATION OPTIMIZATION DELTA (2 WEEKS) ===")
    print(f"{'Metric':<20} {'Baseline':<10} {'Optimized':<10} {'Delta':<10} {'% Change':<10}")
    print(f"{'Units Delivered':<20} {base_units:<10} {opt_units:<10} {delta_units:<+10} {pct_units:+.2f}%")
    print(f"{'Distance (units)':<20} {base_distance:<10} {opt_distance:<10} {delta_distance:<+10} {pct_distance:+.2f}%")
    print(f"{'Energy (kWh)':<20} {base_energy:<10} {opt_energy:<10} {delta_energy:<+10} {pct_energy:+.2f}%")

if __name__ == "__main__":
    baseline_kpis = run_simulation(mode='baseline')
    optimized_kpis = run_simulation(mode='optimized')
    print_comparison(baseline_kpis, optimized_kpis)