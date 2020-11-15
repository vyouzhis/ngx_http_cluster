/*
 * =====================================================================================
 *
 *       Filename:  ngx_cluster.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2020年08月10日 11时50分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  etomc2@etomc2.com (), etomc2@etomc2.com
 *   Organization:  etomc2.com
 *
 * =====================================================================================
 */
#ifndef NGX_CLUSTER_INC
#define NGX_CLUSTER_INC

#define NGX_CLUSTER_VERSION "0.0.1"

#include <ctype.h>
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_http.h>
#include <ngx_http_core_module.h>
#include <ngx_md5.h>
#include <pcre.h>
#include <sys/times.h>
#include <unistd.h>

#define LFEATURE 1
#define DFEATURE 1
#ifndef __NGX_CLUSTER_LOG
#define __NGX_CLUSTER_LOG
#define NX_LOG(LOG, ...)                                           \
    do {                                                           \
        if (LFEATURE)                                              \
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, LOG, \
                          ##__VA_ARGS__);                          \
    } while (0)
#endif

#ifndef __NGX_CLUSTER_DEBUG
#define __NGX_CLUSTER_DEBUG
#define NX_DEBUG(LOG, ...)                                                \
    do {                                                                  \
        if (DFEATURE)                                                     \
            ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, LOG, \
                          ##__VA_ARGS__);                                 \
    } while (0)
#endif

#ifndef __NGX_CLUSTER_CONF_DEBUG
#define __NGX_CLUSTER_CONF_DEBUG
#define NX_CONF_DEBUG(LOG, ...)                                       \
    do {                                                              \
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, LOG, ##__VA_ARGS__); \
    } while (0)
#endif

//---    define ----

#define NGX_WEB_API "cluster_api"
#define NGX_CLUSTER_MAIN "ngx_cluster_main"
#define NGX_CLUSTER_BRANCH "ngx_cluster_branch"

#define NGX_CLUSTER_NODE "ngx_cluster_node"

#define WEB_NGX_CLUSTER " "

//--- define ---
/* The empty string. */
static u_char ngx_web_cluster[] = WEB_NGX_CLUSTER;

typedef struct {
    ngx_int_t status;

} ngx_http_cluster_ctx_t;

typedef struct {
    ngx_flag_t ngx_cluster_main;
    ngx_array_t *ngx_cluster_branch;

    // ngx_flag_t ngx_cluster_node;

    u_char *confname;
} ngx_http_cluster_loc_conf_t; /* ----------  end of struct
                                ngx_http_cluster_loc_conf_t  ---------- */
#endif                         /* ----- #ifndef NGX_CLUSTER_INC  ----- */

