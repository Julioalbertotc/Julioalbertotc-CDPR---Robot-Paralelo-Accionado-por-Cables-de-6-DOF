// =============================================================================
// GEOMETRÍA DEL ROBOT (Debe coincidir exactamente con Config.h / kinematics.py)
// =============================================================================
const POLE_POSITIONS = [
    {x: -22.5, y: -21.0, z: 45.0}, // 0
    {x: -21.0, y: -22.5, z: 45.0}, // 1
    {x: 21.0,  y: -22.5, z: 45.0}, // 2
    {x: 22.5,  y: -21.0, z: 45.0}, // 3
    {x: 22.5,  y: 21.0,  z: 45.0}, // 4
    {x: 21.0,  y: 22.5,  z: 45.0}, // 5
    {x: -21.0, y: 22.5,  z: 45.0}, // 6
    {x: -22.5, y: 21.0,  z: 45.0}  // 7
];

const ANCHOR_POSITIONS = [
    {x: -5.0, y: -4.0, z: 0.0}, // 0
    {x: -4.0, y: -5.0, z: 0.0}, // 1
    {x: 4.0,  y: -5.0, z: 0.0}, // 2
    {x: 5.0,  y: -4.0, z: 0.0}, // 3
    {x: 5.0,  y: 4.0,  z: 0.0}, // 4
    {x: 4.0,  y: 5.0,  z: 0.0}, // 5
    {x: -4.0, y: 5.0,  z: 0.0}, // 6
    {x: -5.0, y: 4.0,  z: 0.0}  // 7
];

// Estado global de conexión y sliders
let socket = null;
let systemState = 0; // 0: UNHOMED, 1: HOMING, 2: READY, 3: ESTOP
let isConnected = false;

// Elementos de la interfaz
const connDot = document.getElementById("connection-dot");
const connText = document.getElementById("connection-status");
const robotDot = document.getElementById("robot-dot");
const robotText = document.getElementById("robot-state");

const sliders = {
    x: document.getElementById("slider-x"),
    y: document.getElementById("slider-y"),
    z: document.getElementById("slider-z"),
    r: document.getElementById("slider-r"),
    p: document.getElementById("slider-p"),
    yaw: document.getElementById("slider-yaw")
};

const vals = {
    x: document.getElementById("val-x"),
    y: document.getElementById("val-y"),
    z: document.getElementById("val-z"),
    r: document.getElementById("val-r"),
    p: document.getElementById("val-p"),
    yaw: document.getElementById("val-yaw")
};

const btnHoming = document.getElementById("btn-homing");
const btnCenter = document.getElementById("btn-center");
const btnEstop = document.getElementById("btn-estop");
const cablesList = document.getElementById("cables-list");

// =============================================================================
// INICIALIZACIÓN DE LA LISTA DE TELEMETRÍA (DOM)
// =============================================================================
function initCablesList() {
    cablesList.innerHTML = "";
    for (let i = 0; i < 8; i++) {
        const row = document.createElement("div");
        row.className = "cable-row";
        row.innerHTML = `
            <span class="cable-id">#${i}</span>
            <span id="c-tgt-${i}" class="c-val">0.00 cm</span>
            <span id="c-cur-${i}" class="c-val">0.00 cm</span>
            <div class="bar-container">
                <div id="c-bar-${i}" class="bar-fill"></div>
            </div>
        `;
        cablesList.appendChild(row);
    }
}
initCablesList();

// =============================================================================
// WEBSOCKETS - TELEMETRÍA Y COMANDOS
// =============================================================================
function connectWebSocket() {
    // Si corre directamente desde LittleFS en el ESP32, toma el host de la URL, sino usa fallback
    const host = window.location.hostname || "192.168.4.1";
    socket = new WebSocket(`ws://${host}/ws`);

    socket.onopen = () => {
        isConnected = true;
        connDot.className = "dot connected";
        connText.innerText = "Conectado";
        console.log("WebSocket Conectado a:", host);
    };

    socket.onclose = () => {
        isConnected = false;
        systemState = 0;
        connDot.className = "dot disconnected";
        connText.innerText = "Desconectado";
        updateStateUI();
        console.log("WebSocket Desconectado. Reintentando...");
        setTimeout(connectWebSocket, 2000); // Reconexión automática
    };

    socket.onmessage = (event) => {
        const data = JSON.parse(event.data);
        systemState = data.state;
        updateStateUI();
        
        // Actualizar valores numéricos en sliders si el usuario no los está moviendo
        if (systemState === 2) { // READY
            updateTelemetryDisplay(data);
        }
        
        // Actualizar visualización 3D
        update3DScene(data.pose);
    };
}

// Cambiar estado visual de controles según estado del robot
function updateStateUI() {
    // Configurar color del indicador de estado del robot
    robotDot.className = "dot";
    if (systemState === 0) { // UNHOMED
        robotDot.classList.add("unhomed");
        robotText.innerText = "Homing Pendiente";
        setControlsDisabled(true);
        btnHoming.disabled = false;
        btnCenter.disabled = true;
    } else if (systemState === 1) { // HOMING
        robotDot.classList.add("homing");
        robotText.innerText = "Homing en Proceso...";
        setControlsDisabled(true);
        btnHoming.disabled = true;
        btnCenter.disabled = true;
    } else if (systemState === 2) { // READY
        robotDot.classList.add("ready");
        robotText.innerText = "Listo";
        setControlsDisabled(false);
        btnHoming.disabled = false; // Permite re-homing
        btnCenter.disabled = false;
    } else if (systemState === 3) { // ESTOP
        robotDot.classList.add("estop");
        robotText.innerText = "PARO DE EMERGENCIA";
        setControlsDisabled(true);
        btnHoming.disabled = false; // Permite re-homing para desbloquear
        btnCenter.disabled = true;
    }
}

function setControlsDisabled(disabled) {
    Object.values(sliders).forEach(slider => {
        slider.disabled = disabled || !isConnected;
    });
}

function updateTelemetryDisplay(data) {
    for (let i = 0; i < 8; i++) {
        document.getElementById(`c-tgt-${i}`).innerText = `${data.target_lengths[i].toFixed(2)} cm`;
        document.getElementById(`c-cur-${i}`).innerText = `${data.lengths[i].toFixed(2)} cm`;
        
        // Barra de PWM (carga del motor)
        const pwm = Math.abs(data.pwms[i]);
        const percent = Math.min((pwm / 255) * 100, 100);
        const bar = document.getElementById(`c-bar-${i}`);
        bar.style.width = `${percent}%`;
        
        // Cambiar color de la barra según carga
        if (percent > 80) bar.style.backgroundColor = "var(--accent-red)";
        else if (percent > 40) bar.style.backgroundColor = "var(--accent-yellow)";
        else bar.style.backgroundColor = "var(--accent-blue)";
    }
}

// Envío de pose debouncera (limitada a 20 Hz / cada 50ms)
let sendTimeout = null;
function queuePoseSend() {
    if (!isConnected || systemState !== 2) return;
    
    if (sendTimeout) clearTimeout(sendTimeout);
    
    sendTimeout = setTimeout(() => {
        // Convertir grados de sliders a radianes para cinemática en ESP32
        const deg2rad = Math.PI / 180;
        const msg = {
            cmd: "pose",
            x: parseFloat(sliders.x.value),
            y: parseFloat(sliders.y.value),
            z: parseFloat(sliders.z.value),
            r: parseFloat(sliders.r.value) * deg2rad,
            p: parseFloat(sliders.p.value) * deg2rad,
            yaw: parseFloat(sliders.yaw.value) * deg2rad
        };
        socket.send(JSON.stringify(msg));
    }, 50);
}

// Vinculación de eventos a sliders
Object.keys(sliders).forEach(key => {
    sliders[key].addEventListener("input", (e) => {
        let suffix = " cm";
        if (["r", "p", "yaw"].includes(key)) suffix = "°";
        vals[key].innerText = e.target.value + suffix;
        queuePoseSend();
    });
});

// Botones de Comando
btnHoming.addEventListener("click", () => {
    if (isConnected) socket.send(JSON.stringify({cmd: "homing"}));
});
btnCenter.addEventListener("click", () => {
    if (isConnected) {
        socket.send(JSON.stringify({cmd: "center"}));
        // Resetear visualmente sliders
        sliders.x.value = 0; vals.x.innerText = "0.0 cm";
        sliders.y.value = 0; vals.y.innerText = "0.0 cm";
        sliders.z.value = 22.5; vals.z.innerText = "22.5 cm";
        sliders.r.value = 0; vals.r.innerText = "0°";
        sliders.p.value = 0; vals.p.innerText = "0°";
        sliders.yaw.value = 0; vals.yaw.innerText = "0°";
    }
});
btnEstop.addEventListener("click", () => {
    if (isConnected) socket.send(JSON.stringify({cmd: "estop"}));
});

// =============================================================================
// VISUALIZACIÓN 3D CON THREE.JS
// =============================================================================
let scene, camera, renderer, controls;
let frameCube, platformMesh;
let cablesLines = [];

function init3D() {
    const container = document.getElementById("canvas-container");
    
    // Escena y Cámara
    scene = new THREE.Scene();
    scene.background = new THREE.Color(0x0e1118);

    camera = new THREE.PerspectiveCamera(45, container.clientWidth / container.clientHeight, 0.1, 500);
    // Posicionar cámara viendo desde el frente/esquina superior
    camera.position.set(45, 50, 65);

    // Renderer
    renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(container.clientWidth, container.clientHeight);
    renderer.setPixelRatio(window.devicePixelRatio);
    container.appendChild(renderer.domElement);

    // Controles de Orbita
    controls = new THREE.OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.05;
    controls.maxPolarAngle = Math.PI / 2 - 0.05; // No bajar del piso

    // Iluminación
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.4);
    scene.add(ambientLight);

    const dirLight = new THREE.DirectionalLight(0xffffff, 0.8);
    dirLight.position.set(20, 60, 20);
    scene.add(dirLight);

    // Cuadrícula de base (suelo)
    const gridHelper = new THREE.GridHelper(60, 20, 0x3b82f6, 0x1f2937);
    gridHelper.position.y = 0;
    scene.add(gridHelper);

    // 1. Estructura Cubo de Acero (45 x 45 x 45 cm)
    // El origen de coordenadas está en el centro de la base.
    const cubeGeo = new THREE.BoxGeometry(45, 45, 45);
    // Crear el cubo con líneas
    const edges = new THREE.EdgesGeometry(cubeGeo);
    frameCube = new THREE.LineSegments(edges, new THREE.LineBasicMaterial({ color: 0x4b5563, linewidth: 2 }));
    frameCube.position.set(0, 22.5, 0); // Desplazar Z (Y en Three.js) para que coincida con el origen en la base
    scene.add(frameCube);

    // Dibujar poleas como pequeñas esferas
    const pulleyGeo = new THREE.SphereGeometry(0.8, 8, 8);
    const pulleyMat = new THREE.MeshLambertMaterial({ color: 0xfbbf24 });
    POLE_POSITIONS.forEach(pos => {
        const pulleyMesh = new THREE.Mesh(pulleyGeo, pulleyMat);
        // Mapear Coordenadas (Robot XYZ -> Three.js X, Z, Y)
        // En Three.js: X=X, Y=Z (up), Z=-Y
        pulleyMesh.position.set(pos.x, pos.z, -pos.y);
        scene.add(pulleyMesh);
    });

    // 2. Plataforma Móvil (pequeña placa cuadrada de 10x10 cm)
    const platGeo = new THREE.BoxGeometry(10, 1.0, 10);
    const platMat = new THREE.MeshLambertMaterial({ color: 0x3b82f6 });
    platformMesh = new THREE.Mesh(platGeo, platMat);
    platformMesh.position.set(0, 22.5, 0); // Iniciar en el centro seguro
    scene.add(platformMesh);

    // 3. Crear Líneas para los 8 Cables
    const lineMat = new THREE.LineBasicMaterial({ color: 0x10b981, linewidth: 2 });
    for (let i = 0; i < 8; i++) {
        const points = [];
        points.push(new THREE.Vector3(0, 0, 0)); // Polea
        points.push(new THREE.Vector3(0, 0, 0)); // Anclaje plataforma
        const lineGeo = new THREE.BufferGeometry().setFromPoints(points);
        const line = new THREE.Line(lineGeo, lineMat);
        scene.add(line);
        cablesLines.push(line);
    }

    animate();
}

function animate() {
    requestAnimationFrame(animate);
    controls.update();
    renderer.render(scene, camera);
}

// Actualizar la pose de la plataforma y de las líneas de los cables
function update3DScene(pose) {
    if (!pose || !platformMesh) return;
    
    // Mapear posición (Robot XYZ -> Three.js X, Z, -Y)
    platformMesh.position.set(pose.x, pose.z, -pose.y);
    
    // Calcular cosenos y senos para la matriz de rotación pura R_zyx
    const cr = Math.cos(pose.r), sr = Math.sin(pose.r);
    const cp = Math.cos(pose.p), sp = Math.sin(pose.p);
    const cy = Math.cos(pose.yaw), sy = Math.sin(pose.yaw);

    const r11 = cy * cp;
    const r12 = cy * sp * sr - sy * cr;
    const r13 = cy * sp * cr + sy * sr;

    const r21 = sy * cp;
    const r22 = sy * sp * sr + cy * cr;
    const r23 = sy * sp * cr - cy * sr;

    const r31 = -sp;
    const r32 = cp * sr;
    const r33 = cp * cr;

    // Aplicar la matriz de rotación del robot mapeada a Three.js (Permutación: X_3d=X_rob, Y_3d=Z_rob, Z_3d=-Y_rob)
    const m = new THREE.Matrix4();
    m.set(
        r11,  r13, -r12, 0,
        r31,  r33, -r32, 0,
       -r21, -r23,  r22, 0,
        0,    0,    0,   1
    );
    platformMesh.rotation.setFromRotationMatrix(m);

    // Calcular las posiciones de anclaje para estirar las líneas
    for (let i = 0; i < 8; i++) {
        const ax = ANCHOR_POSITIONS[i].x;
        const ay = ANCHOR_POSITIONS[i].y;
        const az = ANCHOR_POSITIONS[i].z;

        // Anclaje rotado y trasladado
        const qx = pose.x + (r11 * ax + r12 * ay + r13 * az);
        const qy = pose.y + (r21 * ax + r22 * ay + r23 * az);
        const qz = pose.z + (r31 * ax + r32 * ay + r33 * az);

        // Actualizar geometría de línea
        const pole = POLE_POSITIONS[i];
        
        // Puntos en el espacio Three.js (Robot XYZ -> X, Z, -Y)
        const p1 = new THREE.Vector3(pole.x, pole.z, -pole.y);
        const p2 = new THREE.Vector3(qx, qz, -qy);
        
        cablesLines[i].geometry.setFromPoints([p1, p2]);
    }
}

// Redimensionar visor si la ventana cambia
window.addEventListener("resize", () => {
    const container = document.getElementById("canvas-container");
    if (!container || !renderer) return;
    camera.aspect = container.clientWidth / container.clientHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(container.clientWidth, container.clientHeight);
});

// Arrancar conexiones
window.onload = () => {
    init3D();
    connectWebSocket();
};
