#!/bin/bash

# Define a URL base para as requisições e o endpoint para limpar o banco
BASE_URL="http://localhost:9999"
PURGE_PAYMENTS_ENDPOINT="${BASE_URL}/purge-payments"

# Envia uma requisição POST para o endpoint de limpeza o banco
curl --location --request POST "$PURGE_PAYMENTS_ENDPOINT" --header 'Content-Type: application/json' 