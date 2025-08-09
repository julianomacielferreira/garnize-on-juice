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
#include <thread>
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
     * @brief Tamanho do buffer para leitura de dados em bytes.
     *
     * Essa constante define o tamanho do buffer usado para ler dados de uma conexão de rede.
     * O valor é de 1024 bytes.
     */
    static const uint16_t BUFFER_SIZE = 1024;

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
     * @brief Mensagem de erro para requisição inválida.
     */
    inline static const string INVALID_REQUEST_MSG = "Requisição inválida";

    /**
     * @brief Chave para o ID de correlação em uma requisição.
     *
     * Essa constante define a chave padrão para o campo "correlationId" em uma requisição.
     */
    inline static const string KEY_CORRELATION_ID = "correlationId";

    /**
     * @brief Chave para o valor do amount em uma requisição.
     *
     * Essa constante define a chave padrão para o campo "amount" em uma requisição.
     */
    inline static const string KEY_AMOUNT = "amount";

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
     * @brief Faz o parse de um JSON e retorna um map content os valores.
     *
     * @param jsonString String JSON a ser parseada.
     * @return std::map<std::string, std::string> Um map contendo os valores
     */
    static map<string, string> parseJson(const string &jsonString)
    {

        const string JSON = removeUnnecessarySpaces(jsonString);

        map<string, string> data;

        size_t pos = 0;

        while (pos < JSON.size())
        {
            // Encontra o início da chave
            size_t keyStart = JSON.find('"', pos) + 1;

            // Encontra o fim da chave
            size_t keyEnd = JSON.find('"', keyStart);

            // Extrai a chave
            string key = JSON.substr(keyStart, keyEnd - keyStart);

            // Encontra o início do valor
            pos = JSON.find(':', keyEnd) + 1;

            // Encontra o fim do valor, que pode ser uma vírgula ou o fechamento do objeto.
            size_t valueEnd = JSON.find_first_of(",}", pos);

            if (valueEnd == string::npos)
                valueEnd = JSON.size();

            string value = JSON.substr(pos, valueEnd - pos);

            // Verifica se o valor está entre aspas
            if (value[0] == '"' && value[value.size() - 1] == '"')
            {
                // remove as aspas
                value = value.substr(1, value.size() - 2);
            }

            data[key] = value;

            pos = valueEnd + 1;

            // Verifica se chegou ao fim do objeto para não ficar em loop infinito
            if (JSON[pos - 1] == '}')
                break;
        }

        return data;
    }

private:
    /**
     * @brief Remove caracteres não imprimíveis da string JSON.
     *
     * Essa função itera sobre a string JSON e remove todos os caracteres que não são imprimíveis ASCII (código entre 32 e 126).
     *
     * @param jsonString String JSON a ser limpa.
     * @return String JSON limpa, sem caracteres não imprimíveis.
     */
    static string removeInvalidCharacters(const string &jsonString)
    {
        string cleaned;

        for (char character : jsonString)
        {
            if (character >= 32 && character <= 126)
            { // caracteres imprimíveis ASCII
                cleaned += character;
            }
        }

        return cleaned;
    }

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
        return regex_replace(removeInvalidCharacters(jsonString), regex("\\s+(?=(?:[^\"']*[\"'][^\"']*[\"'])*[^\"']*$)"), "");
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
            throw invalid_argument(Constants::INVALID_REQUEST_MSG);
        }

        size_t pathEnd = request.find(" ", methodEnd + 1);

        if (pathEnd == string::npos)
        {
            throw invalid_argument(Constants::INVALID_REQUEST_MSG);
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
    string requestedAt;
};

/**
 * @brief Classe responsável por lidar com pagamentos.
 *
 * Essa classe fornece métodos estáticos para lidar com requisições de pagamento, incluindo
 * a criação de pagamentos e o cálculo do total de pagamentos em um período.
 */
class PaymentProcessor
{
public:
    /**
     * @brief Lida com requisições POST /payments.
     *
     * Esse método verifica se a requisição é válida, parseia o corpo da requisição,
     * cria um pagamento e o armazena no mapa de pagamentos.
     *
     * @param body Corpo da requisição.
     * @return Resposta HTTP.
     */
    static string payment(const string &body)
    {
        Timer timer;

        size_t pos = body.find(Constants::KEY_CORRELATION_ID);

        if (pos == string::npos)
        {
            // Retornar um json de request invalida (faltou parametro 'correlationId')
            return Constants::BAD_REQUEST_RESPONSE;
        }

        pos = body.find(Constants::KEY_AMOUNT);

        if (pos == string::npos)
        {
            // Retornar um json de request invalida (faltou parametro 'amount')
            return Constants::BAD_REQUEST_RESPONSE;
        }

        // Parse do corpo da requisição
        map<string, string> json = JsonParser::parseJson(body);

        string correlationId = json.at(Constants::KEY_CORRELATION_ID);

        double amount = stod(json.at(Constants::KEY_AMOUNT));

        // Cria o payload e chama o default processor ou o fallback
        Payment payment;
        payment.correlationId = correlationId;
        payment.amount = amount;
        payment.requestedAt = TimeUtils::getTimestampUTC();

        payments[correlationId] = payment;

        // Aqui adicionar uma chamada a rinha
        return Constants::CREATED_RESPONSE;
    }

    /**
     * @brief Lida com requisições GET /payments-summary.
     *
     * Esse método verifica se a query string é válida, calcula o total de pagamentos
     * no período especificado e retorna a resposta.
     *
     * @param query Query string da requisição.
     * @return Resposta HTTP.
     */
    static string paymentSummary(const string &query)
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
            if (payment.second.requestedAt >= from && payment.second.requestedAt <= to)
            {
                total += payment.second.amount;
            }
        }

        // Aqui adicionar uma chamada a rinha

        // Retorna o total de pagamentos
        return Constants::OK_RESPONSE + "Total: " + to_string(total);
    }

private:
    /**
     * @brief Mapa para armazenar pagamentos realizados.
     *
     * Esse mapa armazena os pagamentos realizados, indexados pelo ID de correlação (correlationId).
     *
     * @key string: ID de correlação do pagamento.
     * @value Payment: Objeto que representa o pagamento.
     */
    static map<string, Payment> payments;
};

// Inicialização do mapa estático
map<string, Payment> PaymentProcessor::payments;

/**
 * @brief Classe responsável por lidar com requisições recebidas em um socket.
 *
 * Essa classe fornece um método estático para lidar com requisições recebidas em um socket.
 *
 */
class RequestHandler
{
public:
    /**
     * @brief Lida com uma requisição recebida em um socket.
     *
     * Essa função lê a requisição do socket, parseia o método e o caminho da requisição,
     * e então processa a requisição de acordo com o método e o caminho.
     *
     * @param socket O socket que recebeu a requisição.
     *
     * @details
     * A função segue os seguintes passos:
     * 1. Lê a requisição do socket e armazena em um buffer.
     * 2. Parseia o método e o caminho da requisição.
     * 3. Verifica se o método e o caminho são válidos e processa a requisição de acordo.
     * 4. Envia uma resposta ao cliente.
     * 5. Fecha a conexão.
     *
     * @note
     * A função assume que o socket é válido e que a requisição é bem-formada.
     * Se a requisição for inválida, a função envia uma resposta de erro ao cliente.
     */
    static void handle(int socket)
    {

        // O número 1024 é usado aqui como o tamanho do buffer para ler os dados da conexão de rede.
        // Isso significa que o programa está alocando um espaço de memória de 1024 bytes para armazenar os dados que estão sendo lidos da conexão.
        char buffer[Constants::BUFFER_SIZE];

        // Ler a requisição
        read(socket, buffer, Constants::BUFFER_SIZE);

        string request(buffer);

        // Parse da requisição
        size_t pos = request.find(" ");

        if (pos == string::npos)
        {
            LOGGER::error(Constants::INVALID_REQUEST_MSG);
            return;
        }

        string method = request.substr(0, pos);

        pos = request.find(" ", pos + 1);

        if (pos == string::npos)
        {
            LOGGER::error(Constants::INVALID_REQUEST_MSG);
            return;
        }

        string path = HttpRequestParser::extractMethod(request);

        if (method == "POST" && path == "/payments")
        {
            LOGGER::info("POST request para /payments");

            size_t bodyPos = request.find("\r\n\r\n");

            if (bodyPos != string::npos)
            {
                string body = request.substr(bodyPos + 4);
                string response = PaymentProcessor::payment(body);
                send(socket, response.c_str(), response.size(), 0);
            }
            else
            {
                string response = Constants::BAD_REQUEST_RESPONSE;
                send(socket, response.c_str(), response.size(), 0);
            }
        }
        else if (method == "GET" && path.find("/payments-summary") == 0)
        {

            LOGGER::info("GET request para /payments-summary");

            size_t queryPos = path.find("?");

            if (queryPos != string::npos)
            {
                string query = path.substr(queryPos + 1);
                string response = PaymentProcessor::paymentSummary(query);
                send(socket, response.c_str(), response.size(), 0);
            }
            else
            {
                string response = Constants::BAD_REQUEST_RESPONSE;
                send(socket, response.c_str(), response.size(), 0);
            }
        }
        else
        {
            string response = Constants::NOT_FOUND_RESPONSE;
            send(socket, response.c_str(), response.size(), 0);
        }

        // Fechar a conexão
        close(socket);
    }
};

/**
 * @brief Função principal do programa que inicia o servidor.
 *
 * Essa função é responsável por criar o socket, bindá-lo ao endereço e porta especificados,
 * e escutar conexões. Quando uma conexão é estabelecida, a função cria uma thread para
 * lidar com a requisição.
 *
 * @return int O código de saída do programa.
 *
 * @details
 * A função segue os seguintes passos:
 * 1. Cria um socket usando a função `socket`.
 * 2. Configura a opção `SO_REUSEADDR` para permitir que o socket seja reutilizado.
 * 3. Bind o socket ao endereço e porta especificados.
 * 4. Escuta conexões usando a função `listen`.
 * 5. Aceita conexões e cria uma thread para lidar com cada requisição.
 *
 * @note
 * A função usa a biblioteca `thread` para criar threads que lidam com as requisições.
 * A função também usa a classe `LOGGER` para registrar erros e informações.
 *
 * @see
 * handleRequest: Função que lida com as requisições recebidas.
 */
int main()
{
    int server_fd;
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

    LOGGER::info("Garnize on Juice iniciado na porta 9999, escutando somente requests POST e GET");

    while (true)
    {
        // Aceitar uma conexão
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0)
        {
            LOGGER::error("Falha ao aceitar conexão");
            continue;
        }

        thread thread(RequestHandler::handle, new_socket);
        thread.detach();
    }

    return EXIT_SUCCESS;
}