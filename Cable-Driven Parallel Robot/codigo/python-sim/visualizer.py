import math
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.animation as animation
from kinematics import calculate_inverse_kinematics, POLE_POSITIONS, ANCHOR_POSITIONS

# Generar trayectoria de prueba: Espiral ascendente con inclinación (Roll/Pitch/Yaw dinámico)
def generate_trajectory(steps=100):
    trajectory = []
    for i in range(steps):
        t = i / float(steps)
        # Espiral circular en el plano XY
        x = 6.0 * math.sin(2 * math.pi * 2 * t)
        y = 6.0 * math.cos(2 * math.pi * 2 * t)
        # Ascenso en Z
        z = 18.0 + 10.0 * t
        
        # Variación de actitud (inclinaciones leves en Roll, Pitch, Yaw)
        roll = 0.15 * math.sin(2 * math.pi * t)
        pitch = 0.15 * math.cos(2 * math.pi * t)
        yaw = 0.25 * math.sin(2 * math.pi * t)
        
        trajectory.append((x, y, z, roll, pitch, yaw))
    return trajectory

def get_anchor_world_positions(x, y, z, roll, pitch, yaw):
    """
    Rotación y traslación de los 8 anclajes en la plataforma
    """
    cr, sr = math.cos(roll), math.sin(roll)
    cp, sp = math.cos(pitch), math.sin(pitch)
    cy, sy = math.cos(yaw), math.sin(yaw)

    # Matriz de rotación R_zyx
    r11 = cy * cp
    r12 = cy * sp * sr - sy * cr
    r13 = cy * sp * cr + sy * sr

    r21 = sy * cp
    r22 = sy * sp * sr + cy * cr
    r23 = sy * sp * cr - cy * sr

    r31 = -sp
    r32 = cp * sr
    r33 = cp * cr

    world_anchors = []
    for i in range(8):
        ax = ANCHOR_POSITIONS[i]["x"]
        ay = ANCHOR_POSITIONS[i]["y"]
        az = ANCHOR_POSITIONS[i]["z"]

        qx = x + (r11 * ax + r12 * ay + r13 * az)
        qy = y + (r21 * ax + r22 * ay + r23 * az)
        qz = z + (r31 * ax + r32 * ay + r33 * az)
        world_anchors.append((qx, qy, qz))
        
    return world_anchors

def animate_cdpr():
    trajectory = generate_trajectory()
    
    fig = plt.figure(figsize=(8, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    # Configurar límites del cubo de 45 cm
    ax.set_xlim([-25, 25])
    ax.set_ylim([-25, 25])
    ax.set_zlim([0, 50])
    ax.set_xlabel('X (cm)')
    ax.set_ylabel('Y (cm)')
    ax.set_zlabel('Z (cm)')
    ax.set_title('Simulador 3D CDPR - Trayectoria Helicoidal de Prueba')
    
    # Dibujar las poleas fijas como esferas amarillas
    pole_x = [p["x"] for p in POLE_POSITIONS]
    pole_y = [p["y"] for p in POLE_POSITIONS]
    pole_z = [p["z"] for p in POLE_POSITIONS]
    ax.scatter(pole_x, pole_y, pole_z, color='gold', s=100, label='Poleas fijas (Cubo)')
    
    # Inicializar elementos a animar
    platform_plot, = ax.plot([], [], [], 'b-', linewidth=2, label='Plataforma Móvil')
    cable_lines = [ax.plot([], [], [], 'g-', alpha=0.6)[0] for _ in range(8)]
    
    # Dibujar líneas del armazón de forma dinámica a partir de POLE_POSITIONS
    min_x = min(p["x"] for p in POLE_POSITIONS)
    max_x = max(p["x"] for p in POLE_POSITIONS)
    min_y = min(p["y"] for p in POLE_POSITIONS)
    max_y = max(p["y"] for p in POLE_POSITIONS)
    min_z = 0.0
    max_z = max(p["z"] for p in POLE_POSITIONS)

    # Base
    ax.plot([min_x, max_x, max_x, min_x, min_x], [min_y, min_y, max_y, max_y, min_y], [min_z, min_z, min_z, min_z, min_z], 'k--', alpha=0.3)
    # Techo
    ax.plot([min_x, max_x, max_x, min_x, min_x], [min_y, min_y, max_y, max_y, min_y], [max_z, max_z, max_z, max_z, max_z], 'k--', alpha=0.3)
    # Columnas
    for cx in [min_x, max_x]:
        for cy in [min_y, max_y]:
            ax.plot([cx, cx], [cy, cy], [min_z, max_z], 'k--', alpha=0.3)

    def update(frame):
        x, y, z, roll, pitch, yaw = trajectory[frame]
        
        # Calcular posición de anclajes
        anchors = get_anchor_world_positions(x, y, z, roll, pitch, yaw)
        
        # Calcular longitudes reales usando la cinemática inversa y mostrarlas
        lengths = calculate_inverse_kinematics(x, y, z, roll, pitch, yaw)
        print(f"Frame {frame:03d} | Pose: X={x:5.2f}, Y={y:5.2f}, Z={z:5.2f} cm | Cables: " + 
              ", ".join(f"#{i}:{l:.2f}" for i, l in enumerate(lengths)))
        
        # 1. Graficar cables
        for i in range(8):
            px, py, pz = POLE_POSITIONS[i]["x"], POLE_POSITIONS[i]["y"], POLE_POSITIONS[i]["z"]
            ax_x, ax_y, ax_z = anchors[i]
            cable_lines[i].set_data([px, ax_x], [py, ax_y])
            cable_lines[i].set_3d_properties([pz, ax_z])
            
        # 2. Dibujar la plataforma (perímetro conectando los 8 anclajes secuencialmente)
        plat_x = [anchors[i][0] for i in [0, 1, 2, 3, 4, 5, 6, 7, 0]]
        plat_y = [anchors[i][1] for i in [0, 1, 2, 3, 4, 5, 6, 7, 0]]
        plat_z = [anchors[i][2] for i in [0, 1, 2, 3, 4, 5, 6, 7, 0]]
        platform_plot.set_data(plat_x, plat_y)
        platform_plot.set_3d_properties(plat_z)
        
        return [platform_plot] + cable_lines

    # Crear animación (se cambia blit a False porque Matplotlib 3D no soporta blitting)
    ani = animation.FuncAnimation(fig, update, frames=len(trajectory), interval=50, blit=False)
    plt.legend()
    plt.show()

if __name__ == "__main__":
    animate_cdpr()
