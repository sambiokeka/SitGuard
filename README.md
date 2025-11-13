# SitGuard

Um projeto completo para monitorar a postura de quem está sentado à mesa usando um ESP32 + sensor ultrassônico HC-SR04. O dispositivo fornece feedback local (LEDs, buzzer e LCD) e envia dados em tempo real para um backend na nuvem via MQTT. Um painel web (Flask + Socket.IO) exibe o status e a distância em tempo real.

(simulação Wokwi + backend Flask + Mosquitto)

## Integrantes
- Erick Jooji Bertassoli Yamashita - RM: 564482
- Links úteis:
  - Projeto Wokwi: https://wokwi.com/projects/447550415358261249
  - Vídeo explicativo: 

---

## Índice
- [Integrantes](#integrantes)
- [Visão Geral](#visão-geral)
- [Funcionalidades Principais](#funcionalidades-principais)
- [Arquitetura](#arquitetura)
- [Tópico MQTT e Payload](#tópico-mqtt-e-payload)
- [Componentes e Tecnologias](#componentes-e-tecnologias)
- [Como rodar (Passo a passo)](#como-rodar-passo-a-passo)
- [Payload](#payload)
- [Estrutura do Repositório](#estrutura-do-repositório)

---

## Visão Geral
O projeto mede a distância do usuário até a mesa com o HC-SR04, classifica a postura em três estados ("ok", "ruim", "ausente"), entrega feedback local (LED verde para ok; LED vermelho + buzzer para ruim; LCD mostra texto) e publica um JSON via MQTT para um broker que o backend consome e retransmite ao frontend via WebSockets.

Wokwi

<img width="531" height="501" alt="image" src="https://github.com/user-attachments/assets/7a2e3f4b-0272-4b3c-aaac-ff192fd86643" />


Frontend

<img width="1432" height="901" alt="image" src="https://github.com/user-attachments/assets/7a1c158f-6994-4ed3-b9f6-eef82331d821" />



---

## Funcionalidades Principais
- Monitoramento em tempo real da distância (cm)
- Feedback local imediato:
  - LED Verde + LCD → Postura OK
  - LED Vermelho + Buzzer + LCD → Postura RUIM
  - LCD → Modo de espera quando ausente
- Envio de dados via MQTT (tópico: `fiap/gs/postura`)
- Dashboard web em tempo real (Flask + Socket.IO)
- Detecta ausência do usuário e entra em "Modo de espera"

---

## Arquitetura
1. Dispositivo IoT (ESP32 + HC-SR04) — lê a distância, aciona atuadores e publica mensagens MQTT.
2. Broker MQTT (Mosquitto) — roda em container Docker na VM Azure.
3. Backend (Flask + Paho-MQTT + Flask-SocketIO) — subscreve ao tópico MQTT, recebe mensagens e as transmite para o frontend via WebSockets.
4. Frontend (index.html + JS com Socket.IO) — atualiza UI em tempo real.

Fluxo: ESP32 → (MQTT) → Mosquitto → (Paho-MQTT) → Flask → (Socket.IO) → Browser

---

## Tópico MQTT e Payload
- Tópico: `fiap/gs/postura`
- Payload JSON:
```json
{
  "status": "ok",
  "distancia_cm": 42.5
}
```
status: `"ok" | "ruim" | "ausente"`
distancia_cm: número (cm)

---

## Componentes e Tecnologias
- Hardware (simulado): ESP32, HC-SR04, LED verde/vermelho, buzzer, LCD 16x2 I2C (Wokwi)
- Dispositivo: C++ (Arduino)
- Protocolo: MQTT
- Broker: Mosquitto (Docker)
- Backend: Python 3, Flask, Flask-SocketIO, paho-mqtt
- Frontend: HTML5, CSS3, JavaScript (Socket.IO client)
- Simulação: Wokwi

---

## Como rodar (Passo a passo)

### 1) Hardware (Wokwi / ESP32)
- Abra o projeto no Wokwi (cole o link do seu projeto Wokwi).
- No código .ino altere:
```cpp
const char* mqtt_server = "SEU_IP_AQUI"; // IP público/privado do broker (Usei uma VM da Azure)
```
- Compile e rode a simulação no Wokwi. Verifique o console serial para logs MQTT e leitura do sensor.

### 2) Backend (Flask)
- Acesse sua VM da Azure (ou só abra sua maquina virtual):
```bash
ssh seu_usuario@SEU_IP_AQUI
```

- Clone o repositório:
```bash
git clone https://github.com/sambiokeka/SitGuard.git
cd SitGuard
```

- Instale dependências:
```bash
pip install -r requirements.txt
```

- Rode o servidor:
```bash
python3 app.py
```

Firewall / NSG (Azure):
- Abra a porta 1883 (MQTT).
- Abra a porta 8080 (HTTP)

### 3) Frontend (acesso)
- No navegador, abra:
```
http://SEU_IP_AQUI:8080
```
- A página `index.html` conecta via Socket.IO e atualiza status/distance em tempo real.

### 4) Caso de erro na porta 1883

- Crie diretório e arquivo de configuração:
```bash
mkdir -p ~/mosquitto && cd ~/mosquitto
cat > mosquitto.conf <<'EOF'
listener 1883
allow_anonymous true
EOF
```
- Rode o container:
```bash
docker run -itd --name mosquitto -p 1883:1883 -v ~/mosquitto/mosquitto.conf:/mosquitto/config/mosquitto.conf eclipse-mosquitto
```
- Verifique logs:
```bash
docker logs -f mosquitto
```

---

## Payload
- Mensagem recebida no broker:
```
topic: fiap/gs/postura
payload: {"status":"ruim","distancia_cm":22.3}
```
- Exemplo de log no Flask ao receber MQTT:
```
[MQTT] Mensagem recebida: {'status': 'ok', 'distancia_cm': 45.0}
[WebSocket] Emitindo para clientes: postura_update
```

---

## Estrutura sugerida do repositório
- codigo.ino           → Codigo do wokwi
- diagram.json         → Diagrama de montagem para o wokwi
- app.py               → Aqui tem o codigo que deve ser rodado na VM
- /templates/          → index.html
- README.md

