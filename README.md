# Final Project for INE5424 - Sistemas Operacionais II

Este projeto consiste em desenvolver uma biblioteca de comunicação capaz de garantir a
entrega confiável de mensagens entre os processos participantes de um grupo. Dessa
forma, programas que utilizem a biblioteca irão usufruir de garantias na entrega de
mensagens, como entrega confiável, difusão com entrega para todos os processos corretos
ou nenhum, ou, ainda, garantias de ordem na entrega, como ordenação FIFO e total.

# Execução da Aplicação

Para compilar e executar a aplicação que utiliza a biblioteca de comunicação, siga os passos abaixo:

## Passo 1: Preparação do Ambiente

1. **Certifique-se de ter o `g++` instalado:**
   - Você precisa ter o compilador `g++` instalado em seu sistema. Você pode verificar se ele está instalado executando o seguinte comando no terminal:
     ```bash
     g++ --version
     ```
   - Se o `g++` não estiver instalado, você pode instalá-lo usando seu gerenciador de pacotes. Por exemplo, no Ubuntu, você pode usar:
     ```bash
     sudo apt-get install g++
     ```

2. **Verifique a estrutura do projeto:**
   - Certifique-se de que sua estrutura de diretórios contém as pastas e arquivos necessários. Você deve ter algo semelhante a isso:
     ```
     nome_projeto/
     ├── include/
     │   └── ... (header files)
     ├── src/
     │   ├── application.cpp
     │   ├── reliable_comm.cpp
     │   ├── channels.cpp
     │   ├── main.cpp
     │   ├── message.cpp
     │   └── atomic_broadcast_ring.cpp
     ├── config.txt
     ├── Makefile
     └── ...
     ```

## Passo 2: Compilação do Projeto

1. **Abra o terminal e navegue até o diretório do seu projeto:**
   ```bash
   cd path/to/your_project
   ```
2. **Execute o comando make para compilar o projeto:**
   ```bash
   make
   ```

- Este comando irá compilar todos os arquivos .cpp em arquivos .o (objetos) usando as regras definidas no Makefile.

- Criar um executável (__my_project__).

3. **Verifique se a compilação foi bem-sucedida:**

Após a execução do comando make, verifique se o executável __my_project__ foi criado. Você pode listar os arquivos no diretório atual:
   ```bash
   ls

        nome_projeto/
     ├── include/
     │   └── ... (header files)
     ├── src/
     │   ├── application.cpp
     │   ├── reliable_comm.cpp
     │   ├── channels.cpp
     │   ├── main.cpp
     │   ├── message.cpp
     │   └── atomic_broadcast_ring.cpp
     ├── config.txt
     ├── Makefile
     ├── my_project
     └── ...
   ```
Se __my_project__ estiver presente, a compilação foi bem-sucedida.


## Passo 3: Arquivo `config.txt`

O arquivo `config.txt` é utilizado para configurar os nodos da rede distribuída e o tipo de broadcast a ser utilizado. Ele contém as seguintes seções principais:

### 1. Definição dos Nodos da Rede
- Cada nodo é identificado por um **ID único** (número inteiro) e seu respectivo **endereço IP:porta**.
- A lista de nodos define todos os participantes da rede distribuída.
- Exemplo de configuração:

  ```plaintext
  nodes = {{0, 127.0.0.1:3000}, {1, 127.0.0.1:3001}, {2, 127.0.0.1:3002}, {3, 127.0.0.1:3003}, {4, 127.0.0.1:3004}};
  ```

### 2. Definição dos tipo de broadcast
Especifica o método de envio de mensagens na rede.
Exemplo de configuração:
  ```plaintext
   broadcast = AB;
   ```
A opção AB indica o uso de Atomic Broadcast, garantindo que todas as mensagens sejam entregues aos nodos na mesma ordem, essencial para consistência em sistemas distribuídos.

## Passo 4: Execução da Aplicação

Agora que você tem o executável, você pode executá-lo diretamente no terminal. Abra um terminal para cada nodo da rede e execute duas aplicações que irão se comunicar. Para execução deve passar o nodo da rede e o arquivo de configuração:
```bash
./my_project 1 config.txt
```

```bash
./my_project 2 config.txt
```

```bash
./my_project 0 config.txt
```

## Passo 5: Limpeza dos Arquivos de Build

Se você deseja remover os arquivos objetos e o executável para liberar espaço, você pode usar o comando:
``` bash
make clean
```
Isso irá remover todos os arquivos .o e o executável __my_project__.