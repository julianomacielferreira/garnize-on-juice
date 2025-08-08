/*
 * The MIT License
 *
 * Copyright 2025 juliano.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <chrono>
#include <map>
#include <vector>
#include <regex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

/**
 * @brief Classe que armazena constantes globais.
 */
class Constants
{
public:
    /**
     * @brief Porta padrão para o servidor.
     */
    static const uint16_t PORT = 9999;

    /**
     * @brief Resposta HTTP padrão para requisições inválidas (400 Bad Request).
     */
    inline static const string BAD_REQUEST_RESPONSE = "HTTP/1.1 400 Bad Request\r\n\r\n";

    /**
     * @brief Resposta HTTP padrão para recursos não encontrados (404 Not Found).
     */
    inline static const string NOT_FOUND_RESPONSE = "HTTP/1.1 404 Not Found\r\n\r\n";

    /**
     * @brief Resposta HTTP padrão para recursos criados com sucesso (201 Created).
     */
    inline static const string CREATED_RESPONSE = "HTTP/1.1 201 Created\r\n\r\n";

    /**
     * @brief Resposta HTTP padrão para requisições bem-sucedidas (200 OK).
     */
    inline static const string OK_RESPONSE = "HTTP/1.1 200 OK\r\n\r\n";

    /**
     * @brief URL padrão do processador de pagamentos.
     *
     * Essa URL é usada como padrão quando não há outra configuração específica.
     */
    inline static const string PROCESSOR_DEFAULT = "http://payment-processor-default:8080";

    /**
     * @brief URL de fallback do processador de pagamentos.
     *
     * Essa URL é usada como fallback quando o processador de pagamentos padrão não está disponível.
     */
    inline static const string PROCESSOR_FALLBACK = "http://payment-processor-fallback:8080";
};

/**
 * @brief Classe que fornece métodos para registro de logs.
 */
class LOGGER
{
public:
    /**
     * @brief Método que registra um erro no console.
     *
     * @param message Mensagem de erro a ser registrada.
     */
    static void error(const std::string &message)
    {
        std::cerr << "Erro: " << message << std::endl;
    }

    /**
     * @brief Método que registra uma informação no console.
     *
     * @param message Mensagem de informação a ser registrada.
     */
    static void info(const std::string &message)
    {
        std::cout << "Info: " << message << std::endl;
    }
};

/**
 * @brief Classe que mede o tempo de execução de um bloco de código.
 *
 * Esta classe utiliza a biblioteca `std::chrono` para medir o tempo decorrido entre
 * a construção e destruição de um objeto desta classe.
 */
class Timer
{
public:
    /**
     * @brief Construtor que inicia o temporizador.
     *
     * Registra o tempo atual utilizando `std::chrono::high_resolution_clock`.
     */
    Timer() : start(chrono::high_resolution_clock::now()) {}

    /**
     * @brief Destrutor que para o temporizador e imprime o tempo decorrido.
     *
     * Calcula a duração entre o tempo de início e fim e imprime
     * o resultado no console.
     */
    ~Timer()
    {
        auto end = chrono::high_resolution_clock::now();
        auto duration_ms = chrono::duration_cast<std::chrono::milliseconds>(end - start);
        auto duration_us = chrono::duration_cast<std::chrono::microseconds>(end - start);
        auto duration_ns = chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        cout << "Tempo de execução da request: " << duration_ms.count() << " ms ("
             << duration_us.count() << " us / " << duration_ns.count() << " ns)" << endl;
    }

private:
    /**
     * @brief Ponto de tempo de início.
     *
     * Registra o ponto de tempo quando o objeto é construído.
     */
    chrono::time_point<chrono::high_resolution_clock> start;
};

/**
 * @brief Classe responsável por realizar o parse de uma string em formato JSON.
 */
class JsonParser
{
public:
    /**
     * @brief Faz o parse de um JSON e retorna um array com um map content os valores.
     *
     * @param jsonString String JSON a ser parseada.
     * @return std::vector<std::map<std::string, std::string>> Array com um map contendo os valores
     */
    static vector<map<string, string>> parseJson(const string &jsonString)
    {

        string json = removeUnnecessarySpaces(jsonString);
        json = json.substr(1, json.size() - 2); // Remove as chaves {}

        map<string, string> data;
        size_t pos = 0;

        while (pos < json.size())
        {
            size_t keyEnd = json.find(':');

            string key = json.substr(pos + 1, keyEnd - pos - 2); // remove as aspas

            pos = keyEnd + 1;

            size_t valueEnd = json.find(',', pos);

            if (valueEnd == string::npos)
                valueEnd = json.size();

            string value = json.substr(pos + 1, valueEnd - pos - 2); // remove as aspas

            data[key] = value;

            pos = valueEnd + 1;
        }

        return {data};
    }

private:
    /**
     * @brief Remove os espaços em branco desnecessários de uma string JSON.
     *
     * Esse método utiliza uma expressão regular para remover os espaços em branco que não estão dentro de strings delimitadas por aspas.
     *
     * @param jsonString A string JSON a ser processada.
     * @return A string JSON com os espaços em branco desnecessários removidos.
     */
    static string removeUnnecessarySpaces(const string &jsonString)
    {
        return regex_replace(jsonString, regex("\\s+(?=([^\"']*[\"'][^\"']*[\"'])*[^\"']*$"), "");
    }
};

/**
 * @brief Classe que fornece métodos para parsear requisições HTTP.
 */
class HttpRequestParser
{
public:
    /**
     * @brief Método que extrai o método de uma requisição HTTP.
     *
     * @param request String que representa a requisição HTTP.
     * @return string Método da requisição.
     */
    static string extractMethod(const string &request)
    {
        size_t methodEnd = request.find(" ");

        if (methodEnd == string::npos)
        {
            throw invalid_argument("Requisição inválida");
        }

        size_t pathEnd = request.find(" ", methodEnd + 1);

        if (pathEnd == string::npos)
        {
            throw invalid_argument("Requisição inválida");
        }

        return request.substr(methodEnd + 1, pathEnd - methodEnd - 1);
    }
};

/**
 * @brief Classe responsável por fornecer utilitários de tempo.
 */
class TimeUtils
{
public:
    /**
     * @brief Retorna o timestamp atual no formato ISO 8601 em UTC.
     *
     * O formato retornado é "YYYY-MM-DDTHH:MM:SS.sssZ", onde:
     * - YYYY é o ano em 4 dígitos
     * - MM é o mês em 2 dígitos
     * - DD é o dia do mês em 2 dígitos
     * - T é o caractere separador entre data e hora
     * - HH é a hora em 24 horas
     * - MM é o minuto
     * - SS é o segundo
     * - sss é a fração de segundo em milissegundos
     * - Z é o caractere que indica que o tempo está em UTC
     *
     * @return std::string Timestamp no formato ISO 8601 em UTC.
     */
    static string getTimestampUTC()
    {
        // Obter o tempo atual em UTC
        auto now = chrono::system_clock::now();
        auto now_time_t = chrono::system_clock::to_time_t(now);
        auto now_tm = *gmtime(&now_time_t);

        // Formatar a data e hora no formato ISO
        ostringstream oss;
        oss << put_time(&now_tm, "%Y-%m-%dT%H:%M:%S");
        auto fracao_segundo = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
        oss << "." << setfill('0') << setw(3) << fracao_segundo.count() << "Z";

        // Retornar o timestamp
        return oss.str();
    }
};

/**
 * @brief Estrutura que representa um pagamento.
 */
struct Payment
{
    /**
     * @brief Identificador único do pagamento e do tipo UUID.
     */
    string correlationId;

    /**
     * @brief Valor do pagamento do tipo decimal.
     */
    double amount;

    /**
     * @brief Data do pagamento.
     */
    string date;
};

// Mapa para armazenar pagamentos
map<string, Payment> payments;

// Função para lidar com requisições POST /payments
string handlePostPayment(const string &body)
{
    Timer timer;

    // Parse do corpo da requisição
    size_t pos = body.find("correlationId=");

    if (pos == string::npos)
    {
        // Retornar um json de request invalida (faltou parametro 'amount')
        return Constants::BAD_REQUEST_RESPONSE;
    }

    string correlationId = body.substr(pos + 14);

    pos = body.find("amount=");

    if (pos == string::npos)
    {
        // Retornar um json de request invalida (faltou parametro 'amount')
        return Constants::BAD_REQUEST_RESPONSE;
    }

    double amount = stod(body.substr(pos + 7));

    // Armazena o pagamento
    Payment payment;
    payment.correlationId = correlationId;
    payment.amount = amount;
    payment.date = TimeUtils::getTimestampUTC();
    payments[correlationId] = payment;

    // Aqui adicionar uma chamada a rinha

    return Constants::CREATED_RESPONSE;
}

// Função para lidar com requisições GET /payments-summary
string handleGetPaymentSummary(const string &query)
{
    Timer timer;

    // Parse da query string
    size_t pos = query.find("from=");
    if (pos == string::npos)
    {
        return Constants::BAD_REQUEST_RESPONSE;
    }

    string from = query.substr(pos + 5);

    pos = query.find("to=");
    if (pos == string::npos)
    {
        return Constants::BAD_REQUEST_RESPONSE;
    }

    string to = query.substr(pos + 3);

    // Calcula o total de pagamentos no período
    double total = 0;

    for (const auto &payment : payments)
    {
        if (payment.second.date >= from && payment.second.date <= to)
        {
            total += payment.second.amount;
        }
    }

    // Aqui adicionar uma chamada a rinha

    // Retorna o total de pagamentos
    return Constants::OK_RESPONSE + "Total: " + to_string(total);
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Criar o socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        LOGGER::error("Falha ao criar o socket");
        return EXIT_FAILURE;
    }

    // A opção SO_REUSEADDR permite que você reutilize a mesma porta mesmo se o socket estiver em um estado de espera.
    int ALLOW_REBIND_SAME_PORT = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &ALLOW_REBIND_SAME_PORT, sizeof(ALLOW_REBIND_SAME_PORT)) < 0)
    {
        LOGGER::error("Falha ao setar SO_REUSEADDR");
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(Constants::PORT);

    // Bind do socket ao endereço
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        LOGGER::error("Falha ao bind do socket");
        return EXIT_FAILURE;
    }

    // Escutar conexões
    if (listen(server_fd, 3) < 0)
    {
        LOGGER::error("Falha ao escutar conexões");
        return EXIT_FAILURE;
    }

    LOGGER::info("Garnize on Juice iniciado na porta 9999, escutando requests POST e GET");

    while (true)
    {
        // Aceitar uma conexão
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            LOGGER::error("Falha ao aceitar conexão");
            continue;
        }

        // Ler a requisição
        char buffer[1024];
        read(new_socket, buffer, 1024);
        string request(buffer);

        // Parse da requisição
        size_t pos = request.find(" ");
        if (pos == string::npos)
        {
            LOGGER::error("Requisição inválida");
            continue;
        }

        string method = request.substr(0, pos);

        pos = request.find(" ", pos + 1);

        if (pos == string::npos)
        {
            LOGGER::error("Requisição inválida");
            continue;
        }

        string path = HttpRequestParser::extractMethod(request);

        if (method == "POST" && path == "/payments")
        {
            LOGGER::info("POST request para /payments");

            size_t bodyPos = request.find("\r\n\r\n");

            if (bodyPos != string::npos)
            {
                string body = request.substr(bodyPos + 4);
                string response = handlePostPayment(body);
                send(new_socket, response.c_str(), response.size(), 0);
            }
            else
            {
                string response = Constants::BAD_REQUEST_RESPONSE;
                send(new_socket, response.c_str(), response.size(), 0);
            }
        }
        else if (method == "GET" && path.find("/payments-summary") == 0)
        {

            LOGGER::info("GET request para /payments-summary");

            size_t queryPos = path.find("?");

            if (queryPos != string::npos)
            {
                string query = path.substr(queryPos + 1);
                string response = handleGetPaymentSummary(query);
                send(new_socket, response.c_str(), response.size(), 0);
            }
            else
            {
                string response = Constants::BAD_REQUEST_RESPONSE;
                send(new_socket, response.c_str(), response.size(), 0);
            }
        }
        else
        {
            string response = Constants::NOT_FOUND_RESPONSE;
            send(new_socket, response.c_str(), response.size(), 0);
        }

        // Fechar a conexão
        close(new_socket);
    }

    return EXIT_SUCCESS;
}