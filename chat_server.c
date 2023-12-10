#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define MAX_MSG_LEN 1024
#define PORT 8080

typedef struct {
    int socket;
    char username[50];
} Client;

Client clients[MAX_CLIENTS];
int num_clients = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    char message[MAX_MSG_LEN];
    ssize_t recv_size;

    while ((recv_size = recv(client_socket, message, sizeof(message), 0)) > 0) {
        message[recv_size] = '\0';

        // 모든 클라이언트에게 메시지를 브로드캐스트
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_clients; ++i) {
            if (clients[i].socket != client_socket) {
                send(clients[i].socket, message, strlen(message), 0);
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    // 클라이언트가 연결 해제됨
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_clients; ++i) {
        if (clients[i].socket == client_socket) {
            printf("클라이언트 '%s' 연결 해제됨.\n", clients[i].username);

            // 연결 해제된 클라이언트 제거
            for (int j = i; j < num_clients - 1; ++j) {
                clients[j] = clients[j + 1];
            }
            num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    close(client_socket);

    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 소켓 생성
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("소켓 생성 오류");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 소켓을 포트에 바인딩
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("소켓 바인딩 오류");
        exit(EXIT_FAILURE);
    }

    // 연결을 대기
    if (listen(server_socket, 5) == -1) {
        perror("연결 대기 중 오류");
        exit(EXIT_FAILURE);
    }

    printf("포트 %d에서 대기 중...\n", PORT);

    while (1) {
        // 연결을 수락
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("연결 수락 오류");
            continue;
        }

        // 클라이언트 수 제한
        if (num_clients < MAX_CLIENTS) {
            // 클라이언트로부터 사용자 이름 읽기
            char username[50];
            recv(client_socket, username, sizeof(username), 0);

            // 클라이언트를 배열에 추가
            pthread_mutex_lock(&mutex);
            clients[num_clients].socket = client_socket;
            strcpy(clients[num_clients].username, username);
            num_clients++;
            pthread_mutex_unlock(&mutex);

            printf("클라이언트 '%s' 연결됨.\n", username);

            // 클라이언트를 처리하기 위한 새로운 쓰레드 생성
            pthread_t tid;
            if (pthread_create(&tid, NULL, handle_client, &client_socket) != 0) {
                perror("클라이언트 핸들링 쓰레드 생성 오류");
                exit(EXIT_FAILURE);
            }

            // 쓰레드 분리
            pthread_detach(tid);
        } else {
            // 클라이언트 수가 많아서 연결을 거부함
            char rejection_msg[] = "서버가 가득 찼습니다. 나중에 다시 시도하세요.\n";
            send(client_socket, rejection_msg, sizeof(rejection_msg), 0);
            close(client_socket);
        }
    }

    // 서버 소켓 닫기
    close(server_socket);

    return 0;
}
