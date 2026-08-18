const input = document.getElementById('ssid');
const list = document.getElementById('networks');
const status = document.getElementById('scan-status');
let networks = [];

function renderNetworks() {
  const filter = input.value.toLocaleLowerCase();
  const matches = networks.filter((network) =>
    network.ssid.toLocaleLowerCase().includes(filter)
  );

  list.replaceChildren();

  if (matches.length === 0) {
    const empty = document.createElement('div');
    empty.className = 'empty';
    empty.textContent = networks.length === 0
      ? 'No nearby networks found'
      : 'No matching networks';
    list.appendChild(empty);
    return;
  }

  matches.forEach((network) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'network';

    const name = document.createElement('span');
    name.textContent = network.ssid;

    const details = document.createElement('small');
    details.textContent = `${network.rssi} dBm · ${network.secure ? 'Secured' : 'Open'}`;

    button.append(name, details);
    button.addEventListener('click', () => {
      input.value = network.ssid;
      renderNetworks();
    });
    list.appendChild(button);
  });
}

async function loadNetworks() {
  try {
    const response = await fetch('/networks', { cache: 'no-store' });
    const result = await response.json();

    if (result.scanning) {
      status.textContent = 'Scanning for nearby networks...';
      setTimeout(loadNetworks, 1000);
      return;
    }

    networks = result.networks;
    renderNetworks();
    status.textContent = `${networks.length} network(s) found. You can also type a hidden network name.`;
  } catch (error) {
    status.textContent = 'Could not load nearby networks.';
  }
}

document.getElementById('scan').addEventListener('click', async () => {
  status.textContent = 'Scanning for nearby networks...';
  await fetch('/rescan', { method: 'POST' });
  setTimeout(loadNetworks, 500);
});

input.addEventListener('input', renderNetworks);
loadNetworks();

