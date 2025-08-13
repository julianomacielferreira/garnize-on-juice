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
#include <queue>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <uuid/uuid.h>
#include <unistd.h>
#include <sqlite3.h>

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
     * O valor é de 256 bytes.
     */
    static const uint16_t BUFFER_SIZE = 256;

    /**
     * @brief Timeout das requisições cURL.
     */
    static const uint16_t CURL_TIMEOUT_MS = 7000L;

    /**
     * @brief Timeout do sqlite3 para evitar erro de database is locked.
     */
    static const uint16_t SQLITE_BUSY_TIMEOUT_MS = 2000;

    /**
     * @brief Nome do arquivo de banco de dados SQLite para salvar pagamentos.
     */
    inline static const string DATABASE_PAYMENTS = "database/garnize-payments.sqlite";

    /**
     * @brief Nome do arquivo de banco de dados SQLite para manter os dados de health check.
     */
    inline static const string DATABASE_HEALTH_CHECK = "database/garnize-health-check.sqlite";

    /**
     * @brief Resposta HTTP padrão para requisições inválidas (400 Bad Request).
     */
    inline static const string BAD_REQUEST_RESPONSE = "HTTP/1.1 400 Bad Request";

    /**
     * @brief Resposta HTTP padrão erro interno do servidor (500 Bad Request).
     */
    inline static const string INTERNAL_SERVER_ERROR = "HTTP/1.1 500 Internal Server Error";

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
    // inline static const string PROCESSOR_DEFAULT = getenv("PROCESSOR_DEFAULT");
    inline static const string PROCESSOR_DEFAULT = "http://localhost:8001";

    /**
     * @brief URL de fallback do processador de pagamentos.
     *
     * Essa URL é usada como fallback quando o processador de pagamentos padrão não está disponível.
     */
    // inline static const string PROCESSOR_FALLBACK =  getenv("PROCESSOR_FALLBACK");
    inline static const string PROCESSOR_FALLBACK = "http://localhost:8002";

    /**
     * @brief Endpoint para operações de pagamento.
     *
     * Essa constante define o caminho base para operações de pagamento "/payments".
     */
    inline static const string PAYMENTS_ENDPOINT = "/payments";

    /**
     * @brief Endpoint para resumo de pagamentos.
     *
     * Essa constante define o caminho para obter um resumo de pagamentos "/payments-summary".
     */
    inline static const string PAYMENTS_SUMMARY_ENDPOINT = "/payments-summary";

    /**
     * @brief Endpoint para limpar pagamentos.
     *
     * Essa constante define o caminho para limpar o banco sqlite "/purge-payments".
     */
    inline static const string PURGE_PAYMENTS_ENDPOINT = "/purge-payments";

    /**
     * @brief Endpoint para resumo de pagamentos para administradores.
     *
     * Essa constante define o caminho para obter um resumo de pagamentos com acesso de administrador "/admin/payments-summary".
     */
    inline static const string PAYMENTS_SUMMARY_ADMIN_ENDPOINT = "/admin/payments-summary";

    /**
     * @brief Caminho padrão para o health check do processo.
     *
     * Essa constante define o caminho padrão que é usado para verificar a saúde do processo.
     * O valor padrão é "/payments/service-health".
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
    static void error(const string &message)
    {
        cerr << "Erro: " << message << endl;
    }

    /**
     * @brief Método que registra uma informação no console.
     *
     * @param message Mensagem de informação a ser registrada.
     */
    static void info(const string &message)
    {
        cout << "Info: " << message << endl;
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
        auto duration_ms = chrono::duration_cast<chrono::milliseconds>(end - start);
        auto duration_us = chrono::duration_cast<chrono::microseconds>(end - start);
        auto duration_ns = chrono::duration_cast<chrono::nanoseconds>(end - start);

        ostringstream stringBuilder;

        stringBuilder << "Tempo de execução da request: ";
        stringBuilder << duration_ms.count();
        stringBuilder << " ms (";
        stringBuilder << duration_us.count();
        stringBuilder << " us / ";
        stringBuilder << duration_ns.count();
        stringBuilder << " ns)";
        stringBuilder << endl;

        LOGGER::info(stringBuilder.str());
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
 * @brief Classe utilitária para fazer cURL requests.
 *
 */
class CURLUtils
{
public:
    /**
     * @brief Função de callback para escrever dados recebidos do libcurl.
     *
     * Essa função é chamada pelo libcurl para escrever dados recebidos em uma string.
     *
     * @param contents Ponteiro para os dados recebidos.
     * @param size Tamanho de cada elemento dos dados.
     * @param nmemb Número de elementos dos dados.
     * @param userp Ponteiro para a string que irá armazenar os dados. Deve ser um ponteiro para um objeto std::string.
     *
     * @return O tamanho total dos dados escritos.
     *
     * @note Essa função assume que o ponteiro userp é válido e aponta para um objeto std::string.
     */
    static size_t readCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        ((std::string *)userp)->append((char *)contents, size * nmemb);

        return size * nmemb;
    }

    /**
     * @brief Retorna um ponteiro CURL para a URL com timeout de 1500 millisegundos.
     *
     * @param URL O endereço que será chamado
     * @param payload O payload da requisição em formato JSON.
     * @param responseBuffer O buffer que armazenará a resposta da requisição.
     * @return CURL * O objeto CURL que foi utilizado para fazer a requisição.
     */
    static CURL *setupCurlForPostRequest(const string &URL, const string &payload, string &responseBuffer)
    {
        CURL *curl = curl_easy_init();

        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.length());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLUtils::readCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, Constants::CURL_TIMEOUT_MS);
        }

        return curl;
    }

    /**
     * @brief Retorna um ponteiro CURL para a URL com timeout de 7 segundos.
     *
     * @param URL O endereço que será chamado
     * @param responseBuffer O buffer que armazenará a resposta da requisição.
     * @return CURL * O objeto CURL que foi utilizado para fazer a requisição.
     */
    static CURL *setupCurlForGetRequest(const string &URL, string &responseBuffer)
    {
        CURL *curl = curl_easy_init();

        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // Ativa a opção NOSIGNAL
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLUtils::readCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, Constants::CURL_TIMEOUT_MS);
        }

        return curl;
    }
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
     * Essa função itera sobre a string JSON e remove todos os caracteres que não são imprimíveis ASCII.
     *
     * @param jsonString String JSON a ser limpa.
     * @return String JSON limpa, sem caracteres não imprimíveis.
     */
    static string removeInvalidCharacters(const string &jsonString)
    {
        string validString;

        for (char character : jsonString)
        {
            // caracteres imprimíveis ASCII (código entre 32 e 126)
            if (character >= 32 && character <= 126)
            {
                validString += character;
            }
        }

        return validString;
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

        // Formata a data e hora no formato ISO
        ostringstream stringBuilder;

        stringBuilder << put_time(&now_tm, "%Y-%m-%dT%H:%M:%S");

        /**
         * @brief Calcula a fração de segundo atual em milissegundos (0-999).
         *
         * @details
         * Essa linha de código utiliza a classe `chrono` para calcular o tempo
         * desde a época (epoch) até o momento atual, e então extrai a fração de
         * segundo em milissegundos.
         */
        auto fraction_of_second = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;

        stringBuilder << ".";
        stringBuilder << setfill('0');
        stringBuilder << setw(3);
        stringBuilder << fraction_of_second.count();
        stringBuilder << "Z";

        return stringBuilder.str();
    }
};

/**
 * @brief Classe responsável por gerar UUIDs (Universally Unique Identifiers).
 *
 * Essa classe fornece um método estático para gerar UUIDs aleatórios.
 */
class UUIDGenerator
{
public:
    /**
     * @brief Gera um UUID aleatório.
     *
     * @return Um UUID gerado como uma string.
     */
    static string createUUID()
    {
        uuid_t UUID;
        uuid_generate(UUID);

        char UUIDString[37];
        // Converte o UUID para uma string
        uuid_unparse(UUID, UUIDString);

        return UUIDString;
    }
};

/**
 * @brief Classe responsável por gerenciar a interação o banco de dados SQLite.
 */
class SQLiteDatabaseUtils
{
public:
    static bool setUpMultiThreadedMode()
    {
        // Configura o SQLite para funcionar em modo multi-thread
        int response = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);

        if (response != SQLITE_OK)
        {
            LOGGER::error(string("Erro ao configurar o modo multi-thread: ") + string(sqlite3_errstr(response)));

            return false;
        }

        return true;
    }
    /**
     * @brief Abre uma conexão com o banco de dados SQLite local definido em Constants::DATABASE_NAME.
     *
     * Configura o SQLite para funcionar em modo multi-thread e retorna um ponteiro para o objeto sqlite3.
     *
     * @param DATABASE_NAME Nome do banco que deve ser aberta a conexão
     * @return sqlite3* Ponteiro para o objeto sqlite3 se a conexão for aberta com sucesso, ou nullptr caso contrário.
     */
    static sqlite3 *openConnection(const string &DATABASE_NAME)
    {

        sqlite3 *database;

        int response = sqlite3_open(DATABASE_NAME.c_str(), &database);

        if (response)
        {
            LOGGER::error(string("Erro ao abrir conexão com o banco de dados: ") + string(sqlite3_errmsg(database)));

            sqlite3_close(database);

            return nullptr;
        }

        /**
         * @todo Timeout para tentar evitar erro de database is locked
         */
        sqlite3_busy_timeout(database, Constants::SQLITE_BUSY_TIMEOUT_MS);

        LOGGER::info("Abriu conexão com o banco de dados.");

        return database;
    }

    /**
     * @brief Fecha uma conexão com o banco de dados SQLite local definido em Constants::DATABASE_NAME.
     *
     * @param database Ponteiro para o objeto sqlite3.
     * @return bool True a conexão foi fechada com sucesso, false caso contrário.
     */
    static bool closeConnection(sqlite3 *database)
    {

        int response = sqlite3_close(database);

        if (response != SQLITE_OK)
        {

            LOGGER::error(string("Erro ao fechar conexão com o banco de dados: ") + string(sqlite3_errmsg(database)));

            return false;
        }

        LOGGER::info("Fechou conexão com o banco de dados.");

        return true;
    }
};

/**
 * @class SQLiteConnectionPoolUtils
 * @brief Gerencia um pool de conexões SQLite para melhorar o desempenho em ambientes de alta concorrência.
 *
 * Essa classe fornece uma implementação de pool de conexões SQLite que permite que múltiplas threads compartilhem conexões de forma segura e eficiente.
 *
 * @param max_connections O número máximo de conexões que o pool pode manter.
 * @param max_queue_size O número máximo de threads que podem ser enfileiradas esperando por uma conexão.
 */
class SQLiteConnectionPoolUtils
{
public:
    /**
     * @brief Constrói um objeto SQLiteConnectionPoolUtils.
     *
     * Inicializa o pool de conexões com o número máximo de conexões especificado.
     *
     * @param max_connections O número máximo de conexões que o pool pode manter.
     * @param max_queue_size O número máximo de threads que podem ser enfileiradas esperando por uma conexão.
     */
    SQLiteConnectionPoolUtils(int max_connections, int max_queue_size) : maxConnections(max_connections), maxQueueSize(max_queue_size)
    {
        for (int i = 0; i < maxConnections; i++)
        {
            sqlite3 *connection = SQLiteDatabaseUtils::openConnection(Constants::DATABASE_PAYMENTS);

            if (connection != nullptr)
            {
                connectionsQueue.push(connection);
            }
            else
            {
                LOGGER::error("Erro ao criar conexão no pool");
            }
        }
    }

    /**
     * @brief Obtém uma conexão do pool.
     *
     * Se o pool estiver vazio e o número de threads enfileiradas for menor que o máximo permitido, uma nova conexão é criada.
     * Se o pool estiver vazio é bloqueada até que uma conexão fique disponível.
     *
     * @return Uma conexão SQLite válida ou nullptr em caso de erro.
     */
    sqlite3 *getConnectionFromPool()
    {

        unique_lock<mutex> lock(mutexLock);

        while (connectionsQueue.empty() && queueSize >= maxQueueSize)
        {
            conditionToProceed.wait(lock);
        }

        if (connectionsQueue.empty())
        {
            sqlite3 *connection = SQLiteDatabaseUtils::openConnection(Constants::DATABASE_PAYMENTS);

            if (connection == nullptr)
            {

                LOGGER::error("Erro ao criar conexão para o poool");

                return nullptr;
            }

            connectionsQueue.push(connection);
        }

        sqlite3 *connection = connectionsQueue.front();

        connectionsQueue.pop();

        queueSize++;

        return connection;
    }

    /**
     * @brief Retorna uma conexão ao pool.
     *
     * A conexão é devolvida ao pool e a thread que estava esperando por uma conexão é notificada.
     *
     * @param connection A conexão a ser devolvida ao pool.
     */
    void returnConnectionToPool(sqlite3 *connection)
    {
        lock_guard<mutex> lock(mutexLock);

        connectionsQueue.push(connection);

        queueSize--;

        conditionToProceed.notify_one();
    }

    /**
     * @brief Destroi o objeto SQLiteConnectionPoolUtils.
     *
     * Fecha todas as conexões do pool.
     */
    ~SQLiteConnectionPoolUtils()
    {
        while (!connectionsQueue.empty())
        {
            sqlite3 *connection = connectionsQueue.front();

            connectionsQueue.pop();

            SQLiteDatabaseUtils::closeConnection(connection);
        }
    }

private:
    /**
     * @brief O número máximo de conexões que o pool pode manter.
     */
    int maxConnections;

    /**
     * @brief O número máximo de threads que podem ser enfileiradas esperando por uma conexão.
     */
    int maxQueueSize;

    /**
     * @brief A fila de conexões SQLite.
     */
    queue<sqlite3 *> connectionsQueue;

    /**
     * @brief O mutex para sincronizar o acesso ao pool.
     */
    mutex mutexLock;

    /**
     * @brief A variável de condição para notificar as threads que uma conexão está disponível.
     */
    condition_variable conditionToProceed;

    /**
     * @brief O número atual de threads enfileiradas esperando por uma conexão.
     */
    int queueSize = 0;
};

/**
 * @brief Representa um registro de HealthCheck.
 */
struct HealthCheck
{
    /**
     * @brief Nome do serviço.
     */
    string service;

    /**
     * @brief Indica se o serviço está falhando (0 = não, 1 = sim).
     */
    int failing;

    /**
     * @brief Tempo de resposta mínimo do serviço.
     */
    int minResponseTime;

    /**
     * @brief Data e hora da última verificação do serviço.
     */
    string lastCheck;
};

/**
 * @brief Classe utilitária para gerenciamento de health check.
 *
 * Essa classe fornece métodos estáticos para inicializar e verificar o health check dos serviços de pagamentos.
 */
class HealthCheckUtils
{
public:
    /**
     * @brief Instancia de HealthCheck para verificar se o serviço 'default' está funcionando.
     */
    static HealthCheck healthCheckDefault;
    /**
     * @brief Instancia de HealthCheck para verificar se o serviço'fallback' está funcionando.
     */
    static HealthCheck healthCheckFallback;

    /**
     * @brief Inicializa o health check dos serviços de pagamentos.
     *
     * Esse método cria a tabela de health check se necessário,
     * verifica se os registros de health check para os serviços "default" e "fallback" estão criados.
     *
     * @return true se a inicialização foi bem-sucedida, false caso contrário.
     */
    static bool init()
    {

        bool success = createHealthCkeckTable();
        LOGGER::info(success ? "Tabela do health check OK" : "Erro ao verificar tabela do health check");

        return success;
    }

    /**
     * @brief Escolhe se o serviço 'default' deve ser utilizado.
     *
     * @return true se o serviço está OK, false caso contrário.
     */
    static bool useDefault()
    {
        LOGGER::info(string("Serviço 'default' está funcionando: ") + string((healthCheckDefault.service.size() > 0 && !healthCheckDefault.failing) ? "Sim" : "Não"));

        bool isToUse = !healthCheckDefault.failing;

        if (isToUse && !healthCheckFallback.failing)
        {
            isToUse = (healthCheckDefault.minResponseTime <= healthCheckFallback.minResponseTime);
        }

        return isToUse;
    }

    /**
     * @brief Escolhe se o serviço 'fallback' deve ser utilizado.
     *
     * @return true se o serviço está OK, false caso contrário.
     */
    static bool useFallback()
    {
        LOGGER::info(string("Serviço 'fallback' está funcionando: ") + string((healthCheckFallback.service.size() > 0 && !healthCheckFallback.failing) ? "Sim" : "Não"));

        bool isToUse = !healthCheckFallback.failing;

        if (isToUse && !healthCheckDefault.failing)
        {
            isToUse = (healthCheckFallback.minResponseTime <= healthCheckDefault.minResponseTime);
        }

        return isToUse;
    }

    /**
     * @brief Atualiza um registro na tabela service_health_check.
     *
     * @param healthCheck Registro de HealthCheck a ser atualizado.
     * @return bool True se o registro foi atualizado com sucesso, false caso contrário.
     */
    static bool updateHealthRecord(const HealthCheck &healthCheck)
    {

        sqlite3 *database = getDatabase();

        const char *SQL_QUERY = R"(
            UPDATE service_health_check SET service = ?, failing = ?, minResponseTime = ?, lastCheck = ? WHERE service = ?;
        )";

        sqlite3_stmt *statement;

        int response = sqlite3_prepare_v2(database, SQL_QUERY, -1, &statement, nullptr);
        bool success = (response == SQLITE_OK);

        if (!success)
        {
            LOGGER::error(string("Erro ao preparar a query: ") + string(sqlite3_errmsg(database)));
        }
        else
        {

            sqlite3_bind_text(statement, 1, healthCheck.service.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(statement, 2, healthCheck.failing);
            sqlite3_bind_int(statement, 3, healthCheck.minResponseTime);
            sqlite3_bind_text(statement, 4, healthCheck.lastCheck.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(statement, 5, healthCheck.service.c_str(), -1, SQLITE_STATIC);

            response = sqlite3_step(statement);

            success = (response == SQLITE_DONE);

            if (!success)
            {
                LOGGER::error(string("Erro ao executar a query: ") + string(sqlite3_errmsg(database)));
            }
            else
            {
                auto setHealthCheckFor = [&](HealthCheck &healthcheckService)
                {
                    healthcheckService.service = healthCheck.service;
                    healthcheckService.failing = healthCheck.failing;
                    healthcheckService.minResponseTime = healthCheck.minResponseTime;
                    healthcheckService.lastCheck = healthCheck.lastCheck;
                };

                setHealthCheckFor((healthCheck.service == "default") ? healthCheckDefault : healthCheckFallback);
            }

            sqlite3_finalize(statement);
        }

        SQLiteDatabaseUtils::closeConnection(database);

        return success;
    }

    /**
     * @brief Recupera o registro mais atual da tabela service_health_check.
     *
     * @param service String para escolher qual o serviço que deseja recuperar.
     * @return HealthCheck contendo os valores do registro mais atual.
     */
    static HealthCheck getLastHealthCheck(const string &service)
    {

        sqlite3 *database = getDatabase();

        const char *SQL_QUERY = R"(
            SELECT service, 
                   failing, 
                   minResponseTime, 
                   datetime(lastCheck, 'localtime') AS lastCheck 
              FROM service_health_check 
             WHERE service = ?;
        )";

        HealthCheck healthCheck;

        sqlite3_stmt *statement;

        int response = sqlite3_prepare_v2(database, SQL_QUERY, -1, &statement, nullptr);
        bool success = (response == SQLITE_OK);

        if (!success)
        {
            LOGGER::error(string("Erro ao preparar a query: ") + string(sqlite3_errmsg(database)));
        }
        else
        {

            sqlite3_bind_text(statement, 1, service.c_str(), -1, SQLITE_STATIC);

            response = sqlite3_step(statement);

            if (response == SQLITE_ROW)
            {
                healthCheck.service = reinterpret_cast<const char *>(sqlite3_column_text(statement, 0));
                healthCheck.failing = sqlite3_column_int(statement, 1);
                healthCheck.minResponseTime = sqlite3_column_int(statement, 2);
                healthCheck.lastCheck = reinterpret_cast<const char *>(sqlite3_column_text(statement, 3));
            }
            else if (response == SQLITE_DONE)
            {
                LOGGER::info("Nenhum registro de service_health_check encontrado");
            }
            else
            {
                LOGGER::error(string("Erro ao executar a query: ") + string(sqlite3_errmsg(database)));
            }

            sqlite3_finalize(statement);
        }

        SQLiteDatabaseUtils::closeConnection(database);

        return healthCheck;
    }

private:
    /**
     * @brief Retorna um conexão com o banco de dados de health check.
     *
     * @return sqlite3* Ponteiro para o banco de dados.
     */
    static sqlite3 *getDatabase()
    {
        return SQLiteDatabaseUtils::openConnection(Constants::DATABASE_HEALTH_CHECK);
    }

    /**
     * @brief Cria a tabela service_health_check no banco de dados se ela não existir.
     *
     * @return bool True se a tabela foi criada com sucesso, false caso contrário.
     */
    static bool createHealthCkeckTable()
    {

        sqlite3 *database = getDatabase();

        const char *SQL_QUERY = R"(
            CREATE TABLE IF NOT EXISTS service_health_check (
                service TEXT CHECK(service IN ('default', 'fallback')) NOT NULL,
                failing INTEGER NOT NULL,
                minResponseTime INTEGER NOT NULL,
                lastCheck DATETIME NOT NULL
            );
            INSERT INTO `service_health_check` (`service`, `failing`, `minResponseTime`, `lastCheck`) SELECT 'default', 0, 0, DATETIME('now', 'localtime') WHERE NOT EXISTS (SELECT 1 FROM service_health_check WHERE service = 'default');
            INSERT INTO `service_health_check` (`service`, `failing`, `minResponseTime`, `lastCheck`) SELECT 'fallback', 0, 0, DATETIME('now', 'localtime') WHERE NOT EXISTS (SELECT 1 FROM service_health_check WHERE service = 'fallback');
        )";

        char *error;

        int response = sqlite3_exec(database, SQL_QUERY, nullptr, nullptr, &error);

        bool success = (response == SQLITE_OK);

        if (!success)
        {
            LOGGER::error(string("Erro ao criar tabela service_health_check: ") + string(error));

            sqlite3_free(error);
        }

        SQLiteDatabaseUtils::closeConnection(database);

        healthCheckDefault = getLastHealthCheck("default");
        healthCheckFallback = getLastHealthCheck("fallback");

        return success;
    }
};

HealthCheck HealthCheckUtils::healthCheckDefault;
HealthCheck HealthCheckUtils::healthCheckFallback;

/**
 * @brief Classe responsável por executar o health check dos serviços em uma thread separada.
 *
 * Essa classe fornece métodos estáticos para inicializar e executar o health check dos serviços.
 */
class HealthCheckServiceThread
{
public:
    /**
     * @brief Executa o health check dos serviços.
     *
     * Esse método faz requests para os serviços "default" e "fallback" para verificar seu status.
     */
    static void check()
    {

        /**
         * @todo Débito técnico - Código duplicado
         */
        cout << endl;
        LOGGER::info("Fazendo request de health check para a o serviço 'default'");

        CURLcode responseCodeDefault;
        string URL_DEFAULT = Constants::PROCESSOR_DEFAULT + Constants::HEALTH_CHECK_ENDPOINT;
        string defaultResponseBuffer;

        CURL *curl_default = CURLUtils::setupCurlForGetRequest(URL_DEFAULT, defaultResponseBuffer);

        if (curl_default)
        {
            responseCodeDefault = curl_easy_perform(curl_default);

            if (responseCodeDefault != CURLE_OK)
            {
                LOGGER::error(string("Erro ao fazer curl request para o serviço 'default': ") + string(curl_easy_strerror(responseCodeDefault)));
            }
            else
            {
                LOGGER::info(string("Dados recebidos (default): ") + string(defaultResponseBuffer));

                map<string, string> jsonResponse = JsonParser::parseJson(defaultResponseBuffer);

                HealthCheck healthCheckDefault;
                healthCheckDefault.service = "default";
                healthCheckDefault.failing = (jsonResponse.at("failing") == "true" || jsonResponse.at("failing") == "1");
                healthCheckDefault.minResponseTime = stoi(jsonResponse.at("minResponseTime"));
                healthCheckDefault.lastCheck = TimeUtils::getTimestampUTC();

                LOGGER::info("Atualizando no banco de dados o registro do serviço 'default'");

                HealthCheckUtils::updateHealthRecord(healthCheckDefault);

                LOGGER::info(string("Health ckeck mais atual (default): ") + string(healthCheckDefault.lastCheck));
            }

            curl_easy_cleanup(curl_default);
        }
        //--

        /**
         * @todo Débito técnico - Código duplicado
         */
        cout << endl;
        LOGGER::info("Fazendo request de health check para a o serviço 'fallback'");

        CURLcode responseCodeFallback;
        string URL_FALLBACK = Constants::PROCESSOR_FALLBACK + Constants::HEALTH_CHECK_ENDPOINT;
        string fallbackResponseBuffer;

        CURL *curl_fallback = CURLUtils::setupCurlForGetRequest(URL_FALLBACK, fallbackResponseBuffer);

        if (curl_fallback)
        {
            responseCodeFallback = curl_easy_perform(curl_fallback);

            if (responseCodeFallback != CURLE_OK)
            {
                LOGGER::error(string("Erro ao fazer curl request para o serviço 'fallback': ") + string(curl_easy_strerror(responseCodeFallback)));
            }
            else
            {
                LOGGER::info(string("Dados recebidos (fallback): ") + string(fallbackResponseBuffer));

                map<string, string> jsonResponse = JsonParser::parseJson(fallbackResponseBuffer);

                HealthCheck healthCheckFallback;
                healthCheckFallback.service = "fallback";
                healthCheckFallback.failing = false;
                healthCheckFallback.minResponseTime = 0;
                healthCheckFallback.lastCheck = TimeUtils::getTimestampUTC();

                LOGGER::info("Atualizando no banco de dados o registro do serviço 'fallback'");

                HealthCheckUtils::updateHealthRecord(healthCheckFallback);

                LOGGER::info(string("Health ckeck mais atual (fallback): ") + string(healthCheckFallback.lastCheck));
            }

            curl_easy_cleanup(curl_fallback);
        }
        //--
    }

    /**
     * @brief Inicializa a thread de health check.
     *
     * Esse método cria uma thread que executa o método `check()` a cada 5 segundos.
     * @note A thread é executada em um loop infinito.
     */
    static void init()
    {
        thread([]()
               {
                   while (true)
                   {                    
                       check();

                       // Para a thread por 5 segundos
                       this_thread::sleep_for(chrono::seconds(5));
                   } })
            .detach();
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

    /**
     * @brief Flag que indica se o pagamento foi pelo serviço 'default'.
     */
    bool defaultService;

    /**
     * @brief Flag que indica se esse pagamento foi processado por algum dos serviços.
     */
    bool processed;
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
 * @brief Classe responsável por converter estruturas para string JSON.
 *
 * Fornece um métodos estáticos para converter em uma string JSON.
 */
class PaymentsJSONConverter
{
public:
    /**
     * @brief Converte um PaymentSummary para uma string JSON.
     *
     * @param summary O PaymentSummary a ser convertido.
     * @return std::string A string JSON representando o PaymentsSummary.
     */
    static string summaryToJson(const PaymentsSummary &summary)
    {
        stringstream stringBuilder;

        stringBuilder << std::fixed;
        stringBuilder << std::setprecision(2);
        stringBuilder << "{\"default\":{\"totalRequests\":";
        stringBuilder << summary.defaultStats.totalRequests;
        stringBuilder << ",\"totalAmount\":";
        stringBuilder << summary.defaultStats.totalAmount;
        stringBuilder << "},\"fallback\":{\"totalRequests\":";
        stringBuilder << summary.fallbackStats.totalRequests;
        stringBuilder << ",\"totalAmount\":";
        stringBuilder << summary.fallbackStats.totalAmount;
        stringBuilder << "}}";

        return stringBuilder.str();
    }

    static string toJson(const Payment &payment)
    {

        stringstream stringBuilder;

        stringBuilder << "{\"correlationId\": \"";
        stringBuilder << payment.correlationId;
        stringBuilder << "\", \"amount\": ";
        stringBuilder << to_string(payment.amount);
        stringBuilder << ", \"requestedAt\" : \"";
        stringBuilder << payment.requestedAt;
        stringBuilder << "\"}";

        return stringBuilder.str();
    }
};

/**
 * @class PaymentsUtils
 * @brief Classe utilitária para manipular a tabela de pagamentos no banco de dados SQLite.
 */
class PaymentsUtils
{
public:
    /**
     * @brief Inicializa a tabela de pagamentos no banco de dados SQLite.
     *
     * Cria a tabela de pagamentos e o índice para a coluna correlationId se não existirem.
     *
     * @param database Ponteiro para o objeto sqlite3.
     *
     * @note Essa função deve ser chamada apenas uma vez durante a inicialização do sistema.
     */
    static void init(sqlite3 *database)
    {

        const char *SQL_QUERY = R"(
            CREATE TABLE IF NOT EXISTS payments (
                correlationId TEXT NOT NULL,
                amount REAL NOT NULL,
                requestedAt DATETIME NOT NULL,
                defaultService TINYINT NOT NULL,
                processed TINYINT NOT NULL
            );

            CREATE INDEX IF NOT EXISTS idx_requestedAt ON payments (requestedAt);
            
            CREATE VIEW IF NOT EXISTS payments_default AS SELECT correlationId, amount, requestedAt FROM payments WHERE processed = 1 AND defaultService = 1;

            CREATE VIEW IF NOT EXISTS payments_fallback AS SELECT correlationId, amount, requestedAt FROM payments WHERE processed = 1 AND defaultService = 0;
        )";

        char *error;

        int response = sqlite3_exec(database, SQL_QUERY, nullptr, nullptr, &error);

        if (response != SQLITE_OK)
        {
            LOGGER::error(string("Erro ao criar tabela payments: ") + string(error));

            sqlite3_free(error);
        }
        else
        {
            LOGGER::info("Tabela de pagamentos OK");
        }
    }

    /**
     * @brief Insere um registro na tabela payments.
     *
     * @param database Ponteiro para o objeto sqlite3.
     * @param payment Registro de pagamento a ser inserido.
     * @param defaultService Flag que indica se servico 'default' foi usado
     * @param processed Flag indicando se o pagamento foi processado ou não
     * @return bool True se o registro foi inserido com sucesso, false caso contrário.
     */
    static bool insert(sqlite3 *database, const Payment &payment, bool defaultService, bool processed)
    {
        const char *sql = R"(
            INSERT INTO payments (correlationId, amount, requestedAt, defaultService, processed) VALUES (?, ?, ?, ?, ?);
        )";

        sqlite3_stmt *statement;

        int response = sqlite3_prepare_v2(database, sql, -1, &statement, nullptr);

        if (response != SQLITE_OK)
        {
            LOGGER::error(string("Erro ao preparar a query: ") + string(sqlite3_errmsg(database)));

            return false;
        }

        sqlite3_bind_text(statement, 1, payment.correlationId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(statement, 2, payment.amount);
        sqlite3_bind_text(statement, 3, payment.requestedAt.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(statement, 4, defaultService ? 1 : 0);
        sqlite3_bind_int(statement, 5, processed ? 1 : 0);

        response = sqlite3_step(statement);

        if (response != SQLITE_DONE)
        {
            LOGGER::error(string("Erro ao executar a query: ") + string(sqlite3_errmsg(database)));

            sqlite3_finalize(statement);

            return false;
        }

        sqlite3_finalize(statement);

        return true;
    }

    /**
     * @brief Calcula o total da coluna amount da tabela payments com base nos parâmetros fornecidos.
     *
     * @param database Ponteiro para o objeto sqlite3.
     * @param defaultService Indica se deve considerar apenas os registros com defaultService = 1.
     * @param from Data e hora inicial para a cláusula BETWEEN na coluna requestedAt.
     * @param to Data e hora final para a cláusula BETWEEN na coluna requestedAt.
     * @return double O total da coluna amount.
     */
    static double getTotalAmount(sqlite3 *database, bool defaultService, const string &from, const string &to)
    {
        string SQL_QUERY = "SELECT SUM(amount) FROM payments_default WHERE strftime('%s', requestedAt) >=  strftime('%s', ?) AND strftime('%s', requestedAt) <= strftime('%s', ?)";

        auto extractResult = [](sqlite3_stmt *statement)
        {
            return sqlite3_column_double(statement, 0);
        };

        return executePaymentQuery<double>(database, SQL_QUERY, from, to, defaultService, extractResult);
    }

    /**
     * @brief Calcula o total de registros da tabela payments com base nos parâmetros fornecidos.
     *
     * @param database Ponteiro para o objeto sqlite3.
     * @param defaultService Indica se deve considerar apenas os registros com defaultService = 1.
     * @param from Data e hora inicial para a cláusula BETWEEN na coluna requestedAt.
     * @param to Data e hora final para a cláusula BETWEEN na coluna requestedAt.
     * @return int O total de registros.
     */
    static int getTotalRecords(sqlite3 *database, bool defaultService, const string &from, const string &to)
    {
        string SQL_QUERY = "SELECT COUNT(*) FROM payments_default WHERE strftime('%s', requestedAt) >=  strftime('%s', ?) AND strftime('%s', requestedAt) <= strftime('%s', ?)";

        auto extractResult = [](sqlite3_stmt *statement)
        {
            return sqlite3_column_int(statement, 0);
        };

        return executePaymentQuery<int>(database, SQL_QUERY, from, to, defaultService, extractResult);
    }

    /**
     * @brief Deleta todos os registros da tabela payments.
     *
     * @return bool Indica se a operação foi bem-sucedida.
     */
    static bool deleteAllPayments()
    {

        sqlite3 *database = SQLiteDatabaseUtils::openConnection(Constants::DATABASE_PAYMENTS);

        // Erro ao abrir a conexão
        if (database == nullptr)
        {
            return false;
        }

        // Preparar a query
        sqlite3_stmt *statement;
        const char *SQL_QUERY = "DELETE FROM payments";

        int responseCode = sqlite3_prepare_v2(database, SQL_QUERY, -1, &statement, 0);

        // Erro ao preparar a query
        if (responseCode != SQLITE_OK)
        {
            return false;
        }

        // Executa a query
        responseCode = sqlite3_step(statement);

        // Finalizar a query e fechar a conexão
        sqlite3_finalize(statement);

        SQLiteDatabaseUtils::closeConnection(database);

        return (responseCode == SQLITE_DONE);
    }

private:
    /**
     * @brief Executa uma consulta SQL para obter dados de pagamentos.
     *
     * @param database A conexão com o banco de dados.
     * @param query A consulta SQL a ser executada.
     * @param from A data de início.
     * @param to A data de fim.
     * @param defaultService Indica se o serviço padrão deve ser usado.
     * @param extractResult Função que extrai o resultado da query.
     * @return T O resultado da consulta.
     */
    template <typename T>
    static T executePaymentQuery(sqlite3 *database, std::string query, const string &from, const string &to, bool defaultService, function<T(sqlite3_stmt *)> extractResult)
    {
        if (!defaultService)
        {
            size_t pos = query.find("payments_default");
            if (pos != std::string::npos)
            {
                query.replace(pos, 16, "payments_fallback");
            }
        }

        auto bindParams = [&](sqlite3_stmt *statement)
        {
            sqlite3_bind_text(statement, 1, from.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(statement, 2, to.c_str(), -1, SQLITE_STATIC);
        };

        return executeQuery<T>(database, query, bindParams, extractResult);
    }

    /**
     * @brief Executa uma query no banco de dados e retorna o resultado.
     *
     * @param database Ponteiro para o objeto sqlite3.
     * @param query A query a ser executada.
     * @param bindParams Função que faz o bind dos parâmetros da query.
     * @param extractResult Função que extrai o resultado da query.
     * @return T O resultado da query.
     */
    template <typename T>
    static T executeQuery(sqlite3 *database, const string &query, function<void(sqlite3_stmt *)> bindParams, function<T(sqlite3_stmt *)> extractResult)
    {
        sqlite3_stmt *statement;
        T result;

        // Erro ao abrir a conexão
        if (database == nullptr)
        {
            return -1;
        }

        // Preparar a query
        int responseCode = sqlite3_prepare_v2(database, query.c_str(), -1, &statement, 0);

        // Erro ao preparar a query
        if (responseCode != SQLITE_OK)
        {
            return -1;
        }

        // Bind dos parâmetros
        bindParams(statement);

        // Executa a query
        responseCode = sqlite3_step(statement);

        if (responseCode == SQLITE_ROW)
        {
            result = extractResult(statement);
        }

        // Finaliza a query
        sqlite3_finalize(statement);

        return result;
    }
};

/**
 * @class PaymentsDatabaseWriter
 * @brief Gerencia a escrita de pagamentos no banco de dados de forma segura e eficiente.
 *
 * Essa classe utiliza uma única thread dedicada para guardar em uma fila os pagamentos e a serem persistidos.
 */
class PaymentsDatabaseWriter
{
public:
    /**
     * @brief Constrói um objeto PaymentsDatabaseWriter.
     *
     * @param _connectionPoolUtils O pool de conexões com o banco de dados.
     */
    PaymentsDatabaseWriter(SQLiteConnectionPoolUtils &_connectionPoolUtils)
        : connectionPoolUtils(_connectionPoolUtils), isRunning(true)
    {
        threadWriter = thread([this]()
                              { savePayments(); });
    }

    /**
     * @brief Adiciona um pagamento à fila.
     *
     * @param payment O pagamento a ser salvo no banco de dados.
     */
    void addPaymentToQueue(const Payment &payment)
    {
        lock_guard<mutex> lock(mutualExclusionLock);
        paymentsQueue.push(payment);
        conditionVariable.notify_one();
    }

    /**
     * @brief Destrói o objeto PaymentsDatabaseWriter.
     *
     * Para a thread dedicada e limpa a fila de pagamentos.
     */
    ~PaymentsDatabaseWriter()
    {
        stop();
    }

    /**
     * @brief Para a thread dedicada e limpa a fila de pagamentos.
     */
    void stop()
    {
        {
            lock_guard<mutex> lock(mutualExclusionLock);
            isRunning = false;
        }
        conditionVariable.notify_all();
        if (threadWriter.joinable())
        {
            threadWriter.join();
        }
    }

private:
    /**
     * @brief Função que é executada pela thread dedicada.
     *
     * Lê os dados da fila de pagamentos e os escreve no banco de dados.
     */
    void savePayments()
    {
        while (true)
        {
            Payment payment;
            {
                unique_lock<mutex> lock(mutualExclusionLock);
                conditionVariable.wait(lock, [this]()
                                       { return !paymentsQueue.empty() || !isRunning; });
                if (!isRunning && paymentsQueue.empty())
                {
                    return; // Sai da thread se não estiver mais rodando e a fila estiver vazia
                }
                if (paymentsQueue.empty())
                {
                    continue; // Volta a esperar se a fila estiver vazia
                }
                payment = paymentsQueue.front();
                paymentsQueue.pop();
            }

            sqlite3 *database = connectionPoolUtils.getConnectionFromPool();

            PaymentsUtils::insert(database, payment, payment.defaultService, payment.processed);

            connectionPoolUtils.returnConnectionToPool(database);
        }
    }

    /**
     * @brief  O pool de conexões com o banco de dados.
     */
    SQLiteConnectionPoolUtils &connectionPoolUtils;

    /**
     * @brief A thread dedicada para escrever os dados.
     */
    thread threadWriter;

    /**
     * @brief O mutex para sincronizar o acesso à fila.
     */
    mutex mutualExclusionLock;

    /**
     * @brief A variável de condição para notificar a thread dedicada.
     */
    condition_variable conditionVariable;

    /**
     * @brief  A fila de dados a serem escritos.
     */
    queue<Payment> paymentsQueue;

    /**
     * @brief  Indica se a thread dedicada está em execução.
     */
    bool isRunning;
};

/**
 * @brief Classe responsável por lidar com pagamentos.
 *
 * Essa classe fornece métodos estáticos para lidar com requisições de pagamento, incluindo
 * a criação de pagamentos e o cálculo do total de pagamentos em um período.
 */
class PaymentsProcessor
{
public:
    /**
     * @brief Lida com requisições POST /payments.
     *
     * Esse método verifica se a requisição é válida, parseia o corpo da requisição,
     * cria um pagamento e o armazena na fila compartilhada entre as threads.
     *
     * @param body Corpo da requisição.
     * @param paymentsDatabaseWriter Classe que gerencia a escrita de pagamentos no banco de dados de forma segura e eficiente.
     * @return Resposta HTTP.
     */
    static map<string, string> payment(const string &body, PaymentsDatabaseWriter &paymentsDatabaseWriter)
    {
        Timer timer;

        map<string, string> responseMap = {
            {"status", ""},
            {"response", ""}};

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
            // Retorna um json de request invalida (faltou parametro 'amount')
            responseMap["status"] = Constants::BAD_REQUEST_RESPONSE;
            responseMap["response"] = "{ \"message\":\"Invalid params. Missing 'amount'\" }";

            return responseMap;
        }

        // Parse do corpo da requisição
        map<string, string> json = JsonParser::parseJson(body);

        Payment payment;

        /**
         * @todo Descomentar linha abaixo pois o valor vai ser enviado na request
         */
        // payment.correlationId = json.at(Constants::KEY_CORRELATION_ID);
        payment.correlationId = UUIDGenerator::createUUID();
        payment.amount = stod(json.at(Constants::KEY_AMOUNT));
        payment.requestedAt = TimeUtils::getTimestampUTC();

        /**
         * @todo Débito técnico, o algoritmo deve usar a informação do Delay para escolher qual utilizar
         */
        bool useDefaultService = HealthCheckUtils::useDefault();
        bool useFallbackService = false;

        if (!useDefaultService)
        {
            useFallbackService = HealthCheckUtils::useFallback();
        }

        if (useDefaultService)
        {
            LOGGER::info("Usando 'default' payment service: " + Constants::PROCESSOR_DEFAULT);

            string payload = PaymentsJSONConverter::toJson(payment);
            string defaultResponseBuffer;
            string URL = Constants::PROCESSOR_DEFAULT + Constants::PAYMENTS_ENDPOINT;

            CURL *curl = CURLUtils::setupCurlForPostRequest(URL, payload, defaultResponseBuffer);

            if (curl)
            {
                struct curl_slist *headers = NULL;
                headers = curl_slist_append(headers, "Content-Type: application/json");
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

                // Faz request para o servico 'default'
                CURLcode responseCode = curl_easy_perform(curl);

                if (responseCode != CURLE_OK)
                {
                    LOGGER::error(string("Erro ao fazer curl request para /payments 'default': ") + string(curl_easy_strerror(responseCode)));
                }
                else
                {
                    long HTTP_RESPONSE_CODE = 0;
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &HTTP_RESPONSE_CODE);

                    LOGGER::info("Service /payments 'default' respondeu com o código:  " + to_string(HTTP_RESPONSE_CODE));

                    /**
                     * @todo Débito técnico, código duplicado
                     */
                    payment.defaultService = true;
                    payment.processed = (HTTP_RESPONSE_CODE == 200);

                    // Adiciona na fila do PaymentsDatabaseWriter para ser persistido
                    paymentsDatabaseWriter.addPaymentToQueue(payment);

                    if (HTTP_RESPONSE_CODE == 200)
                    {
                        map<string, string> jsonResponse = JsonParser::parseJson(defaultResponseBuffer);

                        // Inserir o registro de payments processado pelo payments default
                        stringstream stringBuilder;
                        stringBuilder << "Inserindo Payment(correlationId=";
                        stringBuilder << payment.correlationId;
                        stringBuilder << ", amount=";
                        stringBuilder << to_string(payment.amount);
                        stringBuilder << ", requestedAt=";
                        stringBuilder << payment.requestedAt;
                        stringBuilder << ", defaultService=true, processed=";
                        stringBuilder << to_string(HTTP_RESPONSE_CODE == 200);
                        stringBuilder << ")";

                        LOGGER::info(stringBuilder.str());

                        responseMap["status"] = Constants::CREATED_RESPONSE;
                        responseMap["response"] = "{ \"message\":\"" + jsonResponse.at("message") + "\", \"payment\": " + PaymentsJSONConverter::toJson(payment) + "}";
                    }
                    else
                    {
                        responseMap["status"] = Constants::BAD_REQUEST_RESPONSE;
                        responseMap["response"] = "Erro na request payload: " + payload;
                    }
                }

                curl_easy_cleanup(curl);
                curl_slist_free_all(headers);
            }
        }
        else
        {
            if (useFallbackService)
            {
                LOGGER::info("Usando 'fallback' payment service:  " + Constants::PROCESSOR_FALLBACK);

                string payload = PaymentsJSONConverter::toJson(payment);
                string URL = Constants::PROCESSOR_FALLBACK + Constants::PAYMENTS_ENDPOINT;
                string fallbackResponseBuffer;

                CURL *curl = CURLUtils::setupCurlForPostRequest(URL, payload, fallbackResponseBuffer);

                if (curl)
                {
                    struct curl_slist *headers = NULL;
                    headers = curl_slist_append(headers, "Content-Type: application/json");
                    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

                    // Faz request para o servico 'fallback'
                    CURLcode responseCode = curl_easy_perform(curl);

                    if (responseCode != CURLE_OK)
                    {
                        LOGGER::error(string("Erro ao fazer curl request para /payments 'fallback': ") + string(curl_easy_strerror(responseCode)));
                    }
                    else
                    {

                        long HTTP_RESPONSE_CODE = 0;
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &HTTP_RESPONSE_CODE);

                        LOGGER::info("Service /payments 'fallback' respondeu com o código:  " + to_string(HTTP_RESPONSE_CODE));

                        /**
                         * @todo Débito técnico, código duplicado
                         */
                        payment.defaultService = false;
                        payment.processed = (HTTP_RESPONSE_CODE == 200);

                        // Adiciona na fila do PaymentsDatabaseWriter para ser persistido
                        paymentsDatabaseWriter.addPaymentToQueue(payment);

                        if (HTTP_RESPONSE_CODE == 200)
                        {
                            map<string, string> jsonResponse = JsonParser::parseJson(fallbackResponseBuffer);

                            // Inserir o registro de payments processado pelo payments default
                            stringstream stringBuilder;
                            stringBuilder << "Inserindo Payment(correlationId=";
                            stringBuilder << payment.correlationId;
                            stringBuilder << ", amount=";
                            stringBuilder << to_string(payment.amount);
                            stringBuilder << ", requestedAt=";
                            stringBuilder << payment.requestedAt;
                            stringBuilder << ", defaultService=false, processed="; // Unica coisa eu muda do default
                            stringBuilder << to_string(HTTP_RESPONSE_CODE == 200);
                            stringBuilder << ")";

                            LOGGER::info(stringBuilder.str());

                            responseMap["status"] = Constants::CREATED_RESPONSE;
                            responseMap["response"] = "{ \"message\":\"" + jsonResponse.at("message") + "\", \"payment\": " + PaymentsJSONConverter::toJson(payment) + "}";
                        }
                        else
                        {
                            responseMap["status"] = Constants::BAD_REQUEST_RESPONSE;
                        }
                    }

                    curl_easy_cleanup(curl);
                    curl_slist_free_all(headers);
                }
            }
            else
            {

                /**
                 * @todo Implementar uma lógica para tratar esse cenário
                 * @todo Quando os dois serviços estão indisponíveis, não pode haver nenhum tipo de pagamento ?
                 * @todo Refazer a curl request após um timeout ?
                 */
                LOGGER::info("ALERTA!!! Nenhum serviço está funcionando, tanto o 'default' quanto o 'fallback'");
                LOGGER::info("Salvar o payment em alguma estrura e reprocessar após 5 segundos");

                responseMap["status"] = Constants::INTERNAL_SERVER_ERROR;
                responseMap["response"] = "{ \"message\": \"Erro interno do servidor\"}";
            }
        }

        return responseMap;
    }

    /**
     * @brief Lida com requisições GET /payments-summary.
     *
     * Esse método verifica se a query string é válida, calcula o total de pagamentos
     * no período especificado e retorna a resposta.
     *
     * @param query Query string da requisição.
     *
     * @return Um mapa contendo o código e a resposta.
     */
    static map<string, string> payments_summary(const string &query)
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

        pos = query.find("&to=");
        string from = query.substr(5, pos - 5);

        pos = query.find("to=");

        if (pos == string::npos)
        {

            responseMap["status"] = Constants::BAD_REQUEST_RESPONSE;
            responseMap["response"] = "{ \"message\":\"Invalid params. Missing 'to'\" }";

            return responseMap;
        }

        string to = query.substr(pos + 3);

        sqlite3 *database = SQLiteDatabaseUtils::openConnection(Constants::DATABASE_PAYMENTS);
        PaymentsSummary paymentSummary;

        /**
         * @details Sempre buscar primeiro no endpoint e caso não consiga, recuperar da base local (evitando phantom reads)
         */
        auto calculatePaymentSummary = [&](const string &URL, bool defaultService)
        {
            string serviceURL = URL + Constants::PAYMENTS_SUMMARY_ADMIN_ENDPOINT + "?" + query;

            string responseBuffer;

            CURL *curl = CURLUtils::setupCurlForGetRequest(serviceURL, responseBuffer);

            if (curl)
            {
                struct curl_slist *headers = NULL;
                headers = curl_slist_append(headers, "Content-Type: application/json");
                headers = curl_slist_append(headers, Constants::X_RINHA_TOKEN.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

                CURLcode responseCode = curl_easy_perform(curl);

                if (responseCode != CURLE_OK)
                {
                    LOGGER::error(string("Erro ao fazer curl request para /admin/payments-summary ") + string(defaultService ? "'default'" : "'fallback'") + string(curl_easy_strerror(responseCode)));
                }
                else
                {

                    long HTTP_RESPONSE_CODE = 0;
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &HTTP_RESPONSE_CODE);

                    LOGGER::info(string("Endpoint /admin/payments-summary ") + string(defaultService ? "'default'" : "'fallback'") + string(" respondeu com o código:  ") + to_string(HTTP_RESPONSE_CODE));

                    if (HTTP_RESPONSE_CODE == 200)
                    {
                        map<string, string> jsonResponse = JsonParser::parseJson(responseBuffer);

                        if (defaultService)
                        {
                            paymentSummary.defaultStats.totalRequests = stoi(jsonResponse.at("totalRequests"));
                            paymentSummary.defaultStats.totalAmount = stod(jsonResponse.at("totalAmount"));
                        }
                        else
                        {
                            paymentSummary.fallbackStats.totalRequests = stoi(jsonResponse.at("totalRequests"));
                            paymentSummary.fallbackStats.totalAmount = stod(jsonResponse.at("totalAmount"));
                        }
                    }
                    else
                    {
                        if (defaultService)
                        {
                            paymentSummary.defaultStats.totalRequests = PaymentsUtils::getTotalRecords(database, true, from, to);
                            paymentSummary.defaultStats.totalAmount = PaymentsUtils::getTotalAmount(database, true, from, to);
                        }
                        else
                        {
                            paymentSummary.fallbackStats.totalRequests = PaymentsUtils::getTotalRecords(database, false, from, to);
                            paymentSummary.fallbackStats.totalAmount = PaymentsUtils::getTotalAmount(database, false, from, to);
                        }
                    }
                }

                curl_easy_cleanup(curl);
                curl_slist_free_all(headers);
            }
        };

        calculatePaymentSummary(Constants::PROCESSOR_DEFAULT, true);
        calculatePaymentSummary(Constants::PROCESSOR_FALLBACK, false);

        SQLiteDatabaseUtils::closeConnection(database);

        // Retorna o total de pagamentos
        responseMap["status"] = Constants::OK_RESPONSE;
        responseMap["response"] = PaymentsJSONConverter::summaryToJson(paymentSummary);

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
     * @param paymentsDatabaseWriter Classe que gerencia a escrita de pagamentos no banco de dados de forma segura e eficiente.
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
    static void handle(int socket, PaymentsDatabaseWriter &paymentsDatabaseWriter)
    {

        // Define o tamanho do buffer para ler dados da conexão de rede.
        // O buffer tem um tamanho fixo de 512 bytes (definido em Constants::BUFFER_SIZE),
        // o que significa que o programa pode ler até 512 bytes de dados da conexão por vez.
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
            cout << endl;
            LOGGER::info("POST request para /payments");

            size_t bodyPos = request.find("\r\n\r\n");

            if (bodyPos != string::npos)
            {

                string body = request.substr(bodyPos + 4);
                map<string, string> response = PaymentsProcessor::payment(body, paymentsDatabaseWriter);

                string headers = response.at("status") + Constants::CONTENT_TYPE_APPLICATION_JSON + to_string(response.at("response").size()) + "\r\n\r\n";
                send(socket, headers.c_str(), headers.size(), 0);

                send(socket, response.at("response").c_str(), response.at("response").size(), 0);
            }
            else
            {
                string response = Constants::BAD_REQUEST_RESPONSE;
                send(socket, response.c_str(), response.size(), 0);
            }
        }
        else
        {

            if (method == "GET" && path.find(Constants::PAYMENTS_SUMMARY_ENDPOINT) == 0)
            {

                cout << endl;
                LOGGER::info("GET request para /payments-summary " + path);

                size_t queryPos = path.find("?");

                if (queryPos != string::npos)
                {
                    string query = path.substr(queryPos + 1);

                    map<string, string> response = PaymentsProcessor::payments_summary(query);

                    string headers = response.at("status") + Constants::CONTENT_TYPE_APPLICATION_JSON + to_string(response.at("response").size()) + "\r\n\r\n";
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

                cout << endl;
                LOGGER::info("POST request para /purge-payments");

                bool success = PaymentsUtils::deleteAllPayments();

                string msg = "Todas as tabelas do banco foram limpas! Eu espero que você saiba o que acabou de fazer.";

                LOGGER::info(msg);

                map<string, string> response = {
                    {"status", Constants::OK_RESPONSE},
                    {"response", "{ \"message\": \"" + msg + "\", \"success\": " + (success ? "true" : "false") + "}"}};

                string headers = response.at("status") + Constants::CONTENT_TYPE_APPLICATION_JSON + to_string(response.at("response").size()) + "\r\n\r\n";
                send(socket, headers.c_str(), headers.size(), 0);

                send(socket, response.at("response").c_str(), response.at("response").size(), 0);
            }
            else
            {
                cout << endl;
                LOGGER::info("Essa request não está mapeada");

                string response = Constants::NOT_FOUND_RESPONSE;
                send(socket, response.c_str(), response.size(), 0);
            }
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
    int socket_file_descriptor;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Criar o socket
    if ((socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        LOGGER::error("Falha ao criar o socket");
        return EXIT_FAILURE;
    }

    // Habilita a opção SO_REUSEADDR para permitir a reutilização da mesma porta,
    // mesmo se o socket estiver em um estado de espera (TIME_WAIT).
    // Isso evita erros de "endereço em uso" ao reiniciar o servidor.
    int ALLOW_REBIND_SAME_PORT = 1;
    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &ALLOW_REBIND_SAME_PORT, sizeof(ALLOW_REBIND_SAME_PORT)) < 0)
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
    if (bind(socket_file_descriptor, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        LOGGER::error("Falha ao tentar fazer o bind do socket ao IP:PORT");
        return EXIT_FAILURE;
    }

    // Escutar conexões
    if (listen(socket_file_descriptor, 3) < 0)
    {
        LOGGER::error("Falha ao escutar conexões");
        return EXIT_FAILURE;
    }

    // Inicializa modo multithread do SQLite
    if (!SQLiteDatabaseUtils::setUpMultiThreadedMode())
    {
        LOGGER::error("SQLite não está funcionando em modo multithead");
        return EXIT_FAILURE;
    };

    SQLiteConnectionPoolUtils connectionPoolUtils(2, 5000);
    PaymentsDatabaseWriter paymentsDataWriter(connectionPoolUtils);

    sqlite3 *database = connectionPoolUtils.getConnectionFromPool();

    LOGGER::info("Verificando tabelas no banco de dados");
    HealthCheckUtils::init();
    PaymentsUtils::init(database);

    connectionPoolUtils.returnConnectionToPool(database);

    LOGGER::info("Inicializando serviço de Health Check");
    HealthCheckServiceThread::init();

    cout << endl;
    LOGGER::info("Garnize on Juice iniciado na porta 9999, escutando somente requests POST e GET:");

    while (true)
    {
        // Aceitar uma conexão
        int new_socket = accept(socket_file_descriptor, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0)
        {
            LOGGER::error("Falha ao aceitar conexão");
            continue;
        }

        thread([new_socket, &paymentsDataWriter, &connectionPoolUtils]()
               { RequestHandler::handle(new_socket, paymentsDataWriter); })
            .detach();
    }

    return EXIT_SUCCESS;
}