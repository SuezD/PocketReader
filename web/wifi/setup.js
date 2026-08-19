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

const bookServerForm = document.getElementById('book-server-form');
const manifestUrl = document.getElementById('manifest-url');
const accessToken = document.getElementById('access-token');
const bookServerStatus = document.getElementById('book-server-status');

async function loadBookServer() {
  try {
    const response = await fetch('/book-server', { cache: 'no-store' });
    const result = await response.json();
    manifestUrl.value = result.manifestUrl || '';
    accessToken.placeholder = result.hasAccessToken
      ? 'Enter the token again to keep it'
      : 'Leave blank for no access token';
  } catch (error) {
    bookServerStatus.textContent = 'Could not load the saved book server.';
  }
}

bookServerForm.addEventListener('submit', async (event) => {
  event.preventDefault();
  bookServerStatus.textContent = 'Saving...';

  try {
    const body = new URLSearchParams({
      manifestUrl: manifestUrl.value,
      accessToken: accessToken.value
    });
    const response = await fetch('/book-server', { method: 'POST', body });
    if (!response.ok) {
      throw new Error(await response.text());
    }
    const tokenWasSaved = accessToken.value.length > 0;
    accessToken.value = '';
    accessToken.placeholder = tokenWasSaved
      ? 'Enter the token again to keep it'
      : 'Leave blank for no access token';
    bookServerStatus.textContent = 'Saved. Return to the reader.';
  } catch (error) {
    bookServerStatus.textContent = error.message || 'Could not save the book server.';
  }
});

loadBookServer();
