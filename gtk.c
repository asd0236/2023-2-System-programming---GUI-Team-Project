#define MAX_MSG_LEN 1024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

GtkWidget *text_view, *entry, *window;
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

void send_file(int socket, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }

    char buffer[MAX_MSG_LEN];
    while (fgets(buffer, MAX_MSG_LEN, file) != NULL)
    {
        send(socket, buffer, strlen(buffer), 0);
    }

    fclose(file);
}
void upload_file(GtkWidget *widget, gpointer data)
{
    GtkWidget *file_chooser;
    const gchar *file_path;

    // 파일 선택 다이얼로그 생성
    file_chooser = gtk_file_chooser_dialog_new("파일 선택", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN,
                                               "취소", GTK_RESPONSE_CANCEL,
                                               "열기", GTK_RESPONSE_ACCEPT, NULL);

    // 파일 다이얼로그 실행 및 사용자 선택 확인
    if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT)
    {
        // 선택한 파일의 경로를 가져옴
        file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
        send_file(client_socket, file_path);



        g_free((gpointer)file_path); // 메모리 해제
    }

    gtk_widget_destroy(file_chooser);
}

// 파일 다운로드 버튼 콜백 함수
void download_file(GtkWidget *widget, gpointer data)
{
    

    // 파일 요청 메시지
    const char *file_request_message = "REQUEST_FILE:testFile.txt";
    send(client_socket, file_request_message, strlen(file_request_message), 0);

    

    // 서버로부터 파일 데이터 수신
    char buffer[MAX_MSG_LEN];
    ssize_t bytes_received;
    
    FILE *file = fopen("downloaded_file.txt", "w"); // 로컬에 저장할 파일
    if (file == NULL)
    {
        perror("Error creating file");
        return;
    }
    
    if ((bytes_received = recv(client_socket, buffer, MAX_MSG_LEN, 0)) > 0)
    {
        fwrite(buffer, sizeof(char), bytes_received, file); // 파일에 데이터 쓰기
    }
    
    fclose(file);
    fprintf(stdout, "File downloaded successfully.\n");
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
