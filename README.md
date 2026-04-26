# Energy-Efficient CPU Scheduling Algorithm

This project demonstrates a power-aware CPU scheduling approach that combines:
- Dynamic Voltage and Frequency Scaling (DVFS)
- Thermal-aware scheduling
- Fairness and performance balancing

It includes:
- Full theory and project documentation
- Complete C language simulation
- Modern web UI demo (HTML, CSS, JavaScript)
- Energy vs performance comparison graph

## Project Structure

- `docs/report.md` - Final-year style theory, algorithm, pseudocode, outputs, and explanations
- `c-src/energy_scheduler.c` - Complete C simulation code
- `web/index.html` - Frontend interface
- `web/styles.css` - UI styling
- `web/script.js` - Scheduling simulation logic and graph rendering in browser

## How to Run C Program

1. Open terminal in project root.
2. Compile:
   - GCC: `gcc c-src/energy_scheduler.c -o energy_scheduler`
3. Run:
   - Windows: `./energy_scheduler`

## How to Run Web Demo

1. Open `web/index.html` in browser.
2. Add process details.
3. Click **Run Scheduler**.
4. View scheduling result, energy savings, and comparison graph.

## Beginner Notes

- The C program is suitable for algorithm explanation in OS practical/project viva.
- The web demo is suitable for presentation to show interactive behavior.
- Both C and JS implementations use the same core idea for consistency.
