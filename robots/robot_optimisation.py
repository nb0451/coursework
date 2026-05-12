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
import matplotlib.pyplot as plt

plt.close('all')
plt.ion()

def run_baseline():
    es = ecofactory(robots=3, droids=3, drones=3, chargers=[55, 20], pizzas=9)
    home = [40, 20, 0]
    charge_threshold = 0.20

    es.display(show=1, pause=10)
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

    es.tabulate(
        'name', 'kind', 'units_delivered', 'weight_delivered',
        'distance', 'energy', 'damage', 'status', kind_class='Bot'
    )
    return es

if __name__ == "__main__":
    baseline_ecosystem = run_baseline()