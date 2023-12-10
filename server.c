#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define MAX_MSG_LEN 1024
#define PORT 8080

typedef struct
{
    int socket;
    char username[50];
} Client;

Client clients[MAX_CLIENTS];
int num_clients = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg)
{
    int client_socket = *((int *)arg);
    char message[MAX_MSG_LEN];
    ssize_t recv_size;
    char username[50];

    while ((recv_size = recv(client_socket, message, sizeof(message), 0)) > 0)
    {
        // 파일 요청 메시지 확인
        if (strncmp(message, "REQUEST_FILE:", 13) == 0)
        {
            char *filename = message + 13; // 메시지에서 파일 이름 추출
            FILE *file = fopen(filename, "r");
            if (file == NULL)
            {
                perror("파일 열기 실패");
                continue;
            }

            char file_buffer[MAX_MSG_LEN];
            while (fgets(file_buffer, MAX_MSG_LEN, file) != NULL)
            {
                send(client_socket, file_buffer, strlen(file_buffer), 0);
            }

            fclose(file); // 파일이 제대로 닫히도록 함
            continue;     // 루프의 다음 반복으로 이동
        }
        else
        {
            message[recv_size] = '\0';

            // 송신자의 사용자 이름 찾기
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < num_clients; ++i)
            {
                if (clients[i].socket == client_socket)
                {
                    strcpy(username, clients[i].username);
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);

            // 메시지 앞에 사용자 이름을 추가
            char formatted_msg[MAX_MSG_LEN + 50];
            sprintf(formatted_msg, "%s: %s", username, message);

            // 사용자 이름을 포함한 메시지를 모든 클라이언트에게 브로드캐스트
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < num_clients; ++i)
            {
                if (clients[i].socket != client_socket)
                {
                    send(clients[i].socket, formatted_msg, strlen(formatted_msg), 0);
                }
            }
            pthread_mutex_unlock(&mutex);
        }
    }

    // 클라이언트가 연결 해제됨
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_clients; ++i)
    {
        if (clients[i].socket == client_socket)
        {
            printf("Client '%s' disconnected.\n", clients[i].username);

            // 연결 해제된 클라이언트 제거
            for (int j = i; j < num_clients - 1; ++j)
            {
                clients[j] = clients[j + 1];
            }
            num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    close(client_socket); // 클라이언트 소켓 닫기
    pthread_exit(NULL);
}

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0); // 서버 소켓 생성
    if (server_socket == -1)
    {
        perror("소켓 생성 실패");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;         // 주소 체계 설정
    server_addr.sin_addr.s_addr = INADDR_ANY; // 모든 가능한 IP 주소로부터 연결 수락
    server_addr.sin_port = htons(PORT);       // 포트 번호 설정

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("소켓 바인딩 실패");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) == -1)
    {
        perror("리스닝 실패");
        exit(EXIT_FAILURE);
    }

    printf("%d 포트에서 대기 중...\n", PORT);

    while (1)
    {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len); // 클라이언트의 연결을 수락
        if (client_socket == -1)
        {
            perror("연결 수락 실패");
            continue;
        }

        if (num_clients < MAX_CLIENTS)
        {
            char username[50];
            recv(client_socket, username, sizeof(username), 0); // 클라이언트로부터 사용자 이름 수신

            pthread_mutex_lock(&mutex);
            clients[num_clients].socket = client_socket; // 클라이언트 정보 저장
            strcpy(clients[num_clients].username, username);
            num_clients++;
            pthread_mutex_unlock(&mutex);

            printf("Client '%s'가 연결되었습니다.\n", username);

            pthread_t tid;
            if (pthread_create(&tid, NULL, handle_client, &client_socket) != 0)
            {
                perror("스레드 생성 실패");
                exit(EXIT_FAILURE);
            }

            pthread_detach(tid); // 스레드를 분리하여 자원 해제
        }
        else
        {
            char rejection_msg[] = "서버가 가득 찼습니다. 나중에 다시 시도하세요.\n";
            send(client_socket, rejection_msg, sizeof(rejection_msg), 0);
            close(client_socket);
        }
    }

    close(server_socket); // 서버 소켓 닫기
    return 0;
}
