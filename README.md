Configuração do servidor python (computador local linux) utilizando Terminal Ctrl + Alt + T

Instalar o Python<br>
O servidor que gera os gráficos é escrito em Python.<br>
Instalar o Python 3 e o Pip.<br>
Criar um ambiente virtual (venv) para isolar as dependências.<br>
Instalar os pacotes: flask, flask-cors, chart.js (servido via CDN).<br>
Cria uma pasta para o projeto e entra nela<br>
mkdir -p ~/estacao_meteorologica<br>
cd ~/estacao_meteorologica<br>
Cria um ambiente virtual Python (venv). Isso isola as bibliotecas do projeto, evitando conflitos.<br>
python3 -m venv venv<br>
Ativa o ambiente virtual. O terminal mostrará "(venv)" no início da linha.<br>
source venv/bin/activate<br>
Instala as bibliotecas necessárias dentro do ambiente virtual.<br>
Flask: Framework web para criar o servidor.<br>
flask-cors: Permite que o servidor aceite requisições de outras origens.<br>
plotly: Biblioteca para criar gráficos interativos (usada para gerar o HTML dos gráficos).<br>
pandas: Usada para manipular os dados e gerar os gráficos.<br>
pip install flask flask-cors plotly pandas<br>
Cria o arquivo principal do servidor. <br>
nano server.py<br>
(Após colar o código Python, salve com Ctrl+O, Enter, e saia com Ctrl+X)<br>
Roda o servidor. Como o ambiente virtual está ativo, o comando 'python' usa o Python correto.<br>
python server.py<br>
