#include <stdio.h>      // for printf, scanf, perror
#include <stdlib.h>     // for exit
#include <string.h>     // for memset, strlen, strcmp
#include <unistd.h>     // for close, sleep
#include <arpa/inet.h>  // for inet_ntoa, inet_pton
#include <sys/socket.h> // for socket functions
#include <pthread.h>    // for threads

void *recebendo(void *arg){ // Função que Thread vai executar para receber mensagens
    int socket_entrada = *((int *) arg);
    int socket_conexao;
    struct sockaddr_in endereco_cliente; // Estrutura para armazenar o endereço do cliente
    socklen_t tamanho_endereco = sizeof(endereco_cliente); 
    char buffer[1024]; // Buffer para armazenar mensagens recebidas

    socket_conexao = accept(socket_entrada, (struct sockaddr *)&endereco_cliente, &tamanho_endereco);
    if (socket_conexao < 0) {
        perror("Erro ao aceitar conexão");
        pthread_exit(NULL);
    }
    else
        printf("Conexão aceita de %s:%d\n", inet_ntoa(endereco_cliente.sin_addr), ntohs(endereco_cliente.sin_port));
    while (1) {
        memset(buffer, 0, sizeof(buffer)); // Limpa o buffer antes de receber uma nova mensagem
        ssize_t bytes_recebidos = recv(socket_conexao, buffer, sizeof(buffer) - 1, 0); // Usamos sizeof(buffer) - 1 para deixar espaço para o terminador nulo \0
        if (bytes_recebidos > 0){
            printf("\rMensagem Recebida: %s\n", buffer);
        }
        else if (bytes_recebidos == 0){
            printf("Conexão encerrada pelo cliente %s:%d\n", inet_ntoa(endereco_cliente.sin_addr), ntohs(endereco_cliente.sin_port));
            close(socket_conexao);
            pthread_exit(NULL);
        }
        else {
            perror("Erro ao receber mensagem");
            close(socket_conexao);
            pthread_exit(NULL);
        }
    }
    return NULL;
}

void enviando(int socket_cliente, const char *mensagem){ // Função para enviar mensagens
    ssize_t bytes_enviados = send(socket_cliente, mensagem, strlen(mensagem), 0);
    if (bytes_enviados < 0) {
        perror("Erro ao enviar mensagem");
    }
    else {
        printf("Mensagem enviada: %s\n", mensagem);
    }
}

int main(){
    int porta = 8080; // Porta padrão para ambos os peers
    int socket_entrada, socket_cliente;
    struct sockaddr_in endereco_servidor, endereco_cliente;
    char IP_destino[20]; // Buffer para armazenar o IP de destino

    printf("Iniciando peer na porta %d...\n", porta);
    printf("Insira o IP do peer destino: ");
    scanf("%19s", IP_destino); //Limita a leitura para evitar overflow
    getchar(); // Limpa o buffer do stdin após scanf

    // Configuração do socket de entrada (servidor)
    socket_entrada = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_entrada < 0) {
        perror("Erro ao criar socket de entrada");
        return 1;
    }

    int opt = 1;
    if (setsockopt(socket_entrada, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) { // Permite reutilizar o endereço
        perror("setsockopt");
        exit(1);
    }
    memset(&endereco_servidor, 0, sizeof(endereco_servidor));

    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_addr.s_addr = INADDR_ANY; 
    endereco_servidor.sin_port = htons(porta);

    bind(socket_entrada, (struct sockaddr *)&endereco_servidor, sizeof(endereco_servidor));
    listen(socket_entrada, 1);

    // Criação da thread para receber mensagens
    // Multithreading é necessária para permitir envio e recebimento simultâneos
    pthread_t thread_recebimento;
    if (pthread_create(&thread_recebimento, NULL, recebendo, (void *)&socket_entrada) != 0) {
        perror("Erro ao criar thread de recebimento");
        close(socket_entrada);
        return 1;
    }

    // Configuração do socket cliente para enviar mensagens
    socket_cliente = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_cliente < 0) {
        perror("Erro ao criar socket cliente");
        close(socket_entrada);
        return 1;
    }
    memset(&endereco_cliente, 0, sizeof(endereco_cliente));
    endereco_cliente.sin_family = AF_INET;
    endereco_cliente.sin_port = htons(porta);
    inet_pton(AF_INET, IP_destino, &endereco_cliente.sin_addr); // Converte o IP de string para formato binário

    printf("Conectando ao peer %s na porta %d...\n", IP_destino, porta);
    while (connect(socket_cliente, (struct sockaddr *)&endereco_cliente, sizeof(endereco_cliente)) < 0) {
        sleep(1); // Espera 1 segundo antes de tentar novamente
    }
    printf("Conectado ao peer %s:%d\n", IP_destino, porta);

    // Loop para enviar mensagens
    char mensagem[1024];
    while (1) {
        printf("Digite a mensagem para enviar (ou 'sair' para encerrar): ");
        fgets(mensagem, sizeof(mensagem), stdin);
        mensagem[strcspn(mensagem, "\n")] = 0; // Remove o caractere de nova linha

        if (strcmp(mensagem, "sair") == 0) {
            break;
        }

        enviando(socket_cliente, mensagem);
    }

    close(socket_cliente);
    close(socket_entrada);


    return 0;

}