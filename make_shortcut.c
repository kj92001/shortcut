#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  // chmod 함수를 위해 추가

typedef struct {
    GtkWidget *entry_name;
    GtkWidget *entry_exec;
    GtkWidget *entry_icon;
    GtkWidget *entry_wmclass;
    GtkWindow *window;
} AppWidgets;

// 파일 선택 완료 콜백
static void on_file_selected(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    AppWidgets *widgets = (AppWidgets *)user_data;
    GFile *file = gtk_file_dialog_open_finish(dialog, res, NULL);

    if (file) {
        char *path = g_file_get_path(file);
        gtk_editable_set_text(GTK_EDITABLE(widgets->entry_icon), path);
        g_free(path);
        g_object_unref(file);
    }
}

// 아이콘 탐색 버튼 클릭
static void on_icon_browse_clicked(GtkButton *btn, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "아이콘 파일 선택");
    gtk_file_dialog_open(dialog, widgets->window, NULL, on_file_selected, widgets);
    g_object_unref(dialog);
}

// .desktop 파일 생성 로직
static void on_create_clicked(GtkButton *btn, gpointer user_data) {
    AppWidgets *w = (AppWidgets *)user_data;
    const char *name = gtk_editable_get_text(GTK_EDITABLE(w->entry_name));
    const char *exec = gtk_editable_get_text(GTK_EDITABLE(w->entry_exec));
    const char *icon = gtk_editable_get_text(GTK_EDITABLE(w->entry_icon));
    const char *wm_class = gtk_editable_get_text(GTK_EDITABLE(w->entry_wmclass));

    if (strlen(name) == 0 || strlen(exec) == 0) {
        g_print("오류: 이름과 실행 경로는 필수입니다.\n");
        return;
    }

    char *filename = g_strdup_printf("%s.desktop", name);
    char *filepath = g_build_filename(g_get_user_data_dir(), "applications", filename, NULL);

    FILE *f = fopen(filepath, "w");
    if (f) {
        fprintf(f, "[Desktop Entry]\nVersion=1.0\nType=Application\n");
        fprintf(f, "Name=%s\nExec=%s\n", name, exec);
        fprintf(f, "Icon=%s\n", (strlen(icon) > 0) ? icon : "system-run");
        if (strlen(wm_class) > 0) {
            fprintf(f, "StartupWMClass=%s\n", wm_class);
        }
        fprintf(f, "Terminal=false\n");
        fclose(f);
        
        chmod(filepath, 0755); // 이제 정상 작동함
        g_print("성공: %s 저장 완료\n", filepath);
    }

    g_free(filename);
    g_free(filepath);
}

// 입력 필드 추가 헬퍼
GtkWidget* add_field(GtkWidget *box, const char *label_text) {
    gtk_box_append(GTK_BOX(box), gtk_label_new(label_text));
    GtkWidget *entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(box), entry);
    return entry;
}

static void activate(GtkApplication *app, gpointer user_data) {
    AppWidgets *widgets = g_new0(AppWidgets, 1);
    widgets->window = GTK_WINDOW(gtk_application_window_new(app));
    gtk_window_set_title(widgets->window, "Desktop Shortcut Maker");
    gtk_window_set_default_size(widgets->window, 400, -1);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    // gtk_widget_set_margin_all 대신 개별 마진 설정
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(widgets->window, box);

    widgets->entry_name = add_field(box, "앱 이름:");
    widgets->entry_exec = add_field(box, "실행 파일 경로:");

    gtk_box_append(GTK_BOX(box), gtk_label_new("아이콘 선택:"));
    GtkWidget *icon_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    widgets->entry_icon = gtk_entry_new();
    gtk_widget_set_hexpand(widgets->entry_icon, TRUE);
    GtkWidget *btn_browse = gtk_button_new_from_icon_name("folder-open-symbolic");
    g_signal_connect(btn_browse, "clicked", G_CALLBACK(on_icon_browse_clicked), widgets);
    
    gtk_box_append(GTK_BOX(icon_hbox), widgets->entry_icon);
    gtk_box_append(GTK_BOX(icon_hbox), btn_browse);
    gtk_box_append(GTK_BOX(box), icon_hbox);

    widgets->entry_wmclass = add_field(box, "Startup WM_CLASS:");

    GtkWidget *btn_save = gtk_button_new_with_label("저장하기");
    gtk_widget_set_margin_top(btn_save, 15);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_create_clicked), widgets);
    gtk_box_append(GTK_BOX(box), btn_save);

    gtk_window_present(widgets->window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.local.shortcutmaker", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

