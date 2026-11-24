/*******************************************************************************
* Estação Meteorológica Web - ESP8266 + BMP180 (GY-68)
* Modo STA: ESP conecta na rede Wi-Fi da sua rede
* Interface web acessível pelo IP do ESP
* Envia dados para servidor Python via HTTP POST
*******************************************************************************/

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>

// ==========================================
// CONFIGURAÇÕES DA REDE WI-FI
// ==========================================
const char* SSID = “sua-rede";        // Nome da sua rede Wi-Fi
const char* SENHA = "sua-senha";      // Senha da sua rede Wi-Fi

// ==========================================
// CONFIGURAÇÕES DO SERVIDOR PYTHON
// ==========================================
const char* SERVIDOR_IP = "192.168.x.x";    // IP do seu notebook na rede
const int SERVIDOR_PORTA = 5000;

// ==========================================
// VARiÁVEIS GLOBAIS
// ==========================================
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const uint8_t PIN_LED = 2;
Adafruit_BMP085 bmp;
float MINHA_ALTITUDE = 1020.0;
const char *ALIAS_TEMP = "temp";
const char *ALIAS_PRESSAO = "pressao";
const char *ALIAS_ALTITUDE = "altitude";
const char *ALIAS_NOVA_ALTITUDE = "novaAltitude";

const uint32_t TEMPO_ATUALIZACAO_DADOS = 2000;
uint32_t timeout_dados;

// ==========================================
// PÁGINA HTML
// ==========================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Estação Meteorológica - ESP8266</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no"/>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: #0a0e27;
            color: #fff;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            background: linear-gradient(145deg, #141a3a, #0d1230);
            border-radius: 30px;
            padding: 40px 30px;
            max-width: 550px;
            width: 100%;
            box-shadow: 0 20px 60px rgba(0,0,0,0.8);
            border: 1px solid rgba(255,255,255,0.05);
        }
        h1 {
            text-align: center;
            font-size: 24px;
            font-weight: 300;
            letter-spacing: 3px;
            color: #8a9bff;
            margin-bottom: 20px;
            text-transform: uppercase;
            border-bottom: 1px solid rgba(138, 155, 255, 0.2);
            padding-bottom: 15px;
        }
        .config-section {
            background: rgba(255,255,255,0.03);
            border-radius: 16px;
            padding: 15px 20px;
            margin-bottom: 25px;
            border: 1px solid rgba(255,255,255,0.05);
            display: flex;
            align-items: center;
            justify-content: space-between;
            flex-wrap: wrap;
            gap: 10px;
        }
        .config-section label {
            font-size: 14px;
            color: #8a9bff;
            letter-spacing: 0.5px;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .config-section input {
            background: rgba(255,255,255,0.07);
            border: 1px solid rgba(138, 155, 255, 0.2);
            border-radius: 10px;
            padding: 8px 14px;
            color: #fff;
            font-size: 16px;
            width: 90px;
            text-align: center;
            outline: none;
            transition: 0.3s;
        }
        .config-section input:focus {
            border-color: #8a9bff;
            box-shadow: 0 0 20px rgba(138, 155, 255, 0.15);
        }
        .config-section .unit-label {
            color: #6a7bc7;
            font-size: 14px;
            margin-left: -5px;
        }
        .config-section button {
            background: linear-gradient(135deg, #6a7bc7, #4a5a9a);
            border: none;
            border-radius: 10px;
            padding: 8px 20px;
            color: #fff;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: 0.3s;
            letter-spacing: 1px;
        }
        .config-section button:hover {
            transform: scale(1.02);
            box-shadow: 0 0 25px rgba(106, 123, 199, 0.3);
        }
        .config-status {
            font-size: 12px;
            color: #6a7bc7;
            width: 100%;
            text-align: center;
            margin-top: 5px;
        }
        .sensor-grid {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 15px;
            margin-bottom: 25px;
        }
        .card {
            background: rgba(255,255,255,0.03);
            border-radius: 16px;
            padding: 20px 10px;
            text-align: center;
            border: 1px solid rgba(255,255,255,0.05);
            transition: 0.3s;
        }
        .card:hover {
            background: rgba(255,255,255,0.07);
            border-color: rgba(138, 155, 255, 0.3);
        }
        .card .icon { font-size: 28px; margin-bottom: 8px; }
        .card .label {
            font-size: 11px;
            text-transform: uppercase;
            letter-spacing: 1px;
            color: #6a7bc7;
            margin-bottom: 6px;
        }
        .card .value {
            font-size: 26px;
            font-weight: 600;
            color: #fff;
        }
        .card .unit {
            font-size: 14px;
            color: #6a7bc7;
            margin-left: 2px;
        }
        .status-bar {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px 18px;
            background: rgba(255,255,255,0.03);
            border-radius: 12px;
            border: 1px solid rgba(255,255,255,0.05);
        }
        .status-dot {
            display: inline-block;
            width: 10px;
            height: 10px;
            border-radius: 50%;
            margin-right: 8px;
        }
        .status-dot.online {
            background: #00ff88;
            box-shadow: 0 0 12px #00ff8866;
        }
        .status-dot.offline {
            background: #ff4444;
            box-shadow: 0 0 12px #ff444466;
        }
        .status-text {
            font-size: 13px;
            color: #8a9bff;
            letter-spacing: 1px;
        }
        .status-time {
            font-size: 12px;
            color: #4a5a9a;
        }
        .footer {
            text-align: center;
            margin-top: 20px;
            font-size: 11px;
            color: #3a4a7a;
            letter-spacing: 1px;
        }
        .ip-info {
            text-align: center;
            margin-bottom: 15px;
            padding: 8px;
            background: rgba(0,255,136,0.05);
            border-radius: 10px;
            border: 1px solid rgba(0,255,136,0.1);
            font-size: 13px;
            color: #00ff88;

        }        @media (max-width: 480px) {
            .sensor-grid {
                grid-template-columns: 1fr 1fr 1fr;
                gap: 8px;
            }
            .card .value { font-size: 20px; }
            .container { padding: 25px 15px; }
            .config-section { flex-direction: column; align-items: stretch; }
            .config-section label { justify-content: center; }
            .config-section input { width: 100%; }
            .config-section button { width: 100%; }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌤️ Estação Meteorológica</h1>
        
        <div class="ip-info" id="ipInfo">🔗 Conectado Ã  rede local</div>
        
        <div class="config-section">
            <label>📊 Pressão no nível do mar:
                <input type="number" id="altitudeInput" step="0.5" value="1020">
                <span class="unit-label">hPa</span>
            </label>
            <button id="setAltitudeBtn">ATUALIZAR</button>
            <div class="config-status" id="configStatus">Ajuste a pressão e clique em ATUALIZAR</div>
        </div>
        
        <div class="sensor-grid">
            <div class="card">
                <div class="icon">🌡️</div>
                <div class="label">Temperatura</div>
                <div class="value"><span id="temp">--</span><span class="unit">°C</span></div>
            </div>
            <div class="card">
                <div class="icon">📊</div>
                <div class="label">Pressão</div>
                <div class="value"><span id="pressao">--</span><span class="unit">hPa</span></div>
            </div>
            <div class="card">
                <div class="icon">🏔️</div>
                <div class="label">Altitude</div>
                <div class="value"><span id="altitude">--</span><span class="unit">m</span></div>
            </div>
        </div>

        <div class="status-bar">
            <div>
                <span class="status-dot online" id="statusDot"></span>
                <span class="status-text" id="statusText">Online</span>
            </div>
            <div class="status-time" id="lastUpdate">Aguardando dados...</div>
        </div>

        <div class="footer">
            ESP8266 + BMP180 &bull; Dados em tempo real via WebSocket
        </div>
    </div>

    <script>
        var connection = new WebSocket(`ws://${window.location.hostname}/ws`);
        
        connection.onopen = function () {
            console.log('Conectado ao WebSocket');
            document.getElementById('statusText').innerText = 'Online';
            document.getElementById('statusDot').className = 'status-dot online';
            document.getElementById('configStatus').innerText = 'Conectado. Ajuste a altitude!';
            document.getElementById('configStatus').style.color = '#00ff88';
        };
        
        connection.onerror = function (error) {
            console.log('Erro: ' + error);
            document.getElementById('statusText').innerText = 'Erro!';
            document.getElementById('statusDot').className = 'status-dot offline';
        };
        
        connection.onclose = function () {
            console.log('Desconectado');
            document.getElementById('statusText').innerText = 'Offline';
            document.getElementById('statusDot').className = 'status-dot offline';
        };
        
        connection.onmessage = function (e) {
            console.log('Dados: ' + e.data);
            try {
                const data = JSON.parse(e.data);
                
                if (data.temp !== undefined) {
                    document.getElementById('temp').innerText = data.temp.toFixed(1);
                }
                if (data.pressao !== undefined) {
                    document.getElementById('pressao').innerText = data.pressao.toFixed(1);
                }
                if (data.altitude !== undefined) {
                    document.getElementById('altitude').innerText = data.altitude.toFixed(1);
                }
                
                if (data.altitudeConfig !== undefined) {
                    document.getElementById('altitudeInput').value = data.altitudeConfig.toFixed(1);
                    document.getElementById('configStatus').innerText = 'Altitude ajustada para ' + data.altitudeConfig.toFixed(1) + ' m ✓';
                    document.getElementById('configStatus').style.color = '#00ff88';
                }
                
                var now = new Date();
                var timeStr = now.toLocaleTimeString('pt-BR', {hour: '2-digit', minute: '2-digit', second: '2-digit'});
                document.getElementById('lastUpdate').innerText = 'Ãšltima atualizaÃ§Ã£o: ' + timeStr;
                
                // Exibe o IP na interface
                document.getElementById('ipInfo').innerHTML = '🌐 Acesse este ESP em: <strong>' + window.location.hostname + '</strong> | Dados enviados para o servidor';
                
            } catch (error) {
                console.log('Erro ao processar: ' + error);
            }
        };

        document.getElementById('setAltitudeBtn').addEventListener('click', function() {
            var novaAltitude = parseFloat(document.getElementById('altitudeInput').value);
            if (isNaN(novaAltitude) || novaAltitude < -500 || novaAltitude > 10000) {
                document.getElementById('configStatus').innerText = 'Valor invÃ¡lido! Use entre -500 e 10000';
                document.getElementById('configStatus').style.color = '#ff4444';
                return;
            }
            
            if (connection.readyState === WebSocket.OPEN) {
                var msg = { novaAltitude: novaAltitude };
                connection.send(JSON.stringify(msg));
                document.getElementById('configStatus').innerText = 'Enviando...';
                document.getElementById('configStatus').style.color = '#ffaa44';
            } else {
                document.getElementById('configStatus').innerText = 'Sem conexÃ£o com o ESP!';
                document.getElementById('configStatus').style.color = '#ff4444';
            }
        });

        document.getElementById('altitudeInput').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                document.getElementById('setAltitudeBtn').click();
            }
        });

        setInterval(function() {
            if (connection.readyState === WebSocket.CLOSED) {
                console.log('Reconectando...');
                connection = new WebSocket(`ws://${window.location.hostname}/ws`);
            }
        }, 5000);
    </script>
</body>
</html>
)rawliteral";

// ==========================================
// PROTÃ“TIPOS
// ==========================================
void configurar_servidor_web(void);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t length);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t length);
void lerSensor(float *temp, float *press, float *alt);
void enviarParaServidor(float temp, float press, float alt);
void enviarDadosWebSocket(float temp, float press, float alt);

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("========================================");
  Serial.println("Estacao Meteorologica - ESP8266 + BMP180");
  Serial.println("Modo STA - Conecta a rede Wi-Fi");
  Serial.println("========================================\n");

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // ==========================================
  // INICIALIZA O SENSOR
  // ==========================================
  if (!bmp.begin()) {
    Serial.println("ERRO: Sensor BMP180 nao encontrado!");
    while (1) {
      digitalWrite(PIN_LED, HIGH);
      delay(200);
      digitalWrite(PIN_LED, LOW);
      delay(200);
    }
  }
  Serial.println("Sensor BMP180 inicializado com sucesso!");

  // ==========================================
  // CONECTA A REDE WI-FI (MODO STA)
  // ==========================================
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SENHA);
  
  Serial.print("Conectando a rede Wi-Fi");
  
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
    digitalWrite(PIN_LED, !digitalRead(PIN_LED));
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" CONECTADO!");
    Serial.print("IP do ESP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(PIN_LED, HIGH);
  } else {
    Serial.println(" FALHOU!");
    Serial.println("Verifique SSID e SENHA no codigo.");
    while (1) {
      digitalWrite(PIN_LED, HIGH);
      delay(200);
      digitalWrite(PIN_LED, LOW);
      delay(200);
    }
  }

  // ==========================================
  // INICIA O SERVIDOR WEB
  // ==========================================
  configurar_servidor_web();
  server.begin();
  Serial.println("Servidor web iniciado!");
  Serial.printf("Acesse: http://%s\n", WiFi.localIP().toString().c_str());
  Serial.println("========================================\n");

  timeout_dados = millis() + TEMPO_ATUALIZACAO_DADOS;
}

// ==========================================
// LOOP
// ==========================================
void loop() {
  if (millis() > timeout_dados) {
        if (ws.count() > 0) {
            float temperatura, pressao, altitude;
            lerSensor(&temperatura, &pressao, &altitude);
            
            // Chama a função corrigida
            enviarDadosWebSocket(temperatura, pressao, altitude);
            
            digitalWrite(PIN_LED, HIGH);
            delay(50);
            digitalWrite(PIN_LED, LOW);
        }
        timeout_dados = millis() + TEMPO_ATUALIZACAO_DADOS;
  }
}

// ==========================================
// FUNÃ‡Ã•ES
// ==========================================

void lerSensor(float *temp, float *press, float *alt) {
  *temp = bmp.readTemperature();
  float pressao_absoluta = bmp.readPressure() / 100.0;
  *press = pressao_absoluta;
  
  float pressaoNivelMar = bmp.readAltitude(MINHA_ALTITUDE * 100);
  *alt = pressaoNivelMar;
  
  Serial.printf("Temp=%.1f°C | Press=%.1fhPa | Alt=%.1fm\n", *temp, *press, *alt);
}

void enviarDadosWebSocket(float temperatura, float pressao, float altitude) {
    if (ws.count() > 0) {
        char mensagem[128];
        
       
            JsonDocument json;
            json[ALIAS_TEMP] = temperatura;
            json[ALIAS_PRESSAO] = pressao;
            json[ALIAS_ALTITUDE] = altitude;
            serializeJson(json, mensagem, sizeof(mensagem));
      
           
        
        ws.textAll(mensagem);
        
        Serial.printf("Temp=%.1f°C | Press=%.1fhPa | Alt=%.1fm\n", 
                      temperatura, pressao, altitude);
    }
}

// --------------------------------------------------
// Manipulador para mensagens WebSocket
void handleWebSocketMessage(void *arg, uint8_t *data, size_t length) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == length && info->opcode == WS_TEXT) {
        data[length] = 0;
        
        bool processado = false;
        
            JsonDocument json;
            DeserializationError erro = deserializeJson(json, data, length);
            
            if (!erro && json.containsKey(ALIAS_NOVA_ALTITUDE)) {
                float novaAltitude = json[ALIAS_NOVA_ALTITUDE];
                processado = true;
                
                if (novaAltitude >= -500 && novaAltitude <= 10000) {
                    MINHA_ALTITUDE = novaAltitude;
                    Serial.printf("Altitude ajustada para: %.1f m\n", MINHA_ALTITUDE);
                    
                    JsonDocument jsonResposta;
                    jsonResposta["altitudeConfig"] = MINHA_ALTITUDE;
                    char resposta[64];
                    serializeJson(jsonResposta, resposta, sizeof(resposta));
                    ws.textAll(resposta);
                    
                    for (int i = 0; i < 3; i++) {
                        digitalWrite(PIN_LED, HIGH);
                        delay(100);
                        digitalWrite(PIN_LED, LOW);
                        delay(100);
                    }
                } else {
                    Serial.printf("Valor de altitude invalido: %.1f\n", novaAltitude);
                }
            }
        
    }
}

// --------------------------------------------------
// Manipulador dos eventos do WebSocket
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t length) {
    switch (type) {
        case WS_EVT_CONNECT: {
            Serial.printf("Cliente #%u conectado\n", client->id());
            digitalWrite(PIN_LED, HIGH);
            
            char mensagem[64];
            
           
                JsonDocument json;
                json["altitudeConfig"] = MINHA_ALTITUDE;
                serializeJson(json, mensagem, sizeof(mensagem));
            
            client->text(mensagem);
            break;
        }
          
        case WS_EVT_DISCONNECT: {
            Serial.printf("Cliente #%u desconectado\n", client->id());
            if (ws.count() == 0) {
                digitalWrite(PIN_LED, LOW);
            }
            break;
        }
          
        case WS_EVT_DATA: {
            handleWebSocketMessage(arg, data, length);
            break;
        }
          
        case WS_EVT_PONG:
        case WS_EVT_ERROR: {
            break;
        }
    }
}


