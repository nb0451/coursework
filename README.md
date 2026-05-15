# READ ME

# 25WSA032 Coursework — Zaid Haris (F533590)
**This is the readme of Zaid Haris F533590 Coursework project.**

This repository contains the coursework submission for 25WSA032. It covers the four tasks: Git source control, Arduino temperature optimisation, Python robot delivery optimisation, and data analytics.

## Repository Structure
**The repository has 4 main folders**

`documentation/` - Contains all documentation (markdown files)
`arduino/` - Contains all of the elements for Task 2
`robots/` - Contains all of the elements for Task 3
`memory/` - Contains all of the elements for Task 4

## How to Run
**Task 2** 
Open `arduino/temperature_optimisation.ino` in the Arduino IDE or with the VSCode extension.

**Task 3** 
Open the terminal in the root folder and type `python -m robots.robot_optimisation` 

**Task 4** 
Open the terminal in the root folder and type `python -m memory.data_analysis`

## Task Summary
**Task 1 - Git** 
Version controlled throughout using structured commit messages, milestone tags, and documented AI usage.

**Task 2 - Arduino** 
Adaptive temperature monitoring system implementing DFT-based frequency analysis, three power modes (ACTIVE/IDLE/POWER_DOWN), Nyquist-based adaptive sampling, moving average prediction, and memory-efficient static buffers.

**Task 3 - Python** 
Bot delivery optimisation implementing per-bot charge thresholds, distributed chargers with nearest-charger routing, opportunistic charging, nearest-pizza allocation, and heavier pizza support. Full optimisation achieved +61% units delivered and +113% weight delivered vs baseline over 52 weeks.

**Task 4 - Data Analytics** 
Recorded 3 cycles of Arduino temperature data with manual thermal variation, performed DFT analysis on the Arduino, and generated 5 matplotlib plots with a written discussion of time-domain and frequency-domain findings.

## AI USAGE

I used AI to assist with code generation throughout this project. AI usage is documented in commit messages and summarised below by date.

27 April: Used AI to debug collect_temperature_data() for any errors
29 April: Studied DFT theory and documented AI usage plan for apply_dft implementation
30 April: Used AI to implement apply_dft function based on pre-planned logical steps
1 May: Used AI to implement send_data_to_pc() and fix DFT accuracy issues
2 May: Used AI to implement decide_power_mode state machine based on self-designed architecture
3 May: Used AI to assist with variation-based mode decisions and adaptive sampling integration
4 May: Used AI to refine decide_power_mode logic and moving average prediction
12 May: Used AI to assist with robot_optimisation.py baseline setup and threshold dictionary
13 May: Used AI to assist with charger routing, opportunistic charging and pizza allocation
14 May: Used AI to assist with staged KPI comparison table and final code quality pass

## MARCH COMMITS STATEMENT

The six commits made in March were the result of troubleshooting and setup issues while configuring GIT across different devices. These were part of the initial setup rather than the project's development.
