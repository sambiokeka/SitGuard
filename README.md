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


- Wokwi

<img width="531" height="501" alt="image" src="https://github.com/user-attachments/assets/7a2e3f4b-0272-4b3c-aaac-ff192fd86643" />

O projeto mede a distância do usuário até a mesa com o HC-SR04, classifica a postura em três estados ("ok", "ruim", "ausente"), entrega feedback local (LED verde para ok; LED vermelho + buzzer para ruim; LCD mostra texto) e publica um JSON via MQTT para um broker que o backend consome e retransmite ao frontend via WebSockets.

- Frontend

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

1) Hardware (Wokwi / ESP32)  
- Abra o projeto no Wokwi:  
  https://wokwi.com/projects/447550415358261249

- No código `.ino` altere a variável `mqtt_server`:
```cpp
const char* mqtt_server = "SEU_IP_AQUI"; // IP público da sua VM Azure
```

- Compile e rode a simulação no Wokwi. Verifique o console serial para logs MQTT e leitura do sensor.

2) Configuração do Broker Mosquitto (Importante)  
- Observação importante: versões recentes do Mosquitto desabilitam conexões anônimas por padrão. Como o sketch do ESP32 está configurado para conectar sem autenticação (anônimo), precisamos habilitar conexões anônimas temporariamente ou configurar credenciais no ESP32 e no Mosquitto. Abaixo segue a forma rápida (para testes) de habilitar anônimo via um arquivo de configuração.

- Acesse sua VM da Azure:
```bash
ssh seu_usuario@SEU_IP_AQUI
```

- Crie o diretório e o arquivo de configuração:
```bash
mkdir -p ~/mosquitto && cd ~/mosquitto
cat > mosquitto.conf <<'EOF'
listener 1883
allow_anonymous true
EOF
```

- Rode o container Docker do Mosquitto:
```bash
docker run -itd --name mosquitto -p 1883:1883 -v ~/mosquitto/mosquitto.conf:/mosquitto/config/mosquitto.conf eclipse-mosquitto
```

- Verifique os logs (opcional, para ver se está funcionando):
```bash
docker logs -f mosquitto
```

- Observação de segurança: `allow_anonymous true` é prático para testes, mas inseguro em produção. Em ambientes reais, configure usuários com senha (arquivo de senhas), restrinja acessos e habilite TLS.

3) Backend (Flask)  
- Na mesma VM, clone o repositório:
```bash
git clone https://github.com/sambiokeka/SitGuard.git
cd SitGuard
```

- Instale as dependências:
```bash
pip install -r requirements.txt
```
(se não funcionar instale manualmente: `pip install flask flask-socketio paho-mqtt eventlet`)

- Ajuste as configurações no `app.py` se necessário:
  - Endereço do broker MQTT (`mqtt_broker = "localhost"` ou IP da VM).
  - Porta do Flask (p.ex. 8080).

- Rode o servidor Flask:
```bash
python3 app.py
```

- Firewall / NSG (Azure): No Portal do Azure, abra o Grupo de Segurança de Rede (NSG) da sua VM e adicione regras de entrada para:
  - Porta 1883 (Protocolo TCP) → Para o Mosquitto/MQTT  
  - Porta 8080 (Protocolo TCP) → Para o Flask/HTTP

4) Frontend (Acesso)  
- No seu navegador, abra:
```
http://SEU_IP_AQUI:8080
```
- A página `index.html` deve carregar e se conectar automaticamente via Socket.IO, exibindo os dados da simulação Wokwi em tempo real.

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

## Estrutura do repositório
- codigo.ino           → Codigo do wokwi
- diagram.json         → Diagrama de montagem para o wokwi
- app.py               → Aqui tem o codigo que deve ser rodado na VM
- /templates/          → index.html
- requirements.txt
- README.md

