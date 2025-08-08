#!/bin/bash

# Define a variável DEBUG com valor 0, que será usada para determinar se o modo de depuração está ativado ou não.
DEBUG=0

# Loop que processa os argumentos passados para o script.
while [[ $# -gt 0 ]]; do
    # Verifica qual é o argumento atual.
    case $1 in
        # Se o argumento for --debug, ativa o modo de depuração.
        --debug)
            # Altera o valor da variável DEBUG para 1.
            DEBUG=1
            # Move para o próximo argumento.
            shift
            ;;
            # Se o argumento não for reconhecido, imprime uma mensagem de erro e sai do script.
            *)
            echo "Opção inválida: $1"
            exit 1
            ;;
    esac
done

# Define as flags de compilação padrão.
COMPILER_FLAGS="-std=c++17 -Wall -Wextra"

# Verifica se o modo de depuração está ativado.
if [ $DEBUG -eq 1 ]; then

  # Se estiver ativado, adiciona as flags de depuração às flags de compilação.
  COMPILER_FLAGS+=" -fdiagnostics-color=always -g"

  # Define o nome do executável para incluir "_debug".
  OUTPUT_NAME="garnize_on_juice_debug"
else

  # Se não estiver ativado, adiciona a flag de otimização às flags de compilação.
  COMPILER_FLAGS+=" -O2"

  # Define o nome do executável padrão.
  OUTPUT_NAME="garnize_on_juice"
fi

# Compila o código C++ usando as flags de compilação definidas.
g++ $COMPILER_FLAGS -o $OUTPUT_NAME src/main.cpp

# Verifica se a compilação foi bem-sucedida.
if [ $? -eq 0 ]; then
  # Se foi bem-sucedida, executa o programa.
  ./$OUTPUT_NAME
else
  # Se não foi bem-sucedida, imprime uma mensagem de erro e sai do script.
  echo "Erro ao compilar"
  exit 1
fi
