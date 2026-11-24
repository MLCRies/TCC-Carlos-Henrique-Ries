Configuração do servidor python (computador local linux)

Instalar o Python<br>
O servidor que gera os gráficos é escrito em Python.
Instalar o Python 3 e o Pip.
Criar um ambiente virtual (venv) para isolar as dependências.
Instalar os pacotes: flask, flask-cors, chart.js (servido via CDN).
Cria uma pasta para o projeto e entra nela
mkdir -p ~/estacao_meteorologica
cd ~/estacao_meteorologica
Cria um ambiente virtual Python (venv). Isso isola as bibliotecas do projeto, evitando conflitos.
python3 -m venv venv
Ativa o ambiente virtual. O terminal mostrará "(venv)" no início da linha.
source venv/bin/activate
Instala as bibliotecas necessárias dentro do ambiente virtual.
Flask: Framework web para criar o servidor.
flask-cors: Permite que o servidor aceite requisições de outras origens.
plotly: Biblioteca para criar gráficos interativos (usada para gerar o HTML dos gráficos).
pandas: Usada para manipular os dados e gerar os gráficos.
pip install flask flask-cors plotly pandas
Cria o arquivo principal do servidor. 
nano server.py
(Após colar o código Python, salve com Ctrl+O, Enter, e saia com Ctrl+X)
Roda o servidor. Como o ambiente virtual está ativo, o comando 'python' usa o Python correto.
python server.py
