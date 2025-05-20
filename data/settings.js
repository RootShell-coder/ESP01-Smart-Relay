document.addEventListener('DOMContentLoaded', function () {
  updateDateTime();
  fetchToggleState();
  fetchRelayState();
  initLanguage();

  setInterval(updateDateTime, 60000);
  setInterval(fetchToggleState, 60000);
  setInterval(fetchRelayState, 60000);

  const sunPositionToggle = document.getElementById('sunposition');
  const sunInversionToggle = document.getElementById('suninvertion');

  document.getElementById('lat').addEventListener('input', checkCoordinatesAvailable);
  document.getElementById('lng').addEventListener('input', checkCoordinatesAvailable);

  loadSettings();

  document.getElementById('settingsForm').addEventListener('submit', saveSettings);

  document.getElementById('toggle').addEventListener('change', syncAllToggles);
  document.getElementById('sunposition').addEventListener('change', syncAllToggles);
  document.getElementById('suninvertion').addEventListener('change', syncAllToggles);
});

function loadSettings() {
  fetch('/api/getsettings')
    .then(response => response.json())
    .then(cfg => {
      const deviceName = (cfg.wifi && cfg.wifi.devname) || 'Устройство';
      document.title = deviceName;
      document.getElementById('device-name').textContent = deviceName;

      document.getElementById('dev').value = (cfg.wifi && cfg.wifi.devname) || '';
      document.getElementById('ssid').value = (cfg.wifi && cfg.wifi.ssid) || '';
      document.getElementById('password').value = (cfg.wifi && cfg.wifi.password) || '';
      document.getElementById('power').value = cfg.wifi && cfg.wifi.power || 0;
      document.getElementById('phy_mode').value = cfg.wifi && cfg.wifi.phy_mode || '';

      document.getElementById('ntp_server').value = (cfg.ntp && cfg.ntp.ntp_server) || '';
      document.getElementById('timezone').value = (cfg.ntp && cfg.ntp.ntp_timezone) || '';

      document.getElementById('lat').value = (cfg.location && cfg.location.lat) || '';
      document.getElementById('lng').value = (cfg.location && cfg.location.lng) || '';

      setTimeout(checkCoordinatesAvailable, 100);
    })
    .catch(error => console.error('Error loading settings:', error));
}

function saveSettings(e) {
  e.preventDefault();
  const saveBtn = this.querySelector('#save-settings-btn');
  saveBtn.disabled = true;
  saveBtn.textContent = 'Сохранение...';

  try {
    const form = this;
    const data = {
      ntp: {
        ntp_server: form.querySelector('[name="ntp_server"]').value.trim(),
        ntp_timezone: form.querySelector('[name="timezone"]').value.trim()
      },
      wifi: {
        devname: form.querySelector('[name="dev"]').value.trim(),
        ssid: form.querySelector('[name="ssid"]').value.trim(),
        password: form.querySelector('[name="password"]').value.trim(),
        power: parseInt(form.querySelector('[name="power"]').value, 10) || 0,
        phy_mode: form.querySelector('[name="phy_mode"]').value
      },
      location: {
        lat: parseFloat(form.querySelector('[name="lat"]').value) || 0,
        lng: parseFloat(form.querySelector('[name="lng"]').value) || 0
      }
    };

    fetch('/api/setsettings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data),
    })
    .then(response => response.text().then(text => ({
      ok: response.ok,
      status: response.status,
      text
    })))
    .then(result => {
      const msgDiv = document.getElementById('msg');
      msgDiv.style.display = 'block';

      if (result.ok) {
        msgDiv.textContent = result.text + ' Устройство перезагружается...';
        msgDiv.className = 'msg success';
        setTimeout(() => { msgDiv.style.display = 'none'; }, 5000);
      } else {
        msgDiv.textContent = 'Ошибка сохранения: ' + result.text + ' (Код: ' + result.status + ')';
        msgDiv.className = 'msg error';
      }
    })
    .catch(error => {
      const msgDiv = document.getElementById('msg');
      msgDiv.textContent = 'Сетевая ошибка при сохранении: ' + error;
      msgDiv.className = 'msg error';
      msgDiv.style.display = 'block';
    })
    .finally(() => {
      saveBtn.disabled = false;
      saveBtn.textContent = 'Сохранить настройки';
    });
  } catch (e) {
    console.error("Error during form processing:", e);
    alert("Произошла ошибка при обработке формы: " + e.message);
    saveBtn.disabled = false;
    saveBtn.textContent = 'Сохранить настройки';
  }
}

function checkCoordinatesAvailable() {
  const lat = document.getElementById('lat').value.trim();
  const lng = document.getElementById('lng').value.trim();
  const hasCoordinates = lat !== '' && lng !== '' && !isNaN(parseFloat(lat)) && !isNaN(parseFloat(lng));

  const sunPositionToggle = document.getElementById('sunposition');
  const sunInversionToggle = document.getElementById('suninvertion');

  sunPositionToggle.disabled = !hasCoordinates;
  sunInversionToggle.disabled = !hasCoordinates;

  if (!hasCoordinates) {
    sunPositionToggle.checked = false;
    sunInversionToggle.checked = false;

    fetch('/api/sun', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ state: 'off' })
    }).catch(err => console.error('Error updating sun settings:', err));

    document.querySelectorAll('.toggle-item').forEach(item => {
      const toggle = item.querySelector('input[type="checkbox"]');
      if (toggle === sunPositionToggle || toggle === sunInversionToggle) {
        item.classList.add('disabled-toggle');
        updateTooltipText(toggle, item, true);
      }
    });
  } else {
    document.querySelectorAll('.toggle-item').forEach(item => {
      const toggle = item.querySelector('input[type="checkbox"]');
      if (toggle === sunPositionToggle || toggle === sunInversionToggle) {
        item.classList.remove('disabled-toggle');
        updateTooltipText(toggle, item, false);
      }
    });
  }
}

function updateTooltipText(toggle, item, isDisabled) {
  const tooltip = item.querySelector('.tooltip .lang-text');
  const currentLang = localStorage.getItem('preferredLanguage') || 'en';

  if (isDisabled) {
    tooltip.setAttribute('data-text-en', 'Fill in latitude and longitude to enable');
    tooltip.setAttribute('data-text-ru', 'Заполните широту и долготу для активации');
  } else {
    if (toggle.id === 'sunposition') {
      tooltip.setAttribute('data-text-en', 'Turn on based on sun position (uses coordinates)');
      tooltip.setAttribute('data-text-ru', 'Включать в зависимости от положения солнца (использует координаты)');
    } else if (toggle.id === 'suninvertion') {
      tooltip.setAttribute('data-text-en', 'Inverts the sun position rule');
      tooltip.setAttribute('data-text-ru', 'Инвертирует правило включения по положению солнца');
    }
  }
  tooltip.textContent = tooltip.getAttribute(`data-text-${currentLang}`);
}

function fetchToggleState() {
  fetch('/api/gettoggle')
    .then(response => response.json())
    .then(data => {
      const toggleSwitch = document.getElementById('toggle');
      const sunPositionToggle = document.getElementById('sunposition');
      const sunInversionToggle = document.getElementById('suninvertion');

      if (toggleSwitch) toggleSwitch.checked = data.state === 'on';
      if (sunPositionToggle && !sunPositionToggle.disabled) {
        sunPositionToggle.checked = data.sunPosition === 'on';
      }
      if (sunInversionToggle && !sunInversionToggle.disabled) {
        sunInversionToggle.checked = data.sunInversion === 'on';
      }
    })
    .catch(error => console.error('Error fetching toggle state:', error));
}

function fetchRelayState() {
  fetch('/api/relaystate')
    .then(response => response.json())
    .then(data => {
      const relayIndicator = document.getElementById('relay-indicator');
      const relayState = document.getElementById('relay-state');

      if (relayIndicator && relayState) {
        const isOn = data.state === 'on';
        relayIndicator.className = 'relay-indicator ' + (isOn ? 'on' : 'off');
        const currentLang = localStorage.getItem('preferredLanguage') || 'en';
        relayState.className = 'lang-text ' + (isOn ? 'on' : 'off');
        relayState.setAttribute('data-text-en', isOn ? 'ON' : 'OFF');
        relayState.setAttribute('data-text-ru', isOn ? 'ВКЛ' : 'ВЫКЛ');
        relayState.textContent = relayState.getAttribute(`data-text-${currentLang}`);
      }
    })
    .catch(error => console.error('Error fetching relay state:', error));
}

function updateDateTime() {
  fetch('/api/datetime')
    .then(response => response.json())
    .then(data => {
      const datetimeElement = document.getElementById('current-datetime');
      if (data.synced) {
        datetimeElement.textContent = data.datetime;
        datetimeElement.classList.remove('not-synced');
      } else {
        datetimeElement.textContent = data.datetime + ' (не синхр.)';
        datetimeElement.classList.add('not-synced');
      }
    })
    .catch(error => {
      console.error("Ошибка получения времени:", error);
      document.getElementById('current-datetime').textContent = "ошибка";
    });
}

function initLanguage() {
  const currentLang = localStorage.getItem('preferredLanguage') || 'en';
  setLanguage(currentLang);

  const langToggle = document.getElementById('lang-toggle');
  langToggle.checked = currentLang === 'ru';

  langToggle.addEventListener('change', function() {
    const lang = this.checked ? 'ru' : 'en';
    setLanguage(lang);
    localStorage.setItem('preferredLanguage', lang);
  });
}

function setLanguage(lang) {
  document.documentElement.lang = lang;
  document.querySelectorAll('.lang-text').forEach(element => {
    const text = element.getAttribute(`data-text-${lang}`);
    if (text) element.textContent = text;
  });
}

function syncAllToggles() {
  const toggleState = document.getElementById('toggle').checked ? 'on' : 'off';
  const sunPositionState = document.getElementById('sunposition').checked ? 'on' : 'off';
  const sunInversionState = document.getElementById('suninvertion').checked ? 'on' : 'off';

  const relayIndicator = document.getElementById('relay-indicator');
  const relayState = document.getElementById('relay-state');

  if (relayIndicator && relayState) {
    const isOn = toggleState === 'on';
    relayIndicator.className = 'relay-indicator ' + (isOn ? 'on' : 'off');
    const currentLang = localStorage.getItem('preferredLanguage') || 'en';
    relayState.className = 'lang-text ' + (isOn ? 'on' : 'off');
    relayState.setAttribute('data-text-en', isOn ? 'ON' : 'OFF');
    relayState.setAttribute('data-text-ru', isOn ? 'ВКЛ' : 'ВЫКЛ');
    relayState.textContent = relayState.getAttribute(`data-text-${currentLang}`);
  }

  const toggleData = {
    state: toggleState,
    sunPosition: sunPositionState,
    sunInversion: sunInversionState
  };

  fetch('/api/toggle', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(toggleData)
  })
  .then(() => {
    fetchRelayState();
  })
  .catch(err => console.error('Error updating toggle states:', err));
}
