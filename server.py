


#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from flask import Flask, render_template_string, request, jsonify, Response, send_file
from flask_cors import CORS
import csv
import os
import json
from datetime import datetime
import time
import threading

app = Flask(__name__)
CORS(app)

ARQUIVO_CSV = "dados.csv"
ULTIMO_DADO = {}
HISTORICO = []
ULTIMO_TIMESTAMP = ""

# ==========================================
# FILA DE EVENTOS SSE
# ==========================================
class EventoSSE:
    def __init__(self):
        self.clientes = []
    
    def adicionar_cliente(self):
        self.clientes.append(True)
        return len(self.clientes) - 1
    
    def remover_cliente(self, idx):
        if idx < len(self.clientes):
            self.clientes[idx] = False
    
    def emitir(self, dados):
        for i, ativo in enumerate(self.clientes):
            if ativo:
                try:
                    yield f"data: {json.dumps(dados)}\n\n"
                except:
                    self.clientes[i] = False

sse = EventoSSE()

# ==========================================
# FUNÇÕES
# ==========================================

def salvar_dados(temperatura, pressao, altitude):
    global ULTIMO_TIMESTAMP
    
    arquivo_existe = os.path.isfile(ARQUIVO_CSV)
    
    with open(ARQUIVO_CSV, mode='a', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        if not arquivo_existe:
            writer.writerow(['timestamp', 'temperatura', 'pressao', 'altitude'])
        
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        writer.writerow([timestamp, f"{temperatura:.2f}", f"{pressao:.2f}", f"{altitude:.2f}"])
        
        ULTIMO_TIMESTAMP = timestamp
        ULTIMO_DADO.update({
            'timestamp': timestamp,
            'temperatura': temperatura,
            'pressao': pressao,
            'altitude': altitude
        })
        
        HISTORICO.append({
            'timestamp': timestamp,
            'temperatura': temperatura,
            'pressao': pressao,
            'altitude': altitude
        })
        if len(HISTORICO) > 100:
            HISTORICO.pop(0)
    
    print(f"✅ {timestamp} | Temp={temperatura:.1f}°C | Press={pressao:.1f}hPa | Alt={altitude:.1f}m")

# ==========================================
# ROTAS
# ==========================================

@app.route('/dados', methods=['POST'])
def receber_dados():
    try:
        dados = request.json
        threading.Thread(target=salvar_dados, args=(
            float(dados.get('temp', 0)),
            float(dados.get('pressao', 0)),
            float(dados.get('altitude', 0))
        )).start()
        return jsonify({"status": "ok"})
    except Exception as e:
        return jsonify({"status": "erro"}), 400

@app.route('/stream')
def stream():
    """Endpoint Server-Sent Events"""
    def gerar():
        idx = sse.adicionar_cliente()
        try:
            yield f"data: {json.dumps({'historico': HISTORICO, 'ultimo': ULTIMO_DADO})}\n\n"
            while True:
                time.sleep(0.5)
                if ULTIMO_DADO:
                    yield f"data: {json.dumps({'novo': ULTIMO_DADO, 'historico': HISTORICO})}\n\n"
        except:
            sse.remover_cliente(idx)
    
    return Response(gerar(), mimetype="text/event-stream")

@app.route('/')
def home():
    return render_template_string("""
    <!DOCTYPE html>
    <html>
    <head>
        <title>Estação Meteorológica</title>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <style>
            * { margin: 0; padding: 0; box-sizing: border-box; }
            body {
                font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
                background: #0a0e27;
                color: #fff;
                padding: 20px;
                min-height: 100vh;
            }
            .container {
                max-width: 1100px;
                margin: 0 auto;
            }
            .header {
                text-align: center;
                padding: 20px 0 30px;
                background: linear-gradient(135deg, #141a3a, #0d1230);
                border-radius: 20px;
                margin-bottom: 30px;
                border: 1px solid rgba(138, 155, 255, 0.1);
            }
            .header h1 {
                font-size: 28px;
                font-weight: 300;
                letter-spacing: 4px;
                background: linear-gradient(90deg, #8a9bff, #6a7bc7);
                -webkit-background-clip: text;
                -webkit-text-fill-color: transparent;
            }
            .header .subtitle {
                color: #4a5a9a;
                font-size: 13px;
                letter-spacing: 2px;
                margin-top: 6px;
            }
            .sensor-grid {
                display: grid;
                grid-template-columns: repeat(3, 1fr);
                gap: 20px;
                margin-bottom: 30px;
            }
            .card {
                background: linear-gradient(145deg, #141a3a, #0d1230);
                border-radius: 16px;
                padding: 25px 20px;
                text-align: center;
                border: 1px solid rgba(138, 155, 255, 0.08);
                box-shadow: 0 8px 32px rgba(0,0,0,0.4);
            }
            .card .icon { font-size: 30px; margin-bottom: 6px; }
            .card .label {
                font-size: 11px;
                text-transform: uppercase;
                letter-spacing: 2px;
                color: #6a7bc7;
                margin-bottom: 8px;
            }
            .card .value {
                font-size: 34px;
                font-weight: 700;
            }
            .card .value.temp { background: linear-gradient(90deg, #ff6b6b, #ff9f43); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
            .card .value.press { background: linear-gradient(90deg, #4ecdc4, #45b7d1); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
            .card .value.alt { background: linear-gradient(90deg, #a29bfe, #6c5ce7); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
            .card .unit {
                font-size: 16px;
                color: #6a7bc7;
                margin-left: 4px;
                -webkit-text-fill-color: #6a7bc7;
            }
            .card .time {
                font-size: 11px;
                color: #3a4a7a;
                margin-top: 8px;
            }
            .chart-container {
                background: linear-gradient(145deg, #141a3a, #0d1230);
                border-radius: 16px;
                padding: 20px;
                border: 1px solid rgba(138, 155, 255, 0.08);
                box-shadow: 0 8px 32px rgba(0,0,0,0.4);
                margin-bottom: 20px;
            }
            .chart-container .chart-title {
                font-size: 13px;
                font-weight: 300;
                letter-spacing: 2px;
                color: #6a7bc7;
                margin-bottom: 12px;
                display: flex;
                justify-content: space-between;
            }
            .chart-container .chart-title span { color: #3a4a7a; }
            .chart-container canvas { max-height: 200px; width: 100% !important; }
            .status-bar {
                background: rgba(255,255,255,0.02);
                border-radius: 12px;
                padding: 12px 20px;
                display: flex;
                justify-content: space-between;
                align-items: center;
                border: 1px solid rgba(138, 155, 255, 0.05);
                margin-top: 10px;
                flex-wrap: wrap;
                gap: 10px;
            }
            .status-bar .status-dot.online { color: #00ff88; }
            .status-bar .status-dot.offline { color: #ff4444; }
            .status-bar .status-text { color: #6a7bc7; font-size: 13px; }
            .status-bar .last-update { color: #3a4a7a; font-size: 12px; }
            @media (max-width: 650px) {
                .sensor-grid { grid-template-columns: 1fr 1fr; }
                .header h1 { font-size: 20px; }
                .card .value { font-size: 26px; }
            }
            @media (max-width: 420px) {
                .sensor-grid { grid-template-columns: 1fr; }
            }
        </style>
    </head>
    <body>
        <div class="container">
            <div class="header">
                <h1>🌤️ Estação Meteorológica</h1>
                <div class="subtitle">ESP8266 + BMP180 • Dados em tempo real</div>
            </div>

            <div class="sensor-grid">
                <div class="card">
                    <div class="icon">🌡️</div>
                    <div class="label">Temperatura</div>
                    <div class="value temp"><span id="temp">--</span><span class="unit">°C</span></div>
                    <div class="time" id="tempTime">Aguardando dados...</div>
                </div>
                <div class="card">
                    <div class="icon">📊</div>
                    <div class="label">Pressão</div>
                    <div class="value press"><span id="pressao">--</span><span class="unit">hPa</span></div>
                    <div class="time" id="pressTime">Aguardando dados...</div>
                </div>
                <div class="card">
                    <div class="icon">🏔️</div>
                    <div class="label">Altitude</div>
                    <div class="value alt"><span id="altitude">--</span><span class="unit">m</span></div>
                    <div class="time" id="altTime">Aguardando dados...</div>
                </div>
            </div>

            <!-- Gráfico 1: Temperatura x Tempo -->
            <div class="chart-container">
                <div class="chart-title">🌡️ Temperatura x Tempo <span id="tempStats">--</span></div>
                <canvas id="graficoTemp"></canvas>
            </div>

            <!-- Gráfico 2: Pressão x Tempo -->
            <div class="chart-container">
                <div class="chart-title">📊 Pressão x Tempo <span id="pressStats">--</span></div>
                <canvas id="graficoPressao"></canvas>
            </div>

            <!-- Gráfico 3: Pressão x Temperatura (SCATTER) -->
            <div class="chart-container">
                <div class="chart-title">📈 Pressão x Temperatura <span id="scatterStats">--</span></div>
                <canvas id="graficoScatter"></canvas>
            </div>

            <div class="status-bar">
                <div>
                    <span class="status-dot online" id="statusDot">●</span>
                    <span class="status-text" id="statusText">Conectado</span>
                </div>
                <div class="last-update" id="lastUpdate">Aguardando dados...</div>
            </div>
        </div>

        <script>
            var graficoTemp, graficoPressao, graficoScatter;

            // ==========================================
            // CRIA GRÁFICO DE LINHA (Temperatura x Tempo)
            // ==========================================
            function criarGraficoLinha(ctx, label, cor, corBg) {
                return new Chart(ctx, {
                    type: 'line',
                    data: {
                        labels: [],
                        datasets: [{
                            label: label,
                            data: [],
                            borderColor: cor,
                            backgroundColor: corBg,
                            borderWidth: 2.5,
                            fill: true,
                            tension: 0.4,
                            pointRadius: 3,
                            pointBackgroundColor: cor,
                            pointBorderColor: '#0a0e27',
                            pointBorderWidth: 1.5
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: true,
                        animation: { duration: 400 },
                        plugins: {
                            legend: {
                                labels: {
                                    color: '#6a7bc7',
                                    font: { size: 11, weight: '300' },
                                    boxWidth: 12,
                                    padding: 15
                                }
                            }
                        },
                        scales: {
                            y: {
                                ticks: { color: '#4a5a9a', font: { size: 10 }, maxTicksLimit: 6 },
                                grid: { color: 'rgba(74,90,154,0.1)', drawBorder: false }
                            },
                            x: {
                                ticks: { color: '#3a4a7a', font: { size: 9 }, maxTicksLimit: 12, maxRotation: 30, minRotation: 20 },
                                grid: { display: false }
                            }
                        }
                    }
                });
            }

            // ==========================================
            // CRIA GRÁFICO DE DISPERSÃO (Pressão x Temperatura)
            // ==========================================
            function criarGraficoScatter(ctx) {
                return new Chart(ctx, {
                    type: 'scatter',
                    data: {
                        datasets: [{
                            label: 'Pressão x Temperatura',
                            data: [],
                            backgroundColor: 'rgba(78, 205, 196, 0.6)',
                            borderColor: '#4ecdc4',
                            borderWidth: 1.5,
                            pointRadius: 5,
                            pointHoverRadius: 8
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: true,
                        animation: { duration: 400 },
                        plugins: {
                            legend: {
                                labels: {
                                    color: '#6a7bc7',
                                    font: { size: 11, weight: '300' },
                                    boxWidth: 12,
                                    padding: 15
                                }
                            }
                        },
                        scales: {
                            x: {
                                title: {
                                    display: true,
                                    text: 'Temperatura (K)',
                                    color: '#6a7bc7',
                                    font: { size: 11 }
                                },
                                ticks: { color: '#4a5a9a', font: { size: 10 } },
                                grid: { color: 'rgba(74,90,154,0.1)', drawBorder: false }
                            },
                            y: {
                                title: {
                                    display: true,
                                    text: 'Pressão (hPa)',
                                    color: '#6a7bc7',
                                    font: { size: 11 }
                                },
                                ticks: { color: '#4a5a9a', font: { size: 10 } },
                                grid: { color: 'rgba(74,90,154,0.1)', drawBorder: false }
                            }
                        }
                    }
                });
            }

            // ==========================================
            // ATUALIZA ESTATÍSTICAS
            // ==========================================
            function atualizarStats(data, tipo) {
                if (!data || data.length === 0) return '--';
                var valores = data.map(d => d[tipo]);
                var min = Math.min(...valores);
                var max = Math.max(...valores);
                var media = valores.reduce((a,b) => a + b, 0) / valores.length;
                return `Min ${min.toFixed(1)} • Máx ${max.toFixed(1)} • Média ${media.toFixed(1)}`;
            }

            function atualizarCorrelacao(historico) {
                if (!historico || historico.length < 2) return 'Aguardando mais dados...';
                var temps = historico.map(d => d.temperatura);
                var press = historico.map(d => d.pressao);
                // Correlação de Pearson simplificada
                var n = temps.length;
                var sumX = temps.reduce((a,b) => a + b, 0);
                var sumY = press.reduce((a,b) => a + b, 0);
                var sumXY = temps.reduce((a,b,i) => a + b * press[i], 0);
                var sumX2 = temps.reduce((a,b) => a + b * b, 0);
                var sumY2 = press.reduce((a,b) => a + b * b, 0);
                var corr = (n * sumXY - sumX * sumY) / Math.sqrt((n * sumX2 - sumX * sumX) * (n * sumY2 - sumY * sumY));
                return `Correlação: ${corr.toFixed(3)}`;
            }

            // ==========================================
            // ATUALIZA TODOS OS GRÁFICOS
            // ==========================================
            function atualizarTudo(historico, ultimo) {
                if (!historico || historico.length === 0) return;
                
                document.getElementById('temp').innerText = ultimo.temperatura.toFixed(1);
                document.getElementById('pressao').innerText = ultimo.pressao.toFixed(1);
                document.getElementById('altitude').innerText = ultimo.altitude.toFixed(1);
                
                var ts = ultimo.timestamp || '--';
                document.getElementById('tempTime').innerText = ts;
                document.getElementById('pressTime').innerText = ts;
                document.getElementById('altTime').innerText = ts;
                document.getElementById('lastUpdate').innerText = 'Última atualização: ' + ts;
                document.getElementById('statusDot').className = 'status-dot online';
                document.getElementById('statusText').innerText = 'Conectado';

                var labels = historico.map(d => d.timestamp.substring(11, 16));
                
                // ========== GRÁFICO 1: Temperatura ==========
                if (!graficoTemp) {
                    graficoTemp = criarGraficoLinha(
                        document.getElementById('graficoTemp').getContext('2d'),
                        'Temperatura (°C)', '#ff6b6b', 'rgba(255,107,107,0.15)'
                    );
                }
                graficoTemp.data.labels = labels;
                graficoTemp.data.datasets[0].data = historico.map(d => d.temperatura);
                graficoTemp.update();

                // ========== GRÁFICO 2: Pressão ==========
                if (!graficoPressao) {
                    graficoPressao = criarGraficoLinha(
                        document.getElementById('graficoPressao').getContext('2d'),
                        'Pressão (hPa)', '#4ecdc4', 'rgba(78,205,196,0.15)'
                    );
                }
                graficoPressao.data.labels = labels;
                graficoPressao.data.datasets[0].data = historico.map(d => d.pressao);
                graficoPressao.update();

                // ========== GRÁFICO 3: Scatter Pressão x Temperatura ==========
                if (!graficoScatter) {
                    graficoScatter = criarGraficoScatter(
                        document.getElementById('graficoScatter').getContext('2d')
                    );
                }
                graficoScatter.data.datasets[0].data = historico.map(d => ({
                    x: d.temperatura + 273.15,
                    y: d.pressao
                }));
                graficoScatter.update();

                // ========== ESTATÍSTICAS ==========
                document.getElementById('tempStats').innerText = '| ' + atualizarStats(historico, 'temperatura');
                document.getElementById('pressStats').innerText = '| ' + atualizarStats(historico, 'pressao');
                document.getElementById('scatterStats').innerText = '| ' + atualizarCorrelacao(historico);
            }

            // ==========================================
            // CONEXÃO SSE (Server-Sent Events)
            // ==========================================
            var eventSource = new EventSource('/stream');

            eventSource.onopen = function() {
                document.getElementById('statusText').innerText = 'Conectado';
                document.getElementById('statusDot').className = 'status-dot online';
            };

            eventSource.onerror = function() {
                document.getElementById('statusText').innerText = 'Erro na conexão';
                document.getElementById('statusDot').className = 'status-dot offline';
            };

            eventSource.onmessage = function(e) {
                try {
                    var data = JSON.parse(e.data);
                    if (data.historico) {
                        atualizarTudo(data.historico, data.ultimo || data.historico[data.historico.length-1]);
                    }
                } catch (err) {
                    console.log('Erro ao processar:', err);
                }
            };
        </script>
    </body>
    </html>
    """)

@app.route('/dados_csv', methods=['GET'])
def baixar_csv():
    if os.path.isfile(ARQUIVO_CSV):
        return send_file(ARQUIVO_CSV, as_attachment=True)
    return jsonify({"erro": "Arquivo não encontrado"}), 404

@app.route('/status', methods=['GET'])
def status():
    num = 0
    if os.path.isfile(ARQUIVO_CSV):
        with open(ARQUIVO_CSV, 'r') as f:
            num = max(0, sum(1 for _ in f) - 1)
    return jsonify({
        "status": "online",
        "dados_coletados": num,
        "historico": len(HISTORICO),
        "ultimo": ULTIMO_DADO
    })

if __name__ == '__main__':
    if not os.path.isfile(ARQUIVO_CSV):
        with open(ARQUIVO_CSV, 'w', newline='', encoding='utf-8') as f:
            csv.writer(f).writerow(['timestamp', 'temperatura', 'pressao', 'altitude'])
    
    print("=" * 50)
    print("🌤️ ESTAÇÃO METEOROLÓGICA - TEMPO REAL (SSE)")
    print("=" * 50)
    print("🌐 http://localhost:5000")
    print("📁 Dados: dados.csv")
    print("📊 Gráficos: Temperatura x Tempo, Pressão x Tempo, Pressão x Temperatura")
    print("=" * 50)
    app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)

