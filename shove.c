#include <stdio.h>
#include <stdlib.h>
#include <wordexp.h>

#include <strophe.h>
#include <confuse.h>

static cfg_opt_t connection_opts[] = {
    CFG_STR("jid", "", CFGF_NONE),
    CFG_STR("password", "", CFGF_NONE),
    CFG_INT("port", 5222, CFGF_NONE),
    CFG_END()
};

static cfg_opt_t config_opts[] = {
    CFG_SEC("connection", connection_opts, CFGF_NONE),
    CFG_END()
};

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

        xmpp_stanza_t *pres = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(pres, "presence");
        xmpp_send(conn, pres);
        xmpp_stanza_release(pres);
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
    wordexp_t exp_result;
    cfg_t *cfg;

    cfg = cfg_init(config_opts, CFGF_NOCASE);

    if(wordexp("~/.shoverc", &exp_result, 0) == 0){
        int ret = cfg_parse(cfg, exp_result.we_wordv[0]);

        if(ret == CFG_FILE_ERROR){
            printf("Config file error.\n");
            exit(1);
        }

        xmpp_initialize();

        log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG);
        ctx = xmpp_ctx_new(NULL, log);

        conn = xmpp_conn_new(ctx);

        cfg_t *conncfg = cfg_getsec(cfg, "connection");
        if(conncfg){
            printf(">>>>>>>%s\n>>>>>>>>%s\n", cfg_getstr(conncfg, "jid"), cfg_getstr(conncfg, "password"));
            xmpp_conn_set_jid(conn, cfg_getstr(conncfg, "jid"));
            xmpp_conn_set_pass(conn, cfg_getstr(conncfg, "password"));
        
            xmpp_connect_client(conn, NULL, 0, conn_handler, ctx);

            xmpp_run(ctx);

            xmpp_conn_release(conn);
            xmpp_ctx_free(ctx);

            xmpp_shutdown();
        }else{
            printf("Connection section missing.\n");
            exit(1);
        }
    }else{
        printf("Config file missing.\n");
        exit(1);
    }

    return 0;
}
