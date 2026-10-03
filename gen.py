import uuid

def guuid(): return str(uuid.uuid4())

sch = []
sch.append('(kicad_sch (version 20211123) (generator eeschema)')
sch.append('  (uuid "{}")'.format(guuid()))
sch.append('  (paper "A3")')
sch.append('  (lib_symbols')

def add_lib_symbol(name, pins):
    sch.append('    (symbol "{}" (in_bom yes) (on_board yes)'.format(name))
    sch.append('      (property "Reference" "U" (id 0) (at 0 5 0) (effects (font (size 1.27 1.27))))')
    sch.append('      (property "Value" "{}" (id 1) (at 0 -5 0) (effects (font (size 1.27 1.27))))'.format(name))
    sch.append('      (symbol "{}_1_1"'.format(name))
    sch.append('        (rectangle (start -10 20) (end 10 -{}) (stroke (width 0.254)) (fill (type none)))'.format(len(pins)*2+5))
    y = 15
    for pnum, pname in pins.items():
        sch.append('        (pin passive line (at -15 {} 0) (length 5) (name "{}" (effects (font (size 1.27 1.27)))) (number "{}" (effects (font (size 1.27 1.27)))))'.format(y, pname, pnum))
        y -= 2
    sch.append('      )')
    sch.append('    )')

add_lib_symbol('ESP32-S3', {1:'GND', 2:'3V3', 3:'EN', 11:'IO18', 12:'IO8', 17:'IO9', 18:'IO10', 19:'IO11', 20:'IO12', 21:'IO13', 22:'IO14', 38:'IO2', 39:'IO1', 40:'GND', 41:'EPAD'})
add_lib_symbol('SLG47910V', {7:'PIN7', 10:'VDDIO', 11:'PIN11', 12:'GND', 16:'PIN16', 17:'PIN17', 18:'PIN18', 19:'PIN19', 21:'VDDIO2', 22:'VDDC'})
add_lib_symbol('MAX30102', {2:'SCL', 3:'SDA', 4:'GND', 9:'VLED', 10:'VLED', 11:'VDD', 12:'GND'})
add_lib_symbol('BME280', {1:'GND', 2:'CSB', 3:'SDA', 4:'SCL', 5:'SDO', 6:'VDDIO', 7:'GND', 8:'VDD'})
add_lib_symbol('AP2112K', {1:'VIN', 2:'GND', 3:'EN', 5:'VOUT'})
add_lib_symbol('OLED', {1:'GND', 2:'VCC', 3:'SCL', 4:'SDA'})
add_lib_symbol('PMS5003', {1:'VCC', 2:'GND', 4:'RX', 5:'TX'})
add_lib_symbol('USB_C', { 'A1':'GND', 'B1':'GND', 'A12':'GND', 'B12':'GND', 'A4':'VBUS', 'B4':'VBUS', 'A9':'VBUS', 'B9':'VBUS', 'A5':'CC1', 'B5':'CC2' })
add_lib_symbol('RES', {1:'1', 2:'2'})
add_lib_symbol('CAP', {1:'1', 2:'2'})
add_lib_symbol('LED', {1:'A', 2:'K'})

sch.append('  )')

instances = []
labels = []

x_pos = 30
def place_comp(ref, val, lib_name, pins, net_map):
    global x_pos
    y_pos = 50
    instances.append('  (symbol (lib_id "{}") (at {} {} 0) (unit 1)'.format(lib_name, x_pos, y_pos))
    instances.append('    (in_bom yes) (on_board yes) (uuid "{}")'.format(guuid()))
    instances.append('    (property "Reference" "{}" (id 0) (at {} {} 0))'.format(ref, x_pos, y_pos-5))
    instances.append('    (property "Value" "{}" (id 1) (at {} {} 0))'.format(val, x_pos, y_pos-8))
    instances.append('  )')
    
    y = y_pos - 15
    for pnum, net in net_map.items():
        if net:
            labels.append('  (global_label "{}" (at {} {} 180) (effects (font (size 1.27 1.27)) (justify right)) (uuid "{}"))'.format(net, x_pos-15, y, guuid()))
        y -= 2
    x_pos += 40

net_maps = {
    'U1': ('ESP32-S3', {1:'GND', 2:'+3V3', 3:'ESP_EN', 11:'PMS_RX_OUT', 12:'PIN_FPGA_EN', 17:'PIN_FPGA_PWR', 18:'SPI_SS_N', 19:'SPI_MOSI', 20:'SPI_SCK', 21:'SPI_MISO', 22:'PMS_TX_IN', 38:'I2C_SCL', 39:'I2C_SDA', 40:'GND', 41:'GND'}),
    'U2': ('SLG47910V', {7:'LED_USER_PULSE', 10:'+3V3', 11:'PIN_FPGA_EN', 12:'GND', 16:'SPI_SCK', 17:'SPI_SS_N', 18:'SPI_MOSI', 19:'SPI_MISO', 21:'+3V3', 22:'VDDC_1V2'}),
    'U3': ('MAX30102', {2:'I2C_SCL', 3:'I2C_SDA', 4:'GND', 9:'+3V3', 10:'+3V3', 11:'+3V3', 12:'GND'}),
    'U4': ('BME280', {1:'GND', 2:'+3V3', 3:'I2C_SDA', 4:'I2C_SCL', 5:'GND', 6:'+3V3', 7:'GND', 8:'+3V3'}),
    'U5': ('AP2112K', {1:'+5V', 2:'GND', 3:'+5V', 5:'+3V3'}),
    'DISP1': ('OLED', {1:'GND', 2:'+3V3', 3:'I2C_SCL', 4:'I2C_SDA'}),
    'J1': ('PMS5003', {1:'+5V', 2:'GND', 4:'PMS_RX_OUT', 5:'PMS_TX_IN'}),
    'J2': ('USB_C', {'A1':'GND', 'B1':'GND', 'A12':'GND', 'B12':'GND', 'A4':'+5V', 'B4':'+5V', 'A9':'+5V', 'B9':'+5V', 'A5':'USB_CC1', 'B5':'USB_CC2'}),
    'R1': ('RES', {1:'+3V3', 2:'I2C_SDA'}),
    'R2': ('RES', {1:'+3V3', 2:'I2C_SCL'}),
    'R3': ('RES', {1:'+3V3', 2:'ESP_EN'}),
    'R4': ('RES', {1:'LED_USER_PULSE', 2:'LED_USER_ANODE'}),
    'R5': ('RES', {1:'USB_CC1', 2:'GND'}),
    'R6': ('RES', {1:'USB_CC2', 2:'GND'}),
    'D1': ('LED', {1:'LED_USER_ANODE', 2:'GND'}),
    'D2': ('LED', {1:'+3V3', 2:'GND'}),
    'C1': ('CAP', {1:'+3V3', 2:'GND'}),
    'C2': ('CAP', {1:'+3V3', 2:'GND'}),
    'C3': ('CAP', {1:'+3V3', 2:'GND'}),
    'C4': ('CAP', {1:'VDDC_1V2', 2:'GND'}),
    'C5': ('CAP', {1:'VDDC_1V2', 2:'GND'}),
    'C6': ('CAP', {1:'+3V3', 2:'GND'}),
    'C7': ('CAP', {1:'+3V3', 2:'GND'}),
    'C8': ('CAP', {1:'+3V3', 2:'GND'}),
    'C9': ('CAP', {1:'+3V3', 2:'GND'}),
    'C10': ('CAP', {1:'+5V', 2:'GND'}),
    'C11': ('CAP', {1:'+3V3', 2:'GND'})
}

for ref, (lib_name, net_map) in net_maps.items():
    place_comp(ref, lib_name, lib_name, net_map, net_map)

sch.extend(instances)
sch.extend(labels)

sch.append('  (sheet_instances (path "/" (page "1")))')
sch.append(')')

with open('SCHEMATICS/valor_system_schematic.kicad_sch', 'w', encoding='utf-8') as f:
    f.write('\n'.join(sch))
print('Done KiCad generation')
