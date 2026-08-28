import urllib.request
import base64
import json
import os

def download_mermaid(code, filename):
    data = {"code": code, "mermaid": {"theme": "default"}}
    b64 = base64.b64encode(json.dumps(data).encode('utf-8')).decode('utf-8')
    url = f"https://mermaid.ink/img/{b64}"
    
    req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
    try:
        with urllib.request.urlopen(req) as response:
            with open(filename, 'wb') as f:
                f.write(response.read())
        print(f"Downloaded {filename}")
    except Exception as e:
        print(f"Failed to download {filename}: {e}")

data_flow = """graph TD
    A["Sensors<br>(Body + Environment)"] -->|"Raw Data"| B["FPGA Accelerator<br>(Cleans data, detects peaks)"]
    B -->|"Filtered Vitals"| C["ARM CPU<br>(Fuses data, runs AI Model)"]
    C -->|"Risk Scores"| D["OLED Display<br>(Shows warnings offline)"]
    
    style A fill:#f8fafc,stroke:#94a3b8,stroke-width:2px
    style B fill:#e0f2fe,stroke:#38bdf8,stroke-width:2px
    style C fill:#f0fdf4,stroke:#4ade80,stroke-width:2px
    style D fill:#fff1f2,stroke:#fb7185,stroke-width:2px
"""

fsm = """stateDiagram-v2
    [*] --> IDLE
    IDLE --> RISING : Signal rises
    RISING --> PEAK_FOUND : Peak detected
    PEAK_FOUND --> FALLING : Signal falls
    FALLING --> IDLE : Signal levels out
"""

download_mermaid(data_flow, "data_flow.png")
download_mermaid(fsm, "fsm.png")

nn_arch = """graph LR
    subgraph Input Layer
        direction TB
        I1[HR]
        I2[RMSSD]
        I3[SpO2]
        I4[Temp]
        I5[Hum]
        I6[PM2.5]
    end
    
    subgraph Hidden Layer
        H[12 Artificial Neurons<br>with ReLU Activation]
    end
    
    subgraph Output Layer
        direction TB
        O1[Heat Risk]
        O2[Pollution Risk]
        O3[Flood/Cold Risk]
    end
    
    I1 & I2 & I3 & I4 & I5 & I6 --> H
    H --> O1 & O2 & O3
    
    style Input Layer fill:#f8fafc,stroke:#94a3b8,stroke-width:2px
    style Hidden Layer fill:#fef3c7,stroke:#f59e0b,stroke-width:2px
    style Output Layer fill:#fee2e2,stroke:#ef4444,stroke-width:2px
"""

download_mermaid(nn_arch, "nn_arch.png")
