/*
 * Copyright (c) 2013-2021 the CivetWeb developers
 * Copyright (c) 2013 No Face Press, LLC
 * License http://opensource.org/licenses/mit-license.php MIT License
 */

/* Note: This example omits some error checking and input validation for a
 * better clarity/readability of the code. Example codes undergo less quality
 * management than the main source files of this project. */

/* Simple example program on how to use CivetWeb embedded into a C program. */
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h> 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "civetweb.h"
#include "embed.h"

#define DOCUMENT_ROOT "."

#define PORT "80"

volatile int exitNow = 0;


void DestroyList(dir_elem_t* list) {
	dir_elem_t* head;
	dir_elem_t* next;
	if (list) {
		head = list;
		while(head->next) {
			next = head;
			head = head->next;
			free(next->name);
			free(next);
		}
	}
}


void AddName(dir_elem_t* elem, const char* name) {
	elem->name = (char*)malloc(strlen(name)+1);
	strncpy(elem->name, name, strlen(name));
	elem->name[strlen(name)]=0;
}


dir_elem_t* GetElem() {
	dir_elem_t *elem;

	elem = (dir_elem_t *)malloc(sizeof(elem));
	elem->name = NULL;
	elem->next = NULL;
	return elem;
}


dir_elem_t* ListFiles(const char *DirName) {
	DIR *d;
	struct dirent *dir;
	dir_elem_t *head, *elem;

	head = GetElem();
	elem = head;

	d = opendir(DirName);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (dir->d_type != DT_DIR) {
				AddName(elem, dir->d_name);
				elem->next = GetElem();
				elem = elem->next;
			}
		}
		// Free extra link
		free(elem->next);
		closedir(d);
	}
	else {
		return NULL;
	}

	return head;
}



int
FileHandler(struct mg_connection *conn, void *cbdata)
{
	/* In this handler, we ignore the req_info and send the file "fileName". */
	const char *fileName = (const char *)cbdata;
	printf("File handler: %s\n", fileName);
	mg_send_file(conn, fileName);
	return 1;
}



int
field_found(const char *key,
            const char *filename,
            char *path,
            size_t pathlen,
            void *user_data)
{

	struct mg_connection *conn = (struct mg_connection *)user_data;

	mg_printf(conn, "\r\n\r\n%s:\r\n", key);

	if (filename && *filename) {

		/* According to
		 * https://datatracker.ietf.org/doc/html/rfc7578#section-4.2: Do not use
		 * path information present in the filename. Drop all "/" (and "\" for
		 * Windows).
		 */
		const char *fname = filename;
		const char *sep = strrchr(fname, '/');
		if (sep) {
			fname = sep + 1;
		}

		snprintf(path, pathlen, "%s%s", TASKS_DIR,fname);

		/* According to https://datatracker.ietf.org/doc/html/rfc7578#section-7:
		 * Do not overwrite existing files.
		 */
		{
			FILE *ftest = fopen(path, "rb");
			printf("STORAGE: %s\n", path);
			if (!ftest) {
				return MG_FORM_FIELD_STORAGE_STORE;
			}
			fclose(ftest);
			/* This is just simple demo code. More sophisticated code could add
			 * numbers to the file name to make filenames unique. However, most
			 * likely file upload will not end up in the temporary path, but in
			 * a user directory - multiple directories for multiple users that
			 * are logged into the web service. In this case, users might want
			 * to overwrite their own code. You need to adapt this example to
			 * your needs.
			 */
		}

		return MG_FORM_FIELD_STORAGE_SKIP;
	}
	return MG_FORM_FIELD_STORAGE_GET;
}


int
delete_file(const char *key,
            const char *filename,
            char *path,
            size_t pathlen,
            void *user_data)
{
	struct mg_connection *conn = (struct mg_connection *)user_data;
	char* cmd = NULL;

	if ((key != NULL) && (key[0] == '\0')) {
		/* Incorrect form data detected */
		return MG_FORM_FIELD_HANDLE_ABORT;
	}

	cmd = (char*)malloc(MAX_PATH);

	if (key && cmd) {
		memset(cmd, 0, MAX_PATH);
		strncat(cmd, "rm ", MAX_PATH);
		strncat(cmd, LOGGER_DIR, MAX_PATH);
		strncat(cmd, key, MAX_PATH);
		path[MAX_PATH-1] = 0;
		printf("%s\n", cmd);
		system(cmd);
		mg_printf(conn, "File %s deleted<br />", key);
		free(cmd);
	}

	/* TODO - DELETE FILE From File system and send back new file list*/
	return 0;
}


int ReadFile(const char *key,
                   const char *value,
                   size_t valuelen,
                   void *user_data)
{
	return 0;
}


void formatList(struct mg_connection *conn, const char* path, const char* method) 
{
	dir_elem_t *elems;
	dir_elem_t *nextelem;
	char *fileName;

	elems = ListFiles(path);
	if (elems) {
		nextelem = elems;
		do 	{
			if(nextelem->name) {
				mg_printf(conn, method, nextelem->name, nextelem->name);
			}
			nextelem = nextelem->next;
		}
		while (nextelem != NULL);
		DestroyList(elems);
	}
}



int ignore_fieldget(const char *key, const char *value, size_t valuelen, void *user_data)
{
	return 0;
}


int ignore_field_stored(const char *path, long long file_size, void *user_data)
{
	return 0;
}

/* Handlers Definition */
int HandleDownload(struct mg_connection *conn, void *cbdata)
{
	char filename[MAX_PATH];
	char fullpath[MAX_PATH];
	printf("HandleDowload handler\n");


	/* Handler may access the request info using mg_get_request_info */
	const struct mg_request_info *req_info = mg_get_request_info(conn);

	/* It would be possible to check the request info here before calling
	 * mg_handle_form_request. */
	sscanf(req_info->local_uri_raw, "/handle_download.callback/%s", filename);
	snprintf(fullpath, MAX_PATH-1, "%s%s", LOGGER_DIR, filename);
	
	printf("local uri: %s\n", fullpath);


	mg_send_file(conn, fullpath);

	/* Call the form handler */
	// mg_printf(conn,HTTP_HEADER);
	// mg_printf(conn,UPLOAD_HEAD);
	// // ret = mg_handle_form_request (conn, &fdh);
	// mg_printf(conn, "%i file(s) uploaded", ret);
	// mg_printf(conn,UPLOAD_TAIL);

	return 1;
}

int HtmlDownload(struct mg_connection *conn, void *cbdata)
{

	dir_elem_t *elems;
	dir_elem_t *nextelem;
	char *fileName;
	

	printf("HtmlDownload Handler\n");
	mg_printf(conn,HTTP_HEADER);
    mg_printf(conn, DOWNLOAD_HEAD);
	formatList(conn, LOGGER_DIR, DOWNLOAD_LINK);
	mg_printf(conn, DOWNLOAD_MIDDLE);
	formatList(conn, TASKS_DIR, DOWNLOAD_LINK);
	mg_printf(conn, DOWNLOAD_TAIL);
	return 1;
}


int HtmlDelete(struct mg_connection *conn, void *cbdata)
{

	dir_elem_t *elems;
	dir_elem_t *nextelem;
	char *fileName;
	

	printf("HtmlDelete Handler\n");
	mg_printf(conn,HTTP_HEADER);
    mg_printf(conn, DELETE_HEAD);
	formatList(conn, LOGGER_DIR, DELETE_CHECKBOX);
	mg_printf(conn, DELETE_MIDDLE);
	formatList(conn, TASKS_DIR, DELETE_CHECKBOX);
	mg_printf(conn, DELETE_TAIL);
	return 1;
}


int HandleDelete(struct mg_connection *conn, void *cbdata)
{	
	/* Handler may access the request info using mg_get_request_info */
	const struct mg_request_info *req_info = mg_get_request_info(conn);
	int ret;
	struct mg_form_data_handler fdh = {delete_file, ignore_fieldget, ignore_field_stored, NULL};

	mg_printf(conn,HTTP_HEADER);
	fdh.user_data = (void *)conn;

	/* Call the form handler */
	mg_printf(conn,"<!DOCTYPE HTML><html>");
	mg_printf(conn,"<head> <meta charset=\"UTF-8\"> <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" /><title>LK8000 Home page</title></head>");
	mg_printf(conn,"<body><h1>LK8000 Delete page</h1>");
	ret = mg_handle_form_request(conn, &fdh);
	mg_printf(conn, "<br />%i Files deleted<br /><br />", ret);
	mg_printf(conn,"<a href=delete.html>Back</a>   / <a href=index.html>Home</a>");
	mg_printf(conn,"</body></html>");

	return 1;
}


int HandleUpload(struct mg_connection *conn, void *cbdata)
{
	mg_printf(conn,
	          "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
	          "close\r\n\r\n");

	mg_printf(conn, "<!DOCTYPE html>\n");
	mg_printf(conn, "<html>\n<head>\n");
	mg_printf(conn, "<meta charset=\"UTF-8\">\n");
	mg_printf(conn, "<title>File upload</title>\n");
	mg_printf(conn, "</head>\n<body>\n");
	mg_printf(conn,
			  " <h1>LK8000 Upload page</h1>"
	          "<form action=\"%s\" method=\"POST\" "
	          "enctype=\"multipart/form-data\">\n",
	          (const char *)cbdata);
	mg_printf(conn, "<input type=\"file\" name=\"filesin\" multiple>\n");
	mg_printf(conn, "<input type=\"submit\" value=\"Submit\">\n");
	mg_printf(conn, "</form>\n<br /><a href=index.html>Home</a></body>\n</html>\n");
	return 1;
}


int UploadCallback(struct mg_connection *conn, void *cbdata)
{
	/* Handler may access the request info using mg_get_request_info */
	const struct mg_request_info *req_info = mg_get_request_info(conn);
	int i, j, ret;
	
	file_t file;

	file.fd = NULL;

	struct mg_form_data_handler fdh = {field_found,
	                                   ReadFile,
	                                   ignore_field_stored,
	                                   (void *)&file};

	printf("uploadCallback\n");
	mg_printf(conn,HTTP_HEADER);
	mg_printf(conn,UPLOAD_HEAD);
	ret = mg_handle_form_request (conn, &fdh);
	mg_printf(conn, "%i file(s) uploaded", ret);
	mg_printf(conn,UPLOAD_TAIL);
	return 1;
}



int
log_message(const struct mg_connection *conn, const char *message)
{
	puts(message);
	return 1;
}


int
main(int argc, char *argv[])
{
	const char *options[] = {
#if !defined(NO_FILES)
		"document_root",
		DOCUMENT_ROOT,
#endif
		"listening_ports",
		PORT,
		"request_timeout_ms",
		"10000",
		"error_log_file",
		"error.log",
		"enable_auth_domain_check",
		"no",
		0
	};
	struct mg_callbacks callbacks;
	struct mg_context *ctx;
	struct mg_server_port ports[32];
	int port_cnt, n;
	int err = 0;

	/* Check if libcivetweb has been built with all required features. */
	if (err) {
		fprintf(stderr, "Cannot start CivetWeb - inconsistent build.\n");
		return EXIT_FAILURE;
	}

	/* Start CivetWeb web server */
	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.log_message = log_message;
	ctx = mg_start(&callbacks, 0, options);

	/* Check return value: */
	if (ctx == NULL) {
		fprintf(stderr, "Cannot start CivetWeb - mg_start failed.\n");
		return EXIT_FAILURE;
	}


	/* LK8000 callback definition */
	mg_set_request_handler(ctx,
	                       "/index.html",
	                       FileHandler,
	                       (void *)"/mnt/onboard/LK8000/kobo/html/home.html");
	mg_set_request_handler(ctx,
	                       "/",
	                       FileHandler,
	                       (void *)"/mnt/onboard/LK8000/kobo/html/home.html");

	mg_set_request_handler(ctx,
	                       "/download.html",
	                       HtmlDownload,
	                       (void *)NULL);


	mg_set_request_handler(ctx,
	                       "/delete.html",
	                       HtmlDelete,
	                       (void *)NULL);


	/* Add handler for form data */
	mg_set_request_handler(ctx,
	                       "/handle_download.callback",
	                       HandleDownload,
	                       (void *)NULL);

	mg_set_request_handler(ctx,
	                       "/upload.html",
	                       HandleUpload,
	                       (void*)"/upload.callback");

	mg_set_request_handler(ctx,
	                       "/upload.callback",
	                       UploadCallback,
	                       (void*)NULL);

	mg_set_request_handler(ctx,
	                       "/handle_delete.callback",
	                       HandleDelete,
	                       (void *)NULL);


 	/* List all listening ports */
	memset(ports, 0, sizeof(ports));
	port_cnt = mg_get_server_ports(ctx, 32, ports);
	printf("\n%i listening ports:\n\n", port_cnt);

	for (n = 0; n < port_cnt && n < 32; n++) {
		const char *proto = ports[n].is_ssl ? "https" : "http";
		const char *host;

		if ((ports[n].protocol & 1) == 1) {
			/* IPv4 */
			host = "127.0.0.1";
			printf("Browse files at %s://%s:%i/\n", proto, host, ports[n].port);
			printf("\n");
		}

	}

	/* Wait until the server should be closed */
	while (!exitNow) {
		sleep(1);
	}

	/* Stop the server */
	mg_stop(ctx);
	printf("Server stopped.\n");
	printf("Bye!\n");

	return EXIT_SUCCESS;
}
