## Garnize on Juice $${\color{red}[in \space progress]}$$

Projeto desenvolvido em C++ para o desafio [Rinha de Backend - 2025](https://github.com/zanfranceschi/rinha-de-backend-2025).

Veja a seção `Detalhes Técnicos e Possíveis Melhorias` (mais abaixo neste README) para entender melhor a solução e também suas possíveis melhorias.

Veja a seção `Desafios` (mais abaixo neste README) para entender onde o caldo engrossou durante o desenvolvimento.

![Garnize On Juice](static/garnize-on-juice.png)

### O que é a Rinha do Backend

De acordo com o repositório oficial [zanfranceschi/rinha-de-backend-2025](https://github.com/zanfranceschi/rinha-de-backend-2025):

```
A Rinha de Backend é um desafio em que é necessário desenvolver uma solução backend em
qualquer tecnologia e tem como principal objetivo o aprendizado e compartilhamento de
conhecimento! Esta é a terceira edição do desafio.

```

### Estrutura do Projeto

```
.
├── compile.sh
├── docker-compose.yml
├── Dockerfile
├── LICENSE
├── README.md
├── src
│   └── main.cpp
└── static
    ├── basic-diagram.png
    ├── garnize-on-juice.png
    └── mesa-digitalizadora-wacom.jpg
```

### Como compilar e rodar o Projeto

Existe um script bash chamado `compile.sh` bastando torná-lo executável com a instrução `chmod +x compile.sh`.

Para usar o script, basta executá-lo com ou sem a flag `--debug`:

```bash
./compile.sh # Compila com flag de otimização
./compile.sh --debug # Compila para depuração
```

<sub>As instruções acima foram testadas somente em ambiente Linux.</sub>

### Rodar com Docker compose

@TODO

### Objetivo

Desenvolver uma API que intermedia pagamentos para dois serviços de processamento de pagamentos com a menor taxa, lidando com instabilidades nos serviços.

O objetivo é processar o máximo de pagamentos possível.

### Diagramas

Primeiro cenário desenvolvido:

- Requests para o processador de pagamentos padrão.

![Basic Diagram](static/basic-diagram.png)

Para criá-los utilizei uma mesa digitalizadora wacom.

![Wacom](static/mesa-digitalizadora-wacom.jpg)

@TODO

### Desafios

- Escrever a lógica do servidor utilizando sockets (tive que ler várias vezes o livro [Build Your Own Redis with C/C++](https://build-your-own.org/redis)) para tratar requisições simultâneas.
- Parsear o JSON sem usar nenhuma biblioteca (Ex.: ``nlohmann/json``).
- Chegar na expressão regular correta que limpava o JSON vindo da request body antes de tentar fazer o parsing.

@TODO

### Detalhes Técnicos e Possíveis Melhorias

#### Estratura de classes criada

@TODO

#### Explicação sobre a expressão regular utilizada para remover espaços em brancos desnecessários em um string JSON:

```c++
   static string removeUnnecessarySpaces(const string &jsonString)
   {
       return regex_replace(jsonString, regex("\\s+(?=(?:[^\"']*[\"'][^\"']*[\"'])*[^\"']*$)"), "");
   }
```

- `\\s+`: Essa parte da expressão regular busca por um ou mais espaços em branco (`\s` é o caractere especial para espaços em branco, e o `+` significa "um ou mais"). O `\\` é usado para escapar o caractere `\` porque em C++ o `\` é um caractere especial.
- `(?=...)`: Essa é uma "asserção de lookahead" positiva. Ela verifica se a expressão regular dentro dos parênteses é verdadeira, mas não consome os caracteres. Em outras palavras, ela verifica se a condição é satisfeita sem incluir os caracteres na correspondência.
- `([^\"']*[\"'][^\"']*[\"'])*`: Essa parte da expressão regular verifica se o espaço em branco está dentro ou fora de uma string delimitada por aspas. Ela funciona da seguinte forma:
- `[^\"']*`: Busca por zero ou mais caracteres que não são aspas (`[^\"']` é uma classe de caracteres negada que inclui todos os caracteres exceto aspas).
- `[\"']`: Busca por uma aspa (`[\"']` é uma classe de caracteres que inclui aspas duplas e simples).
- `[^\"']*`: Busca por zero ou mais caracteres que não são aspas novamente.
- `[\"']`: Busca por outra aspa.
- `*`: O asterisco significa "zero ou mais" da expressão anterior. Isso permite que a expressão regular verifique se há um número par de aspas (ou seja, se as aspas estão balanceadas).
- `[^\"']*$`: Essa parte da expressão regular verifica se a string restante não contém aspas. O `*` significa "zero ou mais" caracteres que não são aspas, e o `$` significa "fim da string".

#### O método `parseJson()` da classe `JsonParser` não trata todos possíveis casos que podem ocorrer em um JSON.

Abaixo, estão listados casos que devem ser tratados em um cenário mais real.

**1. Estrutura de objetos:**

- Objetos vazios (`{}`)
- Objetos com uma ou mais chaves-valor (`{"chave": "valor"}`)
- Objetos aninhados (`{"chave": {"outraChave": "valor"}}`)

**2. Tipos de valores:**

- Números (`123`, `3.14`)
- Booleanos (`true`, `false`)
- Null (`null`)
- Arrays (`[1, 2, 3]`)

**3. Chaves:**

- Chaves com strings complexas (`"chave com espaços" ou "chave com caracteres especiais: !@#$%^&*()"`)

**4. Valores:**

- Valores com strings complexas (`"valor com espaços" ou "valor com caracteres especiais: !@#$%^&*()"`)

**5. Arrays:**

- Arrays vazios (`[]`)
- Arrays com um ou mais elementos (`[1, 2, 3]`)
- Arrays aninhados (`[1, [2, 3], 4]`)

**6. Erros de sintaxe:**

- Chaves ou valores não fechados (`{"chave": "valor"`)
- Vírgulas ou dois-pontos faltando (`{"chave" "valor"}`)
- Caracteres inválidos

**7. Espaçamento:**

- Espaçamento dentro de strings (`"valor com espaços"`)

#### Por que inicializar váriáveis estáticas declaradas dentro de uma classe, fora dela ?

Isso é necessário devido à forma como as variáveis estáticas são tratadas em C++.

Quando você declara uma variável estática dentro de uma classe, você está apenas declarando que a variável existe e tem um determinado tipo. No entanto, a variável em si não é criada até que seja definida fora da classe.

A declaração da variável estática dentro da classe é como uma promessa ao compilador de que a variável será definida em algum lugar. A definição da variável fora da classe é onde a variável é realmente criada e alocada memória.

Se você não definir a variável estática fora da classe, o compilador saberá que a variável existe, mas o linker não encontrará a definição da variável e irá gerar um erro de "undefined reference".

Isso é uma regra do padrão C++ para evitar problemas de múltiplas definições de variáveis estáticas em diferentes unidades de compilação.

## Referências

[Rinha de Backend 2025](https://github.com/zanfranceschi/rinha-de-backend-2025)

[Debug C++ in Visual Studio Code](https://code.visualstudio.com/docs/cpp/cpp-debug)

[Cpp Reference](https://cppreference.com/)

[ISO Cpp](https://isocpp.org/)

[The Shell Scripting Tutorial](https://www.shellscript.sh/)

[Docker](https://docs.docker.com/)

[Build Your Own Redis with C/C++](https://build-your-own.org/redis/)

[The Linux Programming Interface by Michael Kerrisk.](https://man7.org/tlpi/)

[Mesa Digitalizadora One By Wacom CTL472, Pequena, Cor Preto e Vermelho](https://www.mercadolivre.com.br/mesa-digitalizadora-one-by-wacom-ctl472-pequena-cor-preto-e-vermelho)

# License

Please see the [license agreement](https://github.com/julianomacielferreira/garnize-on-juice/blob/main/LICENSE).
