#!/bin/bash

# Loop que será executado 100 vezes
# Envia uma requisição POST para o endpoint /payments
# Define o cabeçalho Content-Type como application/json
# Define o corpo da requisição como um JSON
# Executa o comando em segundo plano, permitindo que o loop continue
# Pausa a execução do loop por 0,05 segundos antes de enviar a próxima requisição 
for i in {1..100}; do curl --location 'http://localhost:9999/payments' \
 --header 'Content-Type: application/json'  \
 --data '{
            "correlationId": "4a7901b8-7d26-4d9d-aa19-4dc1c7cf60b3",
            "amount": 19.90
    }' & sleep 0.05; 
done