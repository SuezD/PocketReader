const input = document.getElementById('ssid');
const list = document.getElementById('networks');
const status = document.getElementById('scan-status');
const wifiSection = document.getElementById('wifi-section');
const wifiSummary = document.getElementById('wifi-summary');
let networks = [];

function setSectionStatus(element, kind, text) {
  element.className = `section-status${kind ? ` ${kind}` : ''}`;
  element.textContent = text;
}

async function loadWifiStatus() {
  try {
    const response = await fetch('/status', { cache: 'no-store' });
    const result = await response.json();
    if (result.state === 'connected') {
      const name = result.network ? `Connected: ${result.network}` : 'Connected';
      setSectionStatus(wifiSummary, 'success', name);
      wifiSection.open = false;
    } else {
      setSectionStatus(wifiSummary, '', 'Not connected');
      wifiSection.open = true;
    }
  } catch (error) {
    setSectionStatus(wifiSummary, 'failure', 'Status unavailable');
  }
}

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
loadWifiStatus();

const bookServerForm = document.getElementById('book-server-form');
const bookServerSection = document.getElementById('book-server-section');
const bookServerSummary = document.getElementById('book-server-summary');
const manifestUrl = document.getElementById('manifest-url');
const accessToken = document.getElementById('access-token');
const bookServerStatus = document.getElementById('book-server-status');
const testBookServer = document.getElementById('test-book-server');

async function loadBookServer() {
  try {
    const response = await fetch('/book-server', { cache: 'no-store' });
    const result = await response.json();
    manifestUrl.value = result.manifestUrl || '';
    setSectionStatus(
      bookServerSummary,
      result.manifestUrl ? 'configured' : '',
      result.manifestUrl ? 'Configured' : 'Not configured'
    );
    bookServerSection.open = true;
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
    setSectionStatus(bookServerSummary, 'configured', 'Configured');
    bookServerStatus.textContent = 'Saved. Return to the reader.';
  } catch (error) {
    bookServerStatus.textContent = error.message || 'Could not save the book server.';
  }
});

testBookServer.addEventListener('click', async () => {
  bookServerStatus.textContent = 'Testing connection...';
  testBookServer.disabled = true;

  try {
    const response = await fetch('/book-server/test', { method: 'POST' });
    const result = await response.json();
    if (!response.ok || !result.success) {
      throw new Error(result.message || 'Connection failed');
    }
    setSectionStatus(bookServerSummary, 'success', 'Connected');
    bookServerStatus.textContent = 'Connection successful.';
    bookServerSection.open = false;
  } catch (error) {
    setSectionStatus(bookServerSummary, 'failure', 'Connection failed');
    bookServerStatus.textContent = error.message || 'Connection failed.';
    bookServerSection.open = true;
  } finally {
    testBookServer.disabled = false;
  }
});

loadBookServer();
