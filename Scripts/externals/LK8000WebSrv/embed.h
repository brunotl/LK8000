#ifndef __EMBED_H__
#define __EMBED_H__


#define MAX_PATH 1024
#define LOGGER_DIR "/mnt/onboard/LK8000/_Logger/"
#define TASKS_DIR "/mnt/onboard/LK8000/_Tasks/"


typedef struct dir_elem_t dir_elem_t;
struct  dir_elem_t {
    char* name;
    dir_elem_t *next;
};

typedef struct file_T {
    FILE* fd;
}file_t;

// ===== HTML PARTS =====

#define HTTP_DONLOAD_HEADER	\
    "HTTP/1.1 200 OK\r\nConnection: close\r\n"\
	"Content-Type: application/octet-stream\r\n"\
    "Content-Length: %d\r\n\r\n"


#define HTTP_HEADER "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: \
	                close\r\n\r\n"


#define DOWNLOAD_LINK "<a href=/handle_download.callback/%s>%s</a><br />"

#define DOWNLOAD_HEAD "<!DOCTYPE HTML>\
    <html><head>\
    <meta charset=\"UTF-8\">\
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\
    <title>LK8000 Download page</title>\
    </head><body>\
    <form>\
    <h1>LK8000 Download page</h1>\
    <fieldset>\
    <legend>Trace files:</legend>"


#define DOWNLOAD_MIDDLE "</fieldset>\
    <fieldset>\
    <legend>Task files:</legend>"

#define DOWNLOAD_TAIL "</fieldset><br />\
    <a href=index.html>Home</a>\
    </form></body></html>"



#define DELETE_CHECKBOX "<input type=\"checkbox\" name=\"%s\" value=\"True\">%s<br />"

#define DELETE_HEAD "<!DOCTYPE HTML>\
    <html><head>\
    <meta charset=\"UTF-8\">\
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\
    <title>LK8000 Delete page</title>\
    </head><body>\
    <form action=\"/handle_delete.callback\">\
    <h1>LK8000 Delete page</h1>\
    <fieldset>\
    <legend>Trace files:</legend>"


#define DELETE_MIDDLE "</fieldset>\
    <fieldset>\
    <input type=\"submit\" value=\"DELETE\" formmethod=\"GET\">\
    </fieldset><br />\
    <fieldset>\
    <legend>Task files:</legend>"

#define DELETE_TAIL "</fieldset>\
    <fieldset>\
    <input type=\"submit\" value=\"DELETE\" formmethod=\"GET\">\
    </fieldset><br />\
    <a href=index.html>Home</a>\
    </form></body></html>"

#define UPLOAD_HEAD "<!DOCTYPE HTML>\
    <html><head>\
    <meta charset=\"UTF-8\">\
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\
    <title>LK8000 Upload page</title>\
    </head><body>\
    <form action=\"/handle_delete.callback\">\
    <h1>LK8000 Upload page</h1>"

#define UPLOAD_TAIL "<br /><br /><a href=/upload.html>Back</a> | <a href=index.html>Home</a>\
    </form></body></html>"



#endif //__EMBED_H__