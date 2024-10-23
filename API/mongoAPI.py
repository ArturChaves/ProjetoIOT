from flask import Flask, request, jsonify
from pymongo import MongoClient
from datetime import datetime

app = Flask(__name__)

# Configurações do MongoDB
db_config = {
    'url': 'mongodb+srv://projeto:9yfp5PxEQCjx4UxC@bancoiot.midi9.mongodb.net/?retryWrites=true&w=majority&tls=true',
    'database': 'BancoIOT'
}

# Conectando ao MongoDB
client = MongoClient(db_config['url'])
db = client[db_config['database']]
collection = db['Sensores']

def insert_data(sensor_id, presence_status):
    try:
        # Preparar o documento com timestamp
        document = {
            'sensor_id': sensor_id,
            'presence_status': presence_status,
            'timestamp': datetime.utcnow()  # Hora UTC
        }

        # Inserir os dados na coleção
        collection.insert_one(document)
        return True
    except Exception as err:
        print(f"Erro: {err}")
        return False

@app.route('/dados', methods=['POST'])
def receive_data():
    if request.method == 'POST':
        data = request.json
        sensor_id = data.get('sensor_id')
        presence_status = data.get('presence_status')

        if not sensor_id or presence_status is None:
            return jsonify({'error': 'Dados inválidos'}), 400

        if insert_data(sensor_id, presence_status):
            return jsonify({'message': 'Dados recebidos com sucesso!'}), 201
        else:
            return jsonify({'error': 'Erro ao inserir no banco de dados'}), 500

    return jsonify({'error': 'Método não permitido'}), 405

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
