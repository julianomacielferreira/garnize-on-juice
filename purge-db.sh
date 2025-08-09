#!/bin/bash

BASE_URL="http://localhost:9999"

PURGE_PAYMENTS_ENDPOINT="${BASE_URL}/purge-payments"

curl --location --request POST "$PURGE_PAYMENTS_ENDPOINT" --header 'Content-Type: application/json' 