with open('firmware/shrikefi/main_shrikefi.c', 'rb') as f:
    data = f.read()

target = b'        esp32_i2c_hal_scan();\r\n    }'
replacement = (
    b'        esp32_i2c_hal_scan();\r\n'
    b'        if (esp32_i2c_hal_probe(0x48) != I2C_HAL_SUCCESS) {\r\n'
    b'            ESP_LOGW("TMP117", "TMP117: Device NOT responding at I2C address 0x48! (NO ACK - Hardware Defective/DOA)");\r\n'
    b'        }\r\n'
    b'    }'
)

if target in data:
    new_data = data.replace(target, replacement, 1)
    with open('firmware/shrikefi/main_shrikefi.c', 'wb') as f:
        f.write(new_data)
    print('SUCCESS: Added yellow warning probe to main_shrikefi.c!')
else:
    print('Target not found in main_shrikefi.c')
