#!/bin/bash

# Script para realizar requisições simultâneas de pagamento e resumo de pagamentos
# para testar a capacidade de carga do servidor.

# Realiza $NUM_REQUISICOES requisições de pagamento para o endpoint $PAYMENTS_ENDPOINT
# e requisições de resumo de pagamentos para o endpoint $PAYMENTS_SUMMARY_ENDPOINT,
# com um intervalo de 0.05 segundos entre cada requisição.

# O script aguarda todas as requisições em segundo plano terminarem antes de sair.
NUM_REQUISICOES=100

BASE_URL="http://localhost:9999"

PAYMENTS_ENDPOINT="${BASE_URL}/payments"

PAYMENTS_SUMMARY_ENDPOINT="${BASE_URL}/payments-summary"

PAYLOAD='{
  "correlationId": "4a7901b8-7d26-4d9d-aa19-4dc1c7cf60b3",
  "amount": 19.90
}'

FROM_DATE="2025-08-09T00%3A15%3A08.174Z"
TO_DATE="2025-08-09T00%3A15%3A08.174Z"

for i in $(seq 1 $NUM_REQUISICOES); do
  curl --location --request POST "$PAYMENTS_ENDPOINT" \
    --header 'Content-Type: application/json' \
    --data "$PAYLOAD" &
  
  sleep 0.05
  
  curl --location --request GET "$PAYMENTS_SUMMARY_ENDPOINT?from=$FROM_DATE&to=$TO_DATE" > /dev/null &
  
  # Aguarde um pouco antes da próxima requisição
  sleep 0.05
done

# Aguarde todas as requisições em segundo plano terminarem
wait
