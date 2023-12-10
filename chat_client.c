#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_MSG_LEN 1024
#define PORT 8080

int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    // 소켓 생성
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("소켓 생성 오류");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버의 IP 주소로 변경
    server_addr.sin_port = htons(PORT);

    // 서버에 연결
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("서버 연결 오류");
        exit(EXIT_FAILURE);
    }

    // 사용자로부터 사용자 이름 입력
    char username[50];
    printf("사용자 이름을 입력하세요: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0'; // 개행 문자 제거

    // 사용자 이름을 서버에 전송
    send(client_socket, username, sizeof(username), 0);

    // 메시지 수신을 처리할 새로운 프로세스 생성
    pid_t pid = fork();
    if (pid == 0) {
        // 자식 프로세스 (메시지 수신)
        char message[MAX_MSG_LEN];
        ssize_t recv_size;

        while ((recv_size = recv(client_socket, message, sizeof(message), 0)) > 0) {
            message[recv_size] = '\0';
            printf("%s\n", message);
        }

        // 서버에서 연결 종료됨
        printf("서버와의 연결이 해제되었습니다.\n");

        // 자식 프로세스 종료
        exit(EXIT_SUCCESS);
    }

    // 부모 프로세스 (메시지 전송)
    char message[MAX_MSG_LEN];

    // 사용자로부터 메시지를 읽고 서버에 전송
    while (1) {
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0'; // 개행 문자 제거

        if (send(client_socket, message, strlen(message), 0) == -1) {
            perror("서버에 메시지 전송 오류");
            break;
        }
    }

    // 클라이언트 소켓 닫기
    close(client_socket);

    return 0;
}


