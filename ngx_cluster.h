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
/*
BSD 3-Clause License

Copyright (c) 2020, fastdbcache
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#define NGX_CLUSTER_SYNC "ngx_cluster_sync"
#define NGX_CLUSTER_MAIN "ngx_cluster_main"
#define NGX_CLUSTER_BRANCH "ngx_cluster_branch"

#define NGX_CLUSTER_NODE "ngx_cluster_node"

#define NGX_CLUSTER_CONF_PATH "ngx_cluster_conf_path"

#define NGX_CLUSTER_FILTER "ngx_cluster_filter"

#define CONFIG_EXT_PATH "/.config/"
#define CONFIG_VHOSTS_PATH "/vhosts/"

#define CONFIG_DEFAULT_SERVER \
    "# HTTPS server \n  \
    include vhosts/%.*s/%.*s.upstream;\n  \
    server {   \n \
#       listen       443 ssl;\n  \
        listen 80;\n  \
       server_name  %.*s;\n\
   \n \
#   ssl_certificate      %.*s.pem;\n\
#        ssl_certificate_key  %.*s.key;\n\
   \n \
#       ssl_session_cache    shared:SSL:1m;\n\
#         ssl_session_timeout  5m;\n\
  \n  \
#       ssl_ciphers  HIGH:!aNULL:!MD5;\n\
#          ssl_prefer_server_ciphers  on;\n\
  \n  \
    include vhosts/%.*s/%.*s.location; \n  \
}\n\
"
#define CONFIG_DEFAULT_LOCATION \
    "\
        location / {\n\
              root   html;\n\
              index  index.html index.htm;\n\
          }\n\
        location /uri {\n\
        proxy_pass http://%.*s_backend;\n\
    }\n\
    "

#define CONFIG_DEFAULT_UPSTREAM \
    "\
upstream %.*s_backend {  \n\
    server 10.8.0.2       weight=5;\n \
    server 10.8.0.3:8080   backup;\n \
}"

typedef struct {
    ngx_int_t status;
} ngx_http_cluster_ctx_t;

#ifndef __NGX_POST_BODY_S
#define __NGX_POST_BODY_S
#define post_body_get(p, k)                                     \
    do {                                                        \
        while (p) {                                             \
            if (ngx_strncmp(p->key.data, k, p->key.len) == 0) { \
                break;                                          \
            }                                                   \
            p = p->next;                                        \
        }                                                       \
    } while (0)

#define post_body_add(rr, p, k)                                                \
    do {                                                                       \
        while (p) {                                                            \
            if (p->key.len == 0) {                                             \
                ngx_str_set(&p->key, k);                                       \
                break;                                                         \
            }                                                                  \
            if (p->next == NULL) {                                             \
                p->next = ngx_pcalloc(rr->pool, sizeof(ngx_http_post_body_t)); \
            }                                                                  \
            p = p->next;                                                       \
        }                                                                      \
    } while (0)
#endif

struct ngx_http_post_body_s {
    ngx_str_t key;
    ngx_str_t data;

    struct ngx_http_post_body_s *next;
}; /* ----------  end of struct ngx_http_post_body_s  ---------- */

typedef struct ngx_http_post_body_s ngx_http_post_body_t;

typedef struct {
    ngx_array_t *ngx_cluster_branch;
    ngx_array_t *ngx_cluster_filter;
    // ngx_flag_t ngx_cluster_sync;

    ngx_flag_t ngx_cluster_node;

    ngx_str_t ngx_cluster_conf_path;
    u_char *confname;
} ngx_http_cluster_loc_conf_t; /* ----------  end of struct
                                ngx_http_cluster_loc_conf_t  ---------- */

ngx_str_t get_uri(ngx_http_request_t *r);

void build_base_dir(ngx_http_request_t *r, ngx_str_t *base_dir,
                    ngx_http_cluster_loc_conf_t *lccf);
void get_config_body(ngx_http_request_t *r);

void body_upload_config(ngx_http_request_t *r);
int config_create_dir(const char *path, const mode_t mode);
void config_create_file(const char *path, const char *cont, int len);
int config_file_read(const char *file, ngx_str_t *data, ngx_http_request_t *r);
long config_file_size(const char *file);
size_t nindex(const char *s, int c);
size_t pindex(const char *s, size_t len, int c);
void post_body_data(ngx_http_request_t *r, ngx_http_post_body_t *pb);
ngx_int_t post_body_header_out(ngx_http_request_t *r, ngx_str_t *resData,
                               int html_json, int finalize);
ngx_str_t *ngx_dir_list_config(ngx_http_request_t *r, ngx_str_t *path,
                               ngx_http_cluster_loc_conf_t *lccf);
void ngx_commit_config(ngx_http_request_t *r, ngx_str_t *path,
                       ngx_http_cluster_loc_conf_t *lccf);

void new_server_config_file(ngx_http_request_t *r, const char *type,
                            ngx_str_t *cfile, ngx_str_t server,
                            ngx_str_t domain);

#endif /* ----- #ifndef NGX_CLUSTER_INC  ----- */

