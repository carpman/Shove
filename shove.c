#include <stdio.h>
#include <stdlib.h>
#include <wordexp.h>
#include <string.h>

#include <strophe.h>
#include <confuse.h>

static cfg_opt_t connection_opts[] = {
    CFG_STR("jid", "", CFGF_NONE),
    CFG_STR("password", "", CFGF_NONE),
    CFG_INT("port", 5222, CFGF_NONE),
    CFG_END()
};

static cfg_opt_t command_opts[] = {
    CFG_STR("execute", "", CFGF_NONE),
    CFG_END()
};

static cfg_opt_t config_opts[] = {
    CFG_SEC("connection", connection_opts, CFGF_NONE),
    CFG_SEC("command", command_opts, CFGF_MULTI | CFGF_TITLE),
    CFG_END()
};

static cfg_t *cfg = NULL;

int message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
                    void * const userdata){
    xmpp_stanza_t *reply, *body, *text;
    char *intext;
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    if(!xmpp_stanza_get_child_by_name(stanza, "body")) return 1;
    if(xmpp_stanza_get_attribute(stanza, "type") != NULL &&
       !strcmp(xmpp_stanza_get_attribute(stanza, "type"), "error")) return 1;

    intext = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stanza, "body"));

    int n = cfg_size(cfg, "command");
    int found = 0;
    int i = 0;
    for(i = 0; i < n; i++){
        cfg_t *command_config = cfg_getnsec(cfg, "command", i);
        if(!strncmp(cfg_title(command_config), intext, strlen(cfg_title(command_config)))){
            found = 1;
            FILE *processio = popen(cfg_getstr(command_config, "execute"), "r");

            if(processio > 0){
                char buffer[128];
                char *result_buffer = NULL;

                while(fgets(buffer, sizeof(buffer), processio) != NULL){
                    if(result_buffer){
                        char *new_result_buffer = malloc(strlen(result_buffer)+sizeof(buffer)+1);
                        strcat(new_result_buffer, result_buffer);
                        strcat(new_result_buffer, buffer);
                        free(result_buffer);
                    }else{
                        result_buffer = malloc(sizeof(buffer)+1);
                        strcpy(result_buffer, buffer);
                    }
                }

                int result = pclose(processio);

                reply = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(reply, "message");
                xmpp_stanza_set_type(reply, xmpp_stanza_get_type(stanza)?xmpp_stanza_get_type(stanza):"chat");
                xmpp_stanza_set_attribute(reply, "to", xmpp_stanza_get_attribute(stanza, "from"));

                body = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(body, "body");

                text = xmpp_stanza_new(ctx);
                sprintf(buffer, result_buffer);
                xmpp_stanza_set_text(text, buffer);

                xmpp_stanza_add_child(body, text);
                xmpp_stanza_add_child(reply, body);

                xmpp_send(conn, reply);
                xmpp_stanza_release(reply);
                free(result_buffer);
            }
            break;
        }
    }

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
