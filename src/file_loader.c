#include <stdio.h>

#include "file_loader.h"

static DocumentSinkFn g_sink = NULL;
static void* g_sink_user = NULL;

void file_loader_set_sink(DocumentSinkFn sink, void* user) {
    g_sink = sink;
    g_sink_user = user;
}

int load_documents_from_directory(const char* path) {
    (void)path;
    (void)g_sink;
    (void)g_sink_user;
    /* Skeleton implementation; recursive file traversal is added next. */
    return 0;
}
