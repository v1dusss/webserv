async function fetchMetrics() {
  try {
    const res = await fetch('/dashboard/metrics');
    const data = await res.json();
    document.getElementById('cpu').textContent =
      data.cpu_load.toFixed(2);
    const memPct = (data.memory.used / data.memory.total) * 100;
    document.getElementById('mem').textContent =
      `${(memPct).toFixed(1)}% (${(data.memory.used/1e9).toFixed(2)} GB)`;
    const diskPct = (data.disk.used / data.disk.total) * 100;
    document.getElementById('disk').textContent =
      `${diskPct.toFixed(1)}% (${(data.disk.used/1e9).toFixed(2)} GB)`;
  } catch(e) {
    console.error('dashboard error', e);
  }
}

// refresh every 5 seconds
fetchMetrics();
setInterval(fetchMetrics, 5000);