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
#include <random>
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
    inline static const string BAD_REQUEST_RESPONSE = "HTTP/1.1 400 Bad Request";

    /**
     * @brief Resposta HTTP padrão para recursos não encontrados (404 Not Found).
     */
    inline static const string NOT_FOUND_RESPONSE = "HTTP/1.1 404 Not Found\r\n\r\n";

    /**
     * @brief Resposta HTTP padrão para recursos criados com sucesso (201 Created).
     */
    inline static const string CREATED_RESPONSE = "HTTP/1.1 201 Created";

    /**
     * @brief Resposta HTTP padrão para requisições bem-sucedidas (200 OK).
     */
    inline static const string OK_RESPONSE = "HTTP/1.1 200 OK";

    /**
     * @brief Cabeçalho Content-Type para respostas JSON.
     *
     * Inclui o tipo de conteúdo e o campo para especificar o tamanho do corpo da resposta.
     */
    inline static const string CONTENT_TYPE_APPLICATION_JSON = "\r\nContent-Type: application/json\r\nContent-Length: ";

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
    inline static const string PROCESSOR_DEFAULT = "http://localhost:8001";

    /**
     * @brief URL de fallback do processador de pagamentos.
     *
     * Essa URL é usada como fallback quando o processador de pagamentos padrão não está disponível.
     */
    inline static const string PROCESSOR_FALLBACK = "http://localhost:8002";

    /**
     * @brief Endpoint para operações de pagamento.
     *
     * Essa constante define o caminho base para operações de pagamento.
     */
    inline static const string PAYMENTS_ENDPOINT = "/payments";

    /**
     * @brief Endpoint para resumo de pagamentos.
     *
     * Essa constante define o caminho para obter um resumo de pagamentos.
     */
    inline static const string PAYMENTS_SUMMARY_ENDPOINT = "/payments-summary";

    /**
     * @brief Endpoint para limpar pagamentos.
     *
     * Essa constante define o caminho para limpar o banco sqlite.
     */
    inline static const string PURGE_PAYMENTS_ENDPOINT = "/purge-payments";

    /**
     * @brief Endpoint para resumo de pagamentos para administradores.
     *
     * Essa constante define o caminho para obter um resumo de pagamentos com acesso de administrador.
     */
    inline static const string PAYMENTS_SUMMARY_ADMIN_ENDPOINT = "/admin/payments-summary";

    /**
     * @brief Caminho padrão para o health check do processo.
     *
     * Essa constante define o caminho padrão que é usado para verificar a saúde do processo.
     * O valor padrão é "payments/service-health".
     */
    inline static const string HEALTH_CHECK_ENDPOINT = "/payments/service-health";

    /**
     * @brief Header de autenticação para a Rinha.
     *
     * Essa constante define o valor do header de autenticação X-Rinha-Token,
     * que é usado para autenticar requisições na Rinha.
     */
    inline static const string X_RINHA_TOKEN = "X-Rinha-Token: 123";
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
        ostringstream outputStringBuffer;

        outputStringBuffer << put_time(&now_tm, "%Y-%m-%dT%H:%M:%S");

        auto fracao_segundo = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;

        outputStringBuffer << "." << setfill('0') << setw(3) << fracao_segundo.count() << "Z";

        return outputStringBuffer.str();
    }
};

/**
 * @brief Classe responsável por gerar UUIDs (Universally Unique Identifiers).
 *
 * Essa classe fornece um método estático para gerar UUIDs baseados no datetime do sistema.
 */
class UUIDGenerator
{
public:
    /**
     * @brief Gera um UUID baseado no datetime do sistema.
     *
     * Esse método usa o datetime do sistema para gerar um UUID. Ele combina o timestamp
     * do sistema com números aleatórios para criar um UUID único.
     *
     * O UUID gerado é uma string no formato `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`, onde
     * cada `x` é um dígito hexadecimal.
     *
     * @return Um UUID gerado como uma string.
     */
    static string createUUID()
    {
        // timestamp do sistema em nanossegundos.
        auto now = chrono::high_resolution_clock::now();

        // tempo atual em nanossegundos desde a época Unix (1 de janeiro de 1970, 00:00:00 UTC).
        auto nanos = chrono::duration_cast<chrono::nanoseconds>(now.time_since_epoch()).count();

        // Gera números aleatórios para combinar com o timestamp
        //  Fonte de números aleatórios não determinística, ou seja, gera números aleatórios baseados em eventos aleatórios do sistema.
        static random_device rd;

        // Gerador de números aleatórios baseado no algoritmo Mersenne Twister.
        // O objeto gen é inicializado com um valor de semente obtido a partir do objeto rd.
        // A inicialização com um valor de semente aleatório garante que a sequência de números gerada
        // seja diferente cada vez que o programa é executado.
        static mt19937 gen(rd());

        // Usado para gerar números aleatórios uniformemente distribuídos entre 0 e 15 (16 dígitos possíveis do Hexadecimal).
        uniform_int_distribution<> dis(0, 15);

        // Formata o UUID como uma string
        stringstream stringBuffer;

        // Configura o stream para usar a notação hexadecimal.
        stringBuffer << std::hex;

        // Obtém os 32 bits menos significativos do valor nanos.
        stringBuffer << (nanos & 0xFFFFFFFF);
        stringBuffer << "-";

        // Obtém os próximos 16 bits do valor nanos.
        stringBuffer << (nanos >> 32 & 0xFFFF);
        stringBuffer << "-";

        // Obtém os próximos 12 bits do valor nanos e define os 4 bits mais significativos para 0100, que é a versão 4 do UUID.
        stringBuffer << ((nanos >> 48 & 0x0FFF) | 0x4000);

        stringBuffer << "-";

        // Gera um valor aleatório para a variante do UUID e define os 2 bits mais significativos para 10,
        // que é a variante DCE (Distributed Computing Environment).
        stringBuffer << ((dis(gen) & 0x3) | 0x8);

        // Gera os demais componentes do UUID aleatoriamente.
        stringBuffer << dis(gen);
        stringBuffer << "-";

        for (int i = 0; i < 12; i++)
        {
            stringBuffer << dis(gen);
        }

        return stringBuffer.str();
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
 * @brief Estrutura que representa um resumo de estatísticas.
 *
 * Contém informações sobre o total de requisições e o valor total.
 */
struct Summary
{
    int totalRequests;
    double totalAmount;
};

/**
 * @brief Estrutura que representa um resumo de pagamentos.
 *
 * Contém estatísticas para o default e o fallback.
 */
struct PaymentsSummary
{
    Summary defaultStats;
    Summary fallbackStats;
};

/**
 * @brief Classe responsável por converter PaymentSummary para string JSON.
 *
 * Fornece um método estático para converter um PaymentSummary em uma string JSON.
 */
class PaymentsSummaryConverter
{
public:
    /**
     * @brief Converte um PaymentSummary para uma string JSON.
     *
     * @param summary O PaymentSummary a ser convertido.
     * @return std::string A string JSON representando o PaymentsSummary.
     */
    static string toJson(const PaymentsSummary &summary)
    {
        stringstream stringBuffer;

        stringBuffer << std::fixed << std::setprecision(2);

        stringBuffer << "{\"default\":{\"totalRequests\":"
                     << summary.defaultStats.totalRequests
                     << ",\"totalAmount\":"
                     << summary.defaultStats.totalAmount
                     << "},\"fallback\":{\"totalRequests\":"
                     << summary.fallbackStats.totalRequests
                     << ",\"totalAmount\":"
                     << summary.fallbackStats.totalAmount
                     << "}}";

        return stringBuffer.str();
    }
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
    static map<string, string> payment(const string &body)
    {
        Timer timer;

        map<string, string> responseMap;

        size_t pos = body.find(Constants::KEY_CORRELATION_ID);

        if (pos == string::npos)
        {
            // Retorna um json de request invalida (faltou parametro 'correlationId')
            responseMap["status"] = Constants::BAD_REQUEST_RESPONSE;
            responseMap["response"] = "{ \"message\":\"Invalid params. Missing 'correlationId'\" }";

            return responseMap;
        }

        pos = body.find(Constants::KEY_AMOUNT);

        if (pos == string::npos)
        {
            // Retornar um json de request invalida (faltou parametro 'amount')
            responseMap["status"] = Constants::BAD_REQUEST_RESPONSE;
            responseMap["response"] = "{ \"message\":\"Invalid params. Missing 'amount'\" }";

            return responseMap;
        }

        // Parse do corpo da requisição
        map<string, string> json = JsonParser::parseJson(body);

        string correlationId = json.at(Constants::KEY_CORRELATION_ID);

        double amount = stod(json.at(Constants::KEY_AMOUNT));

        // Cria o payload e chama o default processor ou o fallback
        Payment payment;
        // Descomentar linha abaixo pois o valor vai ser enviado na request
        // payment.correlationId = correlationId
        payment.correlationId = UUIDGenerator::createUUID();
        payment.amount = amount;
        payment.requestedAt = TimeUtils::getTimestampUTC();

        LOGGER::info("Payment(correlationId=" + payment.correlationId + ", amount=" + std::to_string(payment.amount) + ", requestedAt=" + payment.requestedAt + ")");

        /**
         * 1) @todo Já chamou a 5 segundos o GET /payments/service-health (quanto está demorando pra responder os endpoints default e fallback ?)
         * 2) @todo Este endpoint impõe um limite de chamadas – 1 chamada a cada 5 segundos
         * 3) @todo Salvar o resultado em alguma estrutura de dados que possa ser compartilhada pelas threads
         * 4) @todo Se este limite for ultrapassado, você receberá uma resposta de erro do tipo HTTP 429 - Too Many Requests.
         * 5) @todo decidir como o algoritmo vai fazer as requests para o default ou o fallback
         * 6) @todo Salvar os pagamentos que deram certo para retornar pelo paymemts summary (GET)
         * 7) @todo Lembrar: endpoint paymemts summary (GET) precisa retornar um resumo do que já foi processado em termos de pagamentos.
         */

        // Aqui adicionar uma chamada a rinha
        responseMap["status"] = Constants::CREATED_RESPONSE;
        responseMap["response"] = "{ \"message\":\"payment processed successfull\" }";

        return responseMap;
    }

    /**
     * @brief Lida com requisições GET /payments-summary.
     *
     * Esse método verifica se a query string é válida, calcula o total de pagamentos
     * no período especificado e retorna a resposta.
     *
     * @param query Query string da requisição.
     * @return Um mapa contendo o código e a resposta.
     */
    static map<string, string> paymentSummary(const string &query)
    {
        Timer timer;

        map<string, string> responseMap;

        // Parse da query string
        size_t pos = query.find("from=");

        if (pos == string::npos)
        {
            responseMap["status"] = Constants::BAD_REQUEST_RESPONSE;
            responseMap["response"] = "{ \"message\":\"Invalid params. Missing 'from'\" }";

            return responseMap;
        }

        string from = query.substr(pos + 5);

        pos = query.find("to=");

        if (pos == string::npos)
        {

            responseMap["status"] = Constants::BAD_REQUEST_RESPONSE;
            responseMap["response"] = "{ \"message\":\"Invalid params. Missing 'to'\" }";

            return responseMap;
        }

        string to = query.substr(pos + 3);

        // Calcula o total de pagamentos no período com a o banco de dados
        /**
         * @todo Definir a query (usar parâmetros da query string)
         */
        PaymentsSummary paymentSummary;
        paymentSummary.defaultStats.totalRequests = 43236;
        paymentSummary.defaultStats.totalAmount = 415542345.98;
        paymentSummary.fallbackStats.totalRequests = 423545;
        paymentSummary.fallbackStats.totalAmount = 329347.34;

        // Retorna o total de pagamentos
        responseMap["status"] = Constants::OK_RESPONSE;
        responseMap["response"] = PaymentsSummaryConverter::toJson(paymentSummary);

        return responseMap;
    }
};

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

        // Define o tamanho do buffer para ler dados da conexão de rede.
        // O buffer tem um tamanho fixo de 1024 bytes (definido em Constants::BUFFER_SIZE),
        // o que significa que o programa pode ler até 1024 bytes de dados da conexão por vez.
        char buffer[Constants::BUFFER_SIZE];

        // Ler a requisição
        if (read(socket, buffer, Constants::BUFFER_SIZE) < 0)
        {
            LOGGER::error("Falha ao ler a requisição");
        }

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

        if (method == "POST" && path == Constants::PAYMENTS_ENDPOINT)
        {
            LOGGER::info("POST request para /payments");

            size_t bodyPos = request.find("\r\n\r\n");

            if (bodyPos != string::npos)
            {
                string body = request.substr(bodyPos + 4);
                map<string, string> response = PaymentProcessor::payment(body);

                string headers = response.at("status") + Constants::CONTENT_TYPE_APPLICATION_JSON + std::to_string(response.at("response").size()) + "\r\n\r\n";
                send(socket, headers.c_str(), headers.size(), 0);

                send(socket, response.at("response").c_str(), response.at("response").size(), 0);
            }
            else
            {
                string response = Constants::BAD_REQUEST_RESPONSE;
                send(socket, response.c_str(), response.size(), 0);
            }
        }
        else if (method == "GET" && path.find(Constants::PAYMENTS_SUMMARY_ENDPOINT) == 0)
        {

            LOGGER::info("GET request para /payments-summary");

            size_t queryPos = path.find("?");

            if (queryPos != string::npos)
            {
                string query = path.substr(queryPos + 1);
                map<string, string> response = PaymentProcessor::paymentSummary(query);

                string headers = response.at("status") + Constants::CONTENT_TYPE_APPLICATION_JSON + std::to_string(response.at("response").size()) + "\r\n\r\n";
                send(socket, headers.c_str(), headers.size(), 0);

                send(socket, response.at("response").c_str(), response.at("response").size(), 0);
            }
            else
            {
                string response = Constants::BAD_REQUEST_RESPONSE;
                send(socket, response.c_str(), response.size(), 0);
            }
        }
        else if (method == "POST" && path.find(Constants::PURGE_PAYMENTS_ENDPOINT) == 0)
        {

            LOGGER::info("POST request para /purge-payments");

            string msg = "Todas as tabelas do banco foram limpas! Eu espero que você saiba o que acabou de fazer.";

            LOGGER::info(msg);

            /**
             * @todo Criar uma classe que limpa o banco de dados sqlite
             * Deve lidar com a concorrência para evitar inconsistências de leitura / escrita simultâneas
             */
            map<string, string> response = {
                {"status", Constants::OK_RESPONSE},
                {"response", "{ \"message\": \"" + msg + "\", \"success\": true}"}};

            string headers = response.at("status") + Constants::CONTENT_TYPE_APPLICATION_JSON + std::to_string(response.at("response").size()) + "\r\n\r\n";
            send(socket, headers.c_str(), headers.size(), 0);

            send(socket, response.at("response").c_str(), response.at("response").size(), 0);
        }
        else
        {
            LOGGER::info("Essa request não está mapeada");

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

    // Habilita a opção SO_REUSEADDR para permitir a reutilização da mesma porta,
    // mesmo se o socket estiver em um estado de espera (TIME_WAIT).
    // Isso evita erros de "endereço em uso" ao reiniciar o servidor.
    int ALLOW_REBIND_SAME_PORT = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &ALLOW_REBIND_SAME_PORT, sizeof(ALLOW_REBIND_SAME_PORT)) < 0)
    {
        LOGGER::error("Falha ao setar SO_REUSEADDR");
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;

    // Converte a porta de host para ordem de bytes de rede (Big-endian) usando htons.
    // Isso garante que a porta seja representada corretamente em diferentes arquiteturas,
    // independentemente da ordem de bytes do sistema.
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