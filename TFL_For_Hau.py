import pandas as pd
from sklearn.model_selection import train_test_split
import tensorflow as tf
import numpy as np

PREFIX = "include/model"

# 1. Đọc CSV
data = pd.read_csv("sensor_data_Hau.csv", names=["temp", "humidity"])
values = data.values

# 2. Tạo X, y với window size = 3
window_size = 3
X = []
y = []

for i in range(len(values) - window_size):
    X.append(values[i:i+window_size].flatten())  # 3 dòng * 2 cột = 6 giá trị
    y.append(values[i + window_size])            # dòng tiếp theo (temp + humidity)

X = np.array(X)
y = np.array(y)

# 3. Chia train/test
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, shuffle=False)

# 4. Tạo model hồi quy nhỏ
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(window_size*2,)),  # 6 giá trị đầu vào
    tf.keras.layers.Dense(12, activation='relu'),   # layer ẩn 12 neuron
    tf.keras.layers.Dense(2)                        # output 2 giá trị (temp + humidity)
])

model.compile(loss="mse", optimizer="adam", metrics=["mae"])

# 5. Train model
model.fit(X_train, y_train, epochs=200, validation_data=(X_test, y_test))

# 6. Lưu Keras model
model.save(PREFIX + ".h5")

# 7. Convert sang TFLite
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

with open(PREFIX + ".tflite", "wb") as f:
    f.write(tflite_model)

# 8. Xuất sang file .h để nhúng ESP32
with open(PREFIX + ".tflite", 'rb') as tflite_file:
    tflite_content = tflite_file.read()

hex_lines = [', '.join([f'0x{byte:02x}' for byte in tflite_content[i:i+12]]) 
             for i in range(0, len(tflite_content), 12)]
hex_array = ',\n  '.join(hex_lines)

with open(PREFIX + ".h", 'w') as header_file:
    header_file.write('const unsigned char model[] = {\n  ')
    header_file.write(f'{hex_array}\n')
    header_file.write('};\n\n')
