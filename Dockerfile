FROM alpine:latest

# Instala as dependências necessárias
RUN apk update && apk add --no-cache \
    alpine-sdk \
    g++ \
    && rm -rf /var/lib/apt/lists/*

# Copia o código-fonte para o contêiner
WORKDIR /app
COPY . /app

# Compila o código
# -Wall: Este parâmetro ativa todos os avisos de compilação que são considerados úteis e fáceis de corrigir.
# -Wextra: Este parâmetro ativa avisos adicionais que não são incluídos pelo -Wall.
# -O2: Este parâmetro controla o nível de otimização do compilador.
#      O -O2 é um nível de otimização moderado que equilibra a velocidade de execução do programa com o tempo de compilação.
RUN g++ -std=c++17 -Wall -Wextra -O2 -o garnize_on_juice src/main.cpp

# Exposição da porta
EXPOSE 8080

# Execução do serviço
CMD ["./garnize_on_juice"]