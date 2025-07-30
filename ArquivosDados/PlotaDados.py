import numpy as np
import matplotlib.pyplot as plt

# Lê o arquivo CSV, ignorando o cabeçalho
data = np.loadtxt('imu_data.csv', delimiter=',', skiprows=1)

# Extrai colunas
tempo = data[:, 0]
accel_x = data[:, 1]
accel_y = data[:, 2]
accel_z = data[:, 3]
giro_x = data[:, 4]
giro_y = data[:, 5]
giro_z = data[:, 6]

# Gráfico dos dados de aceleração
plt.figure(figsize=(10, 6))
plt.plot(tempo, accel_x, label='Accel X')
plt.plot(tempo, accel_y, label='Accel Y')
plt.plot(tempo, accel_z, label='Accel Z')
plt.title('Aceleração vs Tempo')
plt.xlabel('Número da Amostra')
plt.ylabel('Aceleração (unidades brutas)')
plt.legend()
plt.grid(True)
plt.savefig('grafico_acelerometro.png')
plt.show()

# Gráfico dos dados do giroscópio
plt.figure(figsize=(10, 6))
plt.plot(tempo, giro_x, label='Giro X')
plt.plot(tempo, giro_y, label='Giro Y')
plt.plot(tempo, giro_z, label='Giro Z')
plt.title('Giroscópio vs Tempo')
plt.xlabel('Número da Amostra')
plt.ylabel('Velocidade Angular (unidades brutas)')
plt.legend()
plt.grid(True)
plt.savefig('grafico_giroscopio.png')
plt.show()