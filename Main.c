#include <gtk/gtk.h>
#include <windows.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <gio/gio.h>
#include <string.h>
GdkPixbuf *iconPixbuf;
GdkPixbuf *scaled_pixbuf4;
GtkWidget *window;
GtkListStore *store;
GtkWidget *treeview;
GtkWidget *Parentbox;
GtkWidget *labelStatus;
GtkWidget *labelPath;
GtkWidget *boxSearch;
GtkWidget *boxPath;
GtkWidget *box1;
GtkWidget *box2;
GtkWidget *box3;
GtkWidget *event1;
GtkWidget *event2;
GtkWidget *event3;
GtkWidget *event4;
GtkWidget *box;
GtkWidget *fullBox;
GdkPixbuf *pixbuf1;
GdkPixbuf *pixbuf2;
GtkWidget *image1;
GtkWidget *image2;
GtkWidget *scrolled_window;
GtkWidget *searchEntry;
GtkWidget *searchButton;
gchar pathway1[MAX_PATH + 1];
gchar FName1[MAX_PATH + 1];
int p = 0;
int i = 0;
int k = 0;
int l = 0;
long long files, dfiles;
long long cnt = 0, dnt = 0;
gdouble fraction = 0.1;
typedef struct nextPath
{
    char path[MAX_PATH + 1];
    struct nextPath *next;
} NextPath;
typedef struct backPath
{
    char path[MAX_PATH + 1];
    struct backPath *next;
} BackPath;

NextPath *head1 = NULL, *newNode1 = NULL;
BackPath *head2 = NULL, *newNode2 = NULL;
typedef struct
{
    GtkWidget *progress_bar;
    gdouble fraction;
    gdouble total_bytes;
    gdouble copied_bytes;
} CopyProgress;

long calculate_total_size(const char *path)
{
    DWORD att = GetFileAttributes(path);
    if (att == INVALID_FILE_ATTRIBUTES)
        return 0;

    long size = 0;

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        DIR *dp = opendir(path);
        if (!dp)
            return 0;
        struct dirent *entry;
        char fullPath[MAX_PATH];
        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, entry->d_name);
            size += calculate_total_size(fullPath);
        }
        closedir(dp);
    }
    else
    {
        FILE *f = fopen(path, "rb");
        if (f)
        {
            fseek(f, 0, SEEK_END);
            size = ftell(f);
            fclose(f);
        }
    }
    return size;
}
gboolean on_progress_close(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_widget_hide(widget);
    return TRUE;
}

long count_files(const char *path)
{
    DWORD att = GetFileAttributes(path);
    if (att == INVALID_FILE_ATTRIBUTES)
        return 0;

    long count = 0;

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        DIR *dp = opendir(path);
        if (!dp)
            return 0;

        struct dirent *entry;
        char fullPath[MAX_PATH];
        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, entry->d_name);
            count += count_files(fullPath);
        }

        closedir(dp);
    }
    else
    {
        count = 1;
    }

    return count;
}

GdkPixbuf *get_file_icon(const char *filepath, int size)
{
    GtkIconTheme *theme = gtk_icon_theme_get_default();
    GFile *file = g_file_new_for_path(filepath);
    GFileInfo *info = g_file_query_info(
        file,
        G_FILE_ATTRIBUTE_STANDARD_ICON "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
        G_FILE_QUERY_INFO_NONE,
        NULL,
        NULL);

    if (!info)
    {
        g_object_unref(file);
        return NULL;
    }

    GIcon *icon = g_file_info_get_icon(info);
    GtkIconInfo *icon_info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, 0);

    GdkPixbuf *pixbuf = NULL;

    if (!icon_info)
    {
        // fallback based on file extension
        const char *ext = strrchr(filepath, '.');
        if (ext)
        {
            if (g_strcmp0(ext, ".mp3") == 0 || g_strcmp0(ext, ".wav") == 0)
            {
                pixbuf = gtk_icon_theme_load_icon(theme, "audio-x-generic", size, 0, NULL);
            }
            else if (g_strcmp0(ext, ".mp4") == 0 || g_strcmp0(ext, ".avi") == 0)
            {
                pixbuf = gtk_icon_theme_load_icon(theme, "video-x-generic", size, 0, NULL);
            }
        }
    }
    else
    {
        GError *error = NULL;
        pixbuf = gtk_icon_info_load_icon(icon_info, &error);
        if (!pixbuf && error)
        {
            g_error_free(error);
        }
        g_object_unref(icon_info);
    }

    // Scale the pixbuf if it exists and is not already the desired size
    if (pixbuf && (gdk_pixbuf_get_width(pixbuf) != size || gdk_pixbuf_get_height(pixbuf) != size))
    {
        GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, size, size, GDK_INTERP_BILINEAR);
        g_object_unref(pixbuf);
        pixbuf = scaled;
    }

    g_object_unref(info);
    g_object_unref(file);

    return pixbuf;
}

void searchFiles(const char *basePath, const char *query, GtkListStore *store)
{
    DIR *dir = opendir(basePath);
    if (!dir)
        return;

    struct dirent *entry;
    char path[1024];
    struct stat st;

    while ((entry = readdir(dir)))
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);
        stat(path, &st);
        gchar *lower_name = g_utf8_strdown(entry->d_name, -1);
        gchar *lower_query = g_utf8_strdown(query, -1);

        if (g_strstr_len(lower_name, -1, lower_query) != NULL)
        {
            // show it in your treeview list
            struct stat fstat;
            stat(path, &fstat);
            GdkPixbuf *scaled_pixbuf;
            if (S_ISDIR(fstat.st_mode))
            {
                scaled_pixbuf = gdk_pixbuf_scale_simple(iconPixbuf, 48, 48, GDK_INTERP_BILINEAR);
            }
            else
            {
                scaled_pixbuf = get_file_icon(entry->d_name, 48);
                if (!scaled_pixbuf)
                {
                    scaled_pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                                             "text-x-generic", 48, 0, NULL);
                    // scaled_pixbuf = gdk_pixbuf_scale_simple(scaled_pixbuf, 48, 48, GDK_INTERP_BILINEAR);
                }
            }
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, scaled_pixbuf, 1, entry->d_name, 2, path, -1);
            if (scaled_pixbuf)
                g_object_unref(scaled_pixbuf);
        }
        g_free(lower_name);
        g_free(lower_query);
        if (S_ISDIR(st.st_mode))
        {
            // recursively search subfolders
            searchFiles(path, query, store);
        }
    }

    closedir(dir);
}
void on_search_clicked(GtkButton *button, gpointer user_data)
{
    const char *query = gtk_entry_get_text(GTK_ENTRY(user_data));
    gtk_list_store_clear(store);
    char currentPath[MAX_PATH] = "";
    getcwd(currentPath, sizeof(currentPath));
    searchFiles(currentPath, query, store);
    gtk_label_set_text(GTK_LABEL(labelStatus), "Search complete");
}

void openDirectory()
{
    gtk_box_pack_start(GTK_BOX(fullBox), boxSearch, FALSE, FALSE, 0);
    if (k == 1 || l == 1)
    {
        gtk_widget_set_opacity(event4, 1);
    }
    if (p == 0)
    {
        gtk_widget_set_opacity(event2, 0.1);
    }
    gtk_widget_set_opacity(event1, 1);
    gtk_widget_set_opacity(event3, 1);
    if (!gtk_widget_get_parent(boxSearch))
        gtk_box_pack_start(GTK_BOX(boxPath), boxSearch, TRUE, TRUE, 0);
    if (i > 0)
        gtk_widget_show(boxSearch);
    GtkTreeIter iter;
    gtk_list_store_clear(store);
    DIR *dp = opendir(".");
    gchar path[MAX_PATH + 1];
    getcwd(path, sizeof(path));
    gchar realPath[MAX_PATH + 50];
    sprintf(realPath, "Current path: %s", path);
    gtk_label_set_text(GTK_LABEL(labelPath), realPath);
    gtk_widget_show_all(Parentbox);
    int h = 0;
    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL)
    {
        DWORD att = GetFileAttributes(entry->d_name);
        if (att == INVALID_FILE_ATTRIBUTES)
        {
            continue;
        }
        if (att & FILE_ATTRIBUTE_HIDDEN || att & FILE_ATTRIBUTE_SYSTEM)
        {
            continue;
        }
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            h++;
            GError *error = NULL;
            // GdkPixbuf *pixbuf3 = gdk_pixbuf_new_from_file("D:\\Project-gtk\\ff.png", &error);
            // GdkPixbuf *scaled_pixbuf3 = gdk_pixbuf_scale_simple(pixbuf3, 40, 40, GDK_INTERP_BILINEAR);
            //  GtkWidget *image3 = gtk_image_new_from_pixbuf(scaled_pixbuf3);
            char fullPath[MAX_PATH] = "";
            snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, entry->d_name);
            struct stat fstat;
            stat(fullPath, &fstat);
            GdkPixbuf *scaled_pixbuf;
            if (S_ISDIR(fstat.st_mode))
            {
                scaled_pixbuf = gdk_pixbuf_scale_simple(iconPixbuf, 48, 48, GDK_INTERP_BILINEAR);
            }
            else
            {
                scaled_pixbuf = get_file_icon(entry->d_name, 48);
                if (!scaled_pixbuf)
                {
                    scaled_pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                                             "text-x-generic", 48, 0, NULL);
                    // scaled_pixbuf = gdk_pixbuf_scale_simple(scaled_pixbuf, 48, 48, GDK_INTERP_BILINEAR);
                }
            }
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, scaled_pixbuf, 1, entry->d_name, 2, fullPath, -1);
            if (scaled_pixbuf)
                g_object_unref(scaled_pixbuf);
        }
    }
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(scrolled_window));
    if (child)
    {
        gtk_container_remove(GTK_CONTAINER(scrolled_window), child);
    }

    if (h == 0)
    {
        GtkWidget *empty_label = gtk_label_new("This folder is empty");
        gtk_container_add(GTK_CONTAINER(scrolled_window), empty_label);
    }
    else
    {
        gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
    }

    gtk_widget_show_all(scrolled_window);
    closedir(dp);
}
void selectDirectory(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data)
{

    gtk_widget_set_opacity(event2, 0.1);
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        gchar *name;
        if (i == 0)
            gtk_tree_model_get(model, &iter, 1, &name, -1);
        else
            gtk_tree_model_get(model, &iter, 2, &name, -1);
        p = 0;
        if (i == 0)
        {
            gchar driveName[MAX_PATH + 1] = "";
            driveName[0] = name[strlen(name) - 3];
            driveName[1] = ':';
            driveName[2] = '\\';
            driveName[3] = '\\';
            driveName[4] = '\0';
            chdir(driveName);
            i++;
            openDirectory();
        }
        else
        {
            struct stat fstat;
            if (stat(name, &fstat) == 0)
            {
                if (S_ISDIR(fstat.st_mode))
                {
                    i++;
                    newNode2 = (BackPath *)malloc(sizeof(BackPath));
                    char currentPath[MAX_PATH] = "";
                    getcwd(currentPath, sizeof(currentPath));
                    strcpy(newNode2->path, currentPath);
                    newNode2->next = NULL;
                    if (head2 == NULL)
                    {
                        head2 = newNode2;
                    }
                    else
                    {
                        newNode2->next = head2;
                        head2 = newNode2;
                    }
                    chdir(name);
                    openDirectory();
                }
                else
                {
                    ShellExecute(NULL, "open", name, NULL, NULL, SW_SHOWNORMAL);
                }
            }
        }
        g_free(name);
    }
}
void showDrives()
{
    gtk_widget_set_opacity(event1, 0.1);
    gtk_widget_set_opacity(event3, 0.1);
    gtk_widget_set_opacity(event4, 0.1);
    if (gtk_widget_get_parent(boxSearch))
        gtk_widget_hide(boxSearch);
    gtk_label_set_text(GTK_LABEL(labelPath), "Devices and Drives");
    gtk_widget_set_name(labelPath, "labelPath");
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
                                    "#labelPath { font-size: 20px; font-family: Arial; font-weight: bold; }",
                                    -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    // GtkWidget *image3 = gtk_image_new_from_pixbuf(scaled_pixbuf3);
    GtkTreeIter iter;
    DWORD drives = GetLogicalDrives();
    gchar drive;
    for (drive = 'A'; drive <= 'Z'; drive++)
    {
        if (drives & (1 << (drive - 'A')))
        {
            gchar path[MAX_PATH + 1] = "";
            gchar name1[MAX_PATH + 1] = "";
            gchar name2[MAX_PATH + 1] = "";
            gchar realName[MAX_PATH + 1] = "";
            path[0] = drive;
            name2[0] = drive;
            strcat(path, ":\\");
            strcat(name2, ":");
            GetVolumeInformation(path, name1, sizeof(name1), NULL, NULL, NULL, NULL, 0);
            if (name1 != "")
            {
                strcat(name1, " ");
            }
            sprintf(realName, "%s(%s)", name1, name2);
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, scaled_pixbuf4, 1, realName, -1);
        }
    }
}
void goToParentFolder(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    gtk_widget_set_opacity(event2, 1);
    if (i != 0)
    {
        i--;
        p++;
        gchar currentPath[MAX_PATH + 1] = "";
        getcwd(currentPath, sizeof(currentPath));
        newNode1 = (NextPath *)malloc(sizeof(NextPath));
        strcpy(newNode1->path, currentPath);
        newNode1->next = NULL;
        if (head1 == NULL)
        {
            head1 = newNode1;
        }
        else
        {
            newNode1->next = head1;
            head1 = newNode1;
        }
        if (i == 0)
        {
            gtk_list_store_clear(store);
            showDrives();
        }
        else
        {
            chdir(head2->path);
            BackPath *dd = head2;
            head2 = head2->next;
            free(dd);
            openDirectory();
        }
    }
}
void goToPreviousFolder(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    if (p != 0)
    {
        p--;
        i++;
        newNode2 = (BackPath *)malloc(sizeof(BackPath));
        char currentPath[MAX_PATH] = "";
        getcwd(currentPath, sizeof(currentPath));
        strcpy(newNode2->path, currentPath);
        newNode2->next = NULL;
        if (head2 == NULL)
        {
            head2 = newNode2;
        }
        else
        {
            newNode2->next = head2;
            head2 = newNode2;
        }
        char folderName[MAX_PATH + 1];
        strcpy(folderName, head1->path);
        NextPath *dd = head1;
        head1 = head1->next;
        free(dd);
        chdir(folderName);
        openDirectory();
    }
}
void createNewFolder(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    if (i != 0)
    {
        GtkWidget *dialog;
        GtkWidget *entry;
        GtkWidget *area;
        gint response;
        dialog = gtk_dialog_new_with_buttons("New Folder creation", GTK_WINDOW(window), GTK_DIALOG_MODAL, "Ok", GTK_RESPONSE_OK, "Cancel", GTK_RESPONSE_CANCEL, NULL);
        entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter a folder name");
        area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_add(GTK_CONTAINER(area), entry);
        while (1)
        {
            gtk_widget_show_all(dialog);
            response = gtk_dialog_run(GTK_DIALOG(dialog));
            const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
            if (response == GTK_RESPONSE_OK)
            {
                gchar *input = g_strdup(text);
                if (input[0] == '\0')
                {

                    if (mkdir("New Folder") != 0)
                    {
                        gchar *folder;
                        int index = 1;
                        while (1)
                        {
                            folder = g_strdup_printf("New folder (%d)", index);
                            if (mkdir(folder) == 0)
                            {
                                g_free(folder);
                                break;
                            }
                            index++;
                            g_free(folder);
                        }
                    }
                    g_free(input);
                    break;
                }
                else
                {
                    if (mkdir(input) != 0)
                    {
                        char *error_message = strerror(errno);
                        GtkWidget *alert = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, error_message);
                        gtk_dialog_run(GTK_DIALOG(alert));
                        gtk_widget_destroy(alert);
                        g_free(input);
                    }
                    else
                        break;
                }
            }
            else
                break;
        }
        gtk_widget_destroy(dialog);
        openDirectory();
    }
}
void Rename(GtkMenuItem *item, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *entry;
    GtkWidget *area;
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkTreePath *path = (GtkTreePath *)data;
    gtk_tree_model_get_iter(model, &iter, path);
    gchar *folderName = NULL;
    gtk_tree_model_get(model, &iter, 1, &folderName, -1);
    dialog = gtk_dialog_new_with_buttons("Rename", GTK_WINDOW(window), GTK_DIALOG_MODAL, "Ok", GTK_RESPONSE_OK, "Cancel", GTK_RESPONSE_CANCEL, NULL);
    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter a new name");
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(area), entry);
    while (1)
    {
        gint response;
        gtk_widget_show_all(dialog);
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response == GTK_RESPONSE_OK)
        {
            const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry));
            if (name[0] != '\0')
            {
                if (rename(folderName, name) != 0)
                {
                    gchar *error_message = strerror(errno);
                    GtkWidget *alert = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, error_message);
                    gtk_dialog_run(GTK_DIALOG(alert));
                    g_free(error_message);
                    gtk_widget_destroy(alert);
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else
            break;
    }
    g_free(folderName);
    gtk_widget_destroy(dialog);
    openDirectory();
}
void deleteFunction(const gchar *path, CopyProgress *progress)
{
    DWORD att = GetFileAttributes(path);
    if (att == INVALID_FILE_ATTRIBUTES)
        return;
    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        DIR *dp = opendir(path);
        if (!dp)
            return;

        struct dirent *entry;
        gchar fullPath[MAX_PATH + 1];

        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, entry->d_name);
            deleteFunction(fullPath, progress);
        }

        closedir(dp);

        if (!RemoveDirectory(path))
        {
            gchar msg[256];
            snprintf(msg, sizeof(msg), "Failed to remove directory: %s", path);
            MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
        }
    }
    else

    {
        if (!DeleteFile(path))
        {
            gchar msg[256];
            snprintf(msg, sizeof(msg), "Failed to delete file: %s", path);
            MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
        }
        else
        {
            cnt++;
            if (cnt >= files)
            {
                fraction = fraction + 0.01;
                cnt = 0;
                if (fraction > 1.0)
                {
                    fraction = 1.0;
                }
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->progress_bar), fraction);
                gchar *text = g_strdup_printf("%.1f%%", fraction * 100);
                gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress->progress_bar), text);
                g_free(text);
                while (gtk_events_pending())
                    gtk_main_iteration_do(FALSE);
            }
        }
    }
}
void Delete(GtkMenuItem *item, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkTreePath *path = (GtkTreePath *)data;

    if (!gtk_tree_model_get_iter(model, &iter, path))
        return;

    gchar *fName = NULL;
    gtk_tree_model_get(model, &iter, 1, &fName, -1);

    if (fName)
    {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_QUESTION,
                                                   GTK_BUTTONS_YES_NO,
                                                   "Are you sure you want to delete '%s'?", fName);

        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        dfiles = count_files(fName);
        if (response == GTK_RESPONSE_YES)
        {
            GtkWidget *progress_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_window_set_title(GTK_WINDOW(progress_window), "Deleting...");
            gtk_window_set_default_size(GTK_WINDOW(progress_window), 400, 60);
            gtk_window_set_position(GTK_WINDOW(progress_window), GTK_WIN_POS_CENTER);
            gtk_container_set_border_width(GTK_CONTAINER(progress_window), 10);
            gtk_window_set_resizable(GTK_WINDOW(progress_window), FALSE);
            gtk_window_set_type_hint(GTK_WINDOW(progress_window), GDK_WINDOW_TYPE_HINT_DIALOG);

            GtkWidget *progress_bar = gtk_progress_bar_new();
            gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
            gtk_container_add(GTK_CONTAINER(progress_window), progress_bar);
            gtk_widget_show_all(progress_window);
            CopyProgress prog = {0};
            prog.progress_bar = progress_bar;
            prog.total_bytes = calculate_total_size(fName);
            prog.copied_bytes = 0;
            prog.fraction = 0.0;
             char fullPath[MAX_PATH] = "";
             char compPath[MAX_PATH] = "";
             char currentPath[MAX_PATH] = "";
             snprintf(compPath, sizeof(compPath), "%s\\%s", pathway1, FName1);
             getcwd(currentPath, sizeof(currentPath));
            snprintf(fullPath, sizeof(fullPath), "%s\\%s", currentPath, fName);
            if(strcmp(fullPath,compPath) == 0)
            {
                 gtk_widget_set_opacity(event4, 0.1);
                 l = 0;
                 k = 0;   
            }
            if (prog.total_bytes >= 0)
            {
                deleteFunction(fullPath, &prog);
            }
            cnt = 0;
            while (gtk_events_pending())
                gtk_main_iteration_do(FALSE);
            g_usleep(400000);
            gtk_widget_destroy(progress_window);
            fraction = 0;
            g_signal_connect(progress_window, "delete-event", G_CALLBACK(on_progress_close), NULL);
            openDirectory();
        }

        g_free(fName);
    }
}
void Copy(GtkMenuItem *item, gpointer data)
{
    k = 1;
    l = 0;
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkTreePath *path = (GtkTreePath *)data;
    if (!gtk_tree_model_get_iter(model, &iter, path))
        return;
    getcwd(pathway1, sizeof(pathway1));
    gchar *gname = NULL;
    gtk_tree_model_get(model, &iter, 1, &gname, -1);
    snprintf(FName1, sizeof(FName1), "%s", gname);
    gtk_widget_set_opacity(event4, 1);
    files = count_files(FName1);
    if (files % 100 != 0)
    {
        files = files / 100;
        files++;
    }
    else
        files = files / 100;
    if (l == 1)
    {
        files = files * 2;
    }
}

void Cut(GtkMenuItem *item, gpointer data)
{
    l = 1;
    k = 0;
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkTreePath *path = (GtkTreePath *)data;
    if (!gtk_tree_model_get_iter(model, &iter, path))
        return;
    getcwd(pathway1, sizeof(pathway1));
    gchar *gname = NULL;
    gtk_tree_model_get(model, &iter, 1, &gname, -1);
    snprintf(FName1, sizeof(FName1), "%s", gname);
    gtk_widget_set_opacity(event4, 1);
}
void deleteFunctionForCut(const gchar *path, CopyProgress *progress)
{
    DWORD att = GetFileAttributes(path);
    if (att == INVALID_FILE_ATTRIBUTES)
        return;
    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        DIR *dp = opendir(path);
        if (!dp)
            return;

        struct dirent *entry;
        gchar fullPath[MAX_PATH + 1];

        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, entry->d_name);
            deleteFunctionForCut(fullPath, progress);
        }

        closedir(dp);

        if (!RemoveDirectory(path))
        {
            cnt++;
            if (cnt >= files)
            {
                fraction = fraction + 0.01;
                cnt = 0;
                if (fraction > 1.0)
                {
                    fraction = 1.0;
                }
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->progress_bar), fraction);
                gchar *text = g_strdup_printf("%.1f%%", fraction * 100);
                gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress->progress_bar), text);
                g_free(text);

                while (gtk_events_pending())
                    gtk_main_iteration_do(FALSE);
            }
            gchar msg[256];
            snprintf(msg, sizeof(msg), "Failed to remove directory: %s", path);
            MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
        }
    }
    else

    {
        if (!DeleteFile(path))
        {
            gchar msg[256];
            snprintf(msg, sizeof(msg), "Failed to delete file: %s", path);
            MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
        }
    }
}

void Paste(const gchar *src, const gchar *dest, CopyProgress *progress)
{
    DWORD att = GetFileAttributes(src);
    if (att == INVALID_FILE_ATTRIBUTES)
        return;

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        mkdir(dest);
        SetFileAttributes(dest, FILE_ATTRIBUTE_NORMAL);
        DIR *dp = opendir(src);
        if (!dp)
            return;

        struct dirent *entry;
        gchar fullSrc[MAX_PATH + 1];
        gchar fullDest[MAX_PATH + 1];

        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(fullSrc, sizeof(fullSrc), "%s\\%s", src, entry->d_name);
            snprintf(fullDest, sizeof(fullDest), "%s\\%s", dest, entry->d_name);
            Paste(fullSrc, fullDest, progress);
        }

        closedir(dp);
    }
    else
    {
        FILE *source = fopen(src, "rb");
        FILE *target = fopen(dest, "wb");
        if (!source || !target)
            return;

        char buffer[8192];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0)
        {
            fwrite(buffer, 1, bytesRead, target);
        }
        cnt++;
        fclose(source);
        fclose(target);
        if (cnt >= files)
        {
            fraction = fraction + 0.01;
            cnt = 0;
            if (fraction > 1.0)
            {
                fraction = 1.0;
            }

            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->progress_bar), fraction);
            gchar *text = g_strdup_printf("%.1f%%", fraction * 100);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress->progress_bar), text);
            g_free(text);

            while (gtk_events_pending())
                gtk_main_iteration_do(FALSE);
        }
    }
}

void copyFunction()
{
    if (l == 1 || k == 1)
    {
        GtkWidget *progress_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        if (k == 1)
            gtk_window_set_title(GTK_WINDOW(progress_window), "Copying...");
        else
            gtk_window_set_title(GTK_WINDOW(progress_window), "Moving...");
        gtk_window_set_default_size(GTK_WINDOW(progress_window), 400, 60);
        gtk_window_set_position(GTK_WINDOW(progress_window), GTK_WIN_POS_CENTER);
        gtk_container_set_border_width(GTK_CONTAINER(progress_window), 10);
        gtk_window_set_resizable(GTK_WINDOW(progress_window), FALSE);
        gtk_window_set_type_hint(GTK_WINDOW(progress_window), GDK_WINDOW_TYPE_HINT_DIALOG);

        GtkWidget *progress_bar = gtk_progress_bar_new();
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
        gtk_container_add(GTK_CONTAINER(progress_window), progress_bar);
        gtk_widget_show_all(progress_window);

        gchar src[MAX_PATH + 1];
        gchar dest[MAX_PATH + 1];
        gchar dest_folder[MAX_PATH + 1];

        getcwd(dest_folder, sizeof(dest_folder));
        snprintf(src, sizeof(src), "%s\\%s", pathway1, FName1);
        snprintf(dest, sizeof(dest), "%s\\%s", dest_folder, FName1);

        CopyProgress prog = {0};
        prog.progress_bar = progress_bar;
        prog.total_bytes = calculate_total_size(src);
        g_print("%ld", prog.total_bytes);
        prog.copied_bytes = 0;
        prog.fraction = 0.0;

        if (prog.total_bytes >= 0)
        {
            Paste(src, dest, &prog);
        }
        cnt = 0;
        if (l == 1)
            deleteFunctionForCut(src, &prog);
        while (gtk_events_pending())
            gtk_main_iteration_do(FALSE);
        g_usleep(400000);
        gtk_widget_destroy(progress_window);
        l = 0;
        gtk_widget_set_opacity(event4, 0.1);
        fraction = 0;
        g_signal_connect(progress_window, "delete-event", G_CALLBACK(on_progress_close), NULL);
        openDirectory();
    }
}

gboolean rightButtonClick(GtkTreeView *treeview, GdkEventButton *event, gpointer data)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == 3 && i > 0)
    {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(treeview, (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL))
        {
            if (gtk_tree_selection_path_is_selected(selection, path))
            {
                GtkWidget *menu = gtk_menu_new();
                GtkWidget *item1 = gtk_menu_item_new_with_label("Rename");
                GtkWidget *item2 = gtk_menu_item_new_with_label("Copy");
                GtkWidget *item3 = gtk_menu_item_new_with_label("Delete");
                GtkWidget *item4 = gtk_menu_item_new_with_label("Cut");

                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item1);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item2);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item4);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item3);
                gtk_widget_show_all(menu);
                gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);

                g_signal_connect(item1, "activate", G_CALLBACK(Rename), path);
                g_signal_connect(item2, "activate", G_CALLBACK(Copy), path);
                g_signal_connect(item4, "activate", G_CALLBACK(Cut), path);
                g_signal_connect(item3, "activate", G_CALLBACK(Delete), path);
            }
            return TRUE;
        }
    }
    else
        return FALSE;
}
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    GtkWidget *label1;
    GtkWidget *label2;
    GtkWidget *label3;
    GtkTreeIter iter;
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;

    labelPath = gtk_label_new(" ");
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "File Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 600);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    boxSearch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    searchEntry = gtk_entry_new();
    searchButton = gtk_button_new_with_label("Search");
    gtk_box_pack_start(GTK_BOX(boxSearch), searchEntry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(boxSearch), searchButton, FALSE, FALSE, 0);

    label1 = gtk_label_new("Back");
    label2 = gtk_label_new("Next");
    label3 = gtk_label_new("New Folder");

    event1 = gtk_event_box_new();
    event2 = gtk_event_box_new();
    event3 = gtk_event_box_new();
    event4 = gtk_event_box_new();
    GError *error = NULL;
    iconPixbuf = gdk_pixbuf_new_from_file("File.png", &error);
    pixbuf1 = gdk_pixbuf_new_from_file("back.png", &error);
    pixbuf2 = gdk_pixbuf_new_from_file("next.png", &error);
    GdkPixbuf *pixbuf3 = gdk_pixbuf_new_from_file("newFolder.png", &error);
    GdkPixbuf *pixbuf4 = gdk_pixbuf_new_from_file("drive.png", &error);
    GdkPixbuf *pixbuf5 = gdk_pixbuf_new_from_file("Paste.png", &error);
    GdkPixbuf *scaled_pixbuf1 = gdk_pixbuf_scale_simple(pixbuf1, 20, 20, GDK_INTERP_BILINEAR);
    GdkPixbuf *scaled_pixbuf2 = gdk_pixbuf_scale_simple(pixbuf2, 20, 20, GDK_INTERP_BILINEAR);
    GdkPixbuf *scaled_pixbuf3 = gdk_pixbuf_scale_simple(pixbuf3, 40, 40, GDK_INTERP_BILINEAR);
    scaled_pixbuf4 = gdk_pixbuf_scale_simple(pixbuf4, 40, 40, GDK_INTERP_BILINEAR);
    GdkPixbuf *scaled_pixbuf5 = gdk_pixbuf_scale_simple(pixbuf5, 30, 30, GDK_INTERP_BILINEAR);
    image1 = gtk_image_new_from_pixbuf(scaled_pixbuf1);
    image2 = gtk_image_new_from_pixbuf(scaled_pixbuf2);
    GtkWidget *image3 = gtk_image_new_from_pixbuf(scaled_pixbuf3);
    GtkWidget *image4 = gtk_image_new_from_pixbuf(scaled_pixbuf5);
    gtk_container_add(GTK_CONTAINER(event1), image1);
    gtk_container_add(GTK_CONTAINER(event2), image2);
    gtk_container_add(GTK_CONTAINER(event3), image3);
    gtk_container_add(GTK_CONTAINER(event4), image4);

    Parentbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    // box1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    // box2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    // box3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    fullBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 250);
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    boxPath = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    gtk_box_pack_start(GTK_BOX(box), event1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), event2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), event3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), event4, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(fullBox), box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(Parentbox), fullBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(boxPath), labelPath, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(Parentbox), boxPath, FALSE, FALSE, 0);

    store = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);

    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    GtkCellRenderer *pixbuf_renderer = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *icon_column = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(icon_column, pixbuf_renderer, TRUE);
    gtk_tree_view_column_add_attribute(icon_column, pixbuf_renderer, "pixbuf", 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), icon_column);
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 1, NULL);
    PangoFontDescription *font_desc = pango_font_description_new();
    pango_font_description_set_size(font_desc, 14 * PANGO_SCALE);
    g_object_set(renderer, "font-desc", font_desc, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 5);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);

    gtk_box_pack_start(GTK_BOX(Parentbox), scrolled_window, TRUE, TRUE, 0);

    g_signal_connect(treeview, "row-activated", G_CALLBACK(selectDirectory), NULL);
    g_signal_connect(event1, "button-press-event", G_CALLBACK(goToParentFolder), NULL);
    g_signal_connect(event2, "button-press-event", G_CALLBACK(goToPreviousFolder), NULL);
    g_signal_connect(treeview, "button-press-event", G_CALLBACK(rightButtonClick), NULL);
    g_signal_connect(event3, "button-press-event", G_CALLBACK(createNewFolder), NULL);
    g_signal_connect(event4, "button-press-event", G_CALLBACK(copyFunction), NULL);
    g_signal_connect(searchButton, "clicked", G_CALLBACK(on_search_clicked), searchEntry);

    labelStatus = gtk_label_new("This folder is empty");
    showDrives();
    gtk_widget_set_opacity(event2, 0.1);
    gtk_container_add(GTK_CONTAINER(window), Parentbox);
    gtk_widget_show_all(window);
    gtk_main();
}
