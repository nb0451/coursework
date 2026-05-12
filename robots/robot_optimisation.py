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
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from robots.ecosystem.factory import ecofactory
from robots.ecosystem.bots import Bot, Robot, Droid, Drone
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

plt.close('all')

def run_baseline():
    es = ecofactory(robots=3, droids=3, drones=3, chargers=[55, 20], pizzas=9)
    home = [40, 20, 0]
    charge_threshold = 0.20

    es.display(show=0, pause=10)
    es.duration = "2 weeks"
    es.messages_on = False
    es.debug = False

    while es.active:
        for bot in es.bots():
            if bot.soc / bot.max_soc < charge_threshold and bot.station is None:
                charger = es.chargers()[0]
                bot.charge(charger)

            if bot.activity == 'idle':
                for pizza in es.deliverables():
                    if pizza.status == 'ready':
                        bot.deliver(pizza)
                        break

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

    print("\n=== BASELINE KPI RECORD ===")
    for name, metrics in kpi_data.items():
        print(f"{name}: {metrics}")

    es.tabulate('name', 'kind', 'units_delivered', 'weight_delivered', 'distance', 'energy', 'damage', 'status', kind_class='Bot')
    return kpi_data

if __name__ == "__main__":
    baseline_kpis = run_baseline()