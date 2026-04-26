const BASELINE_FREQ = 2.5;
const BASELINE_VOLT = 1.2;

const DVFS_LEVELS = [
    { name: "LOW", frequency: 1.2, voltage: 0.9 },
    { name: "MEDIUM", frequency: 1.8, voltage: 1.0 },
    { name: "HIGH", frequency: 2.5, voltage: 1.2 }
];

const processBody = document.getElementById("processBody");
const resultBody = document.getElementById("resultBody");
const summary = document.getElementById("summary");
const addRowBtn = document.getElementById("addRowBtn");
const runBtn = document.getElementById("runBtn");
const canvas = document.getElementById("compareChart");

function createInput(value, min, step = "any") {
    const input = document.createElement("input");
    input.type = "number";
    input.value = value;
    input.min = min;
    input.step = step;
    return input;
}

function addProcessRow(data = {}) {
    const row = document.createElement("tr");

    const idCell = document.createElement("td");
    const currentId = processBody.children.length + 1;
    idCell.textContent = `P${currentId}`;

    const burstCell = document.createElement("td");
    const burstInput = createInput(data.burst ?? 6, 0.1, 0.1);
    burstCell.appendChild(burstInput);

    const priorityCell = document.createElement("td");
    const priorityInput = createInput(data.priority ?? 3, 1, 1);
    priorityCell.appendChild(priorityInput);

    const tempCell = document.createElement("td");
    const tempInput = createInput(data.temperature ?? 65, 20, 0.1);
    tempCell.appendChild(tempInput);

    const powerCell = document.createElement("td");
    const powerInput = createInput(data.basePower ?? 22, 0.1, 0.1);
    powerCell.appendChild(powerInput);

    const actionCell = document.createElement("td");
    const removeBtn = document.createElement("button");
    removeBtn.className = "btn danger";
    removeBtn.textContent = "Remove";
    removeBtn.addEventListener("click", () => {
        row.remove();
        refreshIds();
    });
    actionCell.appendChild(removeBtn);

    row.append(idCell, burstCell, priorityCell, tempCell, powerCell, actionCell);
    processBody.appendChild(row);
}

function refreshIds() {
    [...processBody.children].forEach((row, index) => {
        row.children[0].textContent = `P${index + 1}`;
    });
}

function chooseDvfsLevel(temperature, priority, burst) {
    if (temperature >= 80) return 0;
    if (priority <= 2 || burst <= 4) return 2;
    if (temperature >= 70) return 1;
    if (burst > 10) return 0;
    return 1;
}

function computeScore(proc, orderIndex) {
    const priorityTerm = 50 / proc.priority;
    const shortTerm = 30 / (proc.burst + 1);
    const agingTerm = 2 * orderIndex;
    const thermalPenalty = proc.temperature > 70 ? proc.temperature - 70 : 0;
    return priorityTerm + shortTerm + agingTerm - thermalPenalty;
}

function dynamicPower(basePower, voltage, frequency) {
    const voltageFactor = (voltage * voltage) / (BASELINE_VOLT * BASELINE_VOLT);
    const freqFactor = frequency / BASELINE_FREQ;
    return basePower * voltageFactor * freqFactor;
}

function readProcesses() {
    const rows = [...processBody.children];
    const processes = [];

    for (let i = 0; i < rows.length; i += 1) {
        const cells = rows[i].children;
        const burst = Number(cells[1].querySelector("input").value);
        const priority = Number(cells[2].querySelector("input").value);
        const temperature = Number(cells[3].querySelector("input").value);
        const basePower = Number(cells[4].querySelector("input").value);

        if (burst <= 0 || priority <= 0 || basePower <= 0 || Number.isNaN(burst) || Number.isNaN(priority) || Number.isNaN(temperature) || Number.isNaN(basePower)) {
            throw new Error(`Invalid input in Process P${i + 1}. Use positive numbers.`);
        }

        processes.push({
            id: i + 1,
            burst,
            priority,
            temperature,
            basePower,
            scheduled: false
        });
    }

    if (processes.length === 0) {
        throw new Error("Please add at least one process.");
    }

    return processes;
}

function runSimulation(processes) {
    let baselineEnergy = 0;
    let baselineTime = 0;

    processes.forEach((p) => {
        const execTime = p.burst;
        const power = dynamicPower(p.basePower, BASELINE_VOLT, BASELINE_FREQ);
        baselineEnergy += power * execTime;
        baselineTime += execTime;
    });

    const local = processes.map((p) => ({ ...p }));
    const records = [];

    let optimizedEnergy = 0;
    let optimizedTime = 0;
    let done = 0;

    while (done < local.length) {
        let bestIdx = -1;
        let bestScore = -Infinity;

        local.forEach((p, idx) => {
            if (!p.scheduled) {
                const score = computeScore(p, done);
                if (score > bestScore) {
                    bestScore = score;
                    bestIdx = idx;
                }
            }
        });

        const proc = local[bestIdx];
        const dvfsIndex = chooseDvfsLevel(proc.temperature, proc.priority, proc.burst);
        const level = DVFS_LEVELS[dvfsIndex];

        const execTime = proc.burst * (BASELINE_FREQ / level.frequency);
        const power = dynamicPower(proc.basePower, level.voltage, level.frequency);
        const energy = power * execTime;

        records.push({
            processId: proc.id,
            dvfsName: level.name,
            frequency: level.frequency,
            voltage: level.voltage,
            execTime,
            power,
            energy
        });

        optimizedEnergy += energy;
        optimizedTime += execTime;

        proc.scheduled = true;

        local.forEach((other) => {
            if (!other.scheduled) {
                if (dvfsIndex === 2) other.temperature += 0.6;
                else if (dvfsIndex === 1) other.temperature += 0.2;
                else other.temperature -= 0.4;

                other.temperature = Math.max(25, Math.min(95, other.temperature));
            }
        });

        done += 1;
    }

    const energySaved = baselineEnergy - optimizedEnergy;
    const energySavedPct = (energySaved / baselineEnergy) * 100;
    const timeChangePct = ((optimizedTime - baselineTime) / baselineTime) * 100;

    return {
        records,
        baselineEnergy,
        optimizedEnergy,
        baselineTime,
        optimizedTime,
        energySaved,
        energySavedPct,
        timeChangePct
    };
}

function setSummary(data) {
    const safePct = Number.isFinite(data.energySavedPct) ? data.energySavedPct : 0;
    const timePct = Number.isFinite(data.timeChangePct) ? data.timeChangePct : 0;

    summary.innerHTML = "";
    const items = [
        ["Baseline Energy", `${data.baselineEnergy.toFixed(2)} J`],
        ["Optimized Energy", `${data.optimizedEnergy.toFixed(2)} J`],
        ["Energy Saved", `${data.energySaved.toFixed(2)} J (${safePct.toFixed(2)}%)`],
        ["Baseline Time", `${data.baselineTime.toFixed(2)} units`],
        ["Optimized Time", `${data.optimizedTime.toFixed(2)} units`],
        ["Performance Impact", `${timePct.toFixed(2)}% time change`]
    ];

    items.forEach(([label, value]) => {
        const box = document.createElement("div");
        box.className = "metric";
        box.innerHTML = `<p>${label}</p><strong>${value}</strong>`;
        summary.appendChild(box);
    });
}

function setTable(records) {
    resultBody.innerHTML = "";
    records.forEach((r, i) => {
        const tr = document.createElement("tr");
        tr.innerHTML = `
      <td>${i + 1}</td>
      <td>P${r.processId}</td>
      <td>${r.dvfsName}</td>
      <td>${r.frequency.toFixed(2)}</td>
      <td>${r.voltage.toFixed(2)}</td>
      <td>${r.execTime.toFixed(2)}</td>
      <td>${r.power.toFixed(2)}</td>
      <td>${r.energy.toFixed(2)}</td>
    `;
        resultBody.appendChild(tr);
    });
}

function drawBar(ctx, x, y, width, height, color, label, value) {
    ctx.fillStyle = color;
    ctx.fillRect(x, y - height, width, height);

    ctx.fillStyle = "#0f172a";
    ctx.font = "13px IBM Plex Mono";
    ctx.fillText(label, x, y + 18);
    ctx.fillText(value, x, y - height - 8);
}

function drawChart(data) {
    const ctx = canvas.getContext("2d");
    const w = canvas.width;
    const h = canvas.height;

    ctx.clearRect(0, 0, w, h);

    const bars = [
        { label: "Energy Baseline", value: data.baselineEnergy, color: "#ef4444" },
        { label: "Energy Optimized", value: data.optimizedEnergy, color: "#16a34a" },
        { label: "Time Baseline", value: data.baselineTime, color: "#3b82f6" },
        { label: "Time Optimized", value: data.optimizedTime, color: "#f59e0b" }
    ];

    const maxVal = Math.max(...bars.map((b) => b.value), 1);
    const chartBottom = h - 45;
    const left = 60;
    const gap = 36;
    const bw = 130;

    ctx.strokeStyle = "rgba(15, 23, 42, 0.45)";
    ctx.beginPath();
    ctx.moveTo(left - 18, 20);
    ctx.lineTo(left - 18, chartBottom);
    ctx.lineTo(w - 20, chartBottom);
    ctx.stroke();

    bars.forEach((b, i) => {
        const barHeight = (b.value / maxVal) * (chartBottom - 40);
        const x = left + i * (bw + gap);
        drawBar(ctx, x, chartBottom, bw, barHeight, b.color, b.label, b.value.toFixed(2));
    });

    ctx.fillStyle = "#0f172a";
    ctx.font = "bold 15px Space Grotesk";
    ctx.fillText("Comparison Graph: Energy and Performance", 20, 18);
}

runBtn.addEventListener("click", () => {
    try {
        const processes = readProcesses();
        const result = runSimulation(processes);
        setSummary(result);
        setTable(result.records);
        drawChart(result);
    } catch (error) {
        alert(error.message);
    }
});

addRowBtn.addEventListener("click", () => addProcessRow());

addProcessRow({ burst: 8, priority: 3, temperature: 65, basePower: 22 });
addProcessRow({ burst: 4, priority: 1, temperature: 60, basePower: 26 });
addProcessRow({ burst: 12, priority: 4, temperature: 75, basePower: 20 });
addProcessRow({ burst: 5, priority: 2, temperature: 68, basePower: 24 });
