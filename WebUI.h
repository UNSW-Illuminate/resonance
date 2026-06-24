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
                    <div id="div-mode">
                        <label id="lbl-mode" for="mode">Mode:</label>
                        <select id="mode" name="mode">
                            <option value="ripple">Ripple Simulation</option>
                            <option value="solid">Solid Color</option>
                            <option value="rainbow">Rainbow Demo</option>
                            <option value="glitter">Glitter Sparkle</option>
                            <option value="breathing">Breathing Wave</option>
                            <option value="off">Off</option>
                        </select>
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
                    <div id="div-speed">
                        <label id="lbl-speed" for="speed">Wave Speed:</label>
                        <input type="range" id="speed" name="speed" min="0.1" max="5.0" step="0.1" value="1.0">
                        <span id="val-speed">1.0</span>
                    </div>
                    <div id="div-fadeSpeed">
                        <label id="lbl-fadeSpeed" for="fadeSpeed">Fade Speed:</label>
                        <input type="range" id="fadeSpeed" name="fadeSpeed" min="0.1" max="5.0" step="0.1" value="1.0">
                        <span id="val-fadeSpeed">1.0</span>
                    </div>
                    <div id="div-sharpness">
                        <label id="lbl-sharpness" for="sharpness">Color Sharpness:</label>
                        <input type="range" id="sharpness" name="sharpness" min="1.0" max="10.0" step="0.5" value="2.0">
                        <span id="val-sharpness">2.0</span>
                    </div>
                    <div id="div-thickness">
                        <label id="lbl-thickness" for="thickness">Wave Thickness:</label>
                        <input type="range" id="thickness" name="thickness" min="1.0" max="15.0" step="0.5" value="5.0">
                        <span id="val-thickness">5.0</span>
                    </div>
                    <div id="div-maxRadius">
                        <label id="lbl-maxRadius" for="maxRadius">Max Radius:</label>
                        <input type="range" id="maxRadius" name="maxRadius" min="5.0" max="50.0" step="1.0" value="20.0">
                        <span id="val-maxRadius">20.0</span>
                    </div>
                    <div id="div-numBands">
                        <label id="lbl-numBands" for="numBands">Num Bands:</label>
                        <input type="number" id="numBands" name="numBands" min="1" max="10" value="2">
                    </div>
                    <div id="div-takeOver">
                        <label id="lbl-takeOver" for="takeOver">Ripple Takeover:</label>
                        <input type="checkbox" id="takeOver" name="takeOver" style="width: auto; margin-bottom: 10px;">
                        <span style="font-size: 11px; color: #666; margin-left: 2px;">(Secondary color overrides primary)</span>
                    </div>
                    <button type="submit">Save Settings</button>
                </form>
            </div>
        </div>

        <!-- Node Map & Trigger Column -->
        <div class="flex-child">
            <div class="card">
                <h2>Trigger Ripples</h2>
                <p>Click a node below to trigger a ripple starting at its coordinates:</p>
                <div id="visual-map" style="border: 1px solid #ccc; background: #eaeaea; position: relative; width: 100%; height: 350px; border-radius: 4px; overflow: auto; display: flex; align-items: center; justify-content: center;">
                    <!-- SVG of map nodes will render here -->
                </div>
                <div style="margin-top: 10px;">
                    <span style="display:inline-block; width:15px; height:15px; background:#4caf50; border-radius:50%; border:1px solid #333;"></span> Bush
                    <span style="display:inline-block; width:15px; height:15px; background:#ff9800; border-radius:50%; border:1px solid #333; margin-left:10px;"></span> Reed
                </div>
            </div>
            
            <div class="card">
                <h2>Manage Map Layout</h2>
                <div id="map-stats">Loading map stats...</div>
                <div style="margin-top: 10px;">
                    <label style="display:block; margin-bottom:5px;">Paste JSON Map:</label>
                    <textarea id="map-json-input" placeholder='{"nodes": [{"row": 0, "col": 0}, ...]} or {"bushes": [...], "reeds": [...]}'></textarea>
                    <button id="save-map-btn" style="margin-top: 10px;">Save & Apply Map</button>
                </div>
            </div>
        </div>
    </div>

    <script>
        // Update slider value labels dynamically
        ['brightness', 'speed', 'fadeSpeed', 'sharpness', 'thickness', 'maxRadius'].forEach(id => {
            const input = document.getElementById(id);
            const val = document.getElementById('val-' + id);
            input.addEventListener('input', () => { val.textContent = input.value; });
        });

        // Convert HEX to RGB
        function hexToRgb(hex) {
            const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
            return result ? {
                r: parseInt(result[1], 16),
                g: parseInt(result[2], 16),
                b: parseInt(result[3], 16)
            } : null;
        }

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
            const optionals = ['primary', 'secondary', 'speed', 'fadeSpeed', 'sharpness', 'thickness', 'maxRadius', 'numBands', 'takeOver'];
            optionals.forEach(opt => setVisibility(opt, false));

            if (mode === 'ripple') {
                optionals.forEach(opt => setVisibility(opt, true));
                setLabel('primary', 'Primary Color:');
                setLabel('secondary', 'Secondary Color:');
                setLabel('speed', 'Wave Speed:');
                setLabel('fadeSpeed', 'Fade Speed:');
                setLabel('sharpness', 'Color Sharpness:');
                setLabel('thickness', 'Wave Thickness:');
                setLabel('maxRadius', 'Max Radius:');
                setLabel('numBands', 'Num Bands:');
                setLabel('takeOver', 'Ripple Takeover:');
            } else if (mode === 'solid') {
                setVisibility('primary', true);
                setLabel('primary', 'Solid Color:');
            } else if (mode === 'rainbow') {
                setVisibility('speed', true);
                setLabel('speed', 'Scroll Speed:');
            } else if (mode === 'glitter') {
                setVisibility('primary', true);
                setVisibility('secondary', true);
                setVisibility('speed', true);
                setVisibility('thickness', true);
                setVisibility('fadeSpeed', true);

                setLabel('primary', 'Background Color:');
                setLabel('secondary', 'Glitter Color:');
                setLabel('speed', 'Glitter Intensity:');
                setLabel('thickness', 'Glitter Size:');
                setLabel('fadeSpeed', 'Glitter Fade Speed:');
            } else if (mode === 'breathing') {
                setVisibility('primary', true);
                setVisibility('secondary', true);
                setVisibility('fadeSpeed', true);

                setLabel('primary', 'Primary Color:');
                setLabel('secondary', 'Secondary Color:');
                setLabel('fadeSpeed', 'Breath Fade Speed:');
            } else if (mode === 'off') {
                // Keep everything hidden
            }
        }

        // Handle dropdown mode change
        document.getElementById('mode').addEventListener('change', (e) => {
            updateUIForMode(e.target.value);
        });

        // Fetch settings and map
        async function loadStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                
                // Set settings form values
                document.getElementById('mode').value = data.config.mode;
                updateUIForMode(data.config.mode);

                document.getElementById('brightness').value = data.config.brightness;
                document.getElementById('val-brightness').textContent = data.config.brightness;
                document.getElementById('primary').value = rgbToHex(data.config.primaryR, data.config.primaryG, data.config.primaryB);
                document.getElementById('secondary').value = rgbToHex(data.config.secondaryR, data.config.secondaryG, data.config.secondaryB);
                document.getElementById('speed').value = data.config.waveSpeed;
                document.getElementById('val-speed').textContent = data.config.waveSpeed;
                document.getElementById('fadeSpeed').value = data.config.fadeSpeed;
                document.getElementById('val-fadeSpeed').textContent = data.config.fadeSpeed;
                document.getElementById('sharpness').value = data.config.sharpness;
                document.getElementById('val-sharpness').textContent = data.config.sharpness;
                document.getElementById('thickness').value = data.config.thickness;
                document.getElementById('val-thickness').textContent = data.config.thickness;
                document.getElementById('maxRadius').value = data.config.maxRadius;
                document.getElementById('val-maxRadius').textContent = data.config.maxRadius;
                document.getElementById('numBands').value = data.config.numBands;
                document.getElementById('takeOver').checked = data.config.takeOver;

                // Load map stats
                document.getElementById('map-stats').innerHTML = `
                    <strong>Nodes count:</strong> ${data.map.totalNodes} (mapped to ${data.map.totalNodes * 9} LEDs)<br>
                    <strong>Bushes count:</strong> ${data.map.bushes}<br>
                    <strong>Reeds count:</strong> ${data.map.reeds}
                `;

                renderMap(data.nodes);
            } catch (err) {
                console.error("Failed to load status", err);
            }
        }

        // Render nodes in SVG
        function renderMap(nodes) {
            const container = document.getElementById('visual-map');
            container.innerHTML = '';
            if (!nodes || nodes.length === 0) {
                container.innerHTML = '<div style="padding:20px; color:#666;">No nodes defined in the map.</div>';
                return;
            }

            // Find min/max row and col to scale and position
            let minR = Infinity, maxR = -Infinity;
            let minC = Infinity, maxC = -Infinity;
            nodes.forEach(n => {
                if (n.row < minR) minR = n.row;
                if (n.row > maxR) maxR = n.row;
                if (n.col < minC) minC = n.col;
                if (n.col > maxC) maxC = n.col;
            });

            // Handle edge case where all nodes are on the same line or single node
            if (minR === maxR) { minR -= 1; maxR += 1; }
            if (minC === maxC) { minC -= 1; maxC += 1; }

            const width = maxC - minC;
            const height = maxR - minR;
            
            // SVG setup
            const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
            svg.setAttribute("width", "100%");
            svg.setAttribute("height", "100%");
            // Set viewBox with padding
            const padding = 2;
            svg.setAttribute("viewBox", `${minC - padding} ${minR - padding} ${width + padding*2} ${height + padding*2}`);
            svg.style.display = "block";
            svg.style.maxHeight = "340px";

            nodes.forEach(n => {
                const circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
                circle.setAttribute("cx", n.col);
                circle.setAttribute("cy", n.row);
                circle.setAttribute("r", "0.6");
                circle.setAttribute("fill", n.isBush ? "#4caf50" : "#ff9800");
                circle.setAttribute("stroke", "#333");
                circle.setAttribute("stroke-width", "0.1");
                circle.style.cursor = "pointer";
                circle.style.transition = "transform 0.1s";
                
                // Hover effect
                circle.addEventListener('mouseover', () => {
                    circle.setAttribute("r", "0.9");
                });
                circle.addEventListener('mouseout', () => {
                    circle.setAttribute("r", "0.6");
                });

                // Click handler to trigger ripple
                circle.addEventListener('click', async () => {
                    try {
                        const res = await fetch(`/api/trigger?row=${n.row}&col=${n.col}`, { method: 'POST' });
                        if (res.ok) {
                            // Briefly flash the clicked node
                            circle.setAttribute("fill", "#ffffff");
                            setTimeout(() => {
                                circle.setAttribute("fill", n.isBush ? "#4caf50" : "#ff9800");
                            }, 300);
                        }
                    } catch (err) {
                        console.error("Trigger failed", err);
                    }
                });

                svg.appendChild(circle);
            });

            container.appendChild(svg);
        }

        // Save Settings
        document.getElementById('settings-form').addEventListener('submit', async (e) => {
            e.preventDefault();
            const mode = document.getElementById('mode').value;
            const brightness = parseInt(document.getElementById('brightness').value);
            const primary = hexToRgb(document.getElementById('primary').value);
            const secondary = hexToRgb(document.getElementById('secondary').value);
            const speed = parseFloat(document.getElementById('speed').value);
            const fadeSpeed = parseFloat(document.getElementById('fadeSpeed').value);
            const sharpness = parseFloat(document.getElementById('sharpness').value);
            const thickness = parseFloat(document.getElementById('thickness').value);
            const maxRadius = parseFloat(document.getElementById('maxRadius').value);
            const numBands = parseInt(document.getElementById('numBands').value);
            const takeOver = document.getElementById('takeOver').checked;

            const payload = {
                mode, brightness,
                primaryR: primary.r, primaryG: primary.g, primaryB: primary.b,
                secondaryR: secondary.r, secondaryG: secondary.g, secondaryB: secondary.b,
                waveSpeed: speed, fadeSpeed, sharpness, thickness, maxRadius, numBands, takeOver
            };

            try {
                const res = await fetch('/api/settings', {
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
                console.error("Save settings error", err);
                alert("Network error saving settings.");
            }
        });

        // Load current map JSON into textarea
        async function fetchMapJson() {
            try {
                const res = await fetch('/api/map');
                const json = await res.json();
                document.getElementById('map-json-input').value = JSON.stringify(json, null, 2);
            } catch (err) {
                console.error("Failed to load map JSON", err);
            }
        }

        // Save Map JSON
        document.getElementById('save-map-btn').addEventListener('click', async () => {
            const rawJson = document.getElementById('map-json-input').value;
            try {
                // Validate local JSON
                JSON.parse(rawJson);
                
                const res = await fetch('/api/map', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: rawJson
                });
                if (res.ok) {
                    alert("Map saved and loaded successfully!");
                    loadStatus();
                } else {
                    alert("Server failed to load the map.");
                }
            } catch (err) {
                alert("Invalid JSON format! Please correct it.");
            }
        });

        // Initial setup
        loadStatus();
        fetchMapJson();
        // Periodically refresh stats
        setInterval(async () => {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                document.getElementById('map-stats').innerHTML = `
                    <strong>Nodes count:</strong> ${data.map.totalNodes} (mapped to ${data.map.totalNodes * 9} LEDs)<br>
                    <strong>Bushes count:</strong> ${data.map.bushes}<br>
                    <strong>Reeds count:</strong> ${data.map.reeds}
                `;
            } catch (e) {}
        }, 3000);
    </script>
</body>
</html>
)rawhtml";

#endif // WEB_UI_H
