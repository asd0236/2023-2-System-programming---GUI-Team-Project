// 컴파일 명령어: gcc -o gtk_chat gtk_chat.c `pkg-config --cflags --libs gtk+-3.0`

#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

GtkWidget *text_view;
GtkTextBuffer *buffer;
int client_socket;

void send_message(GtkWidget *widget, gpointer data) {
    const gchar *message;
    GtkTextIter start, end;

    // 텍스트 뷰에서 메시지를 가져옴
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    message = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    // 서버에 메시지 전송
    send(client_socket, message, strlen(message), 0);

    // 메시지 전송 후 텍스트 뷰 초기화
    gtk_text_buffer_set_text(buffer, "", -1);
}

// 받은 메시지를 텍스트 뷰에 업데이트하는 함수
void update_messages(const char *message) {
    GtkTextIter end;

    // 텍스트 버퍼의 끝 반복자를 가져옴
    gtk_text_buffer_get_end_iter(buffer, &end);

    // 받은 메시지를 텍스트 버퍼에 추가
    gtk_text_buffer_insert(buffer, &end, message, -1);
}

// 메시지 수신을 처리하는 스레드 함수
void *receive_messages(void *arg) {
    char message[1024];
    ssize_t recv_size;

    // 서버로부터 메시지 수신
    while ((recv_size = recv(client_socket, message, sizeof(message), 0)) > 0) {
        message[recv_size] = '\0';
        update_messages(message);
    }

    // 서버와의 연결이 닫힘
    update_messages("서버 연결이 종료되었습니다.\n");

    // 스레드 종료
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    GtkWidget *window, *box, *entry, *button, *scroll;
    struct sockaddr_in server_addr;
    pthread_t recv_thread;

    gtk_init(&argc, &argv);

    // 메인 창 생성
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chatting Client");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 300, 200);

    // 수직 상자 생성
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    // 텍스트 뷰와 스크롤 윈도우 생성
    text_view = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), text_view);

    // 상자에 텍스트 뷰와 스크롤 윈도우 추가
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    // 텍스트 입력을 위한 엔트리 생성
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);

    // "Send" 버튼 생성
    button = gtk_button_new_with_label("Send");
    g_signal_connect(button, "clicked", G_CALLBACK(send_message), NULL);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

    // 윈도우 닫기 이벤트 설정
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 창에 있는 모든 위젯 표시
    gtk_widget_show_all(window);

    // 서버와의 연결 설정
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("소켓 생성 오류");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(8080);

    // 서버에 연결
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("서버 연결 오류");
        exit(EXIT_FAILURE);
    }

    // 사용자로부터 사용자 이름 입력
    char username[50];
    printf("사용자 이름을 입력하세요: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    // 사용자 이름을 서버에 전송
    send(client_socket, username, sizeof(username), 0);

    // 메시지 수신을 처리하는 스레드 생성
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
        perror("메시지 수신 스레드 생성 오류");
        exit(EXIT_FAILURE);
    }

    // GTK 메인 루프 실행
    gtk_main();

    // 클라이언트 소켓 및 스레드 정리
    close(client_socket);
    pthread_join(recv_thread, NULL);

    return 0;
}
