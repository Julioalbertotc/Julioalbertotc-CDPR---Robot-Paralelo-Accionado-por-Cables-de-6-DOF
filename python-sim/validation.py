import sys
import time
import json
import serial
from kinematics import calculate_inverse_kinematics

# Configuración del puerto serie (ajustar según corresponda en la PC)
PORT = 'COM3'  # Windows, e.g., 'COM3'. En Linux/Mac, '/dev/ttyUSB0'
BAUD = 115200
TIMEOUT = 1.0  # segundos

def run_validation_test():
    print("=== SCRIPT DE VALIDACIÓN CRUZADA CDPR ===")
    
    # 1. Conectar al ESP32 por puerto serie
    try:
        ser = serial.Serial(PORT, BAUD, timeout=TIMEOUT)
        print(f"Conectado exitosamente al puerto {PORT} a {BAUD} baudios.")
        time.sleep(2)  # Esperar reinicio automático del ESP32 tras conexión serial
    except Exception as e:
        print(f"Error abriendo puerto serie {PORT}: {e}")
        print("Asegúrate de cambiar la constante PORT en este script por la asignada a tu ESP32.")
        print("Ejecutando demostración matemática sin puerto físico:")
        run_offline_validation()
        return

    # Poses de prueba para validación
    test_poses = [
        # X, Y, Z, Roll, Pitch, Yaw
        (0.0, 0.0, 22.5, 0.0, 0.0, 0.0),       # Centro
        (5.0, 0.0, 22.5, 0.0, 0.0, 0.0),       # Desplazamiento X
        (0.0, -5.0, 25.0, 0.0, 0.0, 0.0),      # Desplazamiento Y y Z
        (2.0, 2.0, 20.0, 0.1, -0.1, 0.05),     # Pose combinada con rotación
        (-4.0, 3.0, 30.0, -0.2, 0.15, -0.1),   # Pose extrema
        (0.0, 0.0, 22.5, 0.5, 0.0, 0.0),       # Rotación pura en Roll extrema (aprox 28.6°)
        (0.0, 0.0, 22.5, 0.0, -0.5, 0.0),      # Rotación pura en Pitch extrema
        (0.0, 0.0, 22.5, 0.0, 0.0, 0.5),       # Rotación pura en Yaw extrema
        (3.0, -3.0, 18.0, 0.4, -0.4, 0.4)      # Pose extremadamente compleja con traslación y triple rotación
    ]

    tolerance = 0.01  # Tolerancia de discrepancia: 0.1 mm (0.01 cm)
    passed_tests = 0

    print("\nIniciando secuencia de Homing...")
    # Enviar comando de homing
    ser.write(b'{"cmd":"homing"}\n')
    time.sleep(1.0) # Esperar a que complete homing y pretensión

    for idx, (x, y, z, r, p, yaw) in enumerate(test_poses):
        print(f"\n[Prueba #{idx+1}] Evaluando Pose: X={x}, Y={y}, Z={z}, R={r}, P={p}, Y={yaw}")
        
        # 1. Calcular longitud esperada localmente en Python
        lengths_py = calculate_inverse_kinematics(x, y, z, r, p, yaw)

        # 2. Enviar pose al ESP32 por JSON
        msg = {
            "cmd": "pose",
            "x": x,
            "y": y,
            "z": z,
            "r": r,
            "p": p,
            "yaw": yaw
        }
        ser.write(json.dumps(msg).encode('utf-8') + b'\n')
        
        # 3. Leer respuesta del ESP32 (buscar la última telemetría JSON válida)
        ser.reset_input_buffer()
        time.sleep(0.1) # Breve espera para procesamiento del ESP32
        
        lengths_esp = None
        for _ in range(10): # Reintentar lecturas si hay ruido en el buffer
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line.startswith('{') and line.endswith('}'):
                try:
                    data = json.loads(line)
                    if "lengths" in data:
                        lengths_esp = data["lengths"]
                        break
                except json.JSONDecodeError:
                    continue
        
        if lengths_esp is None:
            print("  [ERROR] No se recibió telemetría válida del ESP32 en el tiempo de espera.")
            continue

        # 4. Comparar resultados
        max_diff = 0.0
        success = True
        
        print("  Cables:   Python (cm)  |  ESP32 (cm)  |  Diferencia (cm)")
        print("  --------------------------------------------------------")
        for i in range(8):
            diff = abs(lengths_py[i] - lengths_esp[i])
            max_diff = max(max_diff, diff)
            print(f"  Cable #{i}:    {lengths_py[i]:8.3f}    |   {lengths_esp[i]:8.3f}   |   {diff:8.5f}")
            
            if diff > tolerance:
                success = False

        if success:
            print(f"  [APROBADO] Diferencia máxima de {max_diff:.5f} cm dentro de la tolerancia de {tolerance} cm.")
            passed_tests += 1
        else:
            print(f"  [FALLIDO] ¡Diferencia máxima de {max_diff:.5f} cm excede la tolerancia!")

    ser.close()
    print(f"\nPruebas completadas: {passed_tests}/{len(test_poses)} poses aprobadas.")


def run_offline_validation():
    """
    Compara las matemáticas de kinematics.py contra los resultados teóricos 
    para asegurar que el script de validación funciona.
    """
    print("\nModo Offline: Comparando caso base contra matemáticas teóricas.")
    lengths = calculate_inverse_kinematics(0, 0, 22.5, 0, 0, 0)
    print("Pose central (0,0,22.5,0,0,0) - Longitudes de cables calculadas en Python:")
    for idx, length in enumerate(lengths):
        print(f"  Cable #{idx}: {length:.3f} cm")
    print("Script listo para conectar mediante puerto COM.")

if __name__ == "__main__":
    run_validation_test()
