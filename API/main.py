from flask import Flask, request, jsonify
import psycopg2

app = Flask(__name__)

# Configurações do banco de dados PostgreSQL
db_config = {
    'host': 'localhost',
    'database': 'banco',
    'user': 'arduino',
    'password': '123'
}

def insert_data(sensor_id, presence_status):
    try:
        conn = psycopg2.connect(**db_config)
        cursor = conn.cursor()

        # Inserir os dados na tabela
        query = "INSERT INTO public.status_objetos (sensor_id, presence_status) VALUES (%s, %s)"

        cursor.execute(query, (sensor_id, presence_status))

        conn.commit()
        cursor.close()
        conn.close()
        return True
    except psycopg2.Error as err:
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
