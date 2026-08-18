async function checkStatus() {
  try {
    const response = await fetch('/status', { cache: 'no-store' });
    const result = await response.json();
    document.getElementById('status').textContent = result.message;

    if (result.state === 'failed') {
      document.querySelector('h1').textContent = 'Connection Failed';
      document.getElementById('retry').style.display = 'block';
    } else if (result.state === 'connected') {
      document.querySelector('h1').textContent = 'Connected';
    } else {
      setTimeout(checkStatus, 1000);
    }
  } catch (error) {
    setTimeout(checkStatus, 1000);
  }
}

setTimeout(checkStatus, 500);

