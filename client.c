#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_MSG_LEN 1024 // 메시지의 최대 길이를 정의합니다.
#define PORT 8080        // 서버와 통신할 포트 번호를 정의합니다.

int main()
{
    int client_socket;              // 클라이언트 소켓을 생성할 변수를 선언합니다.
    struct sockaddr_in server_addr; // 서버와 통신할 주소 구조체를 선언합니다.

    // 클라이언트 소켓 생성
    client_socket = socket(AF_INET, SOCK_STREAM, 0); // IPv4와 TCP 프로토콜을 사용하는 소켓을 생성합니다.
    if (client_socket == -1)
    {
        perror("Socket creation error"); // 소켓 생성 오류 시 에러 메시지를 출력하고 프로그램을 종료합니다.
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;                     // 주소 체계를 설정합니다.
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버의 IP 주소를 설정합니다.
    server_addr.sin_port = htons(PORT);                   // 서버의 포트 번호를 설정합니다.

    // 서버에 연결
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Connection error"); // 연결 오류 시 에러 메시지를 출력하고 프로그램을 종료합니다.
        exit(EXIT_FAILURE);
    }

    char username[50]; // 사용자의 이름을 저장할 변수를 선언합니다.
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);           // 사용자로부터 이름을 입력받습니다.
    username[strcspn(username, "\n")] = '\0';           // 입력된 이름에서 개행 문자를 제거합니다.
    send(client_socket, username, sizeof(username), 0); // 서버에 사용자 이름을 전송합니다.

    pid_t pid = fork(); // 자식 프로세스를 생성합니다.
    if (pid == 0)
    {
        char message[MAX_MSG_LEN]; // 수신한 메시지를 저장할 변수를 선언합니다.
        ssize_t recv_size;

        while ((recv_size = recv(client_socket, message, sizeof(message), 0)) > 0)
        {
            message[recv_size] = '\0';
            printf("%s\n", message); // 발신자의 이름과 함께 메시지를 표시합니다.
        }

        printf("Disconnected from server.\n"); // 서버와의 연결이 종료되었음을 알립니다.
        exit(EXIT_SUCCESS);
    }

    char message[MAX_MSG_LEN]; // 사용자가 입력한 메시지를 저장할 변수를 선언합니다.
    while (1)
    {
        fgets(message, sizeof(message), stdin); // 사용자로부터 메시지를 입력받습니다.
        message[strcspn(message, "\n")] = '\0'; // 입력된 메시지에서 개행 문자를 제거합니다.
        if (send(client_socket, message, strlen(message), 0) == -1)
        {
            perror("Message send error"); // 메시지 전송 오류 시 에러 메시지를 출력합니다.
            break;
        }
    }

    close(client_socket); // 클라이언트 소켓을 닫습니다.
    return 0;
}

// Function to send a file
void send_file(int socket, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char buffer[MAX_MSG_LEN];
    while (fgets(buffer, MAX_MSG_LEN, file) != NULL) {
        send(socket, buffer, strlen(buffer), 0);
    }

    fclose(file);
}
