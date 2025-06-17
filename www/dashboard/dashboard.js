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

const chartOptions = {
    type: 'line',
    options: {
        responsive: true,
        animation: false,
        scales: {
            x: {
                display: false
            },
            y: {
                beginAtZero: true
            }
        },
        plugins: {
            legend: {
                display: false
            }
        }
    }
};

// Create chart for connection count
const connectionCountCtx = document.getElementById('connectionCountChart').getContext('2d');
const connectionCountChart = new Chart(connectionCountCtx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'Connections',
            data: Array(HISTORY_LENGTH).fill(0),
            borderColor: '#4F46E5',
            backgroundColor: 'rgba(79,70,229,0.2)',
            fill: true
        }]
    },
    options: chartOptions.options
});

// Create chart for bytes received
const bytesReceivedCtx = document.getElementById('bytesReceivedChart').getContext('2d');
const bytesReceivedChart = new Chart(bytesReceivedCtx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'Bytes Received',
            data: Array(HISTORY_LENGTH).fill(0),
            borderColor: '#10B981',
            backgroundColor: 'rgba(16,185,129,0.2)',
            fill: true
        }]
    },
    options: chartOptions.options
});

// Create chart for bytes sent
const bytesSentCtx = document.getElementById('bytesSentChart').getContext('2d');
const bytesSentChart = new Chart(bytesSentCtx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'Bytes Sent',
            data: Array(HISTORY_LENGTH).fill(0),
            borderColor: '#F59E0B',
            backgroundColor: 'rgba(245,158,11,0.2)',
            fill: true
        }]
    },
    options: chartOptions.options
});

// Create chart for disconnects
const disconnectsCtx = document.getElementById('disconnectsChart').getContext('2d');
const disconnectsChart = new Chart(disconnectsCtx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'Disconnects',
            data: Array(HISTORY_LENGTH).fill(0),
            borderColor: '#EF4444',
            backgroundColor: 'rgba(239,68,68,0.2)',
            fill: true
        }]
    },
    options: chartOptions.options
});

// Create chart for new connections
const newConnectionsCtx = document.getElementById('newConnectionsChart').getContext('2d');
const newConnectionsChart = new Chart(newConnectionsCtx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'New Connections',
            data: Array(HISTORY_LENGTH).fill(0),
            borderColor: '#34D399',
            backgroundColor: 'rgba(52,211,153,0.2)',
            fill: true
        }]
    },
    options: chartOptions.options
});

// Create chart for requests
const requestsCtx = document.getElementById('requestsChart').getContext('2d');
const requestsChart = new Chart(requestsCtx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'Requests',
            data: Array(HISTORY_LENGTH).fill(0),
            borderColor: '#6366F1',
            backgroundColor: 'rgba(99,102,241,0.2)',
            fill: true
        }]
    },
    options: chartOptions.options
});

// Create chart for responses
const responsesCtx = document.getElementById('responsesChart').getContext('2d');
const responsesChart = new Chart(responsesCtx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'Responses',
            data: Array(HISTORY_LENGTH).fill(0),
            borderColor: '#06B6D4',
            backgroundColor: 'rgba(6,182,212,0.2)',
            fill: true
        }]
    },
    options: chartOptions.options
});

// Create chart for uptime
const uptimeCtx = document.getElementById('uptimeChart').getContext('2d');
const uptimeChart = new Chart(uptimeCtx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'Uptime',
            data: Array(HISTORY_LENGTH).fill(0),
            borderColor: '#A855F7',
            backgroundColor: 'rgba(168,85,247,0.2)',
            fill: true
        }]
    },
    options: chartOptions.options
});

async function fetchMetrics() {
  try {
    const [res1, res2] = await Promise.all([
      fetch('/cgi/metrics.py'),
      fetch('http://api.localhost:8080/metrics')
    ]);

    const [json1, json2] = await Promise.all([
      res1.json(),
      res2.json()
    ]);

    // shift old, push new
    cpuChart.data.datasets[0].data.shift();
    cpuChart.data.datasets[0].data.push(json1.cpu_load);

    const memPct  = (json1.memory.used / json1.memory.total) * 100;
    memChart.data.datasets[0].data.shift();
    memChart.data.datasets[0].data.push(memPct);

    const diskPct = (json1.disk.used   / json1.disk.total) * 100;
    diskChart.data.datasets[0].data.shift();
    diskChart.data.datasets[0].data.push(diskPct);

    // Update webserv metrics charts
    connectionCountChart.data.datasets[0].data.shift();
    connectionCountChart.data.datasets[0].data.push(json2.connection_count);

    bytesReceivedChart.data.datasets[0].data.shift();
    bytesReceivedChart.data.datasets[0].data.push(json2.bytes_received);

    bytesSentChart.data.datasets[0].data.shift();
    bytesSentChart.data.datasets[0].data.push(json2.bytes_send);

    disconnectsChart.data.datasets[0].data.shift();
    disconnectsChart.data.datasets[0].data.push(json2.disconnects);

    newConnectionsChart.data.datasets[0].data.shift();
    newConnectionsChart.data.datasets[0].data.push(json2.new_connections);

    requestsChart.data.datasets[0].data.shift();
    requestsChart.data.datasets[0].data.push(json2.requests);

    responsesChart.data.datasets[0].data.shift();
    responsesChart.data.datasets[0].data.push(json2.responses);

    uptimeChart.data.datasets[0].data.shift();
    uptimeChart.data.datasets[0].data.push(json2.uptime);

    // rotate labels so newest is "0s"
    const newLabels = cpuChart.data.labels.slice(1).concat('0s');
    cpuChart.data.labels = newLabels;
    memChart.data.labels = newLabels;
    diskChart.data.labels = newLabels;
    connectionCountChart.data.labels = newLabels;
    bytesReceivedChart.data.labels = newLabels;
    bytesSentChart.data.labels = newLabels;
    disconnectsChart.data.labels = newLabels;
    newConnectionsChart.data.labels = newLabels;
    requestsChart.data.labels = newLabels;
    responsesChart.data.labels = newLabels;
    uptimeChart.data.labels = newLabels;

    cpuChart.update();
    memChart.update();
    diskChart.update();
    connectionCountChart.update();
    bytesReceivedChart.update();
    bytesSentChart.update();
    disconnectsChart.update();
    newConnectionsChart.update();
    requestsChart.update();
    responsesChart.update();
    uptimeChart.update();

  } catch (err) {
    console.error('Failed to fetch metrics:', err);
  }
}

// initial fetch + interval (5 seconds)
fetchMetrics();
setInterval(fetchMetrics, 5000);