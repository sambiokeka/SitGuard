from flask import Flask, render_template
from flask_socketio import SocketIO
import paho.mqtt.client as mqtt
import json

app = Flask(__name__)
# Chave secreta, pode ser qualquer coisa
app.config['SECRET_KEY'] = 'fiap-gs-2025!' 
socketio = SocketIO(app)

# --- Configuração do MQTT ---
MQTT_BROKER = 'localhost' # O Mosquitto está na mesma máquina
MQTT_PORT = 1883
MQTT_TOPIC = 'fiap/gs/postura'

# --- Conexão MQTT ---
# Esta função será chamada quando o cliente MQTT se conectar
def on_connect(client, userdata, flags, rc):
    print(f"Conectado ao MQTT com resultado: {rc}")
    # Se inscreve no tópico do seu ESP32
    client.subscribe(MQTT_TOPIC)

# Esta função será chamada quando uma MENSAGEM chegar
def on_message(client, userdata, msg):
    print(f"Mensagem recebida do tópico {msg.topic}: {msg.payload.decode()}")
    try:
        # Decodifica a mensagem (ex: {"status": "ok", "distancia_cm": 40.0})
        payload_str = msg.payload.decode()
        data = json.loads(payload_str)
        
        # ENVIA PARA O FRONTEND via WebSocket
        # 'dados_postura' é o nome do "evento" que o JS vai ouvir
        socketio.emit('dados_postura', data)
        print("Dados enviados para o frontend via WebSocket.")
        
    except Exception as e:
        print(f"Erro ao processar mensagem: {e}")

# Configura e inicia o cliente MQTT
client_mqtt = mqtt.Client()
client_mqtt.on_connect = on_connect
client_mqtt.on_message = on_message
client_mqtt.connect(MQTT_BROKER, MQTT_PORT, 60)
# Inicia o loop do MQTT em uma thread separada
client_mqtt.loop_start()

# --- Rotas do Servidor Web (Flask) ---
@app.route('/')
def index():
    # Envia o arquivo index.html para o navegador
    return render_template('index.html')

if __name__ == '__main__':
    # Roda o servidor Flask/SocketIO
    # '0.0.0.0' torna o servidor visível publicamente
    # 'port=80' é a porta HTTP padrão
    socketio.run(app, host='0.0.0.0', port=8080, debug=True, allow_unsafe_werkzeug=True)