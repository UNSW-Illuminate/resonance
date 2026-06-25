#ifndef WEB_UI_H
#define WEB_UI_H

const char WEB_UI_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Resonance Simulator - ESP32</title>
    <style>
        body { font-family: sans-serif; margin: 20px; background: #f0f0f0; color: #333; }
        .card { background: white; padding: 20px; margin-bottom: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h2 { margin-top: 0; }
        label { display: inline-block; width: 150px; margin-bottom: 10px; font-weight: bold; }
        input[type="range"], input[type="number"], select { width: 200px; padding: 5px; margin-bottom: 10px; }
        button { padding: 10px 20px; background: #0078d4; color: white; border: none; border-radius: 4px; cursor: pointer; }
        button:hover { background: #005a9e; }
        textarea { width: 95%; height: 150px; font-family: monospace; padding: 8px; border: 1px solid #ccc; border-radius: 4px; }
        .flex { display: flex; gap: 20px; flex-wrap: wrap; }
        .flex-child { flex: 1; min-width: 300px; }
        .status-badge { display: inline-block; padding: 4px 8px; border-radius: 4px; font-size: 12px; font-weight: bold; }
        .badge-active { background: #d4edda; color: #155724; }
        .badge-inactive { background: #f8d7da; color: #721c24; }
    </style>
</head>
<body>
    <h1>Resonance Simulator ESP32 Controller</h1>
    
    <div class="flex">
        <!-- Settings Column -->
        <div class="flex-child">
            <div class="card">
                <h2>Control Settings</h2>
                <form id="settings-form">
                    <div id="div-enabled">
                        <label id="lbl-enabled" for="enabled">Enable Lighting:</label>
                        <input type="checkbox" id="enabled" name="enabled" style="width: auto; margin-bottom: 10px;">
                    </div>
                    <div id="div-mode">
                        <label id="lbl-mode" for="mode">Mode:</label>
                        <select id="mode" name="mode">
                            <option value="ripple">Ripple Simulation</option>
                            <option value="static">Solid Color</option>
                            <option value="rainbow">Rainbow Demo</option>
                            <option value="palette_ocean">Ocean Palette</option>
                            <option value="palette_lava">Lava Palette</option>
                            <option value="palette_forest">Forest Palette</option>
                            <option value="palette_party">Party Palette</option>
                            <option value="palette_cloud">Cloud Palette</option>
                            <option value="glitter">Glitter Sparkle</option>
                            <option value="off">Off</option>
                        </select>
                    </div>
                    <div id="div-autoRipple">
                        <label id="lbl-autoRipple" for="autoRipple">Auto Ripple:</label>
                        <input type="checkbox" id="autoRipple" name="autoRipple" style="width: auto; margin-bottom: 10px;">
                    </div>
                    <div id="div-autoRippleVariable">
                        <label id="lbl-autoRippleVariable" for="autoRippleVariable">Variable Interval:</label>
                        <input type="checkbox" id="autoRippleVariable" name="autoRippleVariable" style="width: auto; margin-bottom: 10px;">
                    </div>
                    <div id="div-autoRippleInterval">
                        <label id="lbl-autoRippleInterval" for="autoRippleInterval">Auto Ripple Interval (ms):</label>
                        <input type="range" id="autoRippleInterval" name="autoRippleInterval" min="1000" max="10000" step="500" value="5000">
                        <span id="val-autoRippleInterval">5000</span>
                    </div>
                    <div id="div-brightness">
                        <label id="lbl-brightness" for="brightness">Global Brightness:</label>
                        <input type="range" id="brightness" name="brightness" min="0" max="255" value="128">
                        <span id="val-brightness">128</span>
                    </div>
                    <div id="div-primary">
                        <label id="lbl-primary" for="primary">Primary Color:</label>
                        <input type="color" id="primary" name="primary" value="#ff0000">
                    </div>
                    <div id="div-secondary">
                        <label id="lbl-secondary" for="secondary">Secondary Color:</label>
                        <input type="color" id="secondary" name="secondary" value="#000000">
                    </div>
                    <div id="div-reedColor">
                        <label id="lbl-reedColor" for="reedColor">Reed Color:</label>
                        <input type="color" id="reedColor" name="reedColor" value="#0000ff">
                    </div>
                    <div id="div-speed">
                        <label id="lbl-speed" for="speed">Wave Speed:</label>
                        <input type="range" id="speed" name="speed" min="1" max="255" step="1" value="50">
                        <span id="val-speed">50</span>
                    </div>
                    <div id="div-rippleSize">
                        <label id="lbl-rippleSize" for="rippleSize">Ripple Size:</label>
                        <input type="range" id="rippleSize" name="rippleSize" min="1" max="255" step="1" value="32">
                        <span id="val-rippleSize">32</span>
                    </div>
                    <button type="submit">Save Settings</button>
                </form>
            </div>
            
            <div class="card">
                <h2>System Status</h2>
                <div id="wled-status">Checking WLED status...</div>
            </div>

            <div class="card">
                <h2>Manual Node Trigger</h2>
                <div style="display:flex; align-items:center; gap:10px;">
                    <label for="manual-node" style="width:auto; margin:0;">Node ID:</label>
                    <input type="number" id="manual-node" min="0" max="9999" placeholder="e.g. 10" style="margin:0; width:100px;">
                    <button id="btn-trigger-node">Trigger Ripple</button>
                </div>
            </div>
        </div>

        <!-- Node Map Column -->
        <div class="flex-child">
            <div class="card">
                <h2>Manage Map Layout</h2>
                <div id="map-stats">Loading map stats...</div>
                <div style="margin-top: 10px;">
                    <label style="display:block; margin-bottom:5px;">Paste JSON Map:</label>
                    <textarea id="map-json-input" placeholder='{"bushes": [...], "reeds": [...]}'></textarea>
                </div>
                <button id="save-map-btn" style="margin-top: 10px;">Save Map to ESP32</button>
            </div>

            <div class="card">
                <h2>System Logs</h2>
                <div id="ui-logs" style="background:#1e1e1e; color:#4caf50; font-family:monospace; font-size:12px; height:200px; overflow-y:auto; padding:10px; border-radius:4px; line-height:1.4;">
                    [System] UI Initialized. Waiting for events...<br>
                </div>
            </div>
        </div>
    </div>

    <script>
        // Update slider value labels dynamically
        ['brightness', 'speed', 'rippleSize', 'autoRippleInterval'].forEach(id => {
            const input = document.getElementById(id);
            const val = document.getElementById('val-' + id);
            input.addEventListener('input', () => { val.textContent = input.value; });
        });

        // Convert HEX to RGB
        function hexToRgb(hex) {
            const r = parseInt(hex.slice(1, 3), 16);
            const g = parseInt(hex.slice(3, 5), 16);
            const b = parseInt(hex.slice(5, 7), 16);
            return [r, g, b];
        }

        // Logging helper
        function logMessage(msg) {
            const logs = document.getElementById('ui-logs');
            const time = new Date().toLocaleTimeString([], { hour12: false });
            logs.innerHTML += `[${time}] ${msg}<br>`;
            // Auto-scroll to bottom
            logs.scrollTop = logs.scrollHeight;
        }

        // Initialize Server-Sent Events (SSE) for ESP32 backend logs
        const evtSource = new EventSource("/events");
        evtSource.addEventListener("log", (e) => {
            logMessage(`ESP32: ${e.data}`);
        });
        evtSource.onerror = () => {
            // Connection will auto-retry
        };

        // Convert RGB to HEX
        function rgbToHex(r, g, b) {
            return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
        }

        // Update UI dynamic visibility and label naming for current mode
        function updateUIForMode(mode) {
            const setVisibility = (id, visible) => {
                document.getElementById('div-' + id).style.display = visible ? 'block' : 'none';
            };
            const setLabel = (id, text) => {
                document.getElementById('lbl-' + id).textContent = text;
            };

            // Optionals
            const optionals = ['primary', 'secondary', 'reedColor', 'speed', 'rippleSize', 'autoRipple', 'autoRippleVariable', 'autoRippleInterval'];
            optionals.forEach(opt => setVisibility(opt, false));

            if (mode === 'ripple') {
                optionals.forEach(opt => setVisibility(opt, true));
                setLabel('primary', 'Primary Color:');
                setLabel('secondary', 'Secondary Color:');
                setLabel('reedColor', 'Reed Trigger Color:');
                setLabel('speed', 'Wave Speed:');
                setLabel('rippleSize', 'Ripple Size:');
            } else if (mode === 'static') {
                setVisibility('primary', true);
                setLabel('primary', 'Solid Color:');
            } else if (mode === 'rainbow' || mode.startsWith('palette_')) {
                setVisibility('speed', true);
                setLabel('speed', 'Scroll Speed:');
            } else if (mode === 'glitter') {
                setVisibility('primary', true);
                setVisibility('secondary', true);
                setVisibility('speed', true);
                setVisibility('rippleSize', true);

                setLabel('primary', 'Background Color:');
                setLabel('secondary', 'Glitter Color:');
                setLabel('speed', 'Glitter Intensity:');
                setLabel('rippleSize', 'Glitter Size:');
            } else if (mode === 'off') {
                // Keep everything hidden
            }
        }

        // Handle dropdown mode change
        document.getElementById('mode').addEventListener('change', (e) => {
            updateUIForMode(e.target.value);
        });

        let isFormLoaded = false;

        // Fetch settings and map
        async function loadStatus() {
            try {
                const response = await fetch('/api/state');
                const data = await response.json();
                
                // Only set settings form values on initial load to prevent resetting user input
                if (!isFormLoaded) {
                    document.getElementById('enabled').checked = data.enabled;
                    document.getElementById('mode').value = data.mode;
                    updateUIForMode(data.mode);

                    document.getElementById('brightness').value = data.brightness;
                    document.getElementById('val-brightness').textContent = data.brightness;
                    document.getElementById('primary').value = rgbToHex(data.primaryColor[0], data.primaryColor[1], data.primaryColor[2]);
                    document.getElementById('secondary').value = rgbToHex(data.secondaryColor[0], data.secondaryColor[1], data.secondaryColor[2]);
                    if (data.reedColor) {
                        document.getElementById('reedColor').value = rgbToHex(data.reedColor[0], data.reedColor[1], data.reedColor[2]);
                    }
                    document.getElementById('speed').value = data.speed;
                    document.getElementById('val-speed').textContent = data.speed;
                    document.getElementById('rippleSize').value = data.rippleSize;
                    document.getElementById('val-rippleSize').textContent = data.rippleSize;
                    
                    if (data.autoRipple !== undefined) {
                        document.getElementById('autoRipple').checked = data.autoRipple;
                        document.getElementById('autoRippleVariable').checked = data.autoRippleVariable;
                        document.getElementById('autoRippleInterval').value = data.autoRippleInterval;
                        document.getElementById('val-autoRippleInterval').textContent = data.autoRippleInterval;
                    }

                    isFormLoaded = true;
                    logMessage("UI: Loaded initial configuration from ESP32");
                }
            } catch (err) {
                console.error("Failed to load status", err);
            }

            try {
                const wledResponse = await fetch('/api/wled/status');
                const wled = await wledResponse.json();
                const statusDiv = document.getElementById('wled-status');
                if (!wled.enabled) {
                    statusDiv.innerHTML = "WLED Integration Disabled";
                } else {
                    const badge = wled.connected ? 
                        '<span class="status-badge badge-active">Connected</span>' : 
                        '<span class="status-badge badge-inactive">Disconnected</span>';
                    statusDiv.innerHTML = `
                        Status: ${badge}<br>
                        Host: ${wled.host}<br>
                        Last Error: ${wled.lastError || 'None'}
                    `;
                }
            } catch (e) { }
        }

        let allNodes = [];
        async function fetchMapJson() {
            try {
                const res = await fetch('/graph.json');
                const json = await res.json();
                document.getElementById('map-json-input').value = JSON.stringify(json, null, 2);
                parseAndUpdateStats(json);
                logMessage("UI: Map loaded successfully from ESP32");
            } catch (err) {
                console.error("Failed to load map JSON", err);
                document.getElementById('map-stats').innerHTML = "No map loaded. Please paste your JSON below and click Save.";
                logMessage("UI: No map found on ESP32, waiting for upload");
            }
        }

        function parseAndUpdateStats(json) {
            let bushesCount = 0;
            let reedsCount = 0;
            
            if (json.bushes) {
                bushesCount = json.bushes.length;
            }
            if (json.reeds) {
                reedsCount = json.reeds.length;
            }
            
            const totalNodes = bushesCount + reedsCount;
            document.getElementById('map-stats').innerHTML = `
                <strong>Nodes count:</strong> ${totalNodes} (mapped to ${totalNodes * 9} LEDs)<br>
                <strong>Bushes count:</strong> ${bushesCount}<br>
                <strong>Reeds count:</strong> ${reedsCount}
            `;
        }

        // Live Preview for Textarea
        document.getElementById('map-json-input').addEventListener('input', (e) => {
            try {
                const json = JSON.parse(e.target.value);
                parseAndUpdateStats(json);
            } catch (err) {
                // Ignore parse errors while typing
            }
        });

        // Save Map Button
        document.getElementById('save-map-btn').addEventListener('click', async () => {
            const btn = document.getElementById('save-map-btn');
            const jsonText = document.getElementById('map-json-input').value;
            
            try {
                // Validate JSON before sending
                JSON.parse(jsonText);
                
                btn.disabled = true;
                btn.textContent = "Saving...";
                logMessage("UI: Uploading new map to ESP32...");
                
                const res = await fetch('/api/map', {
                    method: 'POST',
                    body: jsonText
                });
                
                if (res.ok) {
                    alert("Map saved successfully!");
                    logMessage("UI: Map uploaded successfully");
                } else {
                    alert("Failed to save map to ESP32.");
                    logMessage("UI: Error - Failed to upload map");
                }
            } catch (err) {
                alert("Invalid JSON map format.");
                logMessage("UI: Error - Invalid map JSON format");
            } finally {
                btn.disabled = false;
                btn.textContent = "Save Map to ESP32";
            }
        });

        // Save Settings
        document.getElementById('settings-form').addEventListener('submit', async (e) => {
            e.preventDefault();
            const mode = document.getElementById('mode').value;
            const enabled = document.getElementById('enabled').checked;
            const brightness = parseInt(document.getElementById('brightness').value);
            const primary = hexToRgb(document.getElementById('primary').value);
            const secondary = hexToRgb(document.getElementById('secondary').value);
            const reedColor = hexToRgb(document.getElementById('reedColor').value);
            const speed = parseInt(document.getElementById('speed').value);
            const rippleSize = parseInt(document.getElementById('rippleSize').value);
            const autoRipple = document.getElementById('autoRipple').checked;
            const autoRippleVariable = document.getElementById('autoRippleVariable').checked;
            const autoRippleInterval = parseInt(document.getElementById('autoRippleInterval').value);

            const payload = {
                enabled, mode, brightness,
                primaryColor: [primary[0], primary[1], primary[2]],
                secondaryColor: [secondary[0], secondary[1], secondary[2]],
                reedColor: [reedColor[0], reedColor[1], reedColor[2]],
                speed, rippleSize,
                autoRipple, autoRippleVariable, autoRippleInterval
            };

            logMessage("UI: Sending updated settings to ESP32...");

            try {
                const res = await fetch('/api/state', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                });
                if (res.ok) {
                    alert("Settings saved successfully!");
                    loadStatus();
                } else {
                    alert("Failed to save settings.");
                }
            } catch (err) {
                console.error("Save failed", err);
                alert("Failed to reach ESP32.");
                logMessage("UI: Error - Failed to reach ESP32");
            }
        });

        // Manual Node Trigger
        document.getElementById('btn-trigger-node').addEventListener('click', async () => {
            const nodeId = parseInt(document.getElementById('manual-node').value);
            if (isNaN(nodeId) || nodeId < 0) {
                alert("Please enter a valid positive Node ID");
                return;
            }

            logMessage(`UI: Sending manual trigger for Node ${nodeId} to ESP32...`);
            try {
                const res = await fetch(`/api/triggerNode?id=${nodeId}`, { method: 'POST' });
                if (res.ok) {
                    logMessage(`UI: Trigger sent successfully`);
                } else {
                    logMessage(`UI: Error - Failed to send trigger`);
                    alert("Failed to send trigger.");
                }
            } catch (err) {
                console.error("Trigger failed", err);
                logMessage("UI: Error - Network failure");
            }
        });

        // Initial setup
        loadStatus();
        fetchMapJson();
        // Periodically refresh stats
        setInterval(loadStatus, 3000);
    </script>
</body>
</html>
)rawhtml";

#endif // WEB_UI_H
