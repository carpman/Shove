#include <stdio.h>

#include <strophe.h>

int message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
                    void * const userdata){
    xmpp_stanza_t *reply, *body, *text;
    char *intext;
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    if(!xmpp_stanza_get_child_by_name(stanza, "body")) return 1;
    if(xmpp_stanza_get_attribute(stanza, "type") != NULL &&
       !strcmp(xmpp_stanza_get_attribute(stanza, "type"), "error")) return 1;

    intext = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stanza, "body"));

    printf("Incoming message: %s\n",intext);

    return 1;
}

void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status,
                  const int error, xmpp_stream_error_t * const stream_error,
                  void * const userdata){
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    if(status == XMPP_CONN_CONNECT){
        xmpp_handler_add(conn, message_handler, NULL, "message", NULL, ctx);
    }else{
        //Disconnect
        xmpp_stop(ctx);
    }
}

int main(int argc, char **argv){
    xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;
    xmpp_log_t *log;
    char *jid, *pass;

    xmpp_initialize();

    log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG);
    ctx = xmpp_ctx_new(NULL, log);

    conn = xmpp_conn_new(ctx);

    xmpp_conn_set_jid(conn, "");
    xmpp_conn_set_pass(conn, "");
    
    xmpp_connect_client(conn, NULL, 0, conn_handler, ctx);

    xmpp_run(ctx);

    xmpp_conn_release(conn);
    xmpp_ctx_free(ctx);

    xmpp_shutdown();

    return 0;
}
