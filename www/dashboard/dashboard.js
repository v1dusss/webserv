const HISTORY_LENGTH = 12; // e.g. last 1 minute @ 5s interval
let labels = Array(HISTORY_LENGTH).fill('').map((_, i) => `${-((HISTORY_LENGTH - i - 1) * 5)}s`);
  
// initial empty datasets
const cpuData = { labels, datasets: [{ label: 'Load', data: Array(HISTORY_LENGTH).fill(0), borderColor: '#3b82f6', backgroundColor: 'rgba(59,130,246,0.2)', fill: true }] };
const memData = { labels, datasets: [{ label: 'Used %', data: Array(HISTORY_LENGTH).fill(0), borderColor: '#10b981', backgroundColor: 'rgba(16,185,129,0.2)', fill: true }] };
const diskData= { labels, datasets: [{ label: 'Used %', data: Array(HISTORY_LENGTH).fill(0), borderColor: '#f59e0b', backgroundColor: 'rgba(245,158,11,0.2)', fill: true }] };

// create charts
function makeChart(ctx, data, yLabel) {
  return new Chart(ctx, {
    type: 'line',
    data,
    options: {
      responsive: true,
      animation: false,
      scales: {
        x: { display: false },
        y: {
          beginAtZero: true,
          ticks: { callback: v => yLabel === '%' ? v.toFixed(0) + '%' : v.toFixed(2) }
        }
      },
      plugins: { legend: { display: false } }
    }
  });
}

const cpuChart  = makeChart(document.getElementById('cpuChart').getContext('2d'), cpuData, '');
const memChart  = makeChart(document.getElementById('memChart').getContext('2d'), memData, '%');
const diskChart = makeChart(document.getElementById('diskChart').getContext('2d'), diskData, '%');

async function fetchMetrics() {
  try {
    const res  = await fetch('/cgi/metrics.py');
    const json = await res.json();

    // shift old, push new
    cpuChart.data.datasets[0].data.shift();
    cpuChart.data.datasets[0].data.push(json.cpu_load);

    const memPct  = (json.memory.used / json.memory.total) * 100;
    memChart.data.datasets[0].data.shift();
    memChart.data.datasets[0].data.push(memPct);

    const diskPct = (json.disk.used   / json.disk.total) * 100;
    diskChart.data.datasets[0].data.shift();
    diskChart.data.datasets[0].data.push(diskPct);

    // rotate labels so newest is "0s"
    const newLabels = cpuChart.data.labels.slice(1).concat('0s');
    cpuChart.data.labels = newLabels;
    memChart.data.labels = newLabels;
    diskChart.data.labels= newLabels;

    cpuChart.update();
    memChart.update();
    diskChart.update();
  } catch (err) {
    console.error('Failed to fetch metrics:', err);
  }
}

// initial fetch + interval (5 seconds)
fetchMetrics();
setInterval(fetchMetrics, 5000);