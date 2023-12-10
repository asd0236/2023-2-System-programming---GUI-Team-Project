#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

GtkWidget *text_view, *entry;
GtkTextBuffer *buffer;
int client_socket;

// 메시지를 전송하는 함수
void send_message(GtkWidget *widget, gpointer data)
{
    const gchar *message;
    GtkTextIter end;

    message = gtk_entry_get_text(GTK_ENTRY(entry));   // 입력 필드에서 메시지 가져오기
    send(client_socket, message, strlen(message), 0); // 클라이언트 소켓을 통해 메시지 전송

    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, message, -1); // 채팅 창에 메시지 추가
    gtk_text_buffer_insert(buffer, &end, "\n", -1);    // 새로운 줄 추가
    gtk_entry_set_text(GTK_ENTRY(entry), "");          // 입력 필드 비우기
}

// 메시지를 채팅 창에 업데이트하는 함수
void update_messages(const char *message)
{
    GtkTextIter end;

    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, message, -1); // 채팅 창에 메시지 추가
    gtk_text_buffer_insert(buffer, &end, "\n", -1);    // 새로운 줄 추가
}

// 메시지 수신을 처리하는 스레드 함수
void *receive_messages(void *arg)
{
    char message[1024];
    ssize_t recv_size;

    while ((recv_size = recv(client_socket, message, sizeof(message), 0)) > 0)
    {
        message[recv_size] = '\0';
        gdk_threads_add_idle((GSourceFunc)update_messages, strdup(message)); // 메시지를 채팅 창에 업데이트
    }

    update_messages("서버와 연결이 끊어졌습니다.\n"); // 서버와 연결이 끊겼을 때 메시지 표시
    pthread_exit(NULL);
}

// 파일 업로드 버튼 콜백 함수
void upload_file(GtkWidget *widget, gpointer data)
{
    // 파일 업로드 로직을 여기에 구현
    // 현재는 빈 함수로 남겨두었으며, 필요한 로직을 추가해야 함
}

// 파일 다운로드 버튼 콜백 함수
void download_file(GtkWidget *widget, gpointer data)
{
    // 파일 다운로드 로직을 여기에 구현
    // 현재는 빈 함수로 남겨두었으며, 필요한 로직을 추가해야 함
}

int main(int argc, char *argv[])
{
    GtkWidget *window, *box, *button, *scroll, *upload_button, *download_button; // 파일 업로드 및 다운로드 버튼 추가
    struct sockaddr_in server_addr;
    pthread_t recv_thread;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "채팅 클라이언트");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 300, 200);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("전송");
    g_signal_connect(button, "clicked", G_CALLBACK(send_message), NULL);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

    // 파일 업로드 버튼 생성
    upload_button = gtk_button_new_with_label("Upload File");
    g_signal_connect(upload_button, "clicked", G_CALLBACK(upload_file), NULL);
    gtk_box_pack_start(GTK_BOX(box), upload_button, FALSE, FALSE, 0);

    // 파일 다운로드 버튼 생성
    download_button = gtk_button_new_with_label("Download File");
    g_signal_connect(download_button, "clicked", G_CALLBACK(download_file), NULL);
    gtk_box_pack_start(GTK_BOX(box), download_button, FALSE, FALSE, 0);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        perror("소켓 생성 오류");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버의 IP 주소
    server_addr.sin_port = htons(8080);                   // 서버의 포트 번호

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("연결 오류");
        exit(EXIT_FAILURE);
    }

    char username[50];
    printf("사용자 이름을 입력하세요: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';
    send(client_socket, username, sizeof(username), 0); // 사용자 이름을 서버에 전송

    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0)
    {
        perror("스레드 생성 오류");
        exit(EXIT_FAILURE);
    }

    gtk_main();

    close(client_socket);
    pthread_join(recv_thread, NULL);

    return 0;
}
